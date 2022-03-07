#include <msp430.h> 

/** W. Ward
 *  11/14/2021
 *  I2C Tx
 */

int dataCnt = 0;
char packet[] = { 0x03, 0x15, 0x26, 0x10, 0x14, 0x00, 0x11, 0x21 }; // 10:26:15 Sunday 11/14/'21//{ 0x03, 0x31, 0x10, 0x08, 0x14, 0x00, 0x11, 0x21 }; // 8:10:31 Sunday 11/14/'21

int delay(int delay){
    int zzz;
    for(zzz=0; zzz<delay; zzz++){}
    return 0;
}


//-- Config Ports
//    P1SEL1 &= ~BIT1;            // P1.1 = SCL
//    P1SEL0 |= BIT1;
//
//    P1SEL1 &= ~BIT2;            // P1.2 = SDA
//    P1SEL0 |= BIT2;
int main(void){

    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5;       // disable LPM

    //-- Put eUSCI_B0 into sw reset
    UCB1CTLW0 |= UCSWRST;

    //-- Config eUSCI_B0
    UCB1CTLW0 |= UCSSEL_3;       // eUSCI clk is 1MHz
    UCB1BRW = 10;               // Prescaler=10 to make SCLK=100kHz

    UCB1CTLW0 |= UCMODE_3;      // put into I2C mode
    UCB1CTLW0 |= UCMST;         // put into master mode
    UCB1CTLW0 |= UCTR;           // Put into Tx mode
    UCB1I2CSA = 0x0012;         // secondary 0x68

    UCB1TBCNT = sizeof(packet) - 1; // # of Bytes in Packet
    UCB1CTLW1 |= UCASTP_2;      // Auto STOP when UCB0TBCNT reached


    //-- Config Ports
    P4SEL1 &= ~BIT7;            // P4.7 = SCL
    P4SEL0 |= BIT7;

    P4SEL1 &= ~BIT6;            // P4.6 = SDA
    P4SEL0 |= BIT6;

    //-- Take eUSCI_B0 out of SW reset
    UCB1CTLW0 &= ~UCSWRST;

    //-- Enable Interrupts
    UCB1IE |= UCTXIE0;          // Enable I2C Tx0 IRQ
    __enable_interrupt();       // Enable Maskable IRQs

    int i;
    while(1) {
        UCB1CTLW0 |= UCTXSTT;   // Genearte START condition
        UCB1IFG |= UCTXIFG0;
        for(i=0; i<100; i=i+1){} //delay loop
    }

    return 0;
}
//-- END main

//-- Interrupt Service Routines -------------------------
#pragma vector = EUSCI_B1_VECTOR
__interrupt void  EUSCI_B1_I2C_ISR(void){
    if (dataCnt == (sizeof(packet) - 1)) {
        UCB1TXBUF = packet[dataCnt];
        UCB1IFG &= ~UCTXIFG0;
        dataCnt = 0;
    } else {
        UCB1TXBUF = packet[dataCnt];
        dataCnt++;
    }
}
