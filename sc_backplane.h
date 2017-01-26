// sc_backplane.h
// Header file containing classes for controlling and updating backplane info

#ifndef SC_BACKPLANE
#define SC_BACKPLANE

#include "sc_protobuf.pb.h"
#include "sc_network.h"

#define N_FEES 32

// Codes for updating data and settings
#define BP_NONE 0
#define BP_VOLTAGES 1

class Backplane
{
private:
    float voltages_[N_FEES];
    int requested_updates_;
    slow_control::Backplane_data data_buffer;
    slow_control::Backplane_settings settings_buffer;
    bool updates_to_send;
public:
    Backplane();

    void update_data(int requested_updates = BP_NONE,
            bool simulation_mode = false);
    void update_settings(int requested_updates = BP_NONE);

    bool synchronize_network(Network_info &netinfo);

    void apply_settings(bool simulation_mode);

    void print_data();

    // Initialize backplane for low level communication
    // Needed for Pi only - at a later point this will be better served using
    // class inheritance
    bool pi_initialize_lowlevel();

    float requested_updates() { return requested_updates_; }
};

#endif
