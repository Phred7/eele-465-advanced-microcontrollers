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

#define ledAddress 0x042
#define lcdAddress 0x069
#define teensyAddress 0x031

// Nubmer of packets for each slave to send/recieve
#define ledPackets 1
#define lcdPackets 16
#define teensySendPackets 9
#define teensyRecievePackets 8

// General globals
unsigned char reset = 0x00;
unsigned char keypadValue = 0x00;
unsigned char keypadEntryCounter = 0x00;
unsigned char keypadEntries[4] = { 0x00, 0x00, 0x00, 0x00 };

// I2C globals
unsigned char lcdDataToSend[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char ledDataToSend[1] = { 0x00 };
unsigned char teensyDataToSend[9] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char teensyDataRecieved[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned int i2cDataCounter = 0;

// Data values
double targetFlywheelVelocity = 0.0;
double actualFlywheelVelocity = 0.0;
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

//  #25 - P4.1 - Switch
//  #26 - P4.0 - Button
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

//-- END Configuration -------------------------

//-- Logic -------------------------

int delay(int delay){
    int zzz;
    for(zzz=0; zzz<delay; zzz++){}
    return 0;
}

void systemReset() {
    /*
     * TODO
     */
    lcdDataToSend[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    ledDataToSend[1] = { 0x00 };
    teensyDataToSend[9] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    teensyDataRecieved[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    targetFlywheelVelocity = 0.0;
    actualFlywheelVelocity = 0.0;
    targetRotationalPosition = 0.0;
    actualRotationalPosition = 0.0;
    adcValue = 0.0;
}

//-- END Logic -------------------------

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

    __enable_interrupt();

    enableTimerInterrupt(1873);  // timer interrupt every .1 seconds.

    while(1) {

        if (resetButtonValue == 0x01) {
            disableTimerInterrupt();
            resetButtonValue = 0x00;
            systemReset();
            /*
             * TODO
             * Put LCD, LED and Teensy into reset states.
             */
            while (resetButtonValue = 0x00) {
            }
        }

        if (timerInterruptFlag >= 0x01) { // triggers on ~0.1sec
            timerInterruptFlag = 0x00;
            /*
             * TODO
             * convert Pot reading to degrees
             * Request from Teensy
             * Update LCD
             * Update LEDs
             */
        }

        if (timerInterruptCounter >= 0x05) { // triggers on ~0.5sec
            timerInterruptCounter = 0x00;
            /*
             * TODO
             * Update Teensy
             */
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

    keypadValue = buttonValue;

    if (keypadValue == 0x012) {
        keypadEntryCounter = 0;
        keypadEntries = { 0x00, 0x00, 0x00, 0x00 };
    } else if (keypadValue == 0x18) {
        keypadEntryCounter = 0;
        /*
         * TODO: publish rpm value.
         */
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
        break;
    case 0x04:
        /*
         * NACK Received;
         */
        UCB1CTLW0 |= UCTXSTP;   // Generate STOP cond.
        break;
    case 0x16:
        /*
         * Data Received; TODO
         */
        switch (UCB1I2CSA) {
        case teensyAddress:
            break;
        default:
            break;
        }
        break;
    case 0x18:
        /*
         * Data Transmission; TODO
         */
        switch (UCB1I2CSA) {
        case lcdAddress:
            UCB1TXBUF = lcdDataToSend[i2cDataCounter];
            break;
        case ledAddress:
            UCB1TXBUF = ledDataToSend[0];
            break;
        case teensyAddress:
            UCB1TXBUF = 0x00;
            break;
        default:
            break;
        }
        break;
    case 26: // Vector 26: BCNTIFG
        counter = UCB1STATW & UCBCNT;
        if ((counter) > 0) {
        }
        break;
    default:
        break;
    }

    return;
}
//-- END I2C B1 ISR
