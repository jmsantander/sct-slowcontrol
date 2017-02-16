// Functions for implementing low level communication to the backplane

#ifndef SC_LOWLEVEL
#define SC_LOWLEVEL

// Initialize low level SPI communication
// Return true if successful, false otherwise
bool initialize_lowlevel();

// Enable or disable trigger
void enable_disable_trigger(unsigned short command_parameters[],
        unsigned short spi_command[], unsigned short spi_data[]);

// Turn FEEs on and off
void power_control_modules(unsigned short command_parameters[],
        unsigned short spi_command[], unsigned short spi_data[]);

// Read in and store FEE housekeeping data
void read_fee_data(int data_type, float fee_buffer[]);

// Read in and store FEEs present
void read_fees_present(unsigned short fees_present[]);

// Read nstimer, tack count and rate, and trigger count and rate
void read_nstimer_trigger_rate(unsigned long long &nstimer,
        unsigned long &tack_count, unsigned long &trigger_count,
        float &tack_rate, float &trigger_rate, unsigned short spi_command[],
        unsigned short spi_data[]);

// Reset trigger and nstimer
void reset_trigger_and_nstimer();

// Set holdoff time
void set_holdoff_time(unsigned short command_parameters[],
        unsigned short spi_command[], unsigned short spi_data[]);

// Set TACK type and mode
void set_tack_type_and_mode(unsigned short command_parameters[],
        unsigned short spi_command[], unsigned short spi_data[]);

// Set trigger
void set_trigger(unsigned short command_parameters[],
        unsigned short spi_command[], unsigned short spi_data[]);

// Set trigger mask
void set_trigger_mask(unsigned short trigger_mask[]);

// Send sync commands
void sync();

#endif
