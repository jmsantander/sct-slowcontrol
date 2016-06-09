// slow_control_server.cc
// Receive data from the Raspberry Pi and transmit to the GUI

#include <iostream>
#include <string>

#include "sc_network.h"
#include "sc_protobuf.pb.h"

int main(int argc, char *argv[])
{
    // Verify that the version of the library we linked against is compatible
    // with the version of the headers we compiled against
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Parse command line arguments
    if (argc != 2) {
        fprintf(stderr, "usage: slow_control_server hostname\n");
        return 1;
    }
    char *hostname = argv[1];

    // Set up protocol buffer
    slow_control::Backplane backplane;
    
    // Connect to the Pi
    std::cout << "connecting to the pi...\n";
    int pi = -1; 
    if (!open_connection(pi, hostname))
        return 1;

    // Receive messages from the Pi
    std::cout << "receiving messages from the pi...\n";
    std::string message;
    while(true) {
        if (!receive_message(pi, message)) {
            // End communication to the Pi when no more messages
            std::cout << "breaking the connection.\n";
            close_connection(pi);
            break;
        }
        if (!backplane.ParseFromString(message))
            return 1;
        std::cout << "updated voltage is: " << backplane.voltage() 
            << std::endl;
        std::cout << "updated current is: " << backplane.current() 
            << std::endl;
    }

    return 0;
}
