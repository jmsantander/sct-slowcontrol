// picontrol.cc
// File containing the implementation for the class for mid-level pi control

#include <iostream>

#include "picontrol.h"

#include "sc_network.h"
#include "backplane_lowlevel.h"

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
    std::vector<Connection>::iterator iter;
    for (iter = netinfo.connections.begin();
            iter != netinfo.connections.end(); ++iter) {
        if ((iter->device == SERVER) &&
                (iter->recv_status == MSG_DONE)) {
            iter->recv_status = MSG_STANDBY;
            if (!backplane_command.ParseFromString(iter->message)) {
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
    for (std::vector<Connection>::iterator iter =
            netinfo.connections.begin(); iter != netinfo.connections.end();
            ++iter) {
        // Proceed to update if a command was received
        if ((iter->device == SERVER) && (iter->recv_status == MSG_DONE)) {
            command_received = true;
            break;
        }
    }
    if (!command_received) { 
        // Nothing to do
        return;
    }
    int command_code = backplane_command.command_code();
    unsigned short command_parameters[N_COMMANDS];
    for (int i = 0; i < N_COMMANDS; i++) {
        command_parameters[i] = backplane_command.command_parameter(i);
    }
    backplane_variables.set_command_code(command_code);
    switch(command_code) {
        case BP_FEE_VOLTAGES:
        {
            float voltages[N_FEES];
            unsigned short spi_command[N_SPI * N_SPI_MESSAGES] = {0};
            unsigned short spi_data[N_SPI * N_SPI_MESSAGES] = {0};
            read_voltages(voltages, spi_command, spi_data);
            for (int i = 0; i < N_FEES; i++) {
                backplane_variables.set_voltage(i, voltages[i]);
            }
            for (int i = 0; i < N_SPI * 4; i++) {
                backplane_variables.set_spi_command(i, spi_command[i]);
                backplane_variables.set_spi_data(i, spi_data[i]);
            }
            backplane_variables.set_n_spi_messages(4);
            break;
        }
        case BP_FEE_CURRENTS:
        {
            float currents[N_FEES];
            unsigned short spi_command[N_SPI * N_SPI_MESSAGES] = {0};
            unsigned short spi_data[N_SPI * N_SPI_MESSAGES] = {0};
            read_currents(currents, spi_command, spi_data);
            for (int i = 0; i < N_FEES; i++) {
                backplane_variables.set_current(i, currents[i]);
            }
            for (int i = 0; i < N_SPI * 4; i++) {
                backplane_variables.set_spi_command(i, spi_command[i]);
                backplane_variables.set_spi_data(i, spi_data[i]);
            }
            backplane_variables.set_n_spi_messages(4);
            break;
        }
        case BP_FEE_PRESENT:
        {
            unsigned short fees_present[N_FEES];
            unsigned short spi_command[N_SPI * N_SPI_MESSAGES] = {0};
            unsigned short spi_data[N_SPI * N_SPI_MESSAGES] = {0};
            read_fees_present(fees_present, spi_command, spi_data);
            for (int i = 0; i < N_FEES; i++) {
                backplane_variables.set_present(i, fees_present[i]);
            }
            for (int i = 0; i < N_SPI; i++) {
                backplane_variables.set_spi_command(i, spi_command[i]);
                backplane_variables.set_spi_data(i, spi_data[i]);
            }
            backplane_variables.set_n_spi_messages(1);
            break;
        }
        case BP_SET_TRIGGER_MASK:
        {
            unsigned short trigger_mask[N_FEES];
            unsigned short spi_command[N_SPI * N_SPI_MESSAGES] = {0};
            unsigned short spi_data[N_SPI * N_SPI_MESSAGES] = {0};
            set_trigger_mask(trigger_mask, spi_command, spi_data);
            for (int i = 0; i < N_FEES; i++) {
                backplane_variables.set_trigger_mask(i, trigger_mask[i]);
            }
            for (int i = 0; i < N_SPI * 4; i++) {
                backplane_variables.set_spi_command(i, spi_command[i]);
                backplane_variables.set_spi_data(i, spi_data[i]);
            }
            backplane_variables.set_n_spi_messages(4);
            break;
        }
        case BP_SET_TRIGGER:
        case BP_ENABLE_DISABLE_TRIGGER:
        case BP_SET_HOLDOFF_TIME:
        case BP_SET_TACK_TYPE_AND_MODE:
        case BP_POWER_CONTROL_MODULES:
        case BP_READ_NSTIMER_TRIGGER_RATE:
        case BP_RESET_TRIGGER_AND_NSTIMER:
        {
            unsigned short spi_command[N_SPI * N_SPI_MESSAGES] = {0};
            unsigned short spi_data[N_SPI * N_SPI_MESSAGES] = {0};
            if (command_code == BP_SET_TRIGGER) {
                set_trigger(command_parameters, spi_command, spi_data);
            } else if (command_code == BP_ENABLE_DISABLE_TRIGGER) {
                enable_disable_trigger(command_parameters, spi_command,
                        spi_data);
            } else if (command_code == BP_SET_HOLDOFF_TIME) {
                set_holdoff_time(command_parameters, spi_command, spi_data);
            } else if (command_code == BP_SET_TACK_TYPE_AND_MODE) {
                set_tack_type_and_mode(command_parameters, spi_command,
                        spi_data);
            } else if (command_code == BP_POWER_CONTROL_MODULES) {
                power_control_modules(command_parameters, spi_command,
                        spi_data);
            } else if (command_code == BP_READ_NSTIMER_TRIGGER_RATE) {
                read_nstimer_trigger_rate(spi_command, spi_data);
            } else if (command_code == BP_RESET_TRIGGER_AND_NSTIMER) {
                reset_trigger_and_nstimer(spi_command, spi_data);
            }
            for (int i = 0; i < N_SPI; i++) {
                backplane_variables.set_spi_command(i, spi_command[i]);
                backplane_variables.set_spi_data(i, spi_data[i]);
            }
            backplane_variables.set_n_spi_messages(1);
            break;
        }
        case BP_SYNC:
        {
            unsigned short spi_command[N_SPI * N_SPI_MESSAGES] = {0};
            unsigned short spi_data[N_SPI * N_SPI_MESSAGES] = {0};
            sync(spi_command, spi_data);
            for (int i = 0; i < N_SPI * 4; i++) {
                backplane_variables.set_spi_command(i, spi_command[i]);
                backplane_variables.set_spi_data(i, spi_data[i]);
            }
            backplane_variables.set_n_spi_messages(4);
            break;
        }
        default:
            return;
    }
    updates_to_send = true;
}
