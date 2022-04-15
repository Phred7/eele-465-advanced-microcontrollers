#include <msp430.h> 

unsigned char recievedData = 0x0;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5;       // Enable GPIO

    P2DIR |= BIT0;        // P1.0
    P2OUT &= ~BIT0;        // Init val = 0

    P1SEL1 &= ~BIT3;            // P1.3 = SCL
    P1SEL0 |= BIT3;

    P1SEL1 &= ~BIT2;            // P1.2 = SDA
    P1SEL0 |= BIT2;

    UCB0CTLW0 |= UCSWRST;

    UCB0CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)

    UCB0CTLW0 &= ~UCMST;
    UCB0I2COA0 = 0x42 | UCOAEN;               // Slave address is 0x42; enable it
    UCB0CTLW0 &= ~UCTR;     // set to receive

    UCB0CTLW0 &= ~UCSWRST;

    UCB0CTLW1 &= ~UCRXIFG0;

    UCB0IE |= UCRXIE0;
    UCB0IE |= UCCLTOIE;
    __enable_interrupt();

    P2OUT |= BIT0;

    while(1);

    return 0;
}

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
        P1SEL0 &= ~(BIT3);      //  then SCL by disconnecting from the I2C
        UCB0CTLW0 |= UCSWRST;   // Reset
        UCB0CTLW0 &= ~UCSWRST;
        P1SEL0 |=  (BIT2|BIT3); // Re-connect pins to I2C
        UCB0IE = r;             // Put IE back
//        UCB1IFG &= ~UCCLTOIFG;
        break;
    case 0x16:
        recievedData = UCB0RXBUF;
        P2OUT ^= BIT0;
        break;
    }
    return;
}
//-- END EUSCI_B0_I2C_ISR

