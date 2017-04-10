// slow_control_pi.cc
/* Listen for and execute remote user commands for the backplane. Continually
 * send out a status report back to the user for display by the GUI. */

#include <iostream>
#include <string>

#include "picontrol.h"

int main(int argc, char *argv[])
{
    // Parse command line arguments
    if (argc != 2) {
        std::cerr << "usage: slow_control_pi hostname" << std::endl;
        return 1;
    }
    std::string hostname = argv[1];

    PiControl pi_control(hostname);
    
    // Communicate with the server: on each loop send updated data and 
    // receive updated settings
    std::cout << "communicating with the server..." << std::endl;
    while (true) {
        // Receive commands and return backplane variables
        pi_control.synchronize_network();
        // Apply new settings, if a command was received
        pi_control.update_backplane_variables();
    }

    return 0;
}
