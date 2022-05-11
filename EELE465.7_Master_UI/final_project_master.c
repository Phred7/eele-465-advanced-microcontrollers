/**
 * W. Ward
 * 05/05/2022
 * I2C Main - System Controller - EELE465 Final Project
 *
 * Port Definitions (MSP2355)
 *  #01 - P1.2 - ADC, potentiometer
 *  #03 - P1.0 - indicator LED
 *  #04 - SBWTCK
 *  #05 - SBWTDIO
 *  #06 - Vcc
 *  #07 - GND
 *  #12 - P4.7 - SCL
 *  #13 - P4.6 - SDA
 *  #16 - P6.6 - indicator LED
 *  #25 - P4.1 - Switch
 *  #26 - P4.0 - Button
 *  #35 - P3.7 - Keypad
 *  #36 - P3.6 - Keypad
 *  #37 - P3.5 - Keypad
 *  #38 - P3.4 - Keypad
 *  #39 - P5.4 - System Reset
 *  #44 - P3.3 - Keypad
 *  #45 - P3.2 - Keypad
 *  #46 - P3.1 - Keypad
 *  #47 - P3.0 - Keypad
 *
 *
 */

#include <msp430.h> 
#include <string.h>
#include <math.h>

#define ledAddress 0x042
#define lcdAddress 0x069
#define teensyAddress 0x031

// Number of packets for each slave to send/receive
#define ledPackets 1
#define lcdPackets 16
#define teensySendPackets 9
#define teensyRecievePackets 8

// General globals
unsigned char reset = 0x00;
unsigned char keypadValue = 0x00;
unsigned char keypadEntryCounter = 0x00;
unsigned char keypadEntries[4] = { 0x00, 0x00, 0x00, 0x00 };
unsigned char publishKeypadValueFlag = 0x00;

// I2C globals TODO: currently i2c packets assume nibbles will be sent... not bytes. 2 hex values can be sent in each section not just 1.
unsigned char lcdDataToSend[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char ledDataToSend[1] = { 0x00 };
unsigned char teensyDataToSend[9] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char teensyDataRecieved[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned int i2cDataCounter = 0;
unsigned char i2cTransmitCompleteFlag = 0x00;   // used in i2c receive... byte counter interrupt?
unsigned char i2cStopReached = 0x00;

// Data values
unsigned char targetFlywheelVelocityHex[4] = { 0x00, 0x00, 0x00, 0x00 };
unsigned char actualFlywheelVelocityHex[4] = { 0x00, 0x00, 0x00, 0x00 };
long targetFlywheelVelocity = 0;
long targetFlywheelVelocityForLEDs = 0;
long lastTargetFlywheelVelocity = 0;
long actualFlywheelVelocity = 0;
long lastActualFlywheelVelcity = 0;
double targetRotationalPosition = 0.0;
double actualRotationalPosition = 0.0;
double adcValue = 0.0;
unsigned char resetButtonValue = 0x00;
int switchValue = 0;

// Counters and Flags
unsigned char timerInterruptFlag = 0x00;
unsigned char timerInterruptCounter = 0x00;

//-- Configuration -------------------------

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
    UCB1BRW = 10; //8
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

    //-- Interrupts
    P3IE |= 0x0FF;
    P3IES &= ~0x0FF;
    P3IFG &= ~0x0FF;
    return;
}

void configButton(void) {
    P4DIR &= ~BIT0;
    P4REN |= BIT0;          // Enable pull-up/pull-down resistor
    P4OUT &= ~BIT0;         // make pull down resistor

    //-- Interrupts
    P4IE |= BIT0;
    P4IES &= ~BIT0; // Interrupt triggered on L->H
    P4IFG &= ~BIT0;
}

void configSwitch(void) {
    P4DIR &= ~BIT1;
    P4OUT &= ~BIT1;
    P4REN |= BIT1;          // Enable pull-up/pull-down resistor
}

void configIndicatorLEDs(void) {
    P1DIR |= BIT0;        // P1.0 LED 1
    P1OUT &= ~BIT0;        // Init val = 0
    P6DIR |= BIT6;        // P6.6 LED 2
    P6OUT &= ~BIT6;        // Init val = 0
}

void configSystemResetPin(void) {
    P5DIR |= BIT4;
    P5OUT &= ~BIT4;
}

//-- END Configuration -------------------------

//-- Logic -------------------------

int delay(int delay){
    int zzz;
    for(zzz=0; zzz<delay; zzz++){}
    return 0;
}

double round(double d)
{
    return floor(d + 0.5);
}

unsigned char convertDoubleToHex(double d) {
    return (unsigned char)((int)d);
}

void systemResetRestart(void) {
    /*
     * TODO: Ensure all values are reset properly
     */
    memset(lcdDataToSend, 0x00, 16);
    memset(ledDataToSend, 0x00, 1);
    memset(teensyDataToSend, 0x00, 9);
    memset(teensyDataRecieved, 0x00, 8);
    memset(targetFlywheelVelocityHex, 0x00, 4);
    memset(actualFlywheelVelocityHex, 0x00, 4);

    keypadValue = 0x00;
    keypadEntryCounter = 0x00;
    keypadEntries[3] = 0x14;
    keypadEntries[2] = 0x14;
    keypadEntries[1] = 0x14;
    keypadEntries[0] = 0x14;
    publishKeypadValueFlag = 0x00;

    targetFlywheelVelocity = 0;
    lastTargetFlywheelVelocity = 0;
    actualFlywheelVelocity = 0;
    lastTargetFlywheelVelocity = 0;
    targetRotationalPosition = 0.0;
    actualRotationalPosition = 0.0;
    adcValue = 0.0;
    resetButtonValue = 0x00;
    switchValue = 0x00;

    timerInterruptFlag = 0x00;
    timerInterruptCounter = 0x00;

    i2cDataCounter = 0;

}

void systemReset(void) {
    /*
     * Sets the System Reset pin high
     */
    P5OUT |= BIT4;
}

void constructFlywheelTargetVelocity(void) {
    /*
     * Populate teensy and lcd i2c packets with target flywheel velocity
     */

    if (publishKeypadValueFlag == 0x01) {
        teensyDataToSend[0] = targetFlywheelVelocityHex[0];
        teensyDataToSend[1] = targetFlywheelVelocityHex[1];
        teensyDataToSend[2] = targetFlywheelVelocityHex[2];
        teensyDataToSend[3] = targetFlywheelVelocityHex[3];
        targetFlywheelVelocityForLEDs = targetFlywheelVelocity;
    } else if (targetFlywheelVelocity == 0) {
        // memset(targetFlywheelVelocityHex, 0x00, 4);
    }

    lcdDataToSend[4] = targetFlywheelVelocityHex[0];
    lcdDataToSend[5] = targetFlywheelVelocityHex[1];
    lcdDataToSend[6] = targetFlywheelVelocityHex[2];
    lcdDataToSend[7] = targetFlywheelVelocityHex[3];
}

void deconstructFlywheelActualVelocity(void) {
    /*
     * Deconstruct teensy i2c actual flywheel velocity into long actualFlywheelVelocity and populate lcd i2c packet
     */
    actualFlywheelVelocityHex[0] = teensyDataRecieved[0];
    actualFlywheelVelocityHex[1] = teensyDataRecieved[1];
    actualFlywheelVelocityHex[2] = teensyDataRecieved[2];
    actualFlywheelVelocityHex[3] = teensyDataRecieved[3];

    lcdDataToSend[0] = teensyDataRecieved[0];
    lcdDataToSend[1] = teensyDataRecieved[1];
    lcdDataToSend[2] = teensyDataRecieved[2];
    lcdDataToSend[3] = teensyDataRecieved[3];

    lastActualFlywheelVelcity = actualFlywheelVelocity;
    actualFlywheelVelocity = (((int)actualFlywheelVelocityHex[0]) * 1000) + (((int)actualFlywheelVelocityHex[1]) * 100) + (((int)actualFlywheelVelocityHex[2]) * 10) + ((int)actualFlywheelVelocityHex[3]);
}

void constructRotationalTargetAngle(void) {
    /*
     * Use pot/ADC to map to rotational angle and populate i2c packets
     */
    double output_end = 360.0;
    double output_start = 0.0;
    double input_end = 4080.0;
    double input_start = 0.0;
    double slope = 1.0 * (output_end - output_start) / (input_end - input_start);
    targetRotationalPosition = output_start + round(slope * (adcValue - input_start));

    if (targetRotationalPosition > 360) {
        targetRotationalPosition = 360.0;
    } else if (targetRotationalPosition < 0) {
        targetRotationalPosition = 0.0;
    }

    unsigned char high;
    unsigned char mid;
    unsigned char low;
    unsigned char dec;

    double tempAngle = targetRotationalPosition;
    high = (unsigned char)((int)(tempAngle/100));
    tempAngle = (tempAngle-(high*100));
    mid = convertDoubleToHex(tempAngle/10);
    tempAngle = (tempAngle - (mid*10));
    low = convertDoubleToHex(tempAngle);
    tempAngle = (tempAngle-low);
    dec = convertDoubleToHex(round(tempAngle*10));

    teensyDataToSend[4] = high;
    teensyDataToSend[5] = mid;
    teensyDataToSend[6] = low;
    teensyDataToSend[7] = dec;

    lcdDataToSend[12] = high;
    lcdDataToSend[13] = mid;
    lcdDataToSend[14] = low;
    lcdDataToSend[15] = dec;
}

void deconstructRotationalActualAngle(void) {
    /*
     * Deconstruct i2c packet into usable data and populate other i2c packets
     */
    lcdDataToSend[8] = teensyDataRecieved[4];
    lcdDataToSend[9] = teensyDataRecieved[5];
    lcdDataToSend[10] = teensyDataRecieved[6];
    lcdDataToSend[11] = teensyDataRecieved[7];

    actualRotationalPosition = (((int)teensyDataRecieved[4]) * 100) + (((int)teensyDataRecieved[5]) * 10) + (((int)teensyDataRecieved[6])) + (((int)teensyDataRecieved[7]) * 0.1);

}

void constructIndexer(void) {
    teensyDataToSend[8] = switchValue;
}

void constructLEDIndicatorPattern(void) {
    /*
     * TODO: use current, last and target fly-wheel data values to determine what pattern to display on the LEDs
     */
    if (targetFlywheelVelocityForLEDs == 0 && actualFlywheelVelocity == 0) {
        ledDataToSend[0] = 0x11;
    } else if (targetFlywheelVelocityForLEDs - 15 > actualFlywheelVelocity) {
        ledDataToSend[0] = 0x81;
    } else if (targetFlywheelVelocityForLEDs + 15 < actualFlywheelVelocity) {
        ledDataToSend[0] = 0x41;
    } else if (actualFlywheelVelocity > 0) {
        ledDataToSend[0] = 0x21;
    } else {
        ledDataToSend[0] = 0x11;
    }
}


void convertFourCharToFlywheelTargetVelocity(void) {
    /*
     * Converts keypadEntries into double targetFlywheelVelocity and updates the targetFlywheelVelocityHex
     */
    int velocityDigitCounter = keypadEntryCounter;
    int itemIndex;
    for (itemIndex = 0; itemIndex<4; itemIndex++) {
        targetFlywheelVelocityHex[itemIndex] = keypadEntries[itemIndex];
    }

    lastTargetFlywheelVelocity = targetFlywheelVelocity;
    targetFlywheelVelocity = 0.0;
    if (velocityDigitCounter < 4) {
        int shiftEntryDecimalPlaces;
        for (shiftEntryDecimalPlaces = 0; shiftEntryDecimalPlaces < (4 - velocityDigitCounter); shiftEntryDecimalPlaces++) {
            targetFlywheelVelocityHex[3] = targetFlywheelVelocityHex[2];
            targetFlywheelVelocityHex[2] = targetFlywheelVelocityHex[1];
            targetFlywheelVelocityHex[1] = targetFlywheelVelocityHex[0];
            targetFlywheelVelocityHex[0] = 0x14;
        }
    }

    int arrayIndex;
    for(arrayIndex = 0; arrayIndex < 4; arrayIndex++) {
        int decimalValue = 0;
        switch(targetFlywheelVelocityHex[arrayIndex]) {
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
        targetFlywheelVelocityHex[arrayIndex] = decimalValue;
        targetFlywheelVelocity += (decimalValue) * pow(10, (3 - arrayIndex));
    }
    return;
}

//-- END Logic -------------------------



//-- I2C Functions -------------------------

int sendI2C(int slaveAddress) {

    switch (slaveAddress) {
    case lcdAddress:
        UCB1TBCNT = lcdPackets;
        break;
    case ledAddress:
        UCB1TBCNT = ledPackets;
        break;
    case teensyAddress:
        UCB1TBCNT = teensySendPackets;
        break;
    default:
        return 1;
    }

    UCB1I2CSA = slaveAddress;

    UCB1CTLW0 |= UCTR;      // put into Tx mode

    i2cDataCounter = 0x00;

    UCB1CTLW0 |= UCTXSTT;   // generate START cond.

//    while ((UCB1IFG & UCSTPIFG) == 0 ); //wait for STOP
//        UCB1IFG &= ~UCSTPIFG;           // clear STOP flag

    while (i2cStopReached == 0x00);
    i2cStopReached = 0x00;
    UCB1IFG &= ~UCSTPIFG;           // clear STOP flag

    UCB1TXBUF = 0x00;

    i2cDataCounter = 0x00;

    return 0;
}

int receiveI2C(int slaveAddress) {

    if (slaveAddress != teensyAddress) {
        return 1;
    }

    /*
     * Receive data bytes
     */
    switch (slaveAddress) {
    case teensyAddress:
        UCB1TBCNT = teensyRecievePackets;
        break;
    default:
        return 1;
    }

    i2cDataCounter = 0x00;

    UCB1I2CSA = slaveAddress;

    UCB1CTLW0 &= ~UCTR;   // Rx

    UCB1CTLW0 |= UCTXSTT;   // Generate START cond.

//    while ((UCB1IFG & UCSTPIFG) == 0 ); //wait for STOP
//        UCB1IFG &= ~UCSTPIFG;           // clear STOP flag

    while (i2cStopReached == 0x00);
    i2cStopReached = 0x00;
    UCB1IFG &= ~UCSTPIFG;           // clear STOP flag

    i2cDataCounter = 0x00;

    return 0;
}

//-- END I2C Functions -------------------------



//-- Main -------------------------

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
    PM5CTL0 &= ~LOCKLPM5;       // disable LPM

    configI2C();
    configTimer();
	configADC();
    configKeypad();
    configSwitch();
    configButton();
    configIndicatorLEDs();
    configSystemResetPin();

    delay(1000);

    __enable_interrupt();

    enableTimerInterrupt(4683);  // timer interrupt 1873 = every .1 seconds.

    while(1) {

        if (resetButtonValue == 0x01) { // handles system reset
            __disable_interrupt();
            P4IE &= ~BIT0;
            disableTimerInterrupt();
            resetButtonValue = 0x00;
            systemReset();

            P1OUT &= ~BIT0;
            P6OUT &= ~BIT6;

            while ((P4IN & BIT0) == 0x01);
            while ((P4IN & BIT0) == 0x00) { // interrupts are disabled here...
                P1OUT |= BIT0;
                P6OUT |= BIT6;
            }
            while ((P4IN & BIT0) == 0x01);
            P4IFG &= ~BIT0;

            P1OUT &= ~BIT0;
            P6OUT &= ~BIT6;
            systemResetRestart();
            enableTimerInterrupt(4683);
            __enable_interrupt();
            resetButtonValue = 0x00;
            P4IFG &= ~BIT0;
            P4IE |= BIT0;
            P4IFG &= ~BIT0;
        }

        if (timerInterruptFlag >= 0x01) { // triggers on ~0.1sec
            timerInterruptFlag = 0x00;
            convertFourCharToFlywheelTargetVelocity();
            constructFlywheelTargetVelocity();      // updates Teensy and LCD
            receiveI2C(teensyAddress);
            constructRotationalTargetAngle();       // updates LCD and Teensy
            deconstructFlywheelActualVelocity();    // relies on receive from Teensy. Updates LCD
            deconstructRotationalActualAngle();     // relies on receive from Teensy. Updates LCD
            constructLEDIndicatorPattern();         // relies on receive from Teensy. Updates LEDs
            sendI2C(lcdAddress);
            sendI2C(ledAddress);
            P6OUT ^= BIT6;

            if (timerInterruptCounter >= 0x05) { // triggers on ~0.5sec
               timerInterruptCounter = 0x00;
               constructIndexer();      // updates teensy
               sendI2C(teensyAddress);
               P1OUT ^= BIT0;
            }
        }
    }

	return 0;
}

//-- END Main -------------------------

//-- Interrupt Service Routines -------------------------

//-- Service TB0CCR0
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void) {
    // Trigger I2C somehow?

    ADCCTL0 |= ADCENC | ADCSC;          // Start ADC
    while((ADCIFG & ADCIFG0) == 0);     // Wait for conversion completion
    adcValue = ADCMEM0;                // Read ADC value

    switchValue = (P4IN & BIT1) >> 1;

    timerInterruptFlag = 0x01;
    timerInterruptCounter++;

    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 flag
}

//-- Service P3 - Keypad
#pragma vector = PORT3_VECTOR
__interrupt void ISR_P3_Keypad(void) {
    int buttonValue = 0b0;
    buttonValue = P3IN;     // Move input to variable

    P3DIR &= ~0b00001111;   // Set columns as input
    P3OUT &= ~0b00001111;   // Set pull-down resistors for rows
    P3DIR |=  0b11110000;   // Set rows as outputs
    P3OUT |=  0b11110000;   // Set rows high

    buttonValue = buttonValue & P3IN;   // Add both nibbles together

//    keypadValue = buttonValue;

    if (buttonValue == 0x012) {
        keypadEntryCounter = 0;
        keypadEntries[3] = 0x14;
        keypadEntries[2] = 0x14;
        keypadEntries[1] = 0x14;
        keypadEntries[0] = 0x14;
        publishKeypadValueFlag = 0x00;
    } else if (buttonValue == 0x18) {
        publishKeypadValueFlag = 0x01;
    } else if (buttonValue == 0x088 || buttonValue == 0x084 || buttonValue == 0x082 || buttonValue == 0x048 || buttonValue == 0x044 || buttonValue == 0x042 || buttonValue == 0x028 || buttonValue == 0x024 || buttonValue == 0x022 || buttonValue == 0x014) {
        if (publishKeypadValueFlag == 0x01) {
            publishKeypadValueFlag = 0x00;
            keypadEntryCounter = 0;
            keypadEntries[3] = 0x14;
            keypadEntries[2] = 0x14;
            keypadEntries[1] = 0x14;
            keypadEntries[0] = 0x14;
        }

        if (keypadEntryCounter < 4) {
            keypadEntries[keypadEntryCounter] = buttonValue;
            keypadEntryCounter++;
        } else if (targetFlywheelVelocity == 0) {
            keypadEntryCounter = 1;
            keypadEntries[3] = 0x14;
            keypadEntries[2] = 0x14;
            keypadEntries[1] = 0x14;
            keypadEntries[0] = buttonValue;
        }
    }

    // Reset Keypad Config
    //-- Setup Ports for Keypad. 3.7 is LeftMost
    P3DIR &= ~0b11110000;   // Set rows as inputs
    P3OUT &= ~0b11110000;   // Set pull-down resistors for rows
    P3DIR |=  0b00001111;   // Set columns as outputs
    P3OUT |=  0b00001111;   // Set columns high
    P3REN |= 0xFF;          // Enable pull-up/pull-down resistors on port 3

    //-- Interrupts
    P3IE |= 0x0FF;
    P3IES &= ~0x0FF;
    P3IFG &= ~0x0FF;

    P3IFG &= ~0x0FF;
}

//-- Service P4 - Button
#pragma vector = PORT4_VECTOR
__interrupt void ISR_P4_Button(void) {
    int portInput = 0b0;
    portInput = P4IN;

    resetButtonValue = portInput & BIT0;
    while ((P4IN & BIT0) == 0x01);
    P4IFG &= ~BIT0;
}

//-- Service I2C B1
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
    case 0x16:
        /*
         * Data Received;
         */
        switch (UCB1I2CSA) {
        case teensyAddress:
            teensyDataRecieved[i2cDataCounter] = UCB1RXBUF;
            i2cDataCounter++;
            break;
        default:
            break;
        }
        break;
    case 0x18:
        /*
         * Data Transmission;
         */
        switch (UCB1I2CSA) {
        case lcdAddress:
            UCB1TXBUF = lcdDataToSend[i2cDataCounter];
            i2cDataCounter++;
            break;
        case ledAddress:
            UCB1TXBUF = ledDataToSend[0];
            break;
        case teensyAddress:
            if (i2cTransmitCompleteFlag == 0x03) { // this means we need to send the address
                UCB1TXBUF = teensyAddress;
            } else {    // otherwise send the data
                UCB1TXBUF = teensyDataToSend[i2cDataCounter];
                i2cDataCounter++;
            }
            break;
        default:
            break;
        }
        break;
    case 0x26: // Vector 26: BCNTIFG (Byte counter)
        if (i2cTransmitCompleteFlag == 0x03) {
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
