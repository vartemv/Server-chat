#include "server_classes.h"
#include "ArgumentsHandler.h"

void init(struct sockaddr_in *server_addr, int port, const char *addr);

int main(int argc, char *argv[]) {

    ArgumentsHandler ah{};
    ah.get_args(argc, argv);

    ThreadPool tp{20};
    std::stack<UserInfo> s;
    synch synch_variables(0);
    struct sockaddr_in *server_addr = new sockaddr_in;

    init(server_addr, ah.get_port(), ah.get_address());

    UDPserver udp{ah.get_retransmissions(), ah.get_timeout(), ah.get_port(), ah.get_address()};
    TCPserver tcp{ah.get_port(), ah.get_address()};

    udp.Initialize(server_addr);
    tcp.Initialize(server_addr);

    std::thread tcpThread(&TCPserver::Listen, &tcp, &tp, &s, &synch_variables);
    std::thread udpThread(&UDPserver::Listen, &udp, &tp, &s, &synch_variables);

    tcpThread.join();
    udpThread.join();

    tcp.Destroy();
    udp.Destroy();

    delete server_addr;
}

void init(struct sockaddr_in *server_addr, int port, const char *addr) {
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    server_addr->sin_addr.s_addr = inet_addr(addr);
}