//
// Created by artem on 4/14/24.
//
#include <arpa/inet.h>
#include "UDPhandler.h"

void UDPhandler::handleUDP(uint8_t *buf, sockaddr_in client_addr, int length, int retransmissions, int timeout,
                           std::vector<sockaddr_in> *addresses) {
    //std::cout << "Running thread with id: " << std::this_thread::get_id() << std::endl;
    UDPhandler udp(retransmissions, timeout);

    sockaddr_in local_copy = client_addr;
    uint8_t internal_buf[1024];
    udp.respond_to_auth(buf, length, local_copy);
    while (true) {
//        for(const auto& addr : *addresses){
//            std::cout << "Running thread with id: " << std::this_thread::get_id() << std::endl;
//            std::cout << "------------------------- " << std::this_thread::get_id() << std::endl;
//            std::cout << "Family: " << addr.sin_family << '\n';
//            std::cout << "Port: " << ntohs(addr.sin_port) << '\n';  // ntohs function is used to convert a uint16_t from TCP/IP network byte order to host byte order
//            std::cout << "Address: " << inet_ntoa(addr.sin_addr) << '\n';
//            std::cout << "------------------------- " << std::this_thread::get_id() << std::endl;
//        }
        std::cout << "Running thread with id: " << std::this_thread::get_id() << std::endl;
        int length_internal = udp.wait_for_the_incoming_connection(local_copy, internal_buf);
        std::cout << "Running thread after with id: " << std::this_thread::get_id() << std::endl;
        if (!udp.decipher_the_message(internal_buf, local_copy, length_internal)) {
            break;
        }
    }
}

bool UDPhandler::decipher_the_message(uint8_t *buf, sockaddr_in client_addr, int length) {
    if (!this->auth) {
        if (buf[0] != 0x02) {
            if (buf[0] != 0x0FF) {
                std::cout << "Client have to sign in before doing anything" << std::endl;
                return true;
            }
        }
    }

    switch (buf[0]) {
        case 0x00://CONFIRM
            break;
        case 0x02://AUTH
            if (!this->auth)
                respond_to_auth(buf, length, client_addr);
            else
                std::cout << "Already authed" << std::endl;
            break;
        case 0x03://JOIN
            break;
        case 0x04://MSG
            break;
        case 0xFF://BYE
            std::cout << "Got bye" << std::endl;
            send_confirm(buf, client_addr);
            return false;
        case 0xFE://ERR
            return false;
    }
    return true;
}

void UDPhandler::respond_to_auth(uint8_t *buf, int message_length, sockaddr_in client_addr) {

    bool valid_message = true;

    if (buf[0] != 0x02) {
        std::cout << "Client have to sign in before doing anything" << std::endl;
        return;
    }

    if (message_length < 5) {
        std::cout << "Problem with length" << std::endl;
        valid_message = false;
    }
    size_t i = 3;
    size_t count = 0;
    while (i < message_length && buf[i] != 0x00 && count < 20) {
        i++;
        count++;
    }

    if (i >= message_length || buf[i] != 0x00 || count < 1)
        valid_message = false;

    ++i;

    count = 0;
    while (i < message_length && buf[i] != 0x00 && count < 20) {
        ++i;
        ++count;
    }

    if (i >= message_length || buf[i] != 0x00 || count < 1)
        valid_message = false;

    ++i;

    count = 0;
    while (i < message_length && buf[i] != 0x00 && count < 5) {
        ++i;
        ++count;
    }

    if (i >= message_length || buf[i] != 0x00 || count < 5)
        valid_message = false;

    if (valid_message) {
        for (size_t i = 3; i < message_length - 1; ++i) {
            std::cout << static_cast<char>(buf[i]);
            if (buf[i] == 0x00) {
                std::cout << ": ";
            }
        }
        std::cout << std::endl;

        send_confirm(buf, client_addr);
        std::string success = "Authentication is succesful";
        send_reply(buf, client_addr, success, true);
        this->auth = true;
    } else {
        send_confirm(buf, client_addr);
        std::string failure = "Authentication is not succesful";
        send_reply(buf, client_addr, failure, false);
    }
}

void UDPhandler::send_confirm(uint8_t *buf, sockaddr_in client_addr) {
    uint8_t buf_out[4];

    ConfirmPacket confirm(0x00, this->global_counter, read_packet_id(buf));

    int len = confirm.construct_message(buf_out);

    socklen_t address_size = sizeof(client_addr);

    long bytes_tx = sendto(this->client_socket, buf_out, len, 0, (struct sockaddr *) &(client_addr), address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");
}

void UDPhandler::send_reply(uint8_t *buf, sockaddr_in client_addr, std::string &message, bool OK) {
    uint8_t buf_out[1024];

    socklen_t address_size = sizeof(client_addr);

    int previous = this->global_counter;
    ReplyPacket reply(0x01, this->global_counter, message, OK ? 1 : 0, read_packet_id(buf));
    this->global_counter++;

    int len = reply.construct_message(buf_out);
    long bytes_tx = sendto(this->client_socket, buf_out, len, 0, (struct sockaddr *) &(client_addr), address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");

    if (!waiting_for_confirm(previous, buf_out, len, client_addr))
        std::cout << "Client didn't confirm" << std::endl;
}

int UDPhandler::read_packet_id(uint8_t *buf) {
    int result = buf[1] << 8 | buf[2];
    return ntohs(result);
}

int UDPhandler::wait_for_the_incoming_connection(sockaddr_in client_addr, uint8_t *buf_out, int timeout) {
    int event_count = epoll_wait(this->epoll_fd, this->events, 1, timeout);

    if (event_count == -1) {
        perror("epoll_wait");
        close(this->epoll_fd);
        exit(EXIT_FAILURE);
    } else if (event_count > 0) {
        socklen_t len_client = sizeof(client_addr);
        for (int j = 0; j < event_count; j++) {
            if (events[j].events & EPOLLIN) { // check if EPOLLIN event has occurred
                int n = recvfrom(this->client_socket, buf_out, 1024, 0, (struct sockaddr *) &client_addr,
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

bool UDPhandler::waiting_for_confirm(int count, uint8_t *buf, int len, sockaddr_in client_addr) {
    uint8_t buffer[1024];
    bool confirmed = false;
    for (int i = 0; i < this->retransmissions; ++i) {
        if (this->wait_for_the_incoming_connection(client_addr, buffer, this->timeout_chat)) {
            if (buffer[0] == 0x00 && read_packet_id(buffer) == count) {
                confirmed = true;
            }
        }
        if (confirmed)
            break;
        else {
            socklen_t len_client = sizeof(client_addr);
            long bytes_tx = sendto(this->client_socket, buf, len, 0, (struct sockaddr *) &(client_addr),
                                   len_client);
            if (bytes_tx < 0) perror("ERROR: sendto");
        }
    }
    return confirmed;
}