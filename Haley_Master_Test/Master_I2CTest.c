#include <msp430.h>

#define motorAddress 0x042
#define lcdAddress 0x027

/**
 * EELE 465 - Final Project
 * H. Ketteler
 * 4/20/2022
 */

    int delayFlag = 0;      //{"1", Hold Delay; "0", No delay}
    unsigned char colorData[4] = {0x0,0x0,0x0,0x0};     // {Status; Red contribution; Yellow Contribution; Blue Contribution}
    // Status flag: {"0", dummy data; "1", Reset; "2", Color Entry}
    unsigned char i2cTransmitCompleteFlag = 0x00;
    unsigned char i2cDataCounter = 0x00;

    unsigned char i2cStopReached = 0x00;

void dataDelay(unsigned int timerCount){
    delayFlag = 1;

    TB0CCR0 = timerCount;       // Counter compare value

    TB0CCTL0 |= CCIE;           // Enable TB0 Overflow IRQ
    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 flag

    while(delayFlag == 1);      // Loop until delay has finished

    TB0CCTL0 &= ~CCIE;          // Disable TB0 Overflow IRQ

    return;
}

void configI2C(void) {
    //-- Config. I2C Master

    //-- Config I2C Ports
    P4SEL1 &= ~BIT7;            // P4.7 = SCL
    P4SEL0 |= BIT7;
    P4SEL1 &= ~BIT6;            // P4.6 = SDA
    P4SEL0 |= BIT6;

    UCB1CTLW0 |= UCSWRST;

    UCB1CTLW0 |= UCSSEL__SMCLK;
    UCB1BRW = 8; //10
    UCB1CTLW0 |= UCMODE_3 + UCSYNC + UCMST;      // put into I2C mode, , put into master mode
//    UCB1CTLW0 |= UCTR;           // Put into Tx mode
    UCB1I2CSA = lcdAddress;
    UCB1CTLW1 |= UCASTP_2;      // Auto STOP when UCB0TBCNT reached
    UCB1TBCNT = 1; // # of Bytes in Packet

    UCB1CTLW1 |= UCCLTO_1;

    UCB1CTLW0 &= ~UCSWRST;

    UCB1IE |= UCRXIE0;
    UCB1IE |= UCTXIE0;
    UCB1IE |= UCCLTOIE;
    UCB1IE |= UCNACKIE;
    UCB1IE |= UCBCNTIE;
    UCB1IE |= UCSTPIE;
    //-- END Config. I2C Master
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

void configTimerB0(void){
    //-- Setup Timer
    TB0CTL |= TBCLR;            // Clear timer and dividers
    TB0CTL |= TBSSEL__SMCLK;    // Source = SMCLK
    TB0CTL |= MC__UP;           // Mode = Up
    TB0CTL |= ID__8;            // Divide by 8
    TB0EX0 |= TBIDEX__7;        // Divide clock by 8 again
    TB0CCR0 = 32791;            // CCR0 = 32791

    //-- Setup Timer Overflow IRQ
    TB0CCTL0 &= ~CCIE;          // Disable TB0 Overflow IRQ
    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 flag
    __enable_interrupt();       // Enable Maskable IRQs

    return;
}

int sendI2C(int slaveAddress){
    if ((slaveAddress != motorAddress) && (slaveAddress != lcdAddress)){
        return 0;
    }

    UCB1TBCNT = 4;

    UCB1I2CSA = 0x69;

    UCB1CTLW0 |= UCTR;      // put into Tx mode

    i2cDataCounter = 0x00;

    UCB1CTLW0 |= UCTXSTT;   // generate START cond.

//    while ((UCB1IFG & UCSTPIFG) == 0 ); //wait for STOP
//        UCB1IFG &= ~UCSTPIFG;           // clear STOP flag

    while (i2cStopReached == 0x00);
    i2cStopReached = 0x00;
    UCB1IFG &= ~UCSTPIFG;           // clear STOP flag

    UCB1TXBUF = 0x00;

    return 0;
}

void sendData2Slaves(int operationStatus){
    if (operationStatus == 0){      // Reset plant
        colorData[0] = 0;
        colorData[1] = 0;
        colorData[2] = 0;
        colorData[3] = 0;

    } else if (operationStatus == 1){   // Primary color selected
        colorData[0] = 1;
        colorData[1] = 20;
        colorData[2] = 0;
        colorData[3] = 0;

    } else if (operationStatus == 2){   // Mixed color selected
        colorData[0] = 2;
        colorData[1] = 45;
        colorData[2] = 35;
        colorData[3] = 25;
    }

    sendI2C(motorAddress);
 //   sendI2C(lcdAddress);

    return;
}

int main(void){
     WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    configI2C();

    configPorts();

    PM5CTL0 &= ~LOCKLPM5;       // Turn on GPIO

    configTimerB0();

    __enable_interrupt();

    // Initialize color plant
    /* motorData2Send[0] = 0xAA;
     * sendData2Slaves(0);
     *
     */

    // Main loop
    while(1){
//        sendData2Slaves(0);
//        dataDelay(1000);

//        sendData2Slaves(1);
//        dataDelay(1000);

        sendData2Slaves(2);
//        dataDelay(1000);

    }
    return(0);
}

//-- Interrupt Service Routines
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void){
    delayFlag = 0;
    TB0CCTL0 &= ~CCIFG;             // Clear CCR0 Flag
}

//-- I2C Service Routines
#pragma vector=EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void){
    /*
     * Switch on address in UCB1I2CSA
     * Make code call sendI2C to each device which sets the secondary's address.
     * Then use a switch on that address to know what to send and when to stop.
     */
    volatile unsigned char r;
    unsigned int counter;
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

        // TODO: reset any counters and flags.
        i2cTransmitCompleteFlag = 0x00;
        i2cDataCounter = 0x00;

        UCB1IFG &= ~UCSTPIFG; // TODO: these may cause problems...
        UCB1STATW &= ~UCBBUSY;
        break;
    case 0x04:
        /*
         * NACK Received;
         */
        UCB1CTLW0 |= UCTXSTP;   // Generate STOP cond.
        UCB1IFG &= ~UCSTPIFG; // TODO: this may cause problems...
        break;
    case 0x08:
        /*
         * STOP Triggered
         */
        i2cStopReached = 0x01;
        break;
    case 0x18:
        /*
         * Data Transmission
         */
        switch(UCB1I2CSA){
        case motorAddress:
            UCB1TXBUF = colorData[i2cDataCounter];
            i2cDataCounter++;
//            if(i2cDataCounter >= 4){
//                i2cDataCounter = 0;
//            }
            break;
        case lcdAddress:
            UCB1TXBUF = colorData[i2cDataCounter];
            i2cDataCounter++;
//            if(i2cDataCounter >= 4){
//                i2cDataCounter = 0;
//            }
            break;
        default:
            // i2cDataCounter = 0;
            break;
        }
        break;

    default:
        break;
    }
    return;
}
