#include "server_classes.h"
#include "ArgumentsHandler.h"

int pipefd[2];

void init(struct sockaddr_in *server_addr, int port, const char *addr);

void handle_sigint(int sig);

int main(int argc, char *argv[]) {

    ArgumentsHandler ah{};
    ah.get_args(argc, argv);

    ThreadPool tp{20};
    std::stack<UserInfo> s;
    synch synch_variables(0);
    struct sockaddr_in *server_addr = new sockaddr_in;

    init(server_addr, ah.get_port(), ah.get_address());

    signal(SIGINT, handle_sigint);
    pipe(pipefd);

    UDPserver udp{ah.get_retransmissions(), ah.get_timeout()};
    TCPserver tcp{};

    udp.Initialize(server_addr);
    tcp.Initialize(server_addr);

    std::thread tcpThread(&TCPserver::Listen, &tcp, &tp, &s, &synch_variables, pipefd[0]);
    std::thread udpThread(&UDPserver::Listen, &udp, &tp, &s, &synch_variables, pipefd[0]);

    tcpThread.join();
    udpThread.join();

    tp.Shutdown();

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

void handle_sigint(int sig) {
    write(pipefd[1], "X", 1);
}


