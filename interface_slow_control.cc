// interfacecontrol.h
// Implementation for the class for the high-level user interface

#include <iostream>

#include "interfacecontrol.h"
#include "sc_network.h"
#include "sc_protobuf.pb.h"

bool InterfaceControl::synchronize_network()
{
    // Send settings to and receive data from server
    if (updates_to_send) {
        std::string run_settings_message;
        run_settings.SerializeToString(&run_settings_message);
        if (!update_network(netinfo, run_settings_message)) {
            return false;
        }
        updates_to_send = false;
    } else {
        if (!update_network(netinfo)) {
            return false;
        }
    }
    // Store received data
    std::vector<Connection>::iterator iter;
    // default: no message received
    message_received = slow_control::MessageWrapper::NONE;
    for (iter = netinfo.connections.begin();
            iter != netinfo.connections.end(); ++iter) {
        if ((iter->device == SERVER) &&
                (iter->recv_status == MSG_DONE)) {
            iter->recv_status = MSG_STANDBY;
            if (!message_wrap.ParseFromString(iter->message)) {
                return false;
            } else {
                std::cout << "Updating data..." << std::endl;
                switch (message_wrap.type()) {
                    case slow_control::MessageWrapper::BP_VARS:
                        backplane_variables =
                            message_wrap.backplane_variables();
                        message_received =
                            slow_control::MessageWrapper::BP_VARS;
                        break;
                    case slow_control::MessageWrapper::TM_VARS:
                        target_variables = message_wrap.target_variables();
                        message_received =
                            slow_control::MessageWrapper::TM_VARS;
                        break;
                    default:
                        std::cout << "Warning: data type not backplane "
                            << "or target variables, cannot read"
                            << std::endl;
                        break;
                }
            }
        }
    }
    return true;
}

void InterfaceControl::update_highlevel_command(std::string highlevel_command,
        std::string highlevel_parameter)
{
    run_settings.set_highlevel_command(highlevel_command);
    run_settings.set_highlevel_parameter(highlevel_parameter);
    updates_to_send = true;
}
