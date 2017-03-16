// slow_control_tm.cc
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
        std::cerr << "usage: slow_control_tm hostname" << std::endl;
        return 1;
    }
    std::string hostname = argv[1];

    // Make a Backplane object and initialize for low level communication
    Backplane backplane;
    
    // Set up networking info
    Network_info netinfo(TM, hostname);

    // Communicate with the server: on each loop send updated data and 
    // receive updated settings
    std::cout << "communicating with the server...\n";
    while (true) {
        // Send and receive messages
        backplane.synchronize_network(netinfo);
    }

    // Shut down network
    if (!shutdown_network(netinfo))
        return 1;
    
    return 0;
}
