// tm_control.cc
// Implementation of the class for mid-level target control

#include <iostream>
#include <string>
#include <vector>

#include "tm_control.h"

bool TMControl::synchronize_network()
{
    // Send data to and receive settings from server
    if (updates_to_send) {
        std::string target_variables_message;
        target_variables.SerializeToString(&target_variables_message);
        if (!update_network(netinfo, target_variables_message)) {
            return false;
        }
        updates_to_send = false;
    } else {
        if (!update_network(netinfo)) {
            return false;
        }
    }
    // Store received settings
    std::vector<Connection>::iterator iter;
    for (iter = netinfo.connections.begin();
            iter != netinfo.connections.end(); ++iter) {
        if ((iter->device == SERVER) &&
                (iter->recv_status == MSG_DONE)) {
            iter->recv_status = MSG_STANDBY;
            if (!target_command.ParseFromString(iter->message)) {
                return false;
            }
            std::cout << "Received command." << std::endl; 
        }
    }
    return true;
}

// Return whether or not a command was received from the server
bool TMControl::command_received()
{
    for (std::vector<Connection>::iterator iter =
            netinfo.connections.begin(); iter != netinfo.connections.end();
            ++iter) {
        if ((iter->device == SERVER) && (iter->recv_status == MSG_DONE)) {
            return true;
        }
    }
    return false;
}

// Save changes to target module variables for logging and display
void TMControl::save_updated_variables()
{
    std::cout << "Functionality to save variables is not yet implemented!!"
        << std::endl;
    updates_to_send = true;
}
