////
//// Created by artem on 4/13/24.
////
//
//#ifndef IPK_SERVER_THREAD_POOL_H
//#define IPK_SERVER_THREAD_POOL_H
//
//#include <cstdio>
//#include <thread>
//#include <functional>
//#include <mutex>
//#include <queue>
//#include <condition_variable>
//#include <future>
//
//#endif //IPK_SERVER_THREAD_POOL_H
//using namespace std;
//
//class ThreadPool {
//public:
//    ThreadPool(const int size);
//
//    ~ThreadPool();
//
//    ThreadPool(const ThreadPool &) = delete;
//
//    ThreadPool(ThreadPool &&) = delete;
//
//    ThreadPool &operator=(const ThreadPool &) = delete;
//
//    ThreadPool &operator=(ThreadPool &&) = delete;
//
//    void Shutdown();
//
//    template<typename F, typename... Args>
//    auto AddTask(F &&f, Args &&... args) -> std::future<decltype(f(args...))>;
//
//    int QueueSize();
//
//
//private:
//    class ThreadWorker {
//    public:
//        ThreadWorker(ThreadPool *pool);
//
//        void operator()();
//    private:
//        ThreadPool *thread_pool;
//    };
//
//public:
//    int busy_threads;
//
//private:
//    mutable std::mutex mutex;
//    std::condition_variable condition_variable;
//    std::vector<std::thread> threads;
//    bool shutdown_requested;
//    std::queue<std::function<void()>> queue;
//};
//

#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool
{
public:
    ThreadPool(const int size) : busy_threads(size), threads(std::vector<std::thread>(size)), shutdown_requested(false)
    {
        for (size_t i = 0; i < size; ++i)
        {
            threads[i] = std::thread(ThreadWorker(this));
        }
    }

    ~ThreadPool()
    {
        Shutdown();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&)      = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&)      = delete;

    // Waits until threads finish their current task and shutdowns the pool
    void Shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            shutdown_requested = true;
            condition_variable.notify_all();
        }

        for (size_t i = 0; i < threads.size(); ++i)
        {
            if (threads[i].joinable())
            {
                threads[i].join();
            }
        }
    }

    template <typename F, typename... Args>
    auto AddTask(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
    {

        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        auto wrapper_func = [task_ptr]() { (*task_ptr)(); };
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(wrapper_func);
            // Wake up one thread if its waiting
            condition_variable.notify_one();
        }

        // Return future from promise
        return task_ptr->get_future();
    }

    int QueueSize()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return queue.size();
    }

private:
    class ThreadWorker
    {
    public:
        ThreadWorker(ThreadPool* pool) : thread_pool(pool)
        {
        }

        void operator()()
        {
            std::unique_lock<std::mutex> lock(thread_pool->mutex);
            while (!thread_pool->shutdown_requested || (thread_pool->shutdown_requested && !thread_pool->queue.empty()))
            {
                thread_pool->busy_threads--;
                thread_pool->condition_variable.wait(lock, [this] {
                    return this->thread_pool->shutdown_requested || !this->thread_pool->queue.empty();
                });
                thread_pool->busy_threads++;

                if (!this->thread_pool->queue.empty())
                {

                    auto func = thread_pool->queue.front();

                    thread_pool->queue.pop();

                    lock.unlock();
                    func();
                    lock.lock();
                }
            }
        }

    private:
        ThreadPool* thread_pool;
    };

public:
    int busy_threads;

private:
    mutable std::mutex mutex;
    std::condition_variable condition_variable;

    std::vector<std::thread> threads;
    bool shutdown_requested;

    std::queue<std::function<void()>> queue;
};