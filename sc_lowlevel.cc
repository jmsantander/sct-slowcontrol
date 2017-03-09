// Implementation of functions for low level communication to the backplane

// Most of the code below is copied directly from the BP SPI interface code
// written by Phil Moore and Richard Bose. 

#include <cstdlib>
#include <cstdio>
#include "bcm2835.h" // Driver for SPI chip
#include "sc_logistics.h"

/* Command Words */
#define  SPI_WRAP_AROUND   	0x0000   /* cmd */
#define  CW_RESET_FEE   	0x0100   /* cmd */
#define  CW_FEEs_PRESENT    0x0200   /* cmd */
#define  CW_FEE_POWER_CTL   0x0400   /* cmd */
#define  CW_RD_FEE0_I	    0x0500   /* cmd */
#define  CW_RD_FEE8_I	    0x0507   /* cmd */
#define  CW_RD_FEE16_I	    0x050F   /* cmd */
#define  CW_RD_FEE24_I	    0x0510   /* cmd */
#define  CW_RD_FEE0_V	    0x0600   /* cmd */
#define  CW_RD_FEE8_V	    0x0607   /* cmd */
#define  CW_RD_FEE16_V	    0x060F   /* cmd */
#define  CW_RD_FEE24_V	    0x0610   /* cmd */
#define  CW_RD_ENV          0x0700   /* cmd */
#define  CW_RD_HKPWB	    0x0800   /* cmd */
#define  CW_PERI_TRIG       0x0900   /* cmd */
#define  CW_TRG_ADCS	    0x0A00   /* cmd */
#define  CW_RD_PWRSTATUS    0x0B00   /* cmd */
#define  CW_DACQ1_PWR_RESET  0x0C00   /* cmd */
#define  CW_DACQ2_PWR_RESET  0x0D00   /* cmd */
#define  SPI_SOM_HKFPGA		0xeb90 // Start of Message HKFPGA
#define  SPI_EOM_HKFPGA		0xeb09 // End of Message HKFPGA

#define  SPI_SOM_TFPGA		0xeb91 // Start of Message TFPGA
#define  SPI_EOM_TFPGA		0xeb0a // End of Message TFPGA
#define  SPI_WRAP_AROUND_TFPGA	0x0000   /* cmd */

#define  SPI_SET_nsTimer_TFPGA   	0x0100   /* cmd */
#define  SPI_READ_nsTimer_TFPGA   	0x0200   /* cmd */
#define  SPI_TRIGGERMASK_TFPGA  0x0300 /* cmd */
#define  SPI_TRIGGERMASK1_TFPGA 0x0400 /* cmd */
#define  SPI_TRIGGERMASK2_TFPGA 0x0500 /* cmd */
#define  SPI_TRIGGERMASK3_TFPGA 0x0600 /* cmd */
#define  SPI_READ_TRIGGER_NSTIMER_TFPGA	0x0700   /* cmd */
#define  SPI_HOLDOFF_TFPGA   	0x0800   /* cmd */
#define  SPI_TRIGGER_TFPGA   	0x0900   /* cmd */
#define  SPI_L1_TRIGGER_EN 0x0a00  /* cmd */
#define  RESET_TRIGGER_COUNT_AND_NSTIMER 0x0b00 /* cmd */
#define  SPI_READ_HIT_PATTERN   	0x0c00   /* cmd */
#define  SPI_READ_HIT_PATTERN1   	0x0d00   /* cmd */
#define  SPI_READ_HIT_PATTERN2   	0x0e00   /* cmd */
#define  SPI_READ_HIT_PATTERN3   	0x0f00   /* cmd */
#define  SPI_SET_ARRAY_SERDES_CONFIG 0x1000  /* cmd */
#define  SPI_SET_TACK_TYPE_MODE 0x1100 /* cmd */
#define  SPI_SET_TRIG_AT_TIME 0x1200 /* cmd */
#define  SPI_READ_DIAT_WORDS 0x1300 /* cmd */

#define  DWnull   	0x0000   /* zero word */

// Conversion factors from SPI readout to units
#define VOLT_CONVERSION_FACTOR 0.006158 // convert to volts
#define AMP_CONVERSION_FACTOR 0.00117 // convert to amps

bool initialize_lowlevel()
{
	if (!bcm2835_init()) {
	  return false;
    }
	bcm2835_spi_begin();
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST); // The default
	// Set Mode to zero
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);  
	// Set clock divider (BCM2835_SPI_CLOCK_DIVIDER_256) to give 1024 ns clock
	// nominal value needed by BP is 640 ns - slower works too
	// Tried 512 ns (BCM2835_SPI_CLOCK_DIVIDER_128) 8-10-2015 Seems OK
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_128); 
	bcm2835_spi_chipSelect(BCM2835_SPI_CS0);              
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);  // the default

    return true;
}
    
/*
	spi_tword
	J. Buckley
	8/5/15
	
	bcm2835 library (and possibly all SPI on Raspberry PI)
	assumes 8bit transfers.   This function turns two 8-bit
	transfers (write and simultaneous read) as 16-bit transfers
	(I hope)
*/
unsigned short spi_tword(unsigned short write_word)
{
	unsigned char write_msb;
    unsigned char write_lsb;
    unsigned char read_msb;
    unsigned char read_lsb;
    unsigned short tword;
    char tbuf[2];
    char rbuf[2];

    tword = (write_word & 0xff00)>>8;
    tbuf[0] = write_msb = (unsigned char)(tword);
    tword = (write_word & 0x00ff);
    tbuf[1] = write_lsb = (unsigned char)(tword);

    bcm2835_spi_transfernb(tbuf, rbuf, 0x00000002);
    read_msb = rbuf[0];
    read_lsb = rbuf[1];
    tword = read_msb;
    tword = (tword<<8)+read_lsb;
    return(tword);
}

/*
	transfer_message()
	J. Buckley
	8/6/15

	Simultaneously send a message (from PI master to slave FPGA) and read
	data (from FPGA slave to PI master).  
	Format of both send message and receive data are:
	SOM word, CMD word, 8 words of data, EOM word.
	Full duplex operation makes this simultaneous transfer of bits,
	bytes and words a bit tricky.
*/ 
void transfer_message(unsigned short *message, unsigned short *pdata)
{
	unsigned short som_word;
	unsigned short cmd_word;

	// Write Start word
	// By causality, nobody is in a state to send anything back on MISO
	// so one reads a dummy word coming back from the slave to the master.
	spi_tword(message[0]); // read a dummy word - AB
	// Write command word
	// word sent now that a specific SPI slave knows that the following
	// command and data is meant for it.   The slave is one byte behind,
	// and sends its start of message word back to the master.  This
	// can be checked to make sure the command response makes sense.
	pdata[0] = som_word = spi_tword(message[1]);
	// Next send the first data word, stored in mesage[2] from master
	// to slave.  Simultaneously read out the returned command word from
	// slave to master.   Full duplex du-suck.
	pdata[1] = cmd_word = spi_tword(message[2]);
	// now, the next 7 words coming back are actually data, but the 
	// last message word message[9] from the 
	pdata[2] = spi_tword(message[3]);
	pdata[3] = spi_tword(message[4]);
	pdata[4] = spi_tword(message[5]);
	pdata[5] = spi_tword(message[6]);
	pdata[6] = spi_tword(message[7]);
	pdata[7] = spi_tword(message[8]);
	pdata[8] = spi_tword(message[9]);
	pdata[9] = spi_tword(0x0000); //Send null word, get 8th data word from slave
	
	// This next bit is tricky.  Since there is a phase shift of one
	// word (received from slave versus sent to slave), you might ask
	// "When I send the 11th word from master to slave, the slave is not
	// done - it still needs to send its last EOM word.  What gives?"
	// Well, it turns out before the master sends the EOM to the slave
	// it first sends out a dummy_word then sends EOM, while the slave
	// SIMULTANEOUSLY sends back its EOM - no causality problems since
	// it's just end of message.
	pdata[10] = spi_tword(message[10]);
}

void trig_adcs()
{
	unsigned short spi_command[11];
	unsigned short data[11];
	
	spi_command[0] = SPI_SOM_HKFPGA; // som
	spi_command[1] = CW_TRG_ADCS; // cw
	spi_command[2] = 0x0111;
	spi_command[3] = 0x1222;
	spi_command[4] = 0x2333;
	spi_command[5] = 0x3444;
	spi_command[6] = 0x4555;
	spi_command[7] = 0x5666;
	spi_command[8] = 0x0000;
	spi_command[9] = 0x0088;
	spi_command[10] = SPI_EOM_HKFPGA; // not used
	transfer_message(spi_command, data);// trig ADCs
	delay(100);
}

// Enable or disable trigger
void enable_disable_trigger(unsigned short command_parameters[],
        unsigned short spi_command[], unsigned short spi_data[])
{
    spi_command[0] = SPI_SOM_TFPGA; //som
	spi_command[1] = SPI_L1_TRIGGER_EN; //cw
	spi_command[2] = command_parameters[0];
	spi_command[3] = 0x0002;
	spi_command[4] = 0x0003;
	spi_command[5] = 0x0004;
	spi_command[6] = 0x0005;
	spi_command[7] = 0x0006;
	spi_command[8] = 0x0007;
	spi_command[9] = 0x0008;			
	spi_command[10] = SPI_EOM_TFPGA; //not used
	transfer_message(spi_command, spi_data);
}

// Turn FEEs on and off
void power_control_modules(unsigned short command_parameters[],
        unsigned short spi_command[], unsigned short spi_data[])
{
    spi_command[0] = SPI_SOM_HKFPGA; //som
    spi_command[1] = CW_FEE_POWER_CTL; //cw
    spi_command[2] = command_parameters[0];
    spi_command[3] = command_parameters[1];
    spi_command[4] = 0x2333;
    spi_command[5] = 0x3444;
    spi_command[6] = 0x4555;
    spi_command[7] = 0x5666;
    spi_command[8] = 0x6777;
    spi_command[9] = 0x7888;			
    spi_command[10] = SPI_EOM_HKFPGA; //not used
    transfer_message(spi_command, spi_data);
}

// Read in and store FEE housekeeping data
void read_fee_data(int data_type, float fee_buffer[],
        unsigned short spi_command[], unsigned short spi_data[])
{
    // Define conversion factor from SPI readout to meaningful unit
    float cf = 1.0;
    if (data_type == FEE_VOLTAGES) {
        cf = VOLT_CONVERSION_FACTOR;
    } else if (data_type == FEE_CURRENTS) {
        cf = AMP_CONVERSION_FACTOR;
    }
	
    trig_adcs();

    for (int mi = 0; mi < 4; mi++) {    
	    spi_command[mi*11 + 0] = SPI_SOM_HKFPGA; // som
	    spi_command[mi*11 + 2] = 0x0111;
	    spi_command[mi*11 + 3] = 0x1222;
	    spi_command[mi*11 + 4] = 0x2333;
	    spi_command[mi*11 + 5] = 0x3444;
	    spi_command[mi*11 + 6] = 0x4555;
	    spi_command[mi*11 + 7] = 0x5666;
	    spi_command[mi*11 + 8] = 0x0000;
	    spi_command[mi*11 + 9] = 0x0088;	
	    spi_command[mi*11 + 10] = SPI_EOM_HKFPGA; // not used
    }
    
    if (data_type == FEE_VOLTAGES) {
        spi_command[1] = CW_RD_FEE0_V;
        spi_command[12] = CW_RD_FEE8_V;
        spi_command[23] = CW_RD_FEE16_V;
        spi_command[34] = CW_RD_FEE24_V;
    } else if (data_type == FEE_CURRENTS) {
        spi_command[1] = CW_RD_FEE0_I;
        spi_command[12] = CW_RD_FEE8_I;
        spi_command[23] = CW_RD_FEE16_I;
        spi_command[34] = CW_RD_FEE24_I;
    }
	
    sleep_msec(10);
	transfer_message(spi_command, spi_data);
	
    fee_buffer[5]  = spi_data[2] * cf;
	fee_buffer[12] = spi_data[3] * cf;
	fee_buffer[6]  = spi_data[4] * cf;
	fee_buffer[17] = spi_data[5] * cf;
	fee_buffer[7]  = spi_data[6] * cf;
	fee_buffer[13] = spi_data[7] * cf;
	fee_buffer[11] = spi_data[8] * cf;
	fee_buffer[18] = spi_data[9] * cf;
		
	sleep_msec(10);
	transfer_message(spi_command + 11, spi_data + 11);
	
    fee_buffer[4]  = spi_data[13] * cf;
	fee_buffer[10] = spi_data[14] * cf;
	fee_buffer[1]  = spi_data[15] * cf;
	fee_buffer[0]  = spi_data[16] * cf;
	fee_buffer[3]  = spi_data[17] * cf;
	fee_buffer[2]  = spi_data[18] * cf;
	fee_buffer[16] = spi_data[19] * cf;
	fee_buffer[22] = spi_data[20] * cf;
	
	sleep_msec(10);
	transfer_message(spi_command + 22, spi_data + 22);

    fee_buffer[28] = spi_data[24] * cf;
	fee_buffer[24] = spi_data[25] * cf;
	fee_buffer[30] = spi_data[26] * cf;
	fee_buffer[23] = spi_data[27] * cf;
	fee_buffer[31] = spi_data[28] * cf;
	fee_buffer[29] = spi_data[29] * cf;
	fee_buffer[26] = spi_data[30] * cf;
	fee_buffer[25] = spi_data[31] * cf;
	
	sleep_msec(10);
	transfer_message(spi_command + 33, spi_data + 33);
	
    fee_buffer[20] = spi_data[35] * cf;
	fee_buffer[8]  = spi_data[36] * cf;
	fee_buffer[27] = spi_data[37] * cf;
	fee_buffer[15] = spi_data[38] * cf;
	fee_buffer[9]  = spi_data[39] * cf;
	fee_buffer[19] = spi_data[40] * cf;
	fee_buffer[21] = spi_data[41] * cf;
	fee_buffer[14] = spi_data[42] * cf;
}

// Determine which FEEs are present
void read_fees_present(unsigned short fees_present[],
        unsigned short spi_command[], unsigned short spi_data[])
{
    spi_command[0] = SPI_SOM_HKFPGA; // som
    spi_command[1] = CW_FEEs_PRESENT; // cw
    spi_command[2] = 0x0111;
    spi_command[3] = 0x1222;
    spi_command[4] = 0x2333;
    spi_command[5] = 0x3444;
    spi_command[6] = 0x4555;
    spi_command[7] = 0x5666;
    spi_command[8] = 0x6777;
    spi_command[9] = 0x7888;
    spi_command[10] = SPI_EOM_HKFPGA; // not used
    transfer_message(spi_command, spi_data);

    // data[2] is FEEs present J0-15
    // data[3] is FEEs present J16-31
    
    fees_present[0] = spi_data[2] & 0x0001;
    fees_present[1] = spi_data[2] & 0x0002 >> 1;
    fees_present[2] = spi_data[2] & 0x0004 >> 2;
    fees_present[3] = spi_data[2] & 0x0008 >> 3;
    fees_present[4] = spi_data[2] & 0x0010 >> 4;
    fees_present[5] = spi_data[2] & 0x0020 >> 5;
    fees_present[6] = spi_data[2] & 0x0040 >> 6;
    fees_present[7] = spi_data[2] & 0x0080 >> 7;
    fees_present[8] = spi_data[2] & 0x0100 >> 8;
    fees_present[9] = spi_data[2] & 0x0200 >> 9;
    fees_present[10] = spi_data[2] & 0x0400 >> 10;
    fees_present[11] = spi_data[2] & 0x0800 >> 11;
    fees_present[12] = spi_data[2] & 0x1000 >> 12;
    fees_present[13] = spi_data[2] & 0x2000 >> 13;
    fees_present[14] = spi_data[2] & 0x4000 >> 14;
    fees_present[15] = spi_data[2] & 0x8000 >> 15;
    fees_present[16] = spi_data[3] & 0x0001;
    fees_present[17] = spi_data[3] & 0x0002 >> 1;
    fees_present[18] = spi_data[3] & 0x0004 >> 2;
    fees_present[19] = spi_data[3] & 0x0008 >> 3;
    fees_present[20] = spi_data[3] & 0x0010 >> 4;
    fees_present[21] = spi_data[3] & 0x0020 >> 5;
    fees_present[22] = spi_data[3] & 0x0040 >> 6;
    fees_present[23] = spi_data[3] & 0x0080 >> 7;
    fees_present[24] = spi_data[3] & 0x0100 >> 8;
    fees_present[25] = spi_data[3] & 0x0200 >> 9;
    fees_present[26] = spi_data[3] & 0x0400 >> 10;
    fees_present[27] = spi_data[3] & 0x0800 >> 11;
    fees_present[28] = spi_data[3] & 0x1000 >> 12;
    fees_present[29] = spi_data[3] & 0x2000 >> 13;
    fees_present[30] = spi_data[3] & 0x4000 >> 14;
    fees_present[31] = spi_data[3] & 0x8000 >> 15;
}

// Read nstimer, tack count and rate, and trigger count and rate
void read_nstimer_trigger_rate(unsigned short spi_command[],
        unsigned short spi_data[])
{
    spi_command[0] = SPI_SOM_TFPGA; //som
    spi_command[1] = SPI_READ_nsTimer_TFPGA; //cw
    spi_command[2] = 0X0001;
    spi_command[3] = 0X0002;
    spi_command[4] = 0X0003;
    spi_command[5] = 0X0004;
    spi_command[6] = 0x0005;
    spi_command[7] = 0x0006;
    spi_command[8] = 0x0007;
    spi_command[9] = 0x0008;			
    spi_command[10] = SPI_EOM_TFPGA; //not used
    transfer_message(spi_command, spi_data);
}

// Reset trigger and nstimer
void reset_trigger_and_nstimer(unsigned short spi_command[],
        unsigned short spi_data[])
{
    spi_command[0] = SPI_SOM_TFPGA; //som
	spi_command[1] = RESET_TRIGGER_COUNT_AND_NSTIMER; //cw
	spi_command[2] = 0x0111;
	spi_command[3] = 0x1222;
	spi_command[4] = 0x2333;
	spi_command[5] = 0x3444;
	spi_command[6] = 0x4555;
	spi_command[7] = 0x5666;
	spi_command[8] = 0x6777;
	spi_command[9] = 0x7888;			
	spi_command[10] = SPI_EOM_TFPGA; //not used
	transfer_message(spi_command, spi_data);
}

// Set holdoff time
void set_holdoff_time(unsigned short command_parameters[],
        unsigned short spi_command[], unsigned short spi_data[])
{
    spi_command[0] = SPI_SOM_TFPGA; //som
    spi_command[1] = SPI_HOLDOFF_TFPGA; //cw
    spi_command[2] = command_parameters[0];
    spi_command[3] = 0x0002;
    spi_command[4] = 0x0003;
    spi_command[5] = 0x0004;
    spi_command[6] = 0x0005;
    spi_command[7] = 0x0006;
    spi_command[8] = 0x0007;
    spi_command[9] = 0x0008;			
    spi_command[10] = SPI_EOM_TFPGA; //not used
    transfer_message(spi_command, spi_data);
}

// Set TACK type and mode
void set_tack_type_and_mode(unsigned short command_parameters[],
        unsigned short spi_command[], unsigned short spi_data[])
{
    spi_command[0] = SPI_SOM_TFPGA; //som
    spi_command[1] = SPI_SET_TACK_TYPE_MODE; //cw
    spi_command[2] = command_parameters[0];
    spi_command[3] = 0x0002;
    spi_command[4] = 0x0003;
    spi_command[5] = 0x0004;
    spi_command[6] = 0x0005;
    spi_command[7] = 0x0006;
    spi_command[8] = 0x0007;
    spi_command[9] = 0x0008;			
    spi_command[10] = SPI_EOM_TFPGA; //not used
    transfer_message(spi_command, spi_data);
}

// Set trigger
void set_trigger(unsigned short command_parameters[],
        unsigned short spi_command[], unsigned short spi_data[])
{
    spi_command[0] = SPI_SOM_TFPGA; //som
	spi_command[1] = SPI_SET_TRIG_AT_TIME; //cw
	spi_command[2] = command_parameters[0];
	spi_command[3] = command_parameters[1];
	spi_command[4] = command_parameters[2];
	spi_command[5] = command_parameters[3];
	spi_command[6] = 0x0005;
	spi_command[7] = 0x0006;
	spi_command[8] = 0x0007;
	spi_command[9] = 0x0008;			
	spi_command[10] = SPI_EOM_TFPGA; //not used
	transfer_message(spi_command, spi_data);
}

// Set trigger mask
void set_trigger_mask(unsigned short trigger_mask[],
        unsigned short spi_command[], unsigned short spi_data[])
{
    unsigned short trigger_mask_commands[32];
    
    // Setting Trigger Mask from file trigger_mask
    FILE *myfile;
    myfile = fopen("trigger_mask", "r");
    for(int i = 0; i < 32; i++) {
        fscanf(myfile, "%hx", &trigger_mask_commands[i]);
    }
    fclose(myfile);
    
    spi_command[0] = SPI_SOM_TFPGA; //som
    spi_command[1] = SPI_TRIGGERMASK_TFPGA; //cw
    spi_command[2] = trigger_mask_commands[0];
    spi_command[3] = trigger_mask_commands[1];
    spi_command[4] = trigger_mask_commands[2];
    spi_command[5] = trigger_mask_commands[3];
    spi_command[6] = trigger_mask_commands[4];
    spi_command[7] = trigger_mask_commands[5];
    spi_command[8] = trigger_mask_commands[6];
    spi_command[9] = trigger_mask_commands[7];			
    spi_command[10] = SPI_EOM_TFPGA; //not used
    transfer_message(spi_command, spi_data);
    for (int i = 0; i < 8; i++) {
        trigger_mask[i] = spi_data[i + 2];
    }

    spi_command[11] = SPI_SOM_TFPGA; //som
    spi_command[12] = SPI_TRIGGERMASK1_TFPGA; //cw
    spi_command[13] = trigger_mask_commands[8];
    spi_command[14] = trigger_mask_commands[9];
    spi_command[15] = trigger_mask_commands[10];
    spi_command[16] = trigger_mask_commands[11];
    spi_command[17] = trigger_mask_commands[12];
    spi_command[18] = trigger_mask_commands[13];
    spi_command[19] = trigger_mask_commands[14];
    spi_command[20] = trigger_mask_commands[15];			
    spi_command[21] = SPI_EOM_TFPGA; //not used
    transfer_message(spi_command + 11, spi_data + 11);
    for (int i = 0; i < 8; i++) {
        trigger_mask[i + 8] = spi_data[i + 13];
    }
    
    spi_command[22] = SPI_SOM_TFPGA; //som
    spi_command[23] = SPI_TRIGGERMASK2_TFPGA; //cw
    spi_command[24] = trigger_mask_commands[16];
    spi_command[25] = trigger_mask_commands[17];
    spi_command[26] = trigger_mask_commands[18];
    spi_command[27] = trigger_mask_commands[19];
    spi_command[28] = trigger_mask_commands[20];
    spi_command[29] = trigger_mask_commands[21];
    spi_command[30] = trigger_mask_commands[22];
    spi_command[31] = trigger_mask_commands[23];			
    spi_command[32] = SPI_EOM_TFPGA; //not used
    transfer_message(spi_command + 22, spi_data + 22);
    for (int i = 0; i < 8; i++) {
        trigger_mask[i + 16] = spi_data[i + 24];
    }
    
    spi_command[33] = SPI_SOM_TFPGA; //som
    spi_command[34] = SPI_TRIGGERMASK3_TFPGA; //cw
    spi_command[35] = trigger_mask_commands[24];
    spi_command[36] = trigger_mask_commands[25];
    spi_command[37] = trigger_mask_commands[26];
    spi_command[38] = trigger_mask_commands[27];
    spi_command[39] = trigger_mask_commands[28];
    spi_command[40] = trigger_mask_commands[29];
    spi_command[41] = trigger_mask_commands[30];
    spi_command[42] = trigger_mask_commands[31];			
    spi_command[43] = SPI_EOM_TFPGA; //not used
    transfer_message(spi_command + 33, spi_data + 33);
    for (int i = 0; i < 8; i++) {
        trigger_mask[i + 24] = spi_data[i + 35];
    }
}

// Send sync commands
void sync(unsigned short spi_command[], unsigned short spi_data[])
{
    // Setup SYNC message.
    // Must do a SYNC before TAACK messages will be effective
    /* If Target module has already been synced, sending a SYNC message will
     * have no effect */
    // Set TYPE (01) and MODE (00) for a SYNC
    spi_command[0] = SPI_SOM_TFPGA; //som
    spi_command[1] = SPI_SET_TACK_TYPE_MODE; //cw
    spi_command[2] = 0x0004; // TYPE 01 MODE 00
    spi_command[3] = 0x0000;
    spi_command[4] = 0x0000;
    spi_command[5] = 0x0000;
    spi_command[6] = 0x0000;
    spi_command[7] = 0x0000;
    spi_command[8] = 0x0000;
    spi_command[9] = 0x0000;			
    spi_command[10] = SPI_EOM_TFPGA; //not used
    transfer_message(spi_command, spi_data);
    sleep_msec(10);
    
    // Set a time to send the SYNC message
    spi_command[11] = SPI_SOM_TFPGA; //som
    spi_command[12] = SPI_SET_TRIG_AT_TIME; //cw
    spi_command[13] = 0x0000;
    spi_command[14] = 0x0000;
    spi_command[15] = 0x0001; // A short time after 0
    spi_command[16] = 0x0004; //RichW0x0000;
    spi_command[17] = 0x0000;
    spi_command[18] = 0x0000;
    spi_command[19] = 0x0000;
    spi_command[20] = 0x0000;			
    // ?? Need a 4 because time in message ends up one tick behind
    // A bug to be investigated in the TFPGA gate array HDL.
    // Also the time has to have 3 LSBs 000
    spi_command[21] = SPI_EOM_TFPGA; //not used
    transfer_message(spi_command + 11, spi_data + 11);
    sleep_msec(10);
    
    // Reset the nsTimer to 0
    spi_command[22] = SPI_SOM_TFPGA; //som
    spi_command[23] = RESET_TRIGGER_COUNT_AND_NSTIMER; //cw
    spi_command[24] = 0x0000;
    spi_command[25] = 0x0000;
    spi_command[26] = 0x0001; // A short time after 0
    spi_command[27] = 0x0004; //RichW0x0000;
    spi_command[28] = 0x0000;
    spi_command[29] = 0x0000;
    spi_command[30] = 0x0000;
    spi_command[31] = 0x0000;			
    spi_command[32] = SPI_EOM_TFPGA; //not used
    transfer_message(spi_command + 22, spi_data + 22);
    sleep_msec(10);
    
    // The TFPGA will send the SYNC message when nsTimer reaches the time set
    // above
    
    // Set TYPE and MODE back in anticipation of sending a TACK
    // Setting TYPE (00) and MODE (00) so subsequent message are TACKs
    spi_command[33] = SPI_SOM_TFPGA; //som
    spi_command[34] = SPI_SET_TACK_TYPE_MODE; //cw
    spi_command[35] = 0x0000; // TYPE 00 MODE 00
    spi_command[36] = 0x0000;
    spi_command[37] = 0x0001; // A short time after 0
    spi_command[38] = 0x0004; //RichW0x0000;
    spi_command[39] = 0x0000;
    spi_command[40] = 0x0000;
    spi_command[41] = 0x0000;
    spi_command[42] = 0x0000;			
    spi_command[43] = SPI_EOM_TFPGA; //not used
    transfer_message(spi_command + 33, spi_data + 33);
}
