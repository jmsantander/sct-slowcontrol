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

    // Make a Backplane object
    Backplane backplane;
    
    // Set up networking info
    Network_info netinfo(PI, hostname);

    // Communicate with the server: on each loop send updated data and 
    // recieve updated settings
    std::cout << "communicating with the server...\n";
    int i = 0, j = 0;
    while (true) {
        // Send and receive messages
        backplane.synchronize_network(netinfo);
        if (i % 200 == 0) {
            // Set data to some numbers
            backplane.update_data(i, j);
            // Apply new settings
            //backplane.apply_settings();
            // Display updated values
            backplane.print_info();
            usleep(2000);
        }
        if (i >= 10000) {
            i = 0;
            j++;
        }
        i++;
    }

    // Shut down network
    if (!shutdown_network(netinfo))
        return 1;
    
    return 0;
}
