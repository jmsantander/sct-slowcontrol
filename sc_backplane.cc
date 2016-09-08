// sc_backplane.cc
// Implementation of classes for controlling and updating backplane info

#include <iostream>
#include <string>

#include "sc_backplane.h"
#include "sc_network.h"
#include "sc_lowlevel.h"
#include "sc_protobuf.pb.h"

Backplane::Backplane()
{
    // Verify that the version of the Protocol Buffer library we linked against
    // is compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    for (int i = 0; i < N_FEES; i++) {
        voltages_[i] = 0.0;
        data_buffer.add_voltage(0.0);
    }

    updates_to_send = false;
}

void Backplane::update_data(int requested_updates)
{
    if (requested_updates == BP_NONE) {
        return;
    } else if (requested_updates == BP_VOLTAGES) {
        read_voltages(voltages_, N_FEES);
        //for (int i = 0; i < N_FEES; i++) {
        //    voltages_[i] += 1;
        //    data_buffer.set_voltage(i, voltages_[i]);
        //}
    }
    
    updates_to_send = true;
}

void Backplane::update_settings(int requested_updates)
{
    requested_updates_ = requested_updates;
    settings_buffer.set_requested_updates(requested_updates_);

    updates_to_send = true;
}
    
bool Backplane::synchronize_network(Network_info &netinfo)
{
    switch (netinfo.device) {
        case PI:
        {
            // Send data to and receive settings from server
            if (updates_to_send) {
                std::string data_message;
                data_buffer.SerializeToString(&data_message);
                if (!update_network(netinfo, data_message)) {
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
                    if (!settings_buffer.ParseFromString(iter->message)) {
                        return false;
                    }
                    requested_updates_ = settings_buffer.requested_updates();
                }
            }
            break;
        }
        case SERVER:
        {
            if (!update_network(netinfo)) {
                return false;
            }
            break;
        }
        case GUI:
        {
            // Send settings to and receive data from server
            if (updates_to_send) {
                std::string settings_message;
                settings_buffer.SerializeToString(&settings_message);
                if (!update_network(netinfo, settings_message)) {
                    return false;
                }
                updates_to_send = false;
                requested_updates_ = BP_NONE;
            } else {
                if (!update_network(netinfo)) {
                    return false;
                }
            }
            // Store received data
            std::vector<Connection>::iterator iter;
            for (iter = netinfo.connections.begin();
                    iter != netinfo.connections.end(); ++iter) {
                if ((iter->device == SERVER) &&
                        (iter->recv_status == MSG_DONE)) {
                    iter->recv_status = MSG_STANDBY;
                    if (!data_buffer.ParseFromString(iter->message)) {
                        return false;
                    }
                    for (int i = 0; i < N_FEES; i++) {
                        voltages_[i] = data_buffer.voltage(i);
                    }
                }
            }
            break;
        }
        default:
        {
            return false;
        }
    }
    return true;
}

bool Backplane::pi_initialize_lowlevel()
{
    return initialize_lowlevel();
}
    
void Backplane::apply_settings()
{
    update_data(requested_updates_);
    requested_updates_ = BP_NONE;
}

void Backplane::print_data()
{
    std::cout << "Voltages:" << std::endl;
    for (int i = 0; i < N_FEES; i++) {
        std::cout << i << ": " << voltages_[i] << std::endl;
    }
}
