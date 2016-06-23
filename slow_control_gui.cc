// slow_control_gui.cc
// Receive data from and transmit settings to the server

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
        fprintf(stderr, "usage: slow_control_gui hostname\n");
        return 1;
    }
    std::string hostname = argv[1];
    
    // Make a Backplane object
    Backplane backplane;
    
    // Set up networking info
    Network_info netinfo(hostname);

    // Connect to the server
    if (!setup_network(netinfo, GUI))
        return 1;

    // Communicate with the server: on each loop receive updated data and 
    // send updated settings
    std::cout << "communicating with the server...\n";
    std::string message;
    float i = 0.0;
    while (true) {
        // Set settings to some numbers
        backplane.update_settings(i, -1*i);
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
