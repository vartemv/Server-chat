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

#define PORT 47356
#endif //IPK_SERVER_SERVER_CLASSES_H

class Server {
public:
    virtual void Initialize() = 0;

    virtual void Listen() = 0;
};

class TCPserver : public Server {
public:
    void Initialize() override;

    void Listen() override;

    void Destroy();

private:
    int sockFD;
};

class UDPserver : public Server {
public:
    int retransmissions;
    int timeout;

    UDPserver(int ret, int t){
        this->retransmissions = ret;
        this->timeout = t;
    }

    void Initialize() override;

    void Listen() override;

    void Destroy();

private:
    int sockFD;
};

