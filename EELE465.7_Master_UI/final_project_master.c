/**
 * W. Ward
 * 05/05/2022
 * I2C Main - System Controller - EELE465 Final Project
 *
 * Port Definitions (MSP2355)
 *  #01 - P1.2 - ADC, potentiometer
 *  #04 - SBWTCK
 *  #05 - SBWTDIO
 *  #06 - Vcc
 *  #07 - GND
 *  #12 - P4.7 - SCL
 *  #13 - P4.6 - SDA
 *  #25 - P4.1 - Switch
 *  #26 - P4.0 - Button
 *  #35 - P3.7 - Keypad
 *  #36 - P3.6 - Keypad
 *  #37 - P3.5 - Keypad
 *  #38 - P3.4 - Keypad
 *  #39 - P5.4 - System Reset
 *  #44 - P3.3 - Keypad
 *  #45 - P3.2 - Keypad
 *  #46 - P3.1 - Keypad
 *  #47 - P3.0 - Keypad
 */

#include <msp430.h> 

#define ledAddress 0x042
#define lcdAddress 0x069

// General globals
unsigned char reset = 0x00;
unsigned char keypadValue = 0x00;
unsigned char currentControlMode = 0x11; //A - 0x081, B - 0x041, C - 0x021, D - 0x011

// I2C globals
unsigned char lcdDataToSend[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char ledDataToSend[1] = { 0x00 };

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	return 0;
}



//-- Interrupt Service Routines -------------------------

//-- Service P3 - Keypad
#pragma vector = PORT3_VECTOR
__interrupt void ISR_P3_Keypad(void) {
    int buttonValue = 0b0;
    buttonValue = P3IN;     // Move input to variable

    P3DIR &= ~0b00001111;   // Set columns as input
    P3OUT &= ~0b00001111;   // Set pull-down resistors for rows
    P3DIR |=  0b11110000;   // Set rows as outputs
    P3OUT |=  0b11110000;   // Set rows high

    buttonValue = buttonValue & P3IN;   // Add both nibbles together

    keypadValue = buttonValue;

    if (keypadValue == 0x012) {
        reset = 1;
        P1OUT &= ~BIT0;
        P6OUT &= ~BIT6;
    }

    //A - 0x081, B - 0x041, C - 0x021, D - 0x011
    if (n != 0x00) {
        if (keypadValue == 0x081 || keypadValue == 0x041 || keypadValue == 0x021 || keypadValue == 0x011) {
            currentControlMode = keypadValue;
            resetRTC();
        }
    }

    configKeypad();

    P3IFG &= ~0x0FF;
}
