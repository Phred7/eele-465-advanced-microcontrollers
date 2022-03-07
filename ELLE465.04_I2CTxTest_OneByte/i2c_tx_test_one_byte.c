#include <msp430.h>

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    PM5CTL0 &= ~LOCKLPM5;       // disable LPM

    //-- Put eUSCI_B0 into sw reset
    UCB1CTLW0 |= UCSWRST;

    UCB1CTLW0 |= UCSSEL_3;
    UCB1BRW = 10;

    UCB1CTLW0 |= UCMODE_3;
    UCB1CTLW0 |= UCMST;
    UCB1CTLW0 |= UCTR;
    UCB1I2CSA = 0x0012;

    UCB1TBCNT = 1; // # of Bytes in Packet
    UCB1CTLW1 |= UCASTP_2;      // Auto STOP when UCB0TBCNT reached

//    P1SEL1 &= ~BIT3;    // SCL
//    P1SEL0 |= BIT3;
//
//    P1SEL1 &= ~BIT2;
//    P1SEL0 |= BIT2;

    //-- Config Ports
    P4SEL1 &= ~BIT7;            // P4.7 = SCL
    P4SEL0 |= BIT7;

    P4SEL1 &= ~BIT6;            // P4.6 = SDA
    P4SEL0 |= BIT6;

    PM5CTL0 &= ~LOCKLPM5;

    UCB1CTLW0 &= ~UCSWRST;

    UCB1IE |= UCTXIE0;

    __enable_interrupt();

    int i;
    while(1) {
        UCB1CTLW0 |= UCTXSTT;
        UCB1IFG |= UCTXIFG0;
        for(i=0; i<100; i=i+1) {}
    }
    return 0;
}

#pragma vector=EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void){
    UCB1TXBUF = 0xBB;
    UCB1IFG &= ~UCTXIFG0;
}
