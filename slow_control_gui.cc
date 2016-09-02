// slow_control_gui.cc
// Receive data from and transmit settings to the server

#include <iostream>
#include <string>
#include <unistd.h>

#include "sc_network.h"
#include "sc_backplane.h"
#include "sc_logistics.h"

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
    // Send and receive messages
    backplane.synchronize_network(netinfo);
    std::string command, value;
    int i = 0;
    while (true) {
        // Read in a command from the user
        read_command(command, value);
        // Execute the command
        if (command.compare("exit") == 0) {
            // Exit the GUI
            // Shut down network
            if (!shutdown_network(netinfo))
                return 1;
            break;
        }
        backplane.update_settings(i, atoi(value.c_str()));
        i++;
        // Send and receive messages
        backplane.synchronize_network(netinfo);
        // Display updated values
        backplane.print_info();
    }

    return 0;
}
