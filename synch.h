//
// Created by artem on 4/20/24.
//

#ifndef IPK_SERVER_SYNCH_H

#define IPK_SERVER_SYNCH_H

#include <mutex>
#include <condition_variable>
#include <string>
#include <stack>
#include <netinet/in.h>
#include <thread>
#include <iostream>
#include <sys/epoll.h>
#include <csignal>
#include <sstream>
#include <vector>
#include <regex>
#include <cstring>
#include <arpa/inet.h>
#include <sys/signalfd.h>
#include <unordered_set>

struct synch {
    std::mutex mtx;
    std::mutex waiting;
    std::mutex un;
    bool ready;
    std::condition_variable cv;
    std::condition_variable cv2;
    int finished;
    std::unordered_set<std::string> usernames;

    explicit synch(int b) : finished(b), ready(false){};
};

struct UserInfo {
    sockaddr_in client;
    int tcp_socket;
    uint8_t *buf;
    int length;
    std::string channel;
    bool tcp;

    UserInfo(sockaddr_in c, uint8_t *m, int l, std::string name, bool t, int cs) : client(c), buf(m), length(l),
                                                                                   channel(std::move(name)), tcp(t),
                                                                                   tcp_socket(cs) {};
};


#endif //IPK_SERVER_SYNCH_H
