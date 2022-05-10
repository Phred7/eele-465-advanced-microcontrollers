#include <msp430.h>

unsigned int receivedData[4] = {0,0,0,0};
unsigned int receiveDataCounter = 0;


void configI2C(void) {
    P4SEL1 &= ~BIT7;                    // P4.7 = SCL
    P4SEL0 |= BIT7;

    P4SEL1 &= ~BIT6;                    // P4.6 = SDA
    P4SEL0 |= BIT6;

    UCB1CTLW0 |= UCSWRST;

    UCB1CTLW0 |= UCSSEL_3;
    UCB1BRW = 1;
    UCB1CTLW0 |= UCMODE_3 + UCSYNC;    // I2C mode, sync mode (Do not set clock in slave mode)

    UCB1CTLW0 &= ~UCMST;
    UCB1I2COA0 = 0x69 | UCOAEN;        // Slave address is 0x77; enable it
    UCB1CTLW0 &= ~UCTR;                // set to receive
    UCB1CTLW1 &= ~UCSWACK;             // auto ACK

    UCB1CTLW1 |= UCCLTO_2;              // clock low timeout delay. 5ms slower than master's delay.

    UCB1CTLW0 &= ~UCSWRST;

    UCB1CTLW1 &= ~UCRXIFG0;

    UCB1IE |= UCRXIE0;
    UCB1IE |= UCTXIE0;
    UCB1IE |= UCCLTOIE;

    return;
}

void configPorts(void){
    // Setup LEDs
    P1DIR |= BIT0;              // Set LED1 as an output
    P1OUT &= ~BIT0;             // Turn off LED1 initially
    P6DIR |= BIT6;              // Set LED2 as an output
    P6OUT &= ~BIT6;             // Turn off LED2 initially

    return;
}

int delay(int delay){
    int zzz;
    for(zzz=0; zzz<delay; zzz++){}
    return 0;
}

int main(void){
     WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    configI2C();

    configPorts();

    PM5CTL0 &= ~LOCKLPM5;       // Turn on GPIO

    __enable_interrupt();

    // Main loop
    while(1){
        delay(100);
    }
    return(0);
}

//-- I2C Service Routines
#pragma vector = EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void){
    volatile unsigned char r;
    switch (UCB1IV) {
    case 0x1C:
        /*
         * Clock Low Time-Out;
         * Adapted from https://e2e.ti.com/support/microcontrollers/msp-low-power-microcontrollers-group/msp430/f/msp-low-power-microcontroller-forum/869750/msp430fr2433-i2c-clock-low-timeout-interrupt
         */
        r = UCB1IE;             // Save current IE bits
        P4SEL0 &= ~(BIT6);      // Generate NACK by releasing SDA
        P4SEL0 &= ~(BIT7);      //  then SCL by disconnecting from the I2C
        UCB1CTLW0 |= UCSWRST;   // Reset
        UCB1CTLW0 &= ~UCSWRST;
        P4SEL0 |=  (BIT6|BIT7); // Re-connect pins to I2C
        UCB1IE = r;             // Put IE back

        receiveDataCounter = 0;
        UCB1IFG &= ~UCSTPIFG;
        UCB1STATW &= ~UCBBUSY;
        break;
    case 0x16:
        /*
         * Data Received;
         */
        receivedData[receiveDataCounter] = UCB1RXBUF;
        receiveDataCounter++;

        if (receiveDataCounter >= 4){
            P6OUT ^= BIT6;
            receiveDataCounter = 0;
        }

        UCB1CTLW1 &= ~UCRXIFG0;
        break;
    default:
        break;
    }

    return;
}
