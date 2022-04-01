#include <msp430.h> 


/**
 * main.c
 */

unsigned int keypadValue = 0x0;


void configKeypad(void){
    //-- Setup Ports for Keypad. 3.7 is LeftMost
    P3DIR &= ~0b11110000;   // Set rows as inputs
    P3OUT &= ~0b11110000;   // Set pull-down resistors for rows
    P3DIR |=  0b00001111;   // Set columns as outputs
    P3OUT |=  0b00001111;   // Set columns high
    P3REN |= 0xFF;          // Enable pull-up/pull-down resistors on port 3
    return;
}


int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

    configKeypad();         // Enable pull-up/pull-down resistors on port 3

    P3IE |= 0x0FF;
    P3IES &= ~0x0FF;
    P3IFG &= ~0x0FF;

    PM5CTL0 &= ~LOCKLPM5;       // disable LPM

    __enable_interrupt();

    while(1);

	return 0;
}

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

    configKeypad();

    P3IFG &= ~0x0FF;
}
