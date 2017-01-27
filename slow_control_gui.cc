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
    int new_settings = BP_NONE;
    while (true) {
        new_settings = BP_NONE;
        // Read in a command from the user
        read_command(command, value);
        // Execute the command
        if (command.compare("e") == 0) {
            // Exit the GUI
            // Shut down network
            std::cout << "Exit." << std::endl;
            if (!shutdown_network(netinfo))
                return 1;
            break;
        } else if (command.compare("v") == 0) {
            // Read FEE housekeeping voltages
            std::cout << "Read voltages." << std::endl;
            new_settings = BP_VOLTAGES;
            backplane.update_settings(new_settings);
            // Send new settings
            backplane.synchronize_network(netinfo);
        } else {
            std::cout << "Command not recognized." << std::endl;
            continue;
        }
        // Get results of command
        sleep_msec(75);
        backplane.synchronize_network(netinfo);
        // Display updated values
        backplane.print_data(new_settings);
    }

    return 0;
}
