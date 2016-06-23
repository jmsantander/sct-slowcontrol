// slow_control_pi.cc
/* Listen for and execute remote user commands for the backplane. Continually
 * send out a status report back to the user for display by the GUI. */

#include <iostream>
#include <string>
#include <unistd.h>
#include <cstdlib>

#include "sc_network.h"
#include "sc_backplane.h"

int main(int argc, char *argv[])
{
    // Parse command line arguments
    if (argc != 2) {
        fprintf(stderr, "usage: slow_control_pi hostname\n");
        return 1;
    }
    std::string hostname = argv[1];

    // Make a Backplane object
    Backplane backplane;
    
    // Set up networking info
    Network_info netinfo(PI, hostname);

    // Communicate with the server: on each loop send updated data and 
    // recieve updated settings
    std::cout << "communicating with the server...\n";
    float i = 0.0;
    while (true) {
        // Set data to some numbers
        backplane.update_data(i, -1*i);
        i = i + 0.01;
        // Send and receive messages
        backplane.update_from_network(netinfo);
        // Display updated values
        backplane.print_info();
    }

    // Shut down network
    if (!shutdown_network(netinfo))
        return 1;
    
    return 0;
}
