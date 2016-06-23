// sc_backplane.h
// Header file containing classes for controlling and updating backplane info

#ifndef SC_BACKPLANE
#define SC_BACKPLANE

#include "sc_protobuf.pb.h"
#include "sc_network.h"

class Backplane
{
private:
    float voltage_;
    float current_;
    float desired_voltage_;
    float desired_current_;
    slow_control::Backplane_data data_buffer;
    slow_control::Backplane_settings settings_buffer;
public:
    Backplane();

    void print_info();

    void update_data(float voltage, float current);
    void update_settings(float desired_voltage, float desired_current);

    bool update_from_network(Network_info &netinfo);

    float voltage() { return voltage_; }
    float current() { return current_; }
    float desired_voltage() { return desired_voltage_; }
    float desired_current() { return desired_current_; }
};

#endif
