#include <msp430.h> 

char recievedData = 0x0;

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

	UCB0CTLW0 |= UCSWRST;

	UCB0CTLW0 |= UCMODE_3;  // Choose I2C mode
	UCB0CTLW0 &= ~UCMST;    // Choose to be slave
	UCB0I2COA0 = 0x0068;    // slave addr
	UCB0I2COA0 |= UCOAEN;  // use slave addr set above
	UCB0CTLW0 &= ~UCTR;     // set to receive
	UCB0CTLW1 &= ~UCSWACK;  // auto ACK

	P1SEL1 &= ~BIT3;         // P1.3 SCL
	P1SEL0 |= BIT3;
	P1SEL1 &= ~BIT2;        // P1.2 SDA
	P1SEL0 |= BIT2;

	UCB0CTLW1 &= ~UCTXIFG0;

	UCB0CTLW0 &= ~UCSWRST;

	UCB0IE |= UCRXIE0;
	__enable_interrupt();

	P1DIR |= BIT0;        // P1.0
    P1OUT &= ~BIT0;        // Init val = 0
	P1DIR |= BIT1;        // P1.1
    P1OUT &= ~BIT1;        // Init val = 0

    PM5CTL0 &= ~LOCKLPM5;       // Enable GPIO

	P1OUT |= BIT0;
	P1OUT |= BIT1;

	while(1) {

	}

	return 0;
}

//-- Service I2C
#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    UCB0IE &= ~UCRXIE0;
    recievedData = UCB0RXBUF;
    P1OUT ^= BIT0;
//    switch(UCB0IV){
//        case 0x68:                  // Id x68 rxifg0
//
//            break;
//        default:
//            break;
//    }
}
//-- END EUSCI_B0_I2C_ISR
