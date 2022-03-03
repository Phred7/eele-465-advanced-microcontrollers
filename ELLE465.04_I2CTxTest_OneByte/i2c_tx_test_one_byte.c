#include <msp430.h>

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    UCB0CTLW0 |= UCSSEL_3;
    UCB0BRW = 10;

    UCB0CTLW0 |= UCMODE_3;
    UCB0CTLW0 |= UCMST;
    UCB0CTLW0 |= UCTR;
    UCB0I2CSA = 0x0068;

    P1SEL1 &= ~BIT3;    // SCL
    P1SEL0 |= BIT3;

    P1SEL1 &= ~BIT2;
    P1SEL0 |= BIT2;

    PM5CTL0 &= ~LOCKLPM5;

    UCB0CTLW0 &= ~UCSWRST;

    UCB0IE |= UCTXIE0;

    __enable_interrupt();

    int i;
    while(1) {
        UCB0CTLW0 |= UCTXSTT;
        UCB0IFG |= UCTXIFG0;
        for(i=0; i<100; i=i+1) {}
    }
    return 0;
}

#pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void){
    UCB0TXBUF = 0xBB;
    UCB0IFG &= ~UCTXIFG0;
}
