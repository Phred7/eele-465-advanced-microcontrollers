/**
 * W. Ward
 * 05/05/2022
 * I2C Secondary - LED Controller - EELE465 Final Project
 *
 * Port Definitions (MSP2310)
 *  #10 - P1.7 - LED
 *  #11 - P1.6 - LED
 *  #13 - P1.5 - LED
 *  #14 - P1.4 - LED
 *  #07 - P2.7 - LED
 *  #08 - P2.6 - LED
 *  #01 - P1.1 - LED
 *  #02 - P1.0 - LED
 *
 *  #03 - SBWTCK
 *  #04 - SBWTDIO
 *  #05 - Vcc
 *  #06 - GND
 *  #09 - P2.0 - System Reset
 *  #15 - P1.2 - SDA
 *  #16 - P1.3 - SCL
 */

#include <msp430.h> 

#define ledAddress 0x042


/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	return 0;
}
