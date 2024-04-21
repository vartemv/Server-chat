//
// Created by artem on 4/19/24.
//

#ifndef IPK_SERVER_ARGUMENTSHANDLER_H
#define IPK_SERVER_ARGUMENTSHANDLER_H

#include <iostream>
#include <cstring>

class ArgumentsHandler {
public:
    void get_args(int argc, char *argv[]);

    int get_timeout();

    int get_retransmissions();

    int get_port() const;

    char *get_address();

private:
    int timeout;
    int port;
    char *address;
    int retransmissions;

    static void print_help();
};


#endif //IPK_SERVER_ARGUMENTSHANDLER_H
