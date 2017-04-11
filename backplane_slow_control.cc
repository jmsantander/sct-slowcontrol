// picontrol.cc
// File containing the implementation for the class for mid-level pi control

#include <iostream>
#include <algorithm>

#include "picontrol.h"

#include "sc_network.h"
#include "backplane_lowlevel.h"

// number of uint parameters to send to backplane low level code
const int NUM_COMMAND_PARAMETERS = 4; 

bool PiControl::synchronize_network()
{
    // Send data to and receive settings from server
    if (updates_to_send) {
        std::string backplane_variables_message;
        backplane_variables.SerializeToString(&backplane_variables_message);
        if (!update_network(netinfo, backplane_variables_message)) {
            return false;
        }
        updates_to_send = false;
    } else {
        if (!update_network(netinfo)) {
            return false;
        }
    }
    // Store received settings
    for (auto it = netinfo.connections.begin();
            it != netinfo.connections.end(); ++it) {
        if ((it->device == SERVER) && (it->recv_status == MSG_DONE)) {
            it->recv_status = MSG_STANDBY;
            if (!backplane_command.ParseFromString(it->message)) {
                return false;
            }
            std::cout << "Received command." << std::endl; 
        }
    }
    return true;
}

void PiControl::update_backplane_variables()
{
    // Only update if a command was received
    bool command_received = false;
    for (auto it = netinfo.connections.begin();
            it != netinfo.connections.end(); ++it) {
        // Proceed to update if a command was received
        if ((it->device == SERVER) && (it->recv_status == MSG_DONE)) {
            command_received = true;
            break;
        }
    }
    if (!command_received) { 
        // Nothing to do
        return;
    }
    
    std::string command_name = backplane_command.command_name();
    unsigned short command_parameters[NUM_COMMAND_PARAMETERS] = {0};
    
    int num_spi_messages_sent = 0;
    unsigned short spi_command[SPI_MESSAGE_LENGTH * MAX_NUM_SPI_MESSAGES] = {0};
    unsigned short spi_data[SPI_MESSAGE_LENGTH * MAX_NUM_SPI_MESSAGES] = {0};
    
    // Take appropriate action depending on received command
    if (command_name == "read_module_voltages") {
        float voltages[NUM_FEES];
        num_spi_messages_sent = read_voltages(voltages, spi_command, spi_data);
        for (int i = 0; i < NUM_FEES; i++) {
            backplane_variables.set_voltage(i, voltages[i]);
        }
    } else if (command_name == "read_module_currents") {
        float currents[NUM_FEES];
        num_spi_messages_sent = read_currents(currents, spi_command, spi_data);
        for (int i = 0; i < NUM_FEES; i++) {
            backplane_variables.set_current(i, currents[i]);
        }
    } else if (command_name == "read_modules_present") {
        unsigned short fees_present[NUM_FEES];
        num_spi_messages_sent = read_fees_present(fees_present, spi_command,
                spi_data);
        for (int i = 0; i < NUM_FEES; i++) {
            backplane_variables.set_present(i, fees_present[i]);
        }
    } else if (command_name == "set_trigger_mask") {
        unsigned short trigger_mask[NUM_FEES];
        num_spi_messages_sent = set_trigger_mask(trigger_mask, spi_command,
                spi_data);
        for (int i = 0; i < NUM_FEES; i++) {
            backplane_variables.set_trigger_mask(i, trigger_mask[i]);
        }
    } else if ((command_name == "set_trigger")
            && (backplane_command.int_args_size() == 4)) {
        for (int i = 0; i < 4; i++) {
            command_parameters[i] = backplane_command.int_args(i);
        }
        num_spi_messages_sent = set_trigger(command_parameters,
                spi_command, spi_data);
    } else if ((command_name == "toggle_trigger_tack")
            && (backplane_command.int_args_size() == 7)) {
        command_parameters[0] = 0;
        for (int i = 0; i < 7; i++) {
            // each argument must be 0 or 1 - enforce this with min
            command_parameters[0] = (command_parameters[0] & ~(1 << i))
                | (std::min(backplane_command.int_args(i), 1u) << i);
        }
        num_spi_messages_sent = enable_disable_trigger(command_parameters,
                spi_command, spi_data);
    } else if ((command_name == "set_holdoff_time")
            && (backplane_command.int_args_size() == 1)) {
        command_parameters[0] = backplane_command.int_args(0);
        num_spi_messages_sent = set_holdoff_time(command_parameters,
                spi_command, spi_data);
    } else if ((command_name == "set_tack_type_and_mode")
            && (backplane_command.int_args_size() == 2)) {
        // type and mode must be 0-3 - enforce this with min
        unsigned int tack_type = std::min(backplane_command.int_args(0), 3u);
        unsigned int tack_mode = std::min(backplane_command.int_args(1), 3u);
        command_parameters[0] = ((tack_type << 2) | tack_mode);
        num_spi_messages_sent = set_tack_type_and_mode(command_parameters,
                spi_command, spi_data);
    } else if ((command_name == "power_control_modules")
            && (backplane_command.int_args_size() == 2)) {
        command_parameters[0] = backplane_command.int_args(0);
        command_parameters[1] = backplane_command.int_args(1);
        num_spi_messages_sent = power_control_modules(command_parameters,
                spi_command, spi_data);
    } else if (command_name == "read_timer_and_trigger_rate") {
        num_spi_messages_sent = read_nstimer_trigger_rate(spi_command,
                spi_data);
    } else if (command_name == "reset_trigger_counter_and_timer") {
        num_spi_messages_sent = reset_trigger_and_nstimer(spi_command,
                spi_data);
    } else if (command_name == "sync") {
        num_spi_messages_sent = sync(spi_command, spi_data);
    } else {
        std::cerr << "Error: command name " << command_name <<
            " not recognized" << std::endl;
        return;
    }
    backplane_variables.set_allocated_command(&backplane_command);
    backplane_variables.set_n_spi_messages(num_spi_messages_sent);
    for (int i = 0; i < SPI_MESSAGE_LENGTH * num_spi_messages_sent; i++) {
        backplane_variables.set_spi_command(i, spi_command[i]);
        backplane_variables.set_spi_data(i, spi_data[i]);
    }
    updates_to_send = true;
}
