#include <msp430.h> 

char recievedData = 0x0;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer


    PM5CTL0 &= ~LOCKLPM5;       // Enable GPIO

    P1DIR |= BIT0;        // P1.0
    P1OUT &= ~BIT0;        // Init val = 0

    P4SEL1 &= ~BIT7;            // P4.7 = SCL
    P4SEL0 |= BIT7;

    P4SEL1 &= ~BIT6;            // P4.6 = SDA
    P4SEL0 |= BIT6;

    UCB1CTLW0 |= UCSWRST;

    UCB1CTLW0 |= UCSSEL_3;
    UCB1BRW = 1;
    UCB1CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)

    UCB1CTLW0 &= ~UCMST;
    UCB1I2COA0 = 0x69 | UCOAEN;               // Slave address is 0x77; enable it
    UCB1CTLW0 &= ~UCTR;     // set to receive
    UCB1CTLW1 &= ~UCSWACK;  // auto ACK

    UCB1CTLW0 &= ~UCSWRST;

    UCB1CTLW1 &= ~UCRXIFG0;

    UCB1IE |= UCRXIE0;
    UCB1IE |= UCTXIE0;
    __enable_interrupt();

    P1OUT |= BIT0;

    int i;
    while(1);

    return 0;
}

//-- Service I2C
#pragma vector = EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void) {
    recievedData = UCB1RXBUF;
    P1OUT &= ~BIT0;
    UCB1CTLW1 &= ~UCRXIFG0;
    return;
}
//-- END EUSCI_B0_I2C_ISR

