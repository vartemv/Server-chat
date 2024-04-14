//
// Created by artem on 4/13/24.
//

#include "server_classes.h"

void UDPserver::Initialize() {

    struct sockaddr_in server_address;

    if((this->sockFD= socket(AF_INET, SOCK_DGRAM, 0))<0 ){
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_address, 0, sizeof (server_address));

    server_address.sin_family = AF_INET;
    //server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(PORT);

    if (bind(UDPserver::sockFD, (const struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("binding failed");
        exit(EXIT_FAILURE);
    }
}


void UDPserver::Listen() {
    ThreadPool tp{10};

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        std::cerr << "Unable to create epoll instance\n";
        exit(EXIT_FAILURE);
    }

    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = this->sockFD;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, UDPserver::sockFD, &event)<0) {
        std::cerr << "Unable to add socket to epoll\n";
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[1];
    while (true) {
        int num_events = epoll_wait(epoll_fd, events, 1, 3000); // 5 seconds timeout

        if (num_events < 0) {
            std::cerr << "Error in epoll_wait\n";
            exit(EXIT_FAILURE);
        }


        for (int i = 0; i < num_events; ++i) {
            std::cout <<"Main port"<<std::endl;
            uint8_t buf[1024];
            struct sockaddr_in client_addr;
            if (!(events[i].events & EPOLLIN))
                continue;

            socklen_t len = sizeof(client_addr);

            int n = recvfrom(UDPserver::sockFD, buf, 1024,
                             0, (struct sockaddr*)&client_addr, &len);

            if (n == -1) {
                std::cerr << "recvfrom failed. errno: " << errno << '\n';
                continue;
            }

            tp.AddTask(std::bind(&UDPhandler::handleUDP, buf, client_addr, n, retransmissions, timeout));
        }

    }
}

void UDPserver::Destroy() {
    close(UDPserver::sockFD);
}