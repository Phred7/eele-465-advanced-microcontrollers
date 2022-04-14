#include <msp430.h> 

#define ledAddress 0x042
#define lcdAddress 0x069
#define rtcAddress 0x068
#define tempAddress 0x018

/**
 * W. Ward and H. Ketteler
 * 04/10/2022
 * I2C Main - System Controller
 */

unsigned char keypadValue = 0x00;
unsigned char currentControlMode = 0x11; //A - 0x081, B - 0x041, C - 0x021, D - 0x011
unsigned char n = 0x00;
unsigned char numberOfReadings = 0x00;
unsigned char newADCReading = 0x00;
unsigned char reset = 0x00;
unsigned char i2cTriggerOneSecond = 0x00;
unsigned char i2cTriggerHalfSecond = 0x00;
unsigned char i2cTransmitCompleteFlag = 0x00;
unsigned char i2cReceiveCompleteFlag = 0x00;
unsigned char i2cDataCounter = 0x00;
unsigned char adcTemp[2] = { 0x00, 0x00 };
unsigned char i2cTemp[2] = { 0x00, 0x00 };
unsigned char lcdDataToSend[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char ledDataToSend[1] = { 0x00 };
unsigned char rtcDataRecieved[2] = { 0x00, 0x00 };  // bcd {sec, min}
unsigned char tempDataRecieved[2] = { 0x00, 0x00 };
unsigned char rtcInitialization[8] = { 0xAD, 0x00, 0x00, 0x00, 0x04, 0x01, 0x01, 0x97 }; // 00:00:00 Thursday 01/01/'97  {time_cal_addr, t.sec, t.min, t.hour, t.wday, t.mday, t.mon, t.year_s}
float adcMovingAverage = 0.0;
float i2cMovingAverage = 0.0;
float i2cTempReadings[10] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
float adcReadings[10] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

void configI2C(void) {
    //-- Config. I2C Master
    //-- Put eUSCI_B0 into SW reset
    UCB1CTLW0 |= UCSWRST;
    UCB1CTLW0 |= UCSSEL_3;
    UCB1BRW = 10;
    UCB1CTLW0 |= UCMODE_3;
    UCB1CTLW0 |= UCMST;
    UCB1CTLW0 |= UCTR;
    UCB1I2CSA = 0x068;
    UCB1CTLW1 |= UCASTP_2;      // Auto STOP when UCB0TBCNT reached
    UCB1TBCNT = 3;              // # of Bytes in Packet

    //-- Config I2C Ports
    P4SEL1 &= ~BIT7;            // P4.7 = SCL
    P4SEL0 |= BIT7;
    P4SEL1 &= ~BIT6;            // P4.6 = SDA
    P4SEL0 |= BIT6;

    UCB1CTLW0 &= ~UCSWRST;

    UCB1IE |= UCTXIE0;
    UCB1IE |= UCRXIE0;
    UCB1IE |= UCCLTOIE;
    //-- END Config. I2C Master
    return;
}

void configTimer(void){
    // Timers
    // TB0
    TB0CTL |= TBCLR;                // Clear timer and divs
    TB0CTL |= TBSSEL__SMCLK;        // SRC = SMCLK
    TB0CTL |= MC__UP;               // Mode = UP
    TB0CTL |= CNTL_0;               // Length = 16-bit
    TB0CTL |= ID__8;                // ste d1 to 8
    TB0EX0 |= TBIDEX__7;            // set d2 to 7
    TB0CCR0 = 9366;                 // CCR0 = (1 s) w/ d2 = 7 1sec: 18732, .5sec = 9366 From pg 297 or TB
    //TB0CCR1 = 18732;
    return;
}

void enableTimerInterrupt(int timerCompareValue0){ // , int timerCompareValue1
    // IRQs
    // Timer Compare IRQ
    TB0CCR0 = timerCompareValue0;
    TB0CCTL0 |= CCIE;               // Enable TB0 CCR0 overflow IRQ
    TB0CCTL0 &= ~CCIFG;             // Clear CCR0 flag
//    TB0CCR1 = timerCompareValue1;
//    TB0CCTL1 |= CCIE;               // Enable TB0 CCR1 overflow IRQ
//    TB0CCTL1 &= ~CCIFG;             // Clear CCR1 flag
}

void disableTimerInterrupt() {
    TB0CCTL0 &= ~CCIE;              // Disable TB0 CCR0 overflow IRQ
    TB0CCTL0 &= ~CCIFG;             // Clear CCR0 flag
//    TB0CCTL1 |= CCIE;               // Disable TB0 CCR1 overflow IRQ
//    TB0CCTL1 &= ~CCIFG;             // Clear CCR1 flag
    return;
}

int send_i2c(int slaveAddress) {

    if (slaveAddress != ledAddress && slaveAddress != lcdAddress && slaveAddress != rtcAddress && slaveAddress != tempAddress) {
        return 0;
    }

    UCB1I2CSA = slaveAddress;

    switch (slaveAddress) {
    case lcdAddress:
        UCB1TBCNT = 8;
        break;
    case ledAddress:
        UCB1TBCNT = 1;
        break;
    case rtcAddress:
        UCB1TBCNT = 1;
        break;
    case tempAddress:
        UCB1TBCNT = 1;
        break;
    default:
        UCB1TBCNT = 1;
        break;
    }

    i2cTransmitCompleteFlag = 0x01;

    UCB1CTLW0 |= UCTR;      // put into Tx mode
    UCB1CTLW0 |= UCTXSTT;   // generate START cond.

    while ((UCB1IFG & UCSTPIFG) == 0 ); // wait for STOP
        UCB1IFG &= ~UCSTPIFG;           // clear STOP flag

    while (i2cTransmitCompleteFlag != 0x00);

    P1OUT ^= BIT0;

    return 0;
}

int recieve_i2c(int slaveAddress) {

    if (slaveAddress != ledAddress && slaveAddress != lcdAddress && slaveAddress != rtcAddress && slaveAddress != tempAddress) {
        return 0;
    }

    UCB1I2CSA = slaveAddress;

    switch (slaveAddress) {
    case lcdAddress:
        UCB1TBCNT = 1;
        break;
    case ledAddress:
        UCB1TBCNT = 1;
        break;
    case rtcAddress:
        send_i2c(rtcAddress);
        UCB1TBCNT = 2;
        break;
    case tempAddress:
        //send_i2c(tempAddress);
        UCB1TBCNT = 2;
        break;
    default:
        UCB1TBCNT = 1;
        break;
    }

    i2cReceiveCompleteFlag = 0x01;

    UCB1CTLW0 &= ~UCTR;     // Put into Rx mode
    UCB1CTLW0 |= UCTXSTT;   // Generate START cond.

    while ((UCB1IFG & UCSTPIFG) == 0 ); // wait for STOP


    while (i2cReceiveCompleteFlag != 0x00);
    UCB1IFG &= ~UCSTPIFG;           // clear STOP flag

    return 0;
}



int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	P1DIR |= BIT0;        // P1.0 LED 1
    P1OUT &= ~BIT0;        // Init val = 0
    P6DIR |= BIT6;        // P6.6 LED 2
    P6OUT &= ~BIT6;        // Init val = 0

    PM5CTL0 &= ~LOCKLPM5;       // disable LPM

    configI2C();

    configTimer();

    __enable_interrupt();

    // start RTC?

    // enable timers? enableTimerInterrupts(9366, 18732);

    // wait for N from user. Dont convert N to Dec.

    //A - 0x081, B - 0x041, C - 0x021, D - 0x011
    /* control peltier based on input mode
     * if input is A heat-only
     * if input is B cool-only
     * if input is C temperature match w/ room temperature
     * if input is D disable the system
    */

    enableTimerInterrupt(9366);

    while(1) {

        if (i2cTriggerHalfSecond == 0x01) {
            i2cTriggerHalfSecond = 0x00;
            recieve_i2c(tempAddress);
            send_i2c(lcdAddress);
            send_i2c(ledAddress);
        }

        if (i2cTriggerOneSecond == 0x02) {
            i2cTriggerOneSecond = 0x00;
            recieve_i2c(rtcAddress);
        }
    }

	return 0;
}



//-- Interrupt Service Routines -------------------------
//-- Service I2C B1
#pragma vector=EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void){
    /*
     * Enable resend after a NAK with UCB1IV and clock low timeout
     */

    /*
     * Switch on address in UCB1I2CSA
     * Make code call sendI2C to each device which sets the secondary's address.
     * Then use a switch on that address to know what to send and when to stop.
     */
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
        i2cTransmitCompleteFlag = 0x00;
        i2cReceiveCompleteFlag = 0x00;
        break;
    case 0x16:
        /*
         * Data Received;
         */
        switch (UCB1I2CSA) {
        case rtcAddress:
            if (i2cDataCounter == 0x02) {
                i2cDataCounter = 0x00;
                i2cReceiveCompleteFlag = 0x00;
            } else {
                rtcDataRecieved[i2cDataCounter] = UCB1RXBUF;
                i2cDataCounter++;
            }
            break;
        case tempAddress:
            if (i2cDataCounter == 0x02) {
                i2cDataCounter = 0x00;
                i2cReceiveCompleteFlag = 0x00;
            } else {
                tempDataRecieved[i2cDataCounter] = UCB1RXBUF;
                i2cDataCounter++;
            }
            break;
        default:
            i2cDataCounter = 0x00;
            i2cReceiveCompleteFlag = 0x00;
            break;
        }

        break;
    case 0x18:
        /*
         * Data Transmission;
         */
        switch (UCB1I2CSA) {
        case rtcAddress:
            UCB1TXBUF = 0x00;
            i2cDataCounter = 0x00;
            i2cTransmitCompleteFlag = 0x00;
            break;
        case lcdAddress:
            if (i2cDataCounter == 0x08) {
                i2cDataCounter = 0x00;
                i2cTransmitCompleteFlag = 0x00;
            } else {
                UCB1TXBUF = lcdDataToSend[i2cDataCounter];
                i2cDataCounter++;
            }
            break;
        case ledAddress:
            UCB1TXBUF = ledDataToSend[0];
            i2cDataCounter = 0x00;
            i2cTransmitCompleteFlag = 0x00;
            break;
        case tempAddress:
            UCB1TXBUF = 0x05;
            i2cDataCounter = 0x00;
            i2cTransmitCompleteFlag = 0x00;
        default:
            i2cDataCounter = 0x00;
            i2cTransmitCompleteFlag = 0x00;
            break;
        }

        break;
    default:
        break;
    }

    UCB1IFG &= ~UCTXIFG0;
    return;
}
//-- END I2C B1 ISR

//-- Service TB0
// Service CCR0
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void) {
    // Trigger I2C somehow?

//    ADCCTL0 |= ADCENC | ADCSC;          // Start ADC
//    while((ADCIFG & ADCIFG0) == 0);     // Wait for conversion completion
//    newADCReading = ADCMEM0;                // Read ADC value

    i2cTriggerHalfSecond = 0x01;
    i2cTriggerOneSecond++;

    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 flag
}

