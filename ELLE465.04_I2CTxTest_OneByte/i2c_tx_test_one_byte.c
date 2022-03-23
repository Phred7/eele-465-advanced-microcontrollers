#include <msp430.h>

int dataSent = 0;

int send_i2c(void) {
    int i;
    UCB1CTLW0 |= UCTXSTT;
    while (UCB0CTLW0 & UCTXSTP);
    for(i=0; i<15; i=i+1){}
    UCB1CTLW0 |= UCTXSTT;
    while (UCB0CTLW0 & UCTXSTP);
    for(i=0; i<15; i=i+1){}
    return 0;
}

int resetLEDs(void) {
    // *
}

int resetLCD(void) {
    // #
}

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
    UCB1I2CSA = 0x0042;

    UCB1CTLW1 |= UCASTP_2;      // Auto STOP when UCB0TBCNT reached
    UCB1TBCNT = 2; // # of Bytes in Packet

    //-- Config Ports
    P4SEL1 &= ~BIT7;            // P4.7 = SCL
    P4SEL0 |= BIT7;

    P4SEL1 &= ~BIT6;            // P4.6 = SDA
    P4SEL0 |= BIT6;

    UCB1CTLW0 &= ~UCSWRST;

    UCB1IE |= UCTXIE0;

    __enable_interrupt();

//    while(1) {
//        UCB1CTLW0 |= UCTXSTT;
//        while (UCB0CTLW0 & UCTXSTP) {}
//        for(i=0; i<100; i=i+1) {}
//    }
    while(1) {
        send_i2c();
    }
    return 0;

}

#pragma vector=EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void){
    if (dataSent == 1) {
        dataSent = 0;
        if (UCB1I2CSA == 0x0042) {
            UCB1I2CSA = 0x0069;
        } else {
            UCB1I2CSA = 0x0042;
        }
        UCB1IFG &= ~UCTXIFG0;
        UCB1CTL1 |= UCTXSTP;
    } else {
        if (UCB1I2CSA == 0x0042) {
            UCB1TXBUF = 0x0BB;
        } else {
            UCB1TXBUF = 0x0AA;
        }
        dataSent = 1;
        UCB1IFG &= ~UCTXIFG0;
    }
    return;

}
