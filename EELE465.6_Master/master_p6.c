#include <msp430.h> 

#define ledAddress 0x042
#define lcdAddress 0x069
#define rtcAddress 0x068
#define tempAddress 0x018
//#define tempAddress 0x048

/**
 * W. Ward and H. Ketteler
 * 04/10/2022
 * I2C Main - System Controller
 */
float convertTempRecievedToTempC(void);

unsigned char keypadValue = 0x00;
unsigned char currentControlMode = 0x11; //A - 0x081, B - 0x041, C - 0x021, D - 0x011
unsigned char n = 0x00;
unsigned char decN = 0;
unsigned char numberOfReadings = 0x00;
unsigned int newADCReading = 0x00;
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
float adcMovingAverageC = 0.0;
float i2cMovingAverage = 0.0;
float i2cTempReadings[10] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
float adcReadings[10] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

// all in degrees Celcius
unsigned char peltierControlLoopStatus = 0x00;
float tempControlLoopUpperThreshold = 0.5;
float tempControlLoopLowerThreshold = -0.5;
unsigned char tempControlLoopThreshold = 0x00;  //0x00 == upper threshold, 0x01 == lower threshold
float tempControlLoopSetPoint = 0.0;

unsigned char timer_i2c_overflow = 0x00;

void configI2C(void) {
    //-- Config. I2C Master
    //-- Put eUSCI_B0 into SW reset

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
    UCB1I2CSA = 0x0068;         // secondary 0x68 RTC
    UCB1CTLW1 |= UCASTP_2;      // Auto STOP when UCB0TBCNT reached
    UCB1TBCNT = 1; // # of Bytes in Packet

    UCB1CTLW1 |= UCCLTO_1;

    UCB1CTLW0 &= ~UCSWRST;

    UCB1IE |= UCRXIE0;
    UCB1IE |= UCTXIE0;
    UCB1IE |= UCCLTOIE;
    UCB1IE |= UCNACKIE;
    UCB1IE |= UCBCNTIE;
    //UCB1IE |= UCBSTPIE;
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

//void configTimer2(void){
//    // Timers
//    // TB1
//    TB1CTL |= TBCLR;                // Clear timer and divs
//    TB1CTL |= TBSSEL__ACLK;         // SRC = SMCLK
//    TB1CTL |= MC__UP;               // Mode = UP
//    TB1CTL |= CNTL_0;               // Length = 16-bit
//    TB1CTL |= ID__8;                // ste d1 to 8
//    TB1EX0 |= TBIDEX__8;            // set d2 to 8
//    TB1CCR0 = 9366;                 // CCR0 = (1 s) w/ d2 = 7 1sec: 18732, .5sec = 9366 From pg 297 or TB
//    //TB0CCR1 = 18732;
//    return;
//}

void enableTimerInterrupt(int timerCompareValue){ // , int timerCompareValue1
    // IRQs
    // Timer Compare IRQ
    TB0CCR0 = timerCompareValue;
    TB0CCTL0 |= CCIE;               // Enable TB0 CCR0 overflow IRQ
    TB0CCTL0 &= ~CCIFG;             // Clear CCR0 flag
}

void disableTimerInterrupt() {
    TB0CCTL0 &= ~CCIE;              // Disable TB0 CCR0 overflow IRQ
    TB0CCTL0 &= ~CCIFG;             // Clear CCR0 flag
    return;
}

//void enableTimerInterrupt2(int timerCompareValue) {
//    TB1CCR0 = timerCompareValue;
//    TB1CCTL0 |= CCIE;               // Enable TB0 CCR1 overflow IRQ
//    TB1CCTL0 &= ~CCIFG;             // Clear CCR1 flag
//}
//
//void disableTimerInterrupt2() {
//    TB1CCTL0 |= CCIE;               // Disable TB0 CCR1 overflow IRQ
//    TB1CCTL0 &= ~CCIFG;             // Clear CCR1 flag
//    TB1R = 0x00;
//    return;
//}

void configADC(void){
    P1SEL1 |= BIT2;                 // Configure A2 ADC
    P1SEL0 |= BIT2;

    PM5CTL0 &= ~LOCKLPM5;

    // Configure ADC protocol
    ADCCTL0 &= ~ADCSHT;
    ADCCTL0 |= ADCSHT_2;
    ADCCTL0 |= ADCON;

    ADCCTL1 |= ADCSSEL_2;
    ADCCTL1 |= ADCSHP;

    ADCCTL2 &= ~ADCRES;
    ADCCTL2 |= ADCRES_2;

    ADCMCTL0 |= ADCINCH_2;
    ADCIE |= ADCIE0;

    return;
}

void configKeypad(void){
    //-- Setup Ports for Keypad. 3.7 is LeftMost
    P3DIR &= ~0b11110000;   // Set rows as inputs
    P3OUT &= ~0b11110000;   // Set pull-down resistors for rows
    P3DIR |=  0b00001111;   // Set columns as outputs
    P3OUT |=  0b00001111;   // Set columns high
    P3REN |= 0xFF;          // Enable pull-up/pull-down resistors on port 3
    return;
}

int delay(int delay){
    int zzz;
    for(zzz=0; zzz<delay; zzz++){}
    return 0;
}

int send_i2c(int slaveAddress) {

    if (slaveAddress != ledAddress && slaveAddress != lcdAddress && slaveAddress != rtcAddress && slaveAddress != tempAddress) {
        return 0;
    }

    //UCB1CTLW0 |= UCSWRST;

    UCB1I2CSA = slaveAddress;

    switch (slaveAddress) {
    case lcdAddress:
        UCB1TBCNT = 8; // 7
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

    i2cDataCounter = 0x00;

    //UCB1CTLW0 &= ~UCSWRST;

    //UCB1IE |= UCTXIE0;

    UCB1CTLW0 |= UCTXSTT;   // generate START cond.

    while ((UCB1IFG & UCSTPIFG) == 0 ); //wait for STOP
        UCB1IFG &= ~UCSTPIFG;           // clear STOP flag

    timer_i2c_overflow = 0x00;

//    while (i2cTransmitCompleteFlag != 0x00);

    //UCB1IE &= ~UCTXIE0;
//
//    P1OUT ^= BIT0;

    return 0;
}

int recieve_i2c(int slaveAddress) {

    if (slaveAddress != ledAddress && slaveAddress != lcdAddress && slaveAddress != rtcAddress && slaveAddress != tempAddress) {
        return 0;
    }

    // UCB1CTLW0 |= UCSWRST;

    UCB1I2CSA = slaveAddress;

    switch (slaveAddress) {
    case lcdAddress:
        UCB1TBCNT = 1;
        break;
    case ledAddress:
        UCB1TBCNT = 1;
        break;
    case rtcAddress:
        break;
    case tempAddress:
        UCB1TBCNT = 2;
        break;
    default:
        UCB1TBCNT = 1;
        break;
    }

    if (slaveAddress == rtcAddress) {
        UCB1CTLW0 |= UCTR;   // Tx

        i2cDataCounter = 0x00;
        UCB1TBCNT = 1;

        UCB1CTLW0 |= UCTXSTT;   // Generate START cond.

        i2cTransmitCompleteFlag = 0x03;

        while ((UCB1IFG & UCSTPIFG) == 0 ); //wait for STOP
            UCB1IFG &= ~UCSTPIFG;           // clear STOP flag

        i2cDataCounter = 0x00;
        UCB1TBCNT = 2;

    } else if (slaveAddress == tempAddress) {
        UCB1CTLW0 |= UCTR;   // Tx

        i2cDataCounter = 0x00;
        UCB1TBCNT = 1;

        UCB1CTLW0 |= UCTXSTT;   // Generate START cond.

        i2cTransmitCompleteFlag = 0x03;

        while ((UCB1IFG & UCSTPIFG) == 0 ); //wait for STOP
            UCB1IFG &= ~UCSTPIFG;           // clear STOP flag

        i2cDataCounter = 0x00;
        UCB1TBCNT = 2;

    }

    i2cTransmitCompleteFlag = 0x00;

    UCB1CTLW0 &= ~UCTR;   // Rx

    UCB1CTLW0 |= UCTXSTT;   // Generate START cond.

    while ((UCB1IFG & UCSTPIFG) == 0 ); //wait for STOP
        UCB1IFG &= ~UCSTPIFG;           // clear STOP flag

    return 0;
}

float getMovingAverage(float averageArray[]) {
    float sumTotal = 0.0;
    int j;
    for(j=0; j<decN; j++) {
        sumTotal += averageArray[j];
    }
    return sumTotal / decN;
}

float convertADCTempToTwoByteTemp(float averageTemp){
    unsigned int val1;
    unsigned int val2;
    float voltConvert;
    voltConvert = (159.65 - (0.0689 * averageTemp));

    // Above the decimal point
    val1 = voltConvert;
    adcTemp[0] = val1;

    // Below the decimal point
    val2 = (voltConvert - val1) * 10;
    adcTemp[1] = val2;

    return voltConvert;
}

void convertI2CTempToTwoByteTemp(float averageTemp){
    unsigned int val1;
    unsigned int val2;
//    float voltConvert;
    //voltConvert = (159.65 - (0.0689 * averageTemp));

    // Above the decimal point
    val1 = averageTemp;
    i2cTemp[0] = val1;

    // Below the decimal point
    val2 = (averageTemp - val1) * 10;
    i2cTemp[1] = val2;

    return;
}

void configPeltier(void){
    P4DIR |= BIT0;        // P4.0
    P4OUT &= ~BIT0;       // Init val = 0
    P4DIR |= BIT1;        // P4.1
    P4OUT &= ~BIT1;       // Init val = 0
    P4DIR |= BIT5;        // P4.5
    P4OUT &= ~BIT5;       // Init val = 0
}

void peltierCool(void) {
    peltierControlLoopStatus = 0x41;
    P4OUT &= ~BIT5;
    delay(10);
    P4OUT |= BIT1;
    P4OUT |= BIT0;
}

void peltierHeat(void) {
    peltierControlLoopStatus = 0x81;
    P4OUT &= ~BIT0;
    P4OUT &= ~BIT1;
    delay(10);
    P4OUT |= BIT5;
}

void peltierDisable() {
    peltierControlLoopStatus = 0x11;
    P4OUT &= ~BIT0;
    P4OUT &= ~BIT1;
    P4OUT &= ~BIT5;
}

void captureStartReadings(void) {
    if(newADCReading > 0) {
        adcReadings[numberOfReadings] = newADCReading;
        recieve_i2c(tempAddress);
        i2cTempReadings[numberOfReadings] = convertTempRecievedToTempC();
        numberOfReadings++;
        newADCReading = 0;
    }
}

void disable(void) {
    rtcDataRecieved[0] = 0x00;
    rtcDataRecieved[1] = 0x00;
    P5OUT &= ~BIT4;
    peltierDisable();
}

void matchTemperature(void) {
    if ((i2cMovingAverage <= (adcMovingAverageC + tempControlLoopUpperThreshold)) && (i2cMovingAverage >= (adcMovingAverageC + tempControlLoopLowerThreshold))) {
        peltierDisable();
    } else if (i2cMovingAverage > (adcMovingAverageC + tempControlLoopUpperThreshold)) {
        peltierCool();
    } else if (i2cMovingAverage < (adcMovingAverageC + tempControlLoopLowerThreshold)) {
        peltierHeat();
    } else {
        peltierDisable();
    }
}

void updateLCDDataToSend(void) {
    lcdDataToSend[0] = n;
    lcdDataToSend[1] = adcTemp[0];
    lcdDataToSend[2] = adcTemp[1];
    lcdDataToSend[3] = currentControlMode;
    lcdDataToSend[4] = rtcDataRecieved[1];
    lcdDataToSend[5] = rtcDataRecieved[0];
    lcdDataToSend[6] = i2cTemp[0];
    lcdDataToSend[7] = i2cTemp[1];
    return;
}

void updateDataToSend(void) {
    adcMovingAverage = getMovingAverage(adcReadings);
    adcMovingAverageC = convertADCTempToTwoByteTemp(adcMovingAverage);
    i2cMovingAverage = getMovingAverage(i2cTempReadings);
    convertI2CTempToTwoByteTemp(i2cMovingAverage);
    updateLCDDataToSend();
    if (currentControlMode == 0x21) {
        ledDataToSend[0] = peltierControlLoopStatus;
    } else {
        ledDataToSend[0] = currentControlMode;
    }
}

unsigned char convertNToDecimal(void) {
    unsigned char decimalValue;
    switch (n) {
    case 0x088:
        decimalValue = 1;
        break;
    case 0x084:
        decimalValue = 2;
        break;
    case 0x082:
        decimalValue = 3;
        break;
    case 0x048:
        decimalValue = 4;
        break;
    case 0x044:
        decimalValue = 5;
        break;
    case 0x042:
        decimalValue = 6;
        break;
    case 0x028:
        decimalValue = 7;
        break;
    case 0x024:
        decimalValue = 8;
        break;
    case 0x022:
        decimalValue = 9;
        break;
    default:
        decimalValue = 0;
        break;
    }
    return decimalValue;
}

float convertTempRecievedToTempC(void) {
    if (tempAddress == 0x18) {
        unsigned char msb;
        unsigned char lsb;
        unsigned short data;
        lsb = tempDataRecieved[1];
        msb = tempDataRecieved[0];
        msb = msb << 3;
        msb = msb >> 3;
        data = ((msb << 8) | lsb);
        return (((float)data)/16) - 129;
    } else {
        unsigned char msb;
        unsigned char lsb;
        unsigned short data;
        lsb = tempDataRecieved[1];
        msb = tempDataRecieved[0];
        lsb = lsb >> 3;
        data = ((msb << 5) | lsb);
        return (((float)(data))*.0625);
        // return 69.420;
    }
}

void resetRTC(void) {
    P5OUT &= ~BIT4;
    delay(100);
    P5OUT |= BIT4;
}


 int main(void) {
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	P1DIR |= BIT0;        // P1.0 LED 1
    P1OUT &= ~BIT0;        // Init val = 0
    P6DIR |= BIT6;        // P6.6 LED 2
    P6OUT &= ~BIT6;        // Init val = 0

    P5DIR |= BIT4;      // temp sensor pwr/rst
    P5OUT &= ~BIT4;

    PM5CTL0 &= ~LOCKLPM5;       // disable LPM

    configI2C();

    configTimer();

    configADC();

    P3IE |= 0x0FF;
    P3IES &= ~0x0FF;
    P3IFG &= ~0x0FF;

    configKeypad();

    configPeltier();

    __enable_interrupt();

    ledDataToSend[0] = 0x11;
    send_i2c(ledAddress);

    // wait for N from user. Dont convert N to Dec.
//    while (n == 0x00) {
//        if (keypadValue > 0x00) {
//            n = keypadValue;
//        }
//    }
    n=0x22;

    enableTimerInterrupt(9366);

    decN = convertNToDecimal();
    while (numberOfReadings < decN) {
        captureStartReadings();
    }

    numberOfReadings = decN + 1;

    //A - 0x081, B - 0x041, C - 0x021, D - 0x011
    /* control peltier based on input mode
     * if input is A heat-only
     * if input is B cool-only
     * if input is C temperature match w/ room temperature
     * if input is D disable the system
    */

    resetRTC();

    while(1) {

        if (i2cTriggerHalfSecond >= 0x01) {
            recieve_i2c(tempAddress);
            unsigned int k;
            for(k=0;k<decN;k++) {
                adcReadings[k] = adcReadings[k+1];
                i2cTempReadings[k] = i2cTempReadings[k+1];
                if (k == (decN-1)) {
                    adcReadings[k] = newADCReading;
                    i2cTempReadings[k] = convertTempRecievedToTempC();
                }
            }
            i2cTriggerHalfSecond = 0x00;
            updateDataToSend();
            send_i2c(lcdAddress);
            send_i2c(ledAddress);
            P6OUT ^= BIT6;
        }

        if (i2cTriggerOneSecond >= 0x02) {
            i2cTriggerOneSecond = 0x00;
            recieve_i2c(rtcAddress);
            P1OUT ^= BIT0;
        }

        if (currentControlMode == 0x081) {
            peltierHeat();
        } else if (currentControlMode == 0x041) {
            peltierCool();
        } else if (currentControlMode == 0x021) {
            matchTemperature();
        } else if (currentControlMode == 0x011) {
            disable();
        } else {
            currentControlMode = 0x011;
        }

        if (rtcDataRecieved[1] >= 0x05) {
            currentControlMode = 0x011;
        }
    }

	return 0;
}



//-- Interrupt Service Routines -------------------------
//-- Service I2C B1
#pragma vector=EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void){
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
//        UCB1IFG &= ~UCCLTOIFG;
        break;
    case 0x04:
        /*
         * NACK Received;
         */
        UCB1CTLW0 |= UCTXSTP;   // Generate STOP cond.
        i2cTransmitCompleteFlag = 0x00;
        i2cReceiveCompleteFlag = 0x00;
        i2cDataCounter = 0x00;
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
//        UCB1IFG &= ~UCRXIFG0;
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
//            UCB1IFG &= ~UCTXIFG0;
            break;
        case lcdAddress:
            UCB1TXBUF = lcdDataToSend[i2cDataCounter];
            i2cDataCounter++;

            if (i2cDataCounter == 0x08) {
                i2cDataCounter = 0x00;
                i2cTransmitCompleteFlag = 0x00;
            }
            break;
        case ledAddress:
            UCB1TXBUF = ledDataToSend[0];
            i2cDataCounter = 0x00;
            i2cTransmitCompleteFlag = 0x00;
//            UCB1IFG &= ~UCTXIFG0;
            break;
        case tempAddress:
            if (tempAddress == 0x18) {
                UCB1TXBUF = 0x05;
            } else {
                UCB1TXBUF = 0x00;
            }
            i2cDataCounter = 0x00;
            i2cTransmitCompleteFlag = 0x00;
//            UCB1IFG &= ~UCTXIFG0;
            break;
        default:
            i2cDataCounter = 0x00;
            i2cTransmitCompleteFlag = 0x00;
//            UCB1IFG &= ~UCTXIFG0;
            break;
        }

        break;
    case 26: // Vector 26: BCNTIFG
        if (i2cTransmitCompleteFlag == 0x03) {
            unsigned int counter;
            counter = UCB1STATW & UCBCNT;
            if ((counter) > 0) {
                i2cTransmitCompleteFlag = 0x00;
            }
        }
        break;
    default:
        break;
    }

    return;
}
//-- END I2C B1 ISR

//-- Service TB0
// Service CCR0
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void) {
    // Trigger I2C somehow?

    ADCCTL0 |= ADCENC | ADCSC;          // Start ADC
    while((ADCIFG & ADCIFG0) == 0);     // Wait for conversion completion
    newADCReading = ADCMEM0;                // Read ADC value

    i2cTriggerHalfSecond = 0x01;
    i2cTriggerOneSecond++;

    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 flag
}

// Service CCR1
//#pragma vector = TIMER1_B0_VECTOR
//__interrupt void ISR_T1B0_CCR0(void) {
//    timer_i2c_overflow = 0x01;
//    // UCB1IFG |= UCCLTOIFG;
//    TB1CCTL0 &= ~CCIFG;         // Clear CCR1 flag
//}
//-- END TB0 ISR

#pragma vector = PORT3_VECTOR
__interrupt void ISR_P3_Keypad(void) {
    int buttonValue = 0b0;
    buttonValue = P3IN;     // Move input to variable

    P3DIR &= ~0b00001111;   // Set columns as input
    P3OUT &= ~0b00001111;   // Set pull-down resistors for rows
    P3DIR |=  0b11110000;   // Set rows as outputs
    P3OUT |=  0b11110000;   // Set rows high

    buttonValue = buttonValue & P3IN;   // Add both nibbles together

    keypadValue = buttonValue;

    if (keypadValue == 0x012) {
        reset = 1;
        P1OUT &= ~BIT0;
        P6OUT &= ~BIT6;
    }

    //A - 0x081, B - 0x041, C - 0x021, D - 0x011
    if (n != 0x00) {
        if (keypadValue == 0x081 || keypadValue == 0x041 || keypadValue == 0x021 || keypadValue == 0x011) {
            currentControlMode = keypadValue;
            resetRTC();
        }
    }

    configKeypad();

    P3IFG &= ~0x0FF;
}

