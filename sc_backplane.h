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
    unsigned short spi_data_[N_SPI];
    int requested_updates_;
    unsigned short commands_[N_COMMANDS];
    slow_control::Backplane_data data_buffer;
    slow_control::Backplane_settings settings_buffer;
    bool updates_to_send;
public:
    Backplane();

    void update_data(int requested_updates,
            unsigned short commands_for_request[],
            bool simulation_mode = false);

    void update_settings(int requested_updates, 
            unsigned short settings_commands[]);

    bool synchronize_network(Network_info &netinfo);

    void apply_settings(bool simulation_mode);

    void print_data(int data_type);

    // Initialize backplane for low level communication
    // Needed for Pi only - at a later point this will be better served using
    // class inheritance
    bool pi_initialize_lowlevel();

    float requested_updates() { return requested_updates_; }
};

#endif
