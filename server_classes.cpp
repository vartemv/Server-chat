//
// Created by artem on 4/13/24.
//
#include "server_classes.h"

void UDPserver::Initialize(struct sockaddr_in *server_address) {
    if ((this->sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (bind(this->sockFD, (const struct sockaddr *) server_address, sizeof(*server_address)) < 0) {
        perror("binding failed udp");
        exit(EXIT_FAILURE);
    }
}

void UDPserver::Listen(ThreadPool *tp, std::stack<UserInfo> *s, synch *synch_variables, int signal_listener) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        std::cerr << "Unable to create epoll instance\n";
        exit(EXIT_FAILURE);
    }

    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = this->sockFD;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->sockFD, &event) < 0) {
        std::cerr << "Unable to add socket to epoll\n";
        exit(EXIT_FAILURE);
    }

    event.data.fd = signal_listener;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, signal_listener, &event) < 0) {
        std::cerr << "Unable to add socket to epoll\n";
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[2];

    bool loop = true;
    while (loop) {
        int num_events = epoll_wait(epoll_fd, events, 2, -1); // 5 seconds timeout

        if (num_events < 0) {
            std::cerr << "Error in epoll_wait\n";
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == this->sockFD) {
                uint8_t buf[1024];
                sockaddr_in client_addr;
                if (!(events[i].events & EPOLLIN))
                    continue;

                socklen_t len = sizeof(client_addr);

                int n = recvfrom(this->sockFD, buf, 1024,
                                 0, (struct sockaddr *) &client_addr, &len);

                if (n == -1) {
                    std::cerr << "recvfrom failed. errno: " << errno << '\n';
                    continue;
                }

                tp->AddTask(std::bind(&UDPhandler::handleUDP, buf, client_addr, n, this->retransmissions, this->timeout,
                                      &tp->busy_threads, s, synch_variables, signal_listener));
            } else {
                loop = false;
                break;
            }
        }
    }
}

void UDPserver::Destroy() {
    close(this->sockFD);
}

void TCPserver::Initialize(struct sockaddr_in *server_address) {
    if ((this->sockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (bind(this->sockFD, (const struct sockaddr *) server_address, sizeof(*server_address)) < 0) {
        perror("binding failed tcp");
        exit(EXIT_FAILURE);
    }
}

void TCPserver::Listen(ThreadPool *tp, std::stack<UserInfo> *s, synch *synch_variables, int signal_listener) {
    struct sockaddr_in client;
    listen(this->sockFD, 5);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        std::cerr << "Unable to create epoll instance\n";
        exit(EXIT_FAILURE);
    }

    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = this->sockFD;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->sockFD, &event) < 0) {
        std::cerr << "Unable to add socket to epoll\n";
        exit(EXIT_FAILURE);
    }

    event.data.fd = signal_listener;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, signal_listener, &event) < 0) {
        std::cerr << "Unable to add socket to epoll\n";
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[2];

    bool loop = true;
    while (loop) {
        int num_events = epoll_wait(epoll_fd, events, 2, -1); // 5 seconds timeout
        if (num_events < 0) {
            std::cerr << "Error in epoll_wait\n";
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == this->sockFD) {
                socklen_t len = sizeof(client);
                int clientSocket = accept(this->sockFD, (struct sockaddr *) &client, &len);
                if (clientSocket < 0) {
                    perror("accept failed");
                    continue;
                }
                tp->AddTask(
                        std::bind(&TCPhandler::handleTCP, clientSocket, &tp->busy_threads, s, synch_variables, client, signal_listener));
            } else {
                loop = false;
                break;
            }
        }
    }
}

void TCPserver::Destroy() {
    shutdown(this->sockFD, SHUT_RDWR);
    close(this->sockFD);
}