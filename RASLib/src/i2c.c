//*******************************
// I2C Functions and Defines 
//*******************************

#include <stdlib.h>
#include <stdarg.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/i2c.h>
#include "i2c.h"
#include "gpio.h"
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <inc/hw_i2c.h>

static unsigned char i2c_buffer[32];

// Definition of struct I2C
// Defined to tI2C in i2c.h
struct I2C {
    // Constant StellarisWare values 
    // specific to each module
    const unsigned long BASE;
    const unsigned long PERIPH;
};

// Buffer of I2C structs to use
// Limited to the available modules
tI2C i2cBuffer[] = {
    {I2C0_MASTER_BASE, SYSCTL_PERIPH_I2C0},
    {I2C1_MASTER_BASE, SYSCTL_PERIPH_I2C1},
    {I2C2_MASTER_BASE, SYSCTL_PERIPH_I2C2},
    {I2C3_MASTER_BASE, SYSCTL_PERIPH_I2C3},
    {I2C4_MASTER_BASE, SYSCTL_PERIPH_I2C4},
    {I2C5_MASTER_BASE, SYSCTL_PERIPH_I2C5},
};

int i2cCount = 0;

tI2C *InitializeI2C(tPin sda, tPin scl) {
    // Grab the next module
    tI2C *i2c = &i2cBuffer[i2cCount++];
    
    // Enable the peripheral
    SysCtlPeripheralEnable(i2c->PERIPH);
    
    // Setup the pins as specified
    GPIOPinTypeI2C(PORT_VAL(sda), PIN_VAL(sda));
    GPIOPinTypeI2CSCL(PORT_VAL(scl), PIN_VAL(scl));
    
    // Setup the clock
    I2CMasterInitExpClk(i2c->BASE, SysCtlClockGet(), false);
    
    // Enable the I2C module
    I2CMasterEnable(i2c->BASE);
    
    // Return the initialized module
    return i2c;
}

// Summary:	Sends 'num' number of characters to specified address
// Parameters:
//		addr:	address to send data to
//		num:	number of characters being sent
//		...:	characters to send, separated by commas
// Note:	Number of characters must be equal to 'num'
void I2CSend(tI2C *i2c, unsigned short addr, int num, ...)
{
	// Make sure data is actually being sent
	if (num > 0 && num < 32){   // Max size of buffer
		// Allocate memory for data
		unsigned char *data = i2c_buffer;
		va_list args;
		int i=0;
		
		// Put characters to send in array
		va_start(args, num);
		for(; i<num; i++)
			data[i] = (unsigned char) va_arg(args, int);
		va_end(args);
	
		I2CMasterSlaveAddrSet(i2c->BASE, addr >> 1, false);
		I2CMasterDataPut(i2c->BASE, *data);
		if (num == 1){
		    I2CMasterControl(i2c->BASE, I2C_MASTER_CMD_SINGLE_SEND);
			while(I2CMasterBusy(i2c->BASE));
			return;
		}
		
		// Start sending consecutive data
		I2CMasterControl(i2c->BASE, I2C_MASTER_CMD_BURST_SEND_START);
		while(I2CMasterBusy(i2c->BASE));
		num--;
		data++;
		
		// Continue sending consecutive data
		while(num > 1){
			I2CMasterDataPut(i2c->BASE, *data);
			I2CMasterControl(i2c->BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
			while(I2CMasterBusy(i2c->BASE));
			num--;
			data++;
		}
		
		// Send last piece of data
		I2CMasterDataPut(i2c->BASE, *data);
		I2CMasterControl(i2c->BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
		while(I2CMasterBusy(i2c->BASE));
	}
}

// Summary:	Recieve/Fetch data from specified address
// Parameters:
//		addr:	address to recieve data from
//		data:	pointer to memory location to save data
//		len:	number of cahracers that will be recieved
void I2CRecieve(tI2C *i2c, unsigned short addr, unsigned char* data, unsigned int len)
{
    if (len < 1)	// Assume I2C Recieving will always return data
        return;
	
	// Set address to read from
	I2CMasterSlaveAddrSet(i2c->BASE, addr >> 1, true);
	
	// Check to see if pointer is to an array
	if (len == 1){
		I2CMasterControl(i2c->BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
		while(I2CMasterBusy(i2c->BASE));
		*data = I2CMasterDataGet(i2c->BASE);
		return;
	}
	
	// Begin reading consecutive data
	I2CMasterControl(i2c->BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
    while(I2CMasterBusy(i2c->BASE));
	*data = I2CMasterDataGet(i2c->BASE);
	len--;
	data++;
	
	// Continue reading consecutive data
	while(len > 1){
		I2CMasterControl(i2c->BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
    	while(I2CMasterBusy(i2c->BASE));
		*data = I2CMasterDataGet(i2c->BASE);
		len--;
		data++;
	}
	
	// Read last character of data	
    I2CMasterControl(i2c->BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
    while(I2CMasterBusy(i2c->BASE));
	*data = I2CMasterDataGet(i2c->BASE);
}




