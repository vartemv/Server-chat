//
// Created by artem on 4/13/24.
//

#ifndef IPK_SERVER_SERVER_CLASSES_H
#define IPK_SERVER_SERVER_CLASSES_H

#include <sys/socket.h>
#include <cstdio>
#include <cstdlib>
#include "netinet/in.h"
#include <cstring>
#include <arpa/inet.h>
#include "thread_pool.h"
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include "UDPhandler.h"
#include "TCPhandler.h"

#define PORT 47356
#endif //IPK_SERVER_SERVER_CLASSES_H

class Server {
public:
    virtual void Initialize(struct sockaddr_in *server_address) = 0;

    virtual void Listen(ThreadPool *tp, std::stack<UserInfo> *s, synch *synch_variables, int signal_listener) = 0;
};

class TCPserver : public Server {
public:

    TCPserver() {

    }

    void Initialize(struct sockaddr_in *server_address) override;

    void Listen(ThreadPool *tp, std::stack<UserInfo> *s, synch *synch_variables, int signal_listener) override;

    void Destroy();

private:
    int sockFD;
};

class UDPserver : public Server {
public:
    int retransmissions;
    int timeout;

    UDPserver(int ret, int t) {
        this->retransmissions = ret;
        this->timeout = t;
    }

    void Initialize(struct sockaddr_in *server_address) override;

    void Listen(ThreadPool *tp, std::stack<UserInfo> *s, synch *synch_variables, int signal_listener) override;

    void Destroy();

private:
    int sockFD;
};

