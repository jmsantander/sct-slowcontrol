// sc_backplane.cc
// Implementation of classes for controlling and updating backplane info

#include <iostream>
#include <string>

#include "sc_backplane.h"
#include "sc_network.h"
#include "sc_protobuf.pb.h"

Backplane::Backplane()
{
    // Verify that the version of the Protocol Buffer library we linked against
    // is compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    voltage_ = 0.0;
    current_ = 0.0;
    desired_voltage_ = 0.0;
    desired_current_ = 0.0;
}

void Backplane::print_info()
{
    std::cout << "updated voltage is: " << voltage_ << std::endl;
    std::cout << "updated current is: " << current_ << std::endl;
    std::cout << "updated desired voltage is: " << desired_voltage_
        << std::endl;
    std::cout << "updated desired current is: " << desired_current_
        << std::endl;
}

void Backplane::update_data(float voltage, float current)
{
    voltage_ = voltage;
    current_ = current;
    
    data_buffer.set_voltage(voltage_);
    data_buffer.set_current(current_);
}

void Backplane::update_settings(float desired_voltage,
        float desired_current)
{
    desired_voltage_ = desired_voltage;
    desired_current_ = desired_current;

    settings_buffer.set_desired_voltage(desired_voltage_);
    settings_buffer.set_desired_current(desired_current_);
}
    
bool Backplane::update_from_network(Network_info &netinfo)
{
    switch (netinfo.device) {
        case PI:
        {
            // Send data to and receive settings from server
            std::string data_message;
            data_buffer.SerializeToString(&data_message);
            if (!update_network(netinfo, data_message)) {
                return false;
            }
            // Store received settings
            std::vector<Connection>::iterator iter;
            for (iter = netinfo.connections.begin();
                    iter != netinfo.connections.end(); ++iter) {
                if ((iter->device == SERVER) &&
                        (iter->recv_status == MSG_DONE)) {
                    if (!settings_buffer.ParseFromString(iter->message)) {
                        return false;
                    }
                    desired_voltage_ = settings_buffer.desired_voltage();
                    desired_current_ = settings_buffer.desired_current();
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
            std::string settings_message;
            settings_buffer.SerializeToString(&settings_message);
            if (!update_network(netinfo, settings_message)) {
                return false;
            }
            // Store received data
            std::vector<Connection>::iterator iter;
            for (iter = netinfo.connections.begin();
                    iter != netinfo.connections.end(); ++iter) {
                if ((iter->device == SERVER) &&
                        (iter->recv_status == MSG_DONE)) {
                    if (!data_buffer.ParseFromString(iter->message)) {
                        return false;
                    }
                    voltage_ = data_buffer.voltage();
                    current_ = data_buffer.current();
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
