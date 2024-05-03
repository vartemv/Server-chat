//
// Created by artem on 4/14/24.
//
#include "UDPhandler.h"

void UDPhandler::handleUDP(uint8_t *buf, sockaddr_in client_addr, int length, int retransmissions, int timeout,
                           int *busy, std::stack<UserInfo> *s, synch *synch_var, int signal_listener) {

    UDPhandler udp(retransmissions, timeout, client_addr, signal_listener);

    bool end = false;

    std::thread sender(read_queue, s, &end, synch_var, busy, &udp);

    uint8_t internal_buf[2048];
    logger(udp.client_addr, "AUTH", "RECV");
    udp.send_confirm(buf);
    int result = udp.respond_to_auth(buf, length, s, synch_var);

    if (result != -1) {
        while (true) {
            int length_internal = udp.wait_for_the_incoming_connection(internal_buf);
            if (length_internal == -1) {
                uint8_t buf_int[256];
                int length_int = udp.create_bye(buf_int);
                udp.send_message(buf_int, length_int, true);
                break;
            }
            if (!udp.decipher_the_message(internal_buf, length_internal, s, synch_var)) {
                break;
            }
        }
    }

    if (synch_var->usernames.find(udp.user_n) != synch_var->usernames.end())
        synch_var->usernames.erase(udp.user_n);

    end = true;
    {
        std::lock_guard<std::mutex> lock(synch_var->mtx);
        synch_var->ready = true;
    }
    synch_var->cv.notify_all();

    sender.join();
    close(udp.client_socket);
}

void read_queue(std::stack<UserInfo> *s, bool *terminate, synch *synch_vars, int *busy, UDPhandler *udp) {
    while (!*terminate) {
        std::unique_lock<std::mutex> lock(synch_vars->mtx);
        synch_vars->cv.wait(lock, [&synch_vars] { return synch_vars->ready; });

        synch_vars->waiting.lock();
        synch_vars->finished++;
        synch_vars->waiting.unlock();

        if (!s->empty() && udp->auth) {

            synch_vars->waiting.lock();
            UserInfo new_uf = s->top();
            synch_vars->waiting.unlock();

            if (new_uf.tcp) {
                if (new_uf.channel == udp->channel_name) {
                    uint8_t buf[3048];
                    int length = udp->convert_from_tcp(buf, new_uf.buf);
                    udp->send_message(buf, length, false);
                }
            } else {
                if ((new_uf.client.sin_port != udp->client_addr.sin_port && new_uf.channel == udp->channel_name)) {
                    udp->send_message(new_uf.buf, new_uf.length, false);
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

bool UDPhandler::decipher_the_message(uint8_t *buf, int length, std::stack<UserInfo> *s, synch *synch_var) {
    if (std::find(this->messages_id.begin(), this->messages_id.end(), read_packet_id(buf)) == this->messages_id.end()) {
        this->messages_id.push_back(read_packet_id(buf));
        if (!this->auth) {
            if (buf[0] != 0x02) {
                if (buf[0] != 0xFF) {
                    if (buf[0] != 0xFE) {
                        uint8_t buf_int[1024];
                        std::string message = "You should log-in before doing anything else";
                        std::string name = "Server";
                        int length = this->create_message(buf_int, message, false, name);
                        this->send_message(buf_int, length, false);
                        return true;
                    }
                }
            }
        }

        switch (buf[0]) {
            case 0x00://CONFIRM
                break;
            case 0x02://AUTH
                logger(this->client_addr, "AUTH", "RECV");
                send_confirm(buf);
                if (!this->auth) {
                    respond_to_auth(buf, length, s, synch_var);
                } else {
                    uint8_t buf_err[1024];
                    std::string message = "Already authed";
                    std::string name = "Server";
                    int length_err = this->create_message(buf_err, message, true, name);
                    this->send_message(buf_err, length_err, false);
                }
                break;
            case 0x03://JOIN
                logger(this->client_addr, "JOIN", "RECV");
                send_confirm(buf);
                respond_to_join(buf, length, s, synch_var);
                break;
            case 0x04://MSG
                logger(this->client_addr, "MSG", "RECV");
                send_confirm(buf);
                this->message(buf, length, s, synch_var, this->channel_name);
                break;
            case 0xFF://BYE
                if (this->auth) {
                    this->client_leaving(s, synch_var);
                }
                logger(this->client_addr, "BYE", "RECV");
                send_confirm(buf);
                return false;
            case 0xFE://ERR
                if (this->auth) {
                    this->client_leaving(s, synch_var);
                }
                logger(this->client_addr, "ERR", "RECV");
                send_confirm(buf);
                return false;
            default:
                uint8_t buf[1024];
                std::string message = "Unknown instruction";
                std::string name = "Server";
                int length_err = this->create_message(buf, message, true, name);
                this->send_message(buf, length_err, false);
                return false;
        }
        return true;
    }else{
        send_confirm(buf);
        logger(this->client_addr, "CONFIRM", "SENT");
        if(this->messages_id.size() == 10)
            this->messages_id.erase(this->messages_id.begin(), this->messages_id.begin()+9);
        return true;
    }
}

int UDPhandler::respond_to_auth(uint8_t *buf, int message_length, std::stack<UserInfo> *s, synch *synch_var) {

    bool valid_message = true;

    if (buf[0] == 0xFF) {
        return -1;
    }
    if (buf[0] != 0x02) {
        uint8_t buf_int[1024];
        std::string message = "You should log-in before doing anything else";
        std::string name = "Server";
        int length = this->create_message(buf_int, message, false, name);
        this->send_message(buf_int, length, false);
        return 0;
    }

    if (!this->buffer_validation(buf, message_length, 3, 7, 3))
        valid_message = false;

    if (valid_message) {
        synch_var->un.lock();
        bool exists = username_exists(buf, synch_var);
        synch_var->un.unlock();
        if (!exists) {
            this->change_display_name(buf, true);
            std::string success = "Authentication is succesful";
            send_reply(buf, success, true);

            std::stringstream ss;
            ss << this->display_name << " has joined general.";
            std::string message = ss.str();
            uint8_t buf_message[1024];
            std::string name = "Server";
            int length = this->create_message(buf_message, message, false, name);
            this->message(buf_message, length, s, synch_var, this->channel_name);
            this->auth = true;
        } else {
            std::string failure = "Username already exists";
            send_reply(buf, failure, false);
        }
    } else {
        std::string failure = "Authentication is not succesful";
        send_reply(buf, failure, false);
    }

    return 0;
}

void UDPhandler::respond_to_join(uint8_t *buf, int message_length, std::stack<UserInfo> *s, synch *synch_var) {
    bool valid = true;

    if (!this->buffer_validation(buf, message_length, 3, 2))
        valid = false;

    if (valid) {
        this->change_display_name(buf, true);

        std::string new_channel = this->read_channel_name(buf);
        if (new_channel != this->channel_name) {
            std::string success = "Join is successful";
            send_reply(buf, success, true);
            std::stringstream ss;
            ss << this->display_name << " has left " << this->channel_name << ".";
            std::string message = ss.str();
            uint8_t buf_message[1024];
            std::string name = "Server";
            int length = this->create_message(buf_message, message, false, name);
            this->message(buf_message, length, s, synch_var, this->channel_name);

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            memset(buf_message, 0, 1024);
            std::stringstream joined;
            this->channel_name = this->read_channel_name(buf);
            joined << this->display_name << " has joined " << this->channel_name << ".";
            std::string message_new = joined.str();
            length = this->create_message(buf_message, message_new, false, name);
            this->message(buf_message, length, s, synch_var, this->channel_name);
        } else {
            std::string failure = "Join isn't successful";
            send_reply(buf, failure, false);
        }
    } else {
        std::string failure = "Join format is wrong";
        send_reply(buf, failure, false);
    }
}

void UDPhandler::message(uint8_t *buf, int message_length, std::stack<UserInfo> *s, synch *synch_var,
                         std::string &channel) {
    bool valid_message = true;

    if (!this->buffer_validation(buf, message_length, 3, 2, 2, 20, 1400))
        valid_message = false;


    if (valid_message) {
        {
            std::lock_guard<std::mutex> lock(synch_var->mtx);
            s->emplace(this->client_addr, buf, message_length, channel, false, 0);
            synch_var->ready = true;
        }
        synch_var->cv.notify_all();
    } else {
        std::cout << "Invalid message" << std::endl;
    }
}

void UDPhandler::client_leaving(std::stack<UserInfo> *s, synch *synch_var) {
    std::stringstream ss;
    ss << this->display_name << " has left " << this->channel_name << ".";
    std::string message = ss.str();
    uint8_t buf_message[1024];
    std::string name = "Server";
    int length = this->create_message(buf_message, message, false, name);
    this->message(buf_message, length, s, synch_var, this->channel_name);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}


void UDPhandler::send_confirm(uint8_t *buf) {
    uint8_t buf_out[4];

    ConfirmPacket confirm(0x00, this->global_counter, read_packet_id(buf));

    int len = confirm.construct_message(buf_out);

    socklen_t address_size = sizeof(this->client_addr);

    long bytes_tx = sendto(this->client_socket, buf_out, len, 0, (struct sockaddr *) &(this->client_addr),
                           address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");

    logger(this->client_addr, "CONFIRM", "SENT");

}

void UDPhandler::send_reply(uint8_t *buf, std::string &message, bool OK) {
    uint8_t buf_out[1024];

    socklen_t address_size = sizeof(this->client_addr);

    ReplyPacket reply(0x01, this->global_counter, message, OK ? 1 : 0, read_packet_id(buf));
    this->global_counter++;

    int len = reply.construct_message(buf_out);
    long bytes_tx = sendto(this->client_socket, buf_out, len, 0, (struct sockaddr *) &(this->client_addr),
                           address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");

    if (!waiting_for_confirm(buf_out, len))
        std::cout << "Client didn't confirm" << std::endl;

    logger(this->client_addr, "REPLY", "SENT");
}

void UDPhandler::send_message(uint8_t *buf, int message_length, bool terminate) {
    socklen_t address_size = sizeof(this->client_addr);

    sockaddr_in backup = this->client_addr;

    sendto(this->client_socket, buf, message_length, 0, (struct sockaddr *) &this->client_addr, address_size);
    this->global_counter++;

    if (!terminate) {
        if (!waiting_for_confirm(buf, message_length))
            std::cout << "Client didn't confirm" << std::endl;
    }
    this->client_addr = backup;
}

int UDPhandler::create_message(uint8_t *buf_out, std::string &msg, bool error, std::string &name) {
    MsgPacket message(error ? 0xFE : 0x04, this->global_counter, msg, name);
    return message.construct_message(buf_out);
}

int UDPhandler::create_bye(uint8_t *buf) {
    Packet bye(0xFF, this->global_counter);
    return bye.construct_message(buf);
}

int UDPhandler::read_packet_id(uint8_t *buf) {
    int result = buf[1] << 8 | buf[2];
    return ntohs(result);
}

int UDPhandler::wait_for_the_incoming_connection(uint8_t *buf_out, int timeout) {
    int event_count = epoll_wait(this->epoll_fd, this->events, 2, timeout);

    if (event_count == -1) {
        perror("epoll_wait");
        close(this->epoll_fd);
        exit(EXIT_FAILURE);
    } else if (event_count > 0) {
        socklen_t len_client = sizeof(this->client_addr);
        for (int j = 0; j < event_count; j++) {
            if (events[j].data.fd == this->client_socket) { // check if EPOLLIN event has occurred
                int n = recvfrom(this->client_socket, buf_out, 1024, 0, (struct sockaddr *) &this->client_addr,
                                 &len_client);
                if (n == -1) {
                    std::cerr << "recvfrom failed. errno: " << errno << '\n';
                    continue;
                }
                if (n > 0) {
                    return n;
                }
            } else {
                return -1;
            }
        }
    }
    return 0;
}

bool UDPhandler::waiting_for_confirm(uint8_t *buf, int len) {
    uint8_t buffer[1024];
    bool confirmed = false;
    for (int i = 0; i < this->retransmissions; ++i) {
        int result = this->wait_for_the_incoming_connection(buffer, this->timeout_chat);
        if (result > 0) {
            if (buffer[0] == 0x00 && read_packet_id(buffer) == read_packet_id(buf)) {
                confirmed = true;
            }
        } else if (result == -1) {
            return true;
        }
        if (confirmed) {
            break;
        } else {
            socklen_t len_client = sizeof(client_addr);
            long bytes_tx = sendto(this->client_socket, buf, len, 0, (struct sockaddr *) &(this->client_addr),
                                   len_client);
            if (bytes_tx < 0) perror("ERROR: sendto");
        }
    }
    return confirmed;
}

bool UDPhandler::buffer_validation(uint8_t *buf, int message_length, int start_position, int minimal_length,
                                   int amount_of_fields, int first_limit, int second_limit, int third_limit) {

    if (message_length < minimal_length)
        return false;

    size_t i = start_position;

    size_t count = 0;
    while (i < message_length && buf[i] != 0x00 && count < first_limit) {
        i++;
        count++;
    }

    if (i >= message_length || buf[i] != 0x00 || count < 1) {
        return false;
    }
    ++i;

    count = 0;
    while (i < message_length && buf[i] != 0x00 && count < second_limit) {
        ++i;
        ++count;
    }

    if (i >= message_length || buf[i] != 0x00 || count < 1) {
        return false;
    }

    ++i;

    if (amount_of_fields == 3) {
        count = 0;
        while (i < message_length && buf[i] != 0x00 && count < third_limit) {
            ++i;
            ++count;
        }

        if (i >= message_length || buf[i] != 0x00 || count < 1) {
            return false;
        }
    }

    return true;
}

std::string UDPhandler::read_channel_name(uint8_t *buf) {
    int i = 3;
    std::string channel;
    while (buf[i] != 0x00) {
        channel.push_back(static_cast<char>(buf[i]));
        i++;
    }
    return channel;
}

void UDPhandler::change_display_name(uint8_t *buf, bool second) {
    this->display_name.clear();
    int i = 3;
    if (second) {
        while (buf[i] != 0x00)
            i++;
    }
    i++;
    while (buf[i] != 0x00) {
        this->display_name.push_back(static_cast<char>(buf[i]));
        i++;
    }
}

int UDPhandler::convert_from_tcp(uint8_t *buf, uint8_t *tcp_buf) {

    std::string message;
    int i = 0;
    while (tcp_buf[i] != 0x0d) {
        message.push_back(static_cast<char>(tcp_buf[i]));
        i++;
    }

    std::regex patternFromToIs(R"(FROM\s(.*?)\sIS)");
    std::smatch matchFromToIs;
    std::regex_search(message, matchFromToIs, patternFromToIs);
    std::string name = matchFromToIs[1].str();

    std::regex patternAfterIs(R"(IS\s(.*))");
    std::smatch matchAfterIs;
    std::regex_search(message, matchAfterIs, patternAfterIs);
    std::string msg = matchAfterIs[1].str();

    return this->create_message(buf, msg, false, name);
}

void logger(sockaddr_in client, const char *type, const char *operation) {
    std::cout << operation << " " << inet_ntoa(client.sin_addr) << ":" << ntohs(client.sin_port) << " | " << type
              << std::endl;
}

bool UDPhandler::username_exists(uint8_t *buf, synch *synch_vars) {
    std::string username;
    int i = 3;
    while (buf[i] != 0x00) {
        username.push_back(static_cast<char>(buf[i]));
        i++;
    }

    if (!synch_vars->usernames.empty()) {
        if (synch_vars->usernames.find(username) != synch_vars->usernames.end())
            return true;
    }
    synch_vars->usernames.insert(username);
    this->user_n = username;
    return false;
}