//
// Created by artem on 4/20/24.
//
#include "TCPhandler.h"

void
TCPhandler::handleTCP(int client_socket, int *busy, std::stack<UserInfo> *s, synch *synch_var, sockaddr_in client,
                      int signal_listener) {

    TCPhandler tcp(client_socket, client, signal_listener);

    bool end = false;

    std::thread sender(read_queue, s, &end, synch_var, busy, &tcp);

    uint8_t internal_buf[1024];

    while (true) {
        int length = tcp.listening_for_incoming_connection(internal_buf, 1024);
        if (length == 0)
            break;
        if (length == -1) {
            tcp.create_bye();
            break;
        }
        if (!tcp.decipher_the_message(internal_buf, length, s, synch_var))
            break;
    }

    end = true;
    {
        std::lock_guard<std::mutex> lock(synch_var->mtx);
        synch_var->ready = true;
    }
    synch_var->cv.notify_all();

    sender.join();
    shutdown(tcp.client_socket, SHUT_RDWR);
    close(tcp.client_socket);
}

void read_queue(std::stack<UserInfo> *s, bool *terminate, synch *synch_vars, int *busy, TCPhandler *tcp) {
    while (!*terminate) {
        std::unique_lock<std::mutex> lock(synch_vars->mtx);
        synch_vars->cv.wait(lock, [&synch_vars] { return synch_vars->ready; });

        synch_vars->waiting.lock();
        synch_vars->finished++;
        synch_vars->waiting.unlock();

        if (!s->empty()) {

            synch_vars->waiting.lock();
            UserInfo new_uf = s->top();
            synch_vars->waiting.unlock();

            if (!new_uf.tcp) {
                if (new_uf.channel == tcp->channel_name) {
                    uint8_t buf[1024];
                    int length = TCPhandler::convert_from_udp(buf, new_uf.buf);
                    tcp->send_buf(buf, length);
                }
            } else {
                if (new_uf.tcp_socket != tcp->client_socket && new_uf.channel == tcp->channel_name) {
                    tcp->send_buf(new_uf.buf, new_uf.length);
                }
            }
        }
        if (synch_vars->finished == *busy) {
            synch_vars->finished = 0;
            synch_vars->ready = false;
            if (!s->empty())
                s->pop();
        }

        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lock.lock();

    }
}

int TCPhandler::listening_for_incoming_connection(uint8_t *buf, int len) {

    int event_count = epoll_wait(this->epoll_fd, this->events, 2, -1);

    if (event_count == -1) {
        perror("epoll_wait");
        close(this->epoll_fd);
        exit(EXIT_FAILURE);
    } else if (event_count > 0) {
        for (int j = 0; j < event_count; j++) {
            if (events[j].data.fd == this->client_socket) { // check if EPOLLIN event has occurred
                int n = recv(this->client_socket, buf, len, 0);
                if (n == -1) {
                    std::cerr << "recvfrom failed. errno: " << errno << '\n';
                    continue;
                } else if (n == 0) {
                    return 0;
                } else if (n > 0) {
                    return n;
                }
            } else {
                return -1;
            }
        }
    }
    return 0;
}

bool TCPhandler::decipher_the_message(uint8_t *buf, int length, std::stack<UserInfo> *s, synch *synch_var) {
    std::string out_str;
    for (int i = 0; i < length - 2; ++i) {
        out_str += static_cast<char>(buf[i]);
    }

    std::istringstream iss(out_str);
    std::vector<std::string> result;
    for (std::string element; std::getline(iss, element, ' ');) {
        result.push_back(element);
    }

    if (result[0] == "AUTH") {
        std::regex e("^AUTH .{1,20} AS .{1,20} USING .{1,6}$");
        if (!std::regex_match(out_str, e)) {
            std::string mes = "Wrong AUTH format";
            std::cout << mes << std::endl;
            create_message(true, "Wrong AUTH format");
            return false;
        }
        tcp_logger(this->client_addr, "AUTH", "RECV");
        this->create_reply("OK", "Authentication is successful");
        this->display_name = result[3];
        this->user_changed_channel(s, synch_var, "joined");

    } else if (result[0] == "MSG") {
        std::regex e("^MSG FROM .{1,20} IS .{1,1400}$");
        if (!std::regex_match(out_str, e)) {
            std::string mes = "Wrong MSG format";
            std::cout << mes << std::endl;
            create_message(true, "Wrong MSG format");
            return false;
        }
        tcp_logger(this->client_addr, "MSG", "RECV");
        this->display_name = result[2];
        this->message(buf, length, s, synch_var, this->channel_name);
    } else if (result[0] == "JOIN") {
        std::regex e("^JOIN .{1,20} AS .{1,20}$");
        if (!std::regex_match(out_str, e)) {
            std::string mes = "Wrong JOIN format";
            create_message(true, mes.c_str());
            this->create_reply("NOK", "Join wasn't successful");
            return false;
        }
        if (result[1] != this->channel_name) {
            tcp_logger(this->client_addr, "JOIN", "RECV");
            this->user_changed_channel(s, synch_var, "left");
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            this->channel_name = result[1];
            this->display_name = result[3];
            this->user_changed_channel(s, synch_var, "joined");
            this->create_reply("OK", "Join was successful");
        } else {
            this->create_reply("NOK", "Tried to join to the current channel");
        }
    } else if (result[0] == "BYE") {
        user_changed_channel(s, synch_var, "left");
    }

    return true;
}

void TCPhandler::message(uint8_t *buf, int message_length, std::stack<UserInfo> *s, synch *synch_var,
                         std::string &channel) {
    struct sockaddr_in blank;
    {
        std::lock_guard<std::mutex> lock(synch_var->mtx);
        s->emplace(blank, buf, message_length, channel, true, this->client_socket);
        synch_var->ready = true;
    }
    synch_var->cv.notify_all();
}

void TCPhandler::create_reply(const char *status, const char *msg) {
    std::string message;
    message = "REPLY " + std::string(status) + " IS " + std::string(msg) + "\r\n";
    tcp_logger(this->client_addr, "REPLY", "SENT");
    this->send_string(message);
}

void TCPhandler::create_message(bool error, const char *msg) {
    std::string message;
    error ? message = "ERR FROM SERVER IS " + std::string(msg) + "\r\n" : message = "MSG FROM SERVER IS " +
                                                                                    std::string(msg) + "\r\n";
    tcp_logger(this->client_addr, "MSG", "SENT");
    this->send_string(message);
}

void TCPhandler::create_bye() {
    std::string bye = "BYE\r\n";
    tcp_logger(this->client_addr, "BYE", "SENT");
    this->send_string(bye);
}

void TCPhandler::send_string(std::string &msg) const {
    const char *message = msg.c_str();
    size_t bytes_left = msg.size();

    ssize_t tx = send(this->client_socket, message, bytes_left, 0);

    if (tx < 0) {
        perror("Error sending message");
    }
}

void TCPhandler::send_buf(uint8_t *buf, int length) const {
    ssize_t tx = send(this->client_socket, buf, length, 0);

    if (tx < 0) {
        perror("Error sending message");
    }
}

void TCPhandler::user_changed_channel(std::stack<UserInfo> *s, synch *synch_var, const char *action) {

    std::stringstream ss;
    ss << this->display_name << " has " << std::string(action) << " " << this->channel_name << ".";
    std::string content = ss.str();

    std::string message = "MSG FROM Server IS " + content + "\r\n";

    uint8_t buffer[1024];

    memcpy(buffer, message.c_str(), message.length());

    this->message(buffer, message.length(), s, synch_var, this->channel_name);
}

int TCPhandler::convert_from_udp(uint8_t *buf, uint8_t *udp_buf) {
    int i = 3;
    std::string display_n;
    std::string contents;

    while (udp_buf[i] != 0x00) {
        display_n.push_back(static_cast<char>(udp_buf[i]));
        i++;
    }

    i++;

    while (udp_buf[i] != 0x00) {
        contents.push_back(static_cast<char>(udp_buf[i]));
        i++;
    }

    std::string message = "MSG FROM " + display_n + " IS " + contents + "\r\n";

    memcpy(buf, message.c_str(), message.length());

    return message.length();
}

void tcp_logger(sockaddr_in client, const char *type, const char *operation) {
    std::cout << operation << " " << inet_ntoa(client.sin_addr) << ":" << ntohs(client.sin_port) << " | " << type
              << std::endl;
}
