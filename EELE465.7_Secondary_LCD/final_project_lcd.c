/**
 * W. Ward
 * 05/05/2022
 * I2C Secondary - LCD Controller - EELE465 Final Project
 *
 * Port Definitions (MSP2355)
 *  P1.7 - DB7 (Pin 14 on LCD)
 *  P1.6 - DB6 (Pin 13 on LCD)
 *  P1.5 - DB5 (Pin 12 on LCD)
 *  P1.4 - DB4 (Pin 11 on LCD)
 *  P3.7 - RS  (Pin 4 on LCD)     Data/Instruction Register Select
 *  P3.6 - E   (Pin 6 on LCD)     Enable Signal
 *
 *  P5.4 - System Reset
 *  P4.6 - SDA
 *  P4.7 - SCL
 */

#include <msp430.h> 

#define lcdAddress 0x069

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	return 0;
}
