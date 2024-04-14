#include "server_classes.h"

int main() {
    UDPserver udp = UDPserver(3, 500);
    udp.Initialize();
    udp.Listen();
}
