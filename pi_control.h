// pi_control.h
// Header file containing the class for mid-level pi control

#ifndef PI_CONTROL_H
#define PI_CONTROL_H

#include <string>

#include "network.h"
#include "slow_control.pb.h"
#include "backplane_spi.h"

// TODO: load constants from config file
const int NUM_FEES = 32; // number of modules allowed for in underlying code
const int SPI_MESSAGE_LENGTH = 11;
const int MAX_NUM_SPI_MESSAGES = 4; // none of the commands here will send more

class PiControl {
protected:
    Network_info netinfo;
    slow_control::BackplaneVariables backplane_variables;
    slow_control::LowLevelCommand backplane_command;
    bool updates_to_send;
public:
    PiControl(std::string hostname) : netinfo(PI, hostname)
    {
        // Verify that the version of the Protocol Buffer library we linked
        // against is compatible with the version of the headers we compiled
        // against.
        GOOGLE_PROTOBUF_VERIFY_VERSION;
        updates_to_send = false;
        initialize_lowlevel();
        for (int i = 0; i < SPI_MESSAGE_LENGTH * MAX_NUM_SPI_MESSAGES; i++) {
            backplane_variables.add_spi_command(0);
            backplane_variables.add_spi_data(0);
        }
        for (int i = 0; i < NUM_FEES; i++) {
            backplane_variables.add_voltage(0.0);
            backplane_variables.add_current(0.0);
            backplane_variables.add_present(0);
            backplane_variables.add_trigger_mask(0);
        }
    }
    bool synchronize_network();
    void update_backplane_variables();
};

#endif
