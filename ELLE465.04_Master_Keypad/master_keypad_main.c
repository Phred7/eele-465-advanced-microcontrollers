#include <msp430.h> 

/** W. Ward
 *  03/18/2022
 *  I2C Main with Keypad
 */

int dataSent = 0;
unsigned int dataToSendI2C = 0x00;
int timerCompareHalfSecond = 9366;


void configI2C(void) {
    //-- Config. I2C Master
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
    //-- Config I2C Ports
    P4SEL1 &= ~BIT7;            // P4.7 = SCL
    P4SEL0 |= BIT7;
    P4SEL1 &= ~BIT6;            // P4.6 = SDA
    P4SEL0 |= BIT6;

    UCB1CTLW0 &= ~UCSWRST;

    UCB1IE |= UCTXIE0;
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
    return;
}


void enableTimerInterrupt(int timerCompareValue){
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


void configKeypad(void){
    //-- Setup Ports for Keypad
    P3DIR &= ~0b11111111;   // Clear P3.0-3.7 for input
    P3OUT &= ~0b11111111;   // Clear input initially
    P3REN |= 0xFF;          // Enable pull-up/pull-down resistors on port 3
    return;
}


int delay(int delay){
    int zzz;
    for(zzz=0; zzz<delay; zzz++){}
    return 0;
}


int send_i2c(unsigned int dataToSend) {
    P1OUT |= BIT0;
    enableTimerInterrupt(timerCompareHalfSecond);

    dataToSendI2C = dataToSend;
    UCB1I2CSA = 0x0042;
    UCB1CTLW0 |= UCTXSTT;
    while (UCB0CTLW0 & UCTXSTP);
    delay(5);

    UCB1I2CSA = 0x0069;
    UCB1CTLW0 |= UCTXSTT;
    while (UCB0CTLW0 & UCTXSTP);
    delay(5);

    dataToSendI2C = 0x00;
    return 0;
}


int send_led_reset(void) {
    // *
    return 0;
}


int send_lcd_reset(void) {
    // #
    return 0;
}


unsigned int convertKeypadToASCII(unsigned int keypadValue) {
    return 0x00;
}


int passcode() {
    unsigned int passcodeFirstDigit = 0x028;  // 7
    unsigned int passcodeSecondDigit = 0x084;  // 2
    unsigned int passcodeThirdDigit = 0x022;  // 9
    return 0;
}


unsigned int checkKeypad() {
    int buttonValue = 0b0;

    P3DIR &= ~0b11110000;   // Set rows as inputs
    P3OUT &= ~0b11110000;   // Set pull-down resistors for rows
    P3DIR |=  0b00001111;   // Set columns as outputs
    P3OUT |=  0b00001111;   // Set columns high

    buttonValue = P3IN;     // Move input to variable

    if (buttonValue == 0b0) {
        return 0x00;
    }

    P3DIR &= ~0b00001111;   // Set columns as input
    P3OUT &= ~0b00001111;   // Set pull-down resistors for rows
    P3DIR |=  0b11110000;   // Set rows as outputs
    P3OUT |=  0b11110000;   // Set rows high

    buttonValue = buttonValue & P3IN;   // Add both nibbles together

    return buttonValue;
}


int main(void){

    WDTCTL = WDTPW | WDTHOLD;

    P1DIR |= BIT0;        // P1.0 LED 1
    P1OUT &= ~BIT0;        // Init val = 0
    P6DIR |= BIT6;        // P6.6 LED 2
    P6OUT &= ~BIT6;        // Init val = 0

    PM5CTL0 &= ~LOCKLPM5;       // disable LPM

    configI2C();

    configTimer();

    configKeypad();

    __enable_interrupt();

//    unsigned int P3 = 0;
//    while(1) {
//        P3DIR &= ~0x0FF;        // make MSNibble an input
//        P3REN |= 0x0FF;         // enable resistor
//        P3OUT &= ~0x0FF;        // make a pull down resistor
//        P3 = P3IN;
//        //unsigned int value = checkKeypad();
//    }
//    unsigned int keypadValue = checkKeypad();
//    keypadValue = 0x011;
//    send_i2c(keypadValue);
//    delay(10000);
    while(1) {
        unsigned int keypadValue = checkKeypad();
        if (keypadValue != 0x00) {
            send_i2c(keypadValue);
        }
//        keypadValue = 0x041;

    }
    return 0;
}
//-- END main


//-- Interrupt Service Routines -------------------------
//-- Service I2C B1
#pragma vector=EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void){
    if (dataSent == 1) {
        UCB1IFG &= ~UCTXIFG0;
        UCB1CTL1 |= UCTXSTP;
        dataSent = 0;
    } else {
        UCB1TXBUF = dataToSendI2C;
        dataSent = 1;
        UCB1IFG &= ~UCTXIFG0;
    }
    return;

}
//-- END I2C B1 ISR

//-- Service TB0
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void) {
    P1OUT ^= BIT0;
    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 flag
}
//-- END TB0 ISR

////-- Service TB0CCR1
//#pragma vector = TIMER0_B1_VECTOR
//__interrupt void ISR_TB0CCR1(void) {
//    TB0CCTL1 &= ~CCIFG;         // Clear CCR1 flag
//}
////-- END TB0CCR1 ISR
