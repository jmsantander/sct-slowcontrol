// slow_control_pi.cc
/* Listen for and execute remote user commands for the backplane. Continually
 * send out a status report back to the user for display by the GUI. */

#include <iostream>
#include <string>
#include <unistd.h>

#include "sc_network.h"
#include "sc_backplane.h"

int main(int argc, char *argv[])
{
    // Parse command line arguments
    if (argc != 2) {
        std::cerr << "usage: slow_control_pi hostname" << std::endl;
        return 1;
    }
    std::string hostname = argv[1];

    // Make a Backplane object and initialize for low level communication
    Backplane backplane;
    if (!backplane.pi_initialize_lowlevel()) {
        std::cerr << "error: could not initialize low level backplane" 
            << std::endl;
        return 1;
    }
    
    // Set up networking info
    Network_info netinfo(PI, hostname);

    // Communicate with the server: on each loop send updated data and 
    // recieve updated settings
    std::cout << "communicating with the server...\n";
    // Send and receive messages
    backplane.synchronize_network(netinfo);
    while (true) {
        // Apply new settings
        backplane.apply_settings();
        // Send and receive messages
        backplane.synchronize_network(netinfo);
        // Display data
        if (backplane.requested_updates() != BP_NONE) {
            backplane.print_data();
        }
    }

    // Shut down network
    if (!shutdown_network(netinfo))
        return 1;
    
    return 0;
}
