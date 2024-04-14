//#include <functional>
//#include <future>
//#include <mutex>
//#include <thread>
//#include <utility>
//#include <vector>
//
//
//ThreadPool::ThreadPool(const int size) : busy_threads(size), threads(std::vector<std::thread>(size)),
//                                         shutdown_requested(false) {
//    for (size_t i = 0; i < size; ++i) {
//        threads[i] = std::thread(ThreadWorker(this));
//    }
//}
//
//ThreadPool::~ThreadPool() {
//    Shutdown();
//}
//
//// Waits until threads finish their current task and shutdowns the pool
//void ThreadPool::Shutdown() {
//    {
//        std::lock_guard<std::mutex> lock(mutex);
//        shutdown_requested = true;
//        condition_variable.notify_all();
//    }
//
//    for (size_t i = 0; i < threads.size(); ++i) {
//        if (threads[i].joinable()) {
//            threads[i].join();
//        }
//    }
//}
//
//template<typename F, typename... Args>
//auto ThreadPool::AddTask(F &&f, Args &&... args) -> std::future<decltype(f(args...))> {
//    auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
//            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
//
//    auto wrapper_func = [task_ptr]() { (*task_ptr)(); };
//    {
//        std::lock_guard<std::mutex> lock(mutex);
//        queue.push(wrapper_func);
//        // Wake up one thread if its waiting
//        condition_variable.notify_one();
//    }
//
//    // Return future from promise
//    return task_ptr->get_future();
//}
//
//int ThreadPool::QueueSize() {
//    std::unique_lock<std::mutex> lock(mutex);
//    return queue.size();
//}
//
//
//ThreadPool::ThreadWorker::ThreadWorker(ThreadPool *pool) : thread_pool(pool) {
//}
//
//void ThreadPool::ThreadWorker::operator()() {
//    std::unique_lock<std::mutex> lock(thread_pool->mutex);
//    while (!thread_pool->shutdown_requested || (thread_pool->shutdown_requested && !thread_pool->queue.empty())) {
//        thread_pool->busy_threads--;
//        thread_pool->condition_variable.wait(lock, [this] {
//            return this->thread_pool->shutdown_requested || !this->thread_pool->queue.empty();
//        });
//        thread_pool->busy_threads++;
//
//        if (!this->thread_pool->queue.empty()) {
//            auto func = thread_pool->queue.front();
//            thread_pool->queue.pop();
//
//            lock.unlock();
//            func();
//            lock.lock();
//        }
//    }
//}