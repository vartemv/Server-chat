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

    UDPhandler(int ret, int t){
        this->retransmissions = ret;
        this->timeout_chat = t;
        this->global_counter = 0;
        this->client_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (this->client_socket < 0) {
            perror("Problem with creating response socket");
            exit(EXIT_FAILURE);
        }
    }

    static void handleUDP(uint8_t *buf, sockaddr_in client_addr, int length, int retransmissions, int timeout);
private:
    bool decipher_the_message(uint8_t *buf);
    void terminate_connection();
    bool respond_to_auth(uint8_t *buf, int length, sockaddr_in client_addr);
    void send_confirm(uint8_t *buf, sockaddr_in client_addr);
    void send_reply(uint8_t *buf,sockaddr_in client_addr, std::string &message);
    static int read_packet_id(uint8_t *buf);
    //void add_to_sent_messages();
    bool waiting_for_confirm(int count, uint8_t *buf, int len, sockaddr_in client_addr);
};


#endif //IPK_SERVER_UDPHANDLER_H
