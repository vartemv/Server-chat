//
// Created by artem on 4/14/24.
//

#ifndef IPK_SERVER_UDPHANDLER_H
#define IPK_SERVER_UDPHANDLER_H

#include <cstdint>

#include <cstdio>
#include <cstdlib>

#include "packets.h"

#include <utility>

#include <algorithm>

#include <queue>
#include "synch.h"


class UDPhandler {
public:
    int retransmissions;
    int timeout_chat;
    int global_counter;
    int client_socket;
    std::vector<int> vec;
    epoll_event events[2];
    int epoll_fd;
    bool auth;
    sockaddr_in client_addr;
    std::string display_name;
    std::string channel_name;
    std::string user_n;

    UDPhandler(int ret, int t, sockaddr_in client, int kill) {
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
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = this->client_socket;

        // add socket file descriptor to epoll
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->client_socket, &ev) == -1) {
            std::cerr << "Failed to add file descriptor to epoll\n";
            close(epoll_fd);
            exit(EXIT_FAILURE);
        }

        ev.data.fd = kill;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, kill, &ev) < 0) {
            std::cerr << "Unable to add socket to epoll\n";
            exit(EXIT_FAILURE);
        }

        auth = false;

        client_addr = client;

        channel_name = "general";

    }

    static void
    handleUDP(uint8_t *buf, sockaddr_in client_addr, int length, int retransmissions, int timeout, int *busy,
              std::stack<UserInfo> *s, synch *synch_var, int signal_listener);

    int create_message(uint8_t *buf_out, std::string &msg, bool error, std::string &name);

    void send_message(uint8_t *buf, int message_length, bool terminate);

    int convert_from_tcp(uint8_t *buf, uint8_t *tcp_buf);

private:
    bool decipher_the_message(uint8_t *buf, int length, std::stack<UserInfo> *s, synch *synch_var);

    int respond_to_auth(uint8_t *buf, int length, std::stack<UserInfo> *s, synch *synch_var);

    void respond_to_join(uint8_t *buf, int length, std::stack<UserInfo> *s, synch *synch_var);

    void send_confirm(uint8_t *buf);

    void send_reply(uint8_t *buf, std::string &message, bool OK);

    static int read_packet_id(uint8_t *buf);

    int wait_for_the_incoming_connection(uint8_t *buf_out, int timeout = -1);

    bool waiting_for_confirm(uint8_t *buf, int len);

    void message(uint8_t *buf, int message_length, std::stack<UserInfo> *s, synch *synch_var, std::string &channel);


    bool buffer_validation(uint8_t *buf, int message_length, int start_position, int minimal_length,
                           int amount_of_fields = 2, int first_limit = 20, int second_limit = 20, int third_limit = 5);

    void change_display_name(uint8_t *buf, bool second);

    void client_leaving(std::stack<UserInfo> *s, synch *synch_var);

    std::string read_channel_name(uint8_t *buf);

    int create_bye(uint8_t *buf);

    bool username_exists(uint8_t *buf, synch *synch_vars);

};

void read_queue(std::stack<UserInfo> *s, bool *terminate, synch *synch_vars, int *busy, UDPhandler *udp);

void logger(sockaddr_in client, const char *type, const char *operation);



#endif //IPK_SERVER_UDPHANDLER_H

