// sc_backplane.h
// Header file containing classes for controlling and updating backplane info

#ifndef SC_BACKPLANE
#define SC_BACKPLANE

#include "sc_protobuf.pb.h"
#include "sc_network.h"
#include "sc_logistics.h"

#define N_FEES 32 // number of FEEs
#define N_COMMANDS 4 // number of commands for sending settings
#define N_SPI 11 // length of an SPI data array

class Backplane
{
private:
    float voltages_[N_FEES];
    float currents_[N_FEES];
    unsigned short present_[N_FEES];
    unsigned short trigger_mask_[N_FEES];
    unsigned short spi_command_[N_SPI];
    unsigned short spi_data_[N_SPI];
    unsigned long long nstimer_;
    unsigned long tack_count_;
    unsigned long trigger_count_;
    float tack_rate_;
    float trigger_rate_;

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
