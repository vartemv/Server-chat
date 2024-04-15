//
// Created by artem on 4/14/24.
//

#ifndef IPK_SERVER_UDPHANDLER_H

#include <cstdint>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "packets.h"
#include <thread>
#include <vector>
#include <algorithm>
#include <sys/epoll.h>
#include <csignal>

#define IPK_SERVER_UDPHANDLER_H


class UDPhandler {
public:
    int retransmissions;
    int timeout_chat;
    int global_counter;
    int client_socket;
    std::vector<int> vec;
    epoll_event events[1];
    int epoll_fd;
    bool auth;

    UDPhandler(int ret, int t) {
        this->retransmissions = ret;
        this->timeout_chat = t;
        this->global_counter = 0;
        this->client_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (this->client_socket < 0) {
            perror("Problem with creating response socket");
            exit(EXIT_FAILURE);
        }

        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            std::cerr << "Failed to create epoll file descriptor\n";
            exit(EXIT_FAILURE);
        }

        // setup epoll event
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = this->client_socket;

        // add socket file descriptor to epoll
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->client_socket, &ev) == -1) {
            std::cerr << "Failed to add file descriptor to epoll\n";
            close(epoll_fd);
            exit(EXIT_FAILURE);
        }

        auth = false;

    }

    static void handleUDP(uint8_t *buf, sockaddr_in client_addr, int length, int retransmissions, int timeout,
                          std::vector<sockaddr_in> *addresses);

private:
    bool decipher_the_message(uint8_t *buf, sockaddr_in client_addr, int length);

    void respond_to_auth(uint8_t *buf, int length, sockaddr_in client_addr);

    void send_confirm(uint8_t *buf, sockaddr_in client_addr);

    void send_reply(uint8_t *buf, sockaddr_in client_addr, std::string &message, bool OK);

    static int read_packet_id(uint8_t *buf);

    //void add_to_sent_messages();
    int wait_for_the_incoming_connection(sockaddr_in client_addr, uint8_t *buf_out, int timeout = -1);

    bool waiting_for_confirm(int count, uint8_t *buf, int len, sockaddr_in client_addr);
};


#endif //IPK_SERVER_UDPHANDLER_H
