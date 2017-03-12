// slow_control_server.cc
// Receive data from the Raspberry Pi and transmit to the GUI

#include <iostream>
#include <string>
#include <vector>

#include "sc_network.h"
#include "sc_logistics.h"

int main(void)
{
    // Set up networking info
    Network_info netinfo(SERVER);

    // Get database password from user
    get_database_credentials();
    
    // Update network
    while (true) {
        update_network(netinfo);
        // If the server received a (data) message from the Pi, log it
        for (std::vector<Connection>::iterator iter =
                netinfo.connections.begin(); iter != netinfo.connections.end();
                ++iter) {
            if ((iter->device == PI) && (iter->recv_status == MSG_DONE)) {
                log_data_message(iter->message);
            }
        }
    }
    
    // Shut down network
    if (!shutdown_network(netinfo)) {
        return 1;
    }

    return 0;
}
