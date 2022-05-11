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

unsigned int receivedData = 0x00;       // pattern from master
unsigned int patternData = 0x00;        // current pattern
unsigned int lastPatternData = 0x00;    // last pattern
unsigned int patternAMask = 0x01;       // incrementing
unsigned int patternBMask = 0x80;       // decrementing
unsigned int patternCMask = 0x00;       // at target
unsigned int patternDMask = 0x00;       // standby
unsigned int patternD1 = 0xAA;      // 0b10101010
unsigned int patternD2 = 0x55;      // 0b01010101

/*
 * Configures I2C pins and registers for slave device on UCB0 with address 42h
 */
void configI2C(void) {

    P1SEL1 &= ~BIT3;      // P1.3 SCL
    P1SEL0 |= BIT3;
    P1SEL1 &= ~BIT2;      // P1.2 SDA
    P1SEL0 |= BIT2;

    UCB0CTLW0 |= UCSWRST;

    UCB0CTLW0 |= UCSSEL_3;
    UCB0BRW = 1;
    UCB0CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)

    UCB0CTLW0 &= ~UCMST;
    UCB0I2COA0 = 0x42 | UCOAEN;               // Slave address is 0x42; enable it
    UCB0CTLW0 &= ~UCTR;     // set to receive
    UCB0CTLW1 &= ~UCSWACK;  // auto ACK

    UCB0CTLW1 |= UCCLTO_2; // clock low reset timeout

    UCB0CTLW0 &= ~UCSWRST;

    UCB0CTLW1 &= ~UCRXIFG0;

    UCB0IE |= UCRXIE0;
    UCB0IE |= UCTXIE0;
    UCB0IE |= UCCLTOIE;
    return;
}

/*
 * Configures the pins and registers for keypad (with polling).
 */
void configTimer(void){
    // Timers
    // TB0
    TB0CTL |= TBCLR;                // Clear timer and divs
    TB0CTL |= TBSSEL__SMCLK;        // SRC = SMCLK
    TB0CTL |= MC__UP;               // Mode = UP
    TB0CTL |= CNTL_0;               // Length = 16-bit
    TB0CTL |= ID__8;                // set d1 to 8
    TB0EX0 |= TBIDEX__7;            // set d2 to 7
    TB0CCR0 = 6178;                 // Set timer compare to 1/3 s
    TB0CCTL0 |= CCIE;               // Enable TB0 CCR0 overflow IRQ
    TB0CCTL0 &= ~CCIFG;             // Clear CCR0 flag

    return;
}

/*
 * Uses the mask to write the correct values to each pin for the LED bar on a 2310.
 */
void writeToLEDBar(unsigned int mask) {
    static const unsigned int P1Mask = 0b11110011;
    static const unsigned int P2Mask = 0b00001100;
    P1OUT = (mask & (P1Mask)) | (P1OUT & (P2Mask));
    unsigned int p2LED = P2OUT & 0x01;
    P2OUT = (mask & (P2Mask)) << 4;
    P2OUT |= p2LED;
    return;
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5;       // Enable GPIO

    // LED Bar:
    P1DIR |= 0b11110011;
    P1OUT &= ~0b11110011;

    P2DIR |= 0b11000000;
    P2OUT &= ~0b11000000;

    P2DIR |= BIT0;
    P2OUT &= ~BIT0;

    configI2C();

    configTimer();

    __enable_interrupt();

    writeToLEDBar(0xAA);

    while(1){
        patternData = receivedData;
        if (patternData != 0x00){
            switch(patternData){
            case 0x081:
                patternData = 0;
                break;
            case 0x041:
                patternData = 1;
                break;
            case 0x021:
                patternData = 3;
                break;
            case 0x011:
                patternData = 2;
                break;
            default:
                break;
            }
        } else {
            switch(lastPatternData){
            case 0:
                patternData = 0;
                break;
            case 1:
                patternData = 1;
                break;
            case 2:
                patternData = 2;
                break;
            case 3:
                patternData = 3;
                break;
            default:
                break;
            }
        }

        if ((patternData != lastPatternData) && (patternData != 0x00)){
            switch (patternData){
            case 0:     // A
                patternAMask = 0x00;
                writeToLEDBar(patternAMask);
                break;
            case 1:     // B
                patternBMask = 0x80;
                writeToLEDBar(patternBMask);
                break;
            case 2:     // D
                patternDMask = patternD1;
                writeToLEDBar(patternDMask);
                break;
            case 3:     // C
                patternCMask = 0x00;
                writeToLEDBar(patternCMask);
                break;
            default:
                break;
            }
        } else {
            switch (patternData){
            case 0:
                writeToLEDBar(patternAMask);
                break;
            case 1:
                writeToLEDBar(patternBMask);
                break;
            case 2:
                writeToLEDBar(patternDMask);
                break;
            case 3:     // C
                writeToLEDBar(patternCMask);
                break;
            default:
                break;
            }
        }

        if(patternData != 0x00){
            lastPatternData = patternData;
        }
    }
    return 0;
}

//-- Interrupt Service Routines -------------------------
//-- Service I2C
#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    volatile unsigned char r;
    switch (UCB0IV) {
    case 0x1C:
        /*
         * Clock Low Time-Out;
         * Adapted from https://e2e.ti.com/support/microcontrollers/msp-low-power-microcontrollers-group/msp430/f/msp-low-power-microcontroller-forum/869750/msp430fr2433-i2c-clock-low-timeout-interrupt
         */
        r = UCB0IE;             // Save current IE bits
        P1SEL0 &= ~(BIT2);      // Generate NACK by releasing SDA
        P1SEL0 &= ~(BIT3);      //  then SCL by disconnecting from the I2C // P1.3 SCL
        UCB0CTLW0 |= UCSWRST;   // Reset
        UCB0CTLW0 &= ~UCSWRST;
        P1SEL0 |=  (BIT2|BIT3); // Re-connect pins to I2C
        UCB0IE = r;             // Put IE back
//        UCB1IFG &= ~UCCLTOIFG;
        break;
    case 0x16:
        /*
         * Data Received;
         */
        receivedData = UCB0RXBUF;
        UCB0CTLW1 &= ~UCRXIFG0;
        P2OUT ^= BIT0;
        break;
    default:
        break;
    }
    return;
}
//-- END EUSCI_B0_I2C_ISR

//-- Service TB0
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void) {
    switch (patternData) {
    case 0:
        if(patternAMask == 0xFF){
            patternAMask = 0x01;
        } else {
            patternAMask <<= 1;
            patternAMask++;
        }
        break;
    case 1:
        if(patternBMask == 0xFF){
            patternBMask = 0x80;
        } else {
            patternBMask >>= 1;
            patternBMask = patternBMask + 0x80;
        }
        break;
    case 2:
        if (patternDMask == patternD1){
            patternDMask = patternD2;
        } else if (patternDMask = patternD2){
            patternDMask = patternD1;
        }
        break;
    case 3:
        if (patternCMask != 0x00) {
            patternCMask = 0x00;
        } else {
            patternCMask = 0xFF;
        }
        break;
    }
    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 flag
}
//-- END TB0 ISR
