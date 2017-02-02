// Functions for implementing low level communication to the backplane

#ifndef SC_LOWLEVEL
#define SC_LOWLEVEL

// Initialize low level SPI communication
// Return true if successful, false otherwise
bool initialize_lowlevel();

// Read in and store FEE housekeeping data
void read_fee_data(int data_type, float fee_buffer[]);

// Read in and store FEEs present
void read_fees_present(unsigned short fees_present[]);

// Reset trigger and nstimer
void reset_trigger_and_nstimer();

// Set trigger
void set_trigger(unsigned short spi_commands[], unsigned short spi_data[]);

#endif
