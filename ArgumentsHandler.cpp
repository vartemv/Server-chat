//
// Created by artem on 4/19/24.
//


#include "ArgumentsHandler.h"

void ArgumentsHandler::print_help() {
    std::cout << R"(|Argument | Default values | Type
____________________________________________________________________________________________________________
| -l      | 127.0.0.1      | IP address                | Server listening IP address for welcome sockets    |
| -p      | 47356          | uint16                    | Server listening port for welcome sockets          |
| -d      | 500            | uint16                    | UDP confirmation timeout                           |
| -r      | 3              | uint8                     | Maximum number of UDP retransmissions              |
| -n      | 20             | uint16                    | Maximum number of threads in the thread pool       |
| -h      |                |                           | Prints program help output and exits               |
)";
}

void ArgumentsHandler::get_args(int argc, char **argv) {
    this->timeout = 500;
    this->retransmissions = 3;
    this->port = 47356;
    this->address = new char[13];
    this->number_of_threads = 20;
    strcpy(address, "127.0.0.1");

    for (int i = 0; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h") {
            print_help();
            exit(0);
        } else if (arg == "-l") {
            i++;
            if (i < argc) {
                address = argv[i];
            } else {
                std::cout << "Nothing passed to address" << std::endl;
                exit(1);
            }
        } else if (arg == "-p") {
            i++;
            if (i < argc) {
                try {
                    port = std::stoi(argv[i]);
                } catch (std::invalid_argument &) {
                    std::cout << "Passed non-int value to port" << std::endl;
                    exit(1);
                }
            } else {
                std::cout << "Nothing passed to port" << std::endl;
                exit(1);
            }
        } else if (arg == "-d") {
            i++;
            if (i < argc) {
                try {
                    this->timeout = std::stoi(argv[i]);
                } catch (std::invalid_argument &) {
                    std::cout << "Passed non-int value to timeout" << std::endl;
                    exit(1);
                }
            } else {
                std::cout << "Nothing passed to timeout" << std::endl;
                exit(1);
            }
        } else if (arg == "-r") {
            i++;
            if (i < argc) {
                try {
                    this->retransmissions = std::stoi(argv[i]);
                } catch (std::invalid_argument &) {
                    std::cout << "Passed non-int value to retransmissions" << std::endl;
                    exit(1);
                }
            } else {
                std::cout << "Nothing passed to retransmissions" << std::endl;
                exit(1);
            }
        } else if (arg == "-n"){
            i++;
            if (i < argc) {
                try {
                    this->number_of_threads = std::stoi(argv[i]);
                } catch (std::invalid_argument &) {
                    std::cout << "Passed non-int value to number of threads" << std::endl;
                    exit(1);
                }
            } else {
                std::cout << "Nothing passed to number of threads" << std::endl;
                exit(1);
            }
        }
    }


}

int ArgumentsHandler::get_port() const {
    return this->port;
}

int ArgumentsHandler::get_retransmissions() {
    return this->retransmissions;
}

int ArgumentsHandler::get_timeout() {
    return this->timeout;
}

char *ArgumentsHandler::get_address() {
    return this->address;
}

int ArgumentsHandler::get_threads() {
    return this->number_of_threads;
}