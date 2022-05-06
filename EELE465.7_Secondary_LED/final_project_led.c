/**
 * W. Ward
 * 05/05/2022
 * I2C Secondary - LED Controller - EELE465 Final Project
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
