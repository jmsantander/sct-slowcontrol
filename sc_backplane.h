// sc_runcontrol.h
// Header file containing the class for high-level run control
// Lower level programs inherit from this, adding their specific functionality

#ifndef SC_BACKPLANE
#define SC_BACKPLANE

#include <string>

#include "sc_network.h"
#include "sc_logistics.h"
#include "sc_protobuf.pb.h"

class Backplane
{
private:
    float voltages_[N_FEES];
    float currents_[N_FEES];
    unsigned short n_spi_messages_;
    unsigned short present_[N_FEES];
    unsigned short trigger_mask_[N_FEES];
    unsigned short spi_command_[N_SPI * N_MESSAGES];
    unsigned short spi_data_[N_SPI * N_MESSAGES];

    int command_code_;
    unsigned short command_parameters_[N_COMMANDS];
    bool updates_to_send;

    slow_control::Backplane_data data_buffer;
    slow_control::Backplane_settings settings_buffer;
public:
    Backplane();

    void update_data(int command_code,
            unsigned short commands_for_request[],
            bool simulation_mode = false);

    void update_settings(int command_code, 
            unsigned short settings_commands[]);

    bool synchronize_network(Network_info &netinfo);

    void apply_settings(bool simulation_mode);

    void print_data(int data_type);

    // Initialize backplane for low level communication
    // Needed for Pi only - at a later point this will be better served using
    // class inheritance
    bool pi_initialize_lowlevel();

    float command_code() { return command_code_; }
};

#endif
