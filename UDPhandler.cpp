//
// Created by artem on 4/14/24.
//



#include "UDPhandler.h"

void UDPhandler::handleUDP(uint8_t *buf, sockaddr_in client_addr, int length, int retransmissions, int timeout) {
    std::cout << "Running thread with id: " << std::this_thread::get_id() << std::endl;
    UDPhandler udp(retransmissions, timeout);

    if (!udp.respond_to_auth(buf, length, client_addr)) {
        exit(EXIT_FAILURE);
        //terminate_connection();
    }

//    while(true){
//        if(!decipher_the_message(buf))
//            break;
//    }

    //terminate_connection();

}

bool UDPhandler::decipher_the_message(uint8_t *buf) {
    switch (buf[0]) {
        case 0x00://CONFIRM
            break;
        case 0x02://AUTH
            //send_err();
            std::cout << "already authed" << std::endl;
            break;
        case 0x03://JOIN
            break;
        case 0x04://MSG
            break;
        case 0xFF://BYE
            return false;
        case 0xFE://ERR
            return false;
    }
    return true;
}

bool UDPhandler::respond_to_auth(uint8_t *buf, int message_length, sockaddr_in client_addr) {
    if (buf[0] != 0x02)
        return false;

    for (size_t i = 3; i < message_length - 1; ++i) {
        std::cout << static_cast<char>(buf[i]);
        if (buf[i] == 0x00) {
            std::cout << ": ";
        }
    }
    std::cout << std::endl;

    send_confirm(buf, client_addr);
    std::string success = "Authentication is succesful";
    send_reply(buf, client_addr, success);

    return true;
}

void UDPhandler::send_confirm(uint8_t *buf, sockaddr_in client_addr) {
    uint8_t buf_out[4];

    ConfirmPacket confirm(0x00, this->global_counter, read_packet_id(buf));

    int len = confirm.construct_message(buf_out);

    socklen_t address_size = sizeof(client_addr);

    long bytes_tx = sendto(this->client_socket, buf_out, len, 0, (struct sockaddr *) &(client_addr), address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");
}

void UDPhandler::send_reply(uint8_t *buf, sockaddr_in client_addr, std::string &message) {
    uint8_t buf_out[1024];

    socklen_t address_size = sizeof(client_addr);

    int previous = this->global_counter;
    ReplyPacket reply(0x01, this->global_counter, message, 1, read_packet_id(buf));
    this->global_counter++;

    int len = reply.construct_message(buf_out);
    long bytes_tx = sendto(this->client_socket, buf_out, len, 0, (struct sockaddr *) &(client_addr), address_size);
    if (bytes_tx < 0) perror("ERROR: sendto");

    //add_to_sent_messages();
    if (!waiting_for_confirm(previous, buf_out, len, client_addr))
        std::cout << "Client didn't confirm" << std::endl;
}

int UDPhandler::read_packet_id(uint8_t *buf) {
    int result = buf[1] << 8 | buf[2];
    return ntohs(result);
}

bool UDPhandler::waiting_for_confirm(int count, uint8_t *buf, int len, sockaddr_in client_addr) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "Failed to create epoll file descriptor\n";
        return false;
    }

    // setup epoll event
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = this->client_socket;

    // add socket file descriptor to epoll
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->client_socket, &ev) == -1) {
        std::cerr << "Failed to add file descriptor to epoll\n";
        close(epoll_fd);
        return false;
    }

    uint8_t buf_out[1024];
    bool confirmed = false;
    // buffer where events are returned
    epoll_event events[1];
    for (int i = 0; i < this->retransmissions; ++i) {
        int event_count = epoll_wait(epoll_fd, events, 1, this->timeout_chat);
        if(event_count == -1) {
            perror("epoll_wait");
            close(epoll_fd);
            return false;
        } else if(event_count == 0) {
            continue;
        } else {
            std::cout << event_count << " events have occurred.\n";
            socklen_t len_client = sizeof(client_addr);
            for(int j = 0; j < event_count; j++) {
                if(events[j].events & EPOLLIN) { // check if EPOLLIN event has occurred
                    int n = recvfrom(this->client_socket, buf_out, 1024, 0, (struct sockaddr*)&client_addr,
                                     &len_client);
                    if (n == -1) {
                        std::cerr << "recvfrom failed. errno: " << errno << '\n';
                        continue;
                    }
                    if (n > 0) {
                        std::cout<<"expected " << count << " real " << read_packet_id(buf_out) << std::endl;
                        if (buf_out[0] == 0x00 && read_packet_id(buf_out) == count) {
                            confirmed = true;
                            break;
                        }
                    }

                }
            }
        }
        if(confirmed)
            break;
        else{
            socklen_t len_client = sizeof(client_addr);
            long bytes_tx = sendto(this->client_socket, buf, len, 0, (struct sockaddr *) &(client_addr),
                                   len_client);
            if (bytes_tx < 0) perror("ERROR: sendto");
        }

    }
    if(!confirmed)
        return false;
    else
        return true;
}