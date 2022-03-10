#include <msp430.h> 

char recievedData = 0x0;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer


    PM5CTL0 &= ~LOCKLPM5;       // Enable GPIO

    P1DIR |= BIT0;        // P1.0
    P1OUT &= ~BIT0;        // Init val = 0

    P1DIR |= BIT1;        // P1.1
    P1OUT &= ~BIT1;        // Init val = 0

    P1SEL1 &= ~BIT3;         // P1.3 SCL
    P1SEL0 |= BIT3;
    P1SEL1 &= ~BIT2;        // P1.2 SDA
    P1SEL0 |= BIT2;

    UCB0CTLW0 |= UCSWRST;

    UCB0CTLW0 |= UCSSEL_3;
    UCB0BRW = 1;
    UCB0CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)

    UCB0CTLW0 &= ~UCMST;
    UCB0I2COA0 = 0x42 | UCOAEN;               // Slave address is 0x77; enable it
    UCB0CTLW0 &= ~UCTR;     // set to receive
    UCB0CTLW1 &= ~UCSWACK;  // auto ACK

    UCB0CTLW0 &= ~UCSWRST;

    UCB0CTLW1 &= ~UCRXIFG0;

    UCB0IE |= UCRXIE0;
    UCB0IE |= UCTXIE0;

    __enable_interrupt();

    P1OUT |= BIT0;
    P1OUT |= BIT1;

    int i;
    while(1);

    return 0;
}

//-- Service I2C
#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    recievedData = UCB0RXBUF;
    P1OUT &= ~BIT0;
    UCB0CTLW1 &= ~UCRXIFG0;
    return;
}
//-- END EUSCI_B0_I2C_ISR

