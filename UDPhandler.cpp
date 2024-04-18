//
// Created by artem on 4/14/24.
//
#include <arpa/inet.h>
#include <sstream>
#include "UDPhandler.h"

void UDPhandler::handleUDP(uint8_t *buf, sockaddr_in client_addr, int length, int retransmissions, int timeout,
                           std::vector<sockaddr_in> *addresses, int *busy, std::stack<UserInfo> *s, synch *synch_var) {
    //std::cout << "Running thread with id: " << std::this_thread::get_id() << std::endl;
    UDPhandler udp(retransmissions, timeout, client_addr);

    bool end = false;

    std::thread sender(read_queue, s, &end, synch_var, busy, udp);

    uint8_t internal_buf[1024];
    udp.send_confirm(buf);
    udp.respond_to_auth(buf, length, s, synch_var);
    while (true) {
        int length_internal = udp.wait_for_the_incoming_connection(internal_buf);
        if (!udp.decipher_the_message(internal_buf, length_internal, s, synch_var)) {
            break;
        }
    }

    //TODO Create semaphore
    auto newEnd = std::remove_if(addresses->begin(), addresses->end(),
                                 [&client_addr](sockaddr_in &addr) {
                                     return addr.sin_port == client_addr.sin_port &&
                                            addr.sin_addr.s_addr ==
                                            client_addr.sin_addr.s_addr;
                                 });
    addresses->erase(newEnd, addresses->end());
    end = true;
    sender.join();
}

void read_queue(std::stack<UserInfo> *s, bool *terminate, synch *synch_vars, int *busy, UDPhandler udp) {
    while (!*terminate) {
        std::unique_lock<std::mutex> lock(synch_vars->mtx);
        synch_vars->cv.wait(lock, [&synch_vars] { return synch_vars->ready; });

        synch_vars->waiting.lock();
        UserInfo new_uf = s->top();
        synch_vars->finished++;
        synch_vars->waiting.unlock();

        if (new_uf.client.sin_port != udp.client_addr.sin_port) {
            udp.send_message(new_uf.buf, new_uf.length);
        }

        if (synch_vars->finished == *busy) {
            synch_vars->finished = 0;
            synch_vars->ready = false;
            s->pop();
        }

        lock.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        lock.lock();

    }
}

bool UDPhandler::decipher_the_message(uint8_t *buf, int length, std::stack<UserInfo> *s, synch *synch_var) {
    if (!this->auth) {
        if (buf[0] != 0x02) {
            if (buf[0] != 0xFF) {
                std::cout << "Client have to sign in before doing anything" << std::endl;
                return true;
            }
        }
    }

    switch (buf[0]) {
        case 0x00://CONFIRM
            break;
        case 0x02://AUTH
            send_confirm(buf);
            if (!this->auth)
                respond_to_auth(buf, length, s, synch_var);
            else
                std::cout << "Already authed" << std::endl;
            break;
        case 0x03://JOIN
            send_confirm(buf);
            respond_to_join(buf, length, s, synch_var);
            break;
        case 0x04://MSG
            send_confirm(buf);
            respond_to_message(buf, length, s, synch_var);
            break;
        case 0xFF://BYE
            std::cout << "Got bye" << std::endl;
            send_confirm(buf);
            return false;
        case 0xFE://ERR
            return false;
    }
    return true;
}

void UDPhandler::respond_to_auth(uint8_t *buf, int message_length, std::stack<UserInfo> *s, synch *synch_var) {

    bool valid_message = true;

    if (buf[0] != 0x02) {
        std::cerr << "Client have to sign in before doing anything" << std::endl;
        valid_message = false;
    }

    if (!this->buffer_validation(buf, message_length, 3, 7, 3))
        valid_message = false;

    if (valid_message) {

        this->change_display_name(buf, true);
        std::string success = "Authentication is succesful";
        send_reply(buf, success, true);

        std::stringstream ss;
        ss << this->display_name << " has joined ch.general.";
        std::string message = ss.str();
        uint8_t buf_message[1024];
        int length = this->create_message(buf_message, message, false);
        this->respond_to_message(buf_message, length, s, synch_var);

        this->auth = true;
    } else {
        std::string failure = "Authentication is not succesful";
        send_reply(buf, failure, false);
    }
}

void UDPhandler::respond_to_join(uint8_t *buf, int message_length, std::stack<UserInfo> *s, synch *synch_var) {
    bool valid = true;

    if(!this->buffer_validation(buf, message_length,3,2))
        valid = false;

    if(valid){
        this->change_display_name(buf, true);
        std::string success = "Join is succesful";
        send_reply(buf, success, true);

        std::stringstream ss;
        ss << this->display_name << " has left"<< this->channel_name;
        std::string message = ss.str();
        uint8_t buf_message[1024];
        int length = this->create_message(buf_message, message, false);
        this->respond_to_message(buf_message, length, s, synch_var);



    }else{
        std::string failure = "Join is not succesful";
        send_reply(buf, failure, false);
    }
}

void UDPhandler::respond_to_message(uint8_t *buf, int message_length, std::stack<UserInfo> *s, synch *synch_var) {
    bool valid_message = true;
    std::cout << "Got message" << std::endl;

    if (!this->buffer_validation(buf, message_length, 3, 2, 2, 20, 1400))
        valid_message = false;

    if (valid_message) {
        std::cout << "Sending" << std::endl;
        {
            std::lock_guard<std::mutex> lock(synch_var->mtx);
            s->emplace(this->client_addr, buf, message_length);
            synch_var->ready = true;
        }
        synch_var->cv.notify_all();
    } else {
        std::cout << "Invalid message" << std::endl;
    }
}


void UDPhandler::send_confirm(uint8_t *buf) {
    uint8_t buf_out[4];

    ConfirmPacket confirm(0x00, this->global_counter, read_packet_id(buf));

    int len = confirm.construct_message(buf_out);

    socklen_t address_size = sizeof(this->client_addr);

    long bytes_tx = sendto(this->client_socket, buf_out, len, 0, (struct sockaddr *) &(this->client_addr),
                           address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");
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
}

void UDPhandler::send_message(uint8_t *buf, int message_length) {
    socklen_t address_size = sizeof(this->client_addr);

    sockaddr_in backup = this->client_addr;

    sendto(this->client_socket, buf, message_length, 0, (struct sockaddr *) &this->client_addr, address_size);
    this->global_counter++;

    if (!waiting_for_confirm(buf, message_length))
        std::cout << "Client didn't confirm" << std::endl;

    this->client_addr = backup;
}

int UDPhandler::create_message(uint8_t *buf_out, std::string &msg, bool error) {
    std::string server = "Server";
    MsgPacket message(error ? 0xFE : 0x04, this->global_counter, msg, server);
    return message.construct_message(buf_out);
}

int UDPhandler::read_packet_id(uint8_t *buf) {
    int result = buf[1] << 8 | buf[2];
    return ntohs(result);
}

int UDPhandler::wait_for_the_incoming_connection(uint8_t *buf_out, int timeout) {
    int event_count = epoll_wait(this->epoll_fd, this->events, 1, timeout);

    if (event_count == -1) {
        perror("epoll_wait");
        close(this->epoll_fd);
        exit(EXIT_FAILURE);
    } else if (event_count > 0) {
        socklen_t len_client = sizeof(this->client_addr);
        for (int j = 0; j < event_count; j++) {
            if (events[j].events & EPOLLIN) { // check if EPOLLIN event has occurred
                int n = recvfrom(this->client_socket, buf_out, 1024, 0, (struct sockaddr *) &this->client_addr,
                                 &len_client);
                if (n == -1) {
                    std::cerr << "recvfrom failed. errno: " << errno << '\n';
                    continue;
                }
                if (n > 0) {
                    return n;
                }
            }
        }
    }
    return 0;
}

bool UDPhandler::waiting_for_confirm(uint8_t *buf, int len) {
    uint8_t buffer[1024];
    bool confirmed = false;
    for (int i = 0; i < this->retransmissions; ++i) {
        if (this->wait_for_the_incoming_connection(buffer, this->timeout_chat)) {
            if (buffer[0] == 0x00 && read_packet_id(buffer) == read_packet_id(buf)) {
                confirmed = true;
            }
        }
        if (confirmed)
            break;
        else {
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

        if (i >= message_length || buf[i] != 0x00 || count < third_limit) {
            return false;
        }
    }

    return true;
}

std::string read_channel_name(uint8_t *buf){
    int i = 3;
    std::string channel;
    while(buf[i] != 0x00){
        channel.push_back(static_cast<char>(buf[i]));
        i++;
    }
    return channel;
}

void UDPhandler::change_display_name(uint8_t *buf, bool second) {
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