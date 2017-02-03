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
        present_[i] = 0;
        data_buffer.add_present(0);
    }

    for (int i = 0; i < N_SPI; i++) {
        spi_data_[i] = 0;
        data_buffer.add_spi_data(0);
    }

    nstimer_ = 0;
    tack_count_ = 0;
    trigger_count_ = 0;
    tack_rate_ = 0.0;
    trigger_rate_ = 0.0;

    for (int i = 0; i < N_COMMANDS; i++) {
        commands_[i] = 0;
        settings_buffer.add_command(0);
    }
    
    requested_updates_ = BP_NONE;
    updates_to_send = false;
}

void Backplane::update_data(int requested_updates,
        unsigned short commands_for_request[], bool simulation_mode)
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
        case FEE_PRESENT:
        {
            unsigned short fees_present[N_FEES];
            if (!simulation_mode) {
                read_fees_present(fees_present);
            } else {
                simulate_fees_present(fees_present, N_FEES);
            }
            for (int i = 0; i < N_FEES; i++) {
                present_[i] = fees_present[i];
                data_buffer.set_present(i, present_[i]);
            }
            break;
        }
        case BP_RESET_TRIGGER_AND_NSTIMER:
        {
            if (!simulation_mode) {
                reset_trigger_and_nstimer();
            } else {
                std::cout << "Simulating resetting trigger and nstimer..."
                    << std::endl;
            }
            break;
        }
        case BP_SET_TRIGGER:
        {
            unsigned short spi_data[11];
            if (!simulation_mode) {
                set_trigger(commands_for_request, spi_data);
            } else {
                // simulate
                for (int i = 0; i < N_SPI; i++) {
                    spi_data[i] = i;
                }
            }
            for (int i = 0; i < N_SPI; i++) {
                spi_data_[i] = spi_data[i];
                data_buffer.set_spi_data(i, spi_data_[i]);
            }
            break;
        }
        case BP_READ_NSTIMER_TRIGGER_RATE:
        {
            unsigned long long nstimer = 0;
            unsigned long tack_count = 0, trigger_count = 0;
            float tack_rate = 0, trigger_rate = 0;
            unsigned short spi_data[11];
            if (!simulation_mode) {
                read_nstimer_trigger_rate(nstimer, tack_count, trigger_count,
                        tack_rate, trigger_rate, spi_data);
            } else {
                // simulate
                nstimer = 10000;
                tack_count = 300;
                trigger_count = 400;
                tack_rate = 10.10;
                trigger_rate = 11.11;
                for (int i = 0; i < N_SPI; i++) {
                    spi_data[i] = i;
                }
            }
            nstimer_ = nstimer;
            data_buffer.set_nstimer(nstimer_);
            tack_count_ = tack_count;
            data_buffer.set_tack_count(tack_count_);
            trigger_count_ = trigger_count;
            data_buffer.set_trigger_count(trigger_count_);
            tack_rate_ = tack_rate;
            data_buffer.set_tack_rate(tack_rate_);
            trigger_rate_ = trigger_rate;
            data_buffer.set_trigger_rate(trigger_rate_);
            for (int i = 0; i < N_SPI; i++) {
                spi_data_[i] = spi_data[i];
                data_buffer.set_spi_data(i, spi_data_[i]);
            }
            break;
        }
        default:
            return;
    }
    
    updates_to_send = true;
}

void Backplane::update_settings(int requested_updates,
        unsigned short settings_commands[])
{
    requested_updates_ = requested_updates;
    settings_buffer.set_requested_updates(requested_updates_);

    for (int i = 0; i < N_COMMANDS; i++) {
        settings_buffer.set_command(i, settings_commands[i]);
    }

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
                    for (int i = 0; i < N_COMMANDS; i++) {
                        commands_[i] = settings_buffer.command(i);
                    }
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
                        present_[i] = data_buffer.present(i);
                    }
                    for (int i = 0; i < N_SPI; i++) {
                        spi_data_[i] = data_buffer.spi_data(i);
                    }
                    nstimer_ = data_buffer.nstimer();
                    tack_count_ = data_buffer.tack_count();
                    trigger_count_ = data_buffer.trigger_count();
                    tack_rate_ = data_buffer.tack_rate();
                    trigger_rate_ = data_buffer.trigger_rate();
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
    update_data(requested_updates_, commands_, simulation_mode);
    requested_updates_ = BP_NONE;
}

void Backplane::print_data(int data_type)
{
    switch(data_type) {
        case BP_NONE:
            return;
        case FEE_PRESENT:
        {
           for (int i = 0; i < N_FEES; i++) {
                if (i == 0 || i == 28) {
                    std::cout << "   " << std::setw(2) << present_[i] 
                        << " ";
                } else if (i == 3 || i == 31) {
                    std::cout << std::setw(2) << present_[i] << "   ";
                } else {
                    std::cout << std::setw(2) << present_[i] << " ";
                }
                if (i == 3 || i == 9 || i == 15 || i == 21 || i == 27 || 
                        i == 31) {
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
            break;
        }
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
        case BP_SET_TRIGGER:
        {
            display_spi_data(spi_data_);
            break;
        }
        case BP_READ_NSTIMER_TRIGGER_RATE:
        {
            display_spi_data(spi_data_);
            printf("nsTimer %llu ns\n", nstimer_);
            printf("TACK Count %lu\n", tack_count_);
            printf("TACK Rate: %6.2f Hz\n", tack_rate_);
            // 4 phases or external trigger can HW trigger
        	printf("Hardware Trigger Count %lu\n", trigger_count_);
        	printf("HW Trigger Rate: %6.2f Hz\n", trigger_rate_);
            break;
        }
    }
}
