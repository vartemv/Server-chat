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
    epoll_event events[2];
    sockaddr_in client_addr;
    bool auth;
    std::string user_n;
    uint8_t storage[5012];
    size_t storage_capacity = 0;

    TCPhandler(int s, sockaddr_in c, int kill) {
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

        ev.data.fd = kill;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, kill, &ev) < 0) {
            std::cerr << "Unable to add socket to epoll\n";
            exit(EXIT_FAILURE);
        }

        client_addr = c;

        auth = false;
    }

    static void handleTCP(int client_socket, int *busy, std::stack<UserInfo> *s, synch *synch_var, sockaddr_in client,
                          int signal_listener);

    void send_buf(uint8_t *buf, int length) const;

    void create_message(bool error, const char *msg);

    int convert_from_udp(uint8_t *buf, uint8_t *tcp_buf);

private:
    int listening_for_incoming_connection(uint8_t *buf, int len);

    bool decipher_the_message(uint8_t *buf, int length, std::stack<UserInfo> *s, synch *synch_var);

    void send_string(std::string &msg) const;

    void create_reply(const char *status, const char *msg);

    void create_bye();

    void message(uint8_t *buf, int message_length, std::stack<UserInfo> *s, synch *synch_var,
                 std::string &channel);

    void user_changed_channel(std::stack<UserInfo> *s, synch *synch_var, const char *action);

    bool username_already_exists(std::string &username, synch *synch_vars);

};

void tcp_logger(sockaddr_in client, const char *type, const char *operation);

void read_queue(std::stack<UserInfo> *s, bool *terminate, synch *synch_vars, int *busy, TCPhandler *tcp);



#endif //IPK_SERVER_TCPHANDLER_H
