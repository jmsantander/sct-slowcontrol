// Functions for slow control logistics, including logging

#include <string>

// Codes for updating data and settings on backplane
#define BP_NONE 0
#define BP_VOLTAGES 1
#define BP_CURRENTS 2
#define FEE_PRESENT 3
#define BP_RESET_TRIGGER_AND_NSTIMER 4
#define BP_SET_TRIGGER 5
#define BP_READ_NSTIMER_TRIGGER_RATE 6

// Read in and store a command from the user from stdin
// Return true if the command is valid, false otherwise
bool read_command(std::string &command, std::string &value);

// Sleep for a given number of milliseconds
void sleep_msec(int msec);

// Simulate reading FEE data, for testing without a real Pi
void simulate_fee_data(float fee_buffer[], const int n_fees);

// Simulate reading FEEs present, for testing without a real Pi
void simulate_fees_present(unsigned short fees_present[], const int n_fees);

// Display SPI data
void display_spi_data(unsigned short spi_data[]);
