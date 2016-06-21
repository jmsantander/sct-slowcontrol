// slow_control_server.cc
// Receive data from the Raspberry Pi and transmit to the GUI

#include <iostream>
#include <string>

#include "sc_network.h"
#include "sc_protobuf.pb.h"

int main(void)
{
    // Verify that the version of the library we linked against is compatible
    // with the version of the headers we compiled against
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Set up protocol buffer
    slow_control::Backplane backplane;
    
    // Set up networking info
    Network_info netinfo;
    
    // Connect to the Pi and GUI
    if (!(setup_network(netinfo, SERVER))) {
        return 1;
    }

    // Update network
    for (int i = 0; i < 10; i++) {
        if (!(update_network(netinfo))) {
            return 1;
        }
    }
    
    // Shut down network
    if (!shutdown_network(netinfo)) {
        return 1;
    }

    
    return 0;
}
