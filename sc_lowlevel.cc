// Implementation of functions for low level communication to the backplane

// Most of the code below is copied directly from the BP SPI interface code
// written by Phil Moore and Richard Bose. 

#include <stdlib.h>
#include <ctime>
#include "bcm2835.h" // Driver for SPI chip

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
	unsigned short dummy_word;
	unsigned short som_word;
	unsigned short cmd_word;
	unsigned short eom_word;

	// Write Start word
	// By causality, nobody is in a state to send anything back on MISO
	// so one reads a dummy word coming back from the slave to the master.
	dummy_word = spi_tword(message[0]);
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
	unsigned short spi_message[11];
	unsigned short data[11];
	
	spi_message[0] = SPI_SOM_HKFPGA; // som
	spi_message[1] = CW_TRG_ADCS; // cw
	spi_message[2] = 0x0111;
	spi_message[3] = 0x1222;
	spi_message[4] = 0x2333;
	spi_message[5] = 0x3444;
	spi_message[6] = 0x4555;
	spi_message[7] = 0x5666;
	spi_message[8] = 0x0000;
	spi_message[9] = 0x0088;
	spi_message[10] = SPI_EOM_HKFPGA; // not used
	transfer_message(spi_message, data);// trig ADCs
	delay(100);
}

//// From BP SPI code. Keeping this for now since I don't know why the code was
//// written this way but there has to be a better way to implement a short 
//// delay. - Ari
//void tdelay(int msec){
//    int i;
//    long j;
//    for (i = 0; i < msec; i++){
//        for (j=0; j<532000; j++);
//    }
//    return;
//}

// Implement a time delay in milliseconds
void tdelay(int msec) {
    struct timespec tim;
    const long NSEC = 1000000 * msec; // convert millisec to nanosec
    tim.tv_sec = 0;
    tim.tv_nsec = NSEC;

    nanosleep(&tim, NULL);
}

// Read in and store FEE housekeeping voltages
void read_voltages(float voltages[], const int n_fees)
{
    unsigned short voltsarray[n_fees];
	unsigned short data[11];
	unsigned short spi_message[11];
	
    trig_adcs();
	
	tdelay(10);
	spi_message[0] = SPI_SOM_HKFPGA; // som
	spi_message[1] = CW_RD_FEE0_V; //cw
	spi_message[2] = 0x0111;
	spi_message[3] = 0x1222;
	spi_message[4] = 0x2333;
	spi_message[5] = 0x3444;
	spi_message[6] = 0x4555;
	spi_message[7] = 0x5666;
	spi_message[8] = 0x0000;
	spi_message[9] = 0x0088;	
	spi_message[10] = SPI_EOM_HKFPGA; // not used
	transfer_message(spi_message, data);
	
    voltsarray[5]  = data[2];
	voltsarray[12] = data[3];
	voltsarray[6]  = data[4];
	voltsarray[17] = data[5];
	voltsarray[7]  = data[6];
	voltsarray[13] = data[7];
	voltsarray[11] = data[8];
	voltsarray[18] = data[9];
		
	tdelay(10);
	spi_message[1] = CW_RD_FEE8_V; //cw
	transfer_message(spi_message,data);
	
    voltsarray[4]  = data[2];
	voltsarray[10] = data[3];
	voltsarray[1]  = data[4];
	voltsarray[0]  = data[5];
	voltsarray[3]  = data[6];
	voltsarray[2]  = data[7];
	voltsarray[16] = data[8];
	voltsarray[22] = data[9];
	
	tdelay(10);
	spi_message[1] = CW_RD_FEE16_V; //cw
	transfer_message(spi_message, data);

    voltsarray[28] = data[2];
	voltsarray[24] = data[3];
	voltsarray[30] = data[4];
	voltsarray[23] = data[5];
	voltsarray[31] = data[6];
	voltsarray[29] = data[7];
	voltsarray[26] = data[8];
	voltsarray[25] = data[9];
	
	tdelay(10);
	spi_message[1] = CW_RD_FEE24_V; //cw
	transfer_message(spi_message, data);
	
    voltsarray[20] = data[2];
	voltsarray[8]  = data[3];
	voltsarray[27] = data[4];
	voltsarray[15] = data[5];
	voltsarray[9]  = data[6];
	voltsarray[19] = data[7];
	voltsarray[21] = data[8];
	voltsarray[14] = data[9];

    // I don't know what this magic number is - Ari
	// FEE voltages should be ~12V
    for (int i = 0; i < n_fees; i++) {
		voltages[i] = voltsarray[i] * 0.006158;
    }
}
