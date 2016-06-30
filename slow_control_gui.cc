// slow_control_gui.cc
// Receive data from and transmit settings to the server

#include <iostream>
#include <string>
#include <unistd.h>

#include "sc_network.h"
#include "sc_backplane.h"

int main(int argc, char *argv[])
{
    // Parse command line arguments
    if (argc != 2) {
        std::cerr << "usage: slow_control_gui hostname" << std::endl;
        return 1;
    }
    std::string hostname = argv[1];
    
    // Make a Backplane object
    Backplane backplane;
    
    // Set up networking info
    Network_info netinfo(GUI, hostname);

    // Communicate with the server: on each loop receive updated data and 
    // send updated settings
    std::cout << "communicating with the server...\n";
    int i = 0, j = 0;
    while (true) {
        // Send and receive messages
        backplane.synchronize_network(netinfo);
        if (i % 200 == 0) {
            // Send and receive messages
            backplane.update_settings(i, j);
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
