// sc_backplane.cc
// Implementation of classes for controlling and updating backplane info

#include <iostream>
#include <iomanip>
#include <string>

#include "sc_backplane.h"
#include "sc_network.h"
#include "sc_lowlevel.h"
#include "sc_logistics.h"
#include "sc_protobuf.pb.h"

Backplane::Backplane()
{
    // Verify that the version of the Protocol Buffer library we linked against
    // is compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    for (int i = 0; i < N_FEES; i++) {
        voltages_[i] = 0.0;
        data_buffer.add_voltage(0.0);
        currents_[i] = 0.0;
        data_buffer.add_current(0.0);
    }
    
    requested_updates_ = BP_NONE;
    updates_to_send = false;
}

void Backplane::update_data(int requested_updates, bool simulation_mode)
{
    switch(requested_updates) {
        case BP_VOLTAGES:
        case BP_CURRENTS:
        {
            float fee_buffer[N_FEES];
            if (!simulation_mode) {
                read_fee_data(requested_updates, fee_buffer);
            } else {
                simulate_fee_data(fee_buffer, N_FEES);
            }
            for (int i = 0; i < N_FEES; i++) {
                if (requested_updates == BP_VOLTAGES) {
                    voltages_[i] = fee_buffer[i];
                    data_buffer.set_voltage(i, voltages_[i]);
                } else if (requested_updates == BP_CURRENTS) {
                    currents_[i] = fee_buffer[i];
                    data_buffer.set_current(i, currents_[i]);
                }
            }
            break;
        }
        default:
            return;
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
                    std::cout << "Received settings.\n"; //TESTING
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
                    std::cout << "Updating data..." << std::endl;
                    for (int i = 0; i < N_FEES; i++) {
                        voltages_[i] = data_buffer.voltage(i);
                        currents_[i] = data_buffer.current(i);
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
    
void Backplane::apply_settings(bool simulation_mode)
{
    update_data(requested_updates_, simulation_mode);
    requested_updates_ = BP_NONE;
}

void Backplane::print_data(int data_type)
{
    switch(data_type) {
        case BP_NONE:
            return;
        case BP_VOLTAGES:
        case BP_CURRENTS:
        {
            float *data_buffer; 
            if (data_type == BP_VOLTAGES) {
                std::cout << std::endl << "FEE voltages:" << std::endl;
                data_buffer = voltages_;
            } else if (data_type == BP_CURRENTS) {
                std::cout << std::endl << "FEE currents:" << std::endl;
                data_buffer = currents_;
            }
            std::cout << std::fixed << std::setprecision(2);
            for (int i = 0; i < N_FEES; i++) {
                if (i == 0 || i == 28) {
                    std::cout << "      " << std::setw(5) << data_buffer[i] 
                        << " ";
                } else if (i == 3 || i == 31) {
                    std::cout << std::setw(5) << data_buffer[i] << "      ";
                } else {
                    std::cout << std::setw(5) << data_buffer[i] << " ";
                }
                if (i == 3 || i == 9 || i == 15 || i == 21 || i == 27 || 
                        i == 31) {
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
            break;
        }
    }
}
