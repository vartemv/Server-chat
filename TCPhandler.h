//
// Created by artem on 4/20/24.
//

#ifndef IPK_SERVER_TCPHANDLER_H
#define IPK_SERVER_TCPHANDLER_H

#include "synch.h"

class TCPhandler {
public:

    std::string channel_name;
    std::string display_name;
    int client_socket;
    int epoll_fd;
    epoll_event events[1];

    TCPhandler(int s) {
        this->channel_name = "general";
        this->client_socket = s;

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

    }

    static void handleTCP(int client_socket, int *busy, std::stack<UserInfo> *s, synch *synch_var);

    void send_buf(uint8_t *buf, int length) const;

    void create_message(bool error, const char *msg);

private:
    int listening_for_incoming_connection(uint8_t *buf, int len);

    bool decipher_the_message(uint8_t *buf, int length, std::stack<UserInfo> *s, synch *synch_var);

    void send_string(std::string &msg) const;

    void create_reply(const char *status, const char *msg);

    void message(uint8_t *buf, int message_length, std::stack<UserInfo> *s, synch *synch_var,
                 std::string &channel);

    void user_changed_channel(std::stack<UserInfo> *s, synch *synch_var, const char* action);

};

void read_queue(std::stack<UserInfo> *s, bool *terminate, synch *synch_vars, int *busy, TCPhandler *tcp);

#endif //IPK_SERVER_TCPHANDLER_H
