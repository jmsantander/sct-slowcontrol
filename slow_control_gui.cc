// slow_control_gui.cc
// Receive data from and transmit settings to the server

#include <iostream>
#include <string>

#include "sc_network.h"
#include "sc_protobuf.pb.h"

int main(int argc, char *argv[])
{
    // Verify that the version of the Protocol Buffer library we linked against
    // is compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Parse command line arguments
    if (argc != 2) {
        fprintf(stderr, "usage: slow_control_gui hostname\n");
        return 1;
    }
    std::string hostname = argv[1];
    
    // Set up protocol buffer
    slow_control::Backplane backplane;

    // Set up networking info
    Network_info netinfo(hostname);

    // Connect to the server
    if (!setup_network(netinfo, GUI))
        return 1;

    // Shut down network
    if (!shutdown_network(netinfo))
        return 1;

    return 0;
}
