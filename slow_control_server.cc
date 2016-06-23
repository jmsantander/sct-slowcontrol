// slow_control_server.cc
// Receive data from the Raspberry Pi and transmit to the GUI

#include <iostream>
#include <string>

#include "sc_network.h"

int main(void)
{
    // Set up networking info
    Network_info netinfo;
    
    // Connect to the Pi and GUI
    if (!(setup_network(netinfo, SERVER))) {
        return 1;
    }

    // Update network
    while (true) {
        update_network(netinfo);
    }
    
    // Shut down network
    if (!shutdown_network(netinfo)) {
        return 1;
    }

    return 0;
}
