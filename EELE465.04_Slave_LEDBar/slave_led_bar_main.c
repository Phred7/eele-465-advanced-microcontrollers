#include <msp430.h> 

/*
 * Slave LED Bar. MSP2310
 * Walker Ward and Haley Ketteler
 * 3/31/2022
 */

unsigned int receivedData = 0x00;
unsigned int patternData = 0x00;
unsigned int lastPatternData = 0x00;
int patternBFlag = 0;
int patternDOn = 0;
int patternDDirection = 0;
int currentPattern = 0;
int passcodeEnteredCorrectly = 0;
unsigned int patternBMask = 0x00;
unsigned int patternCMask = 0x07F;
unsigned int patternDMask = 0x08;

/*
 * Configures I2C pins and registers for slave device on UCB0 with address 42h
 */
void configI2C(void) {

    P1SEL1 &= ~BIT3;      // P1.3 SCL
    P1SEL0 |= BIT3;
    P1SEL1 &= ~BIT2;      // P1.2 SDA
    P1SEL0 |= BIT2;

    UCB0CTLW0 |= UCSWRST;

    UCB0CTLW0 |= UCSSEL_3;
    UCB0BRW = 1;
    UCB0CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)

    UCB0CTLW0 &= ~UCMST;
    UCB0I2COA0 = 0x42 | UCOAEN;               // Slave address is 0x77; enable it
    UCB0CTLW0 &= ~UCTR;     // set to receive
    UCB0CTLW1 &= ~UCSWACK;  // auto ACK

    UCB0CTLW0 &= ~UCSWRST;

    UCB0CTLW1 &= ~UCRXIFG0;

    UCB0IE |= UCRXIE0;
    UCB0IE |= UCTXIE0;
    return;
}

/*
 * Configures the pins and registers for keypad (with polling).
 */
void configTimer(void){
    // Timers
    // TB0
    TB0CTL |= TBCLR;                // Clear timer and divs
    TB0CTL |= TBSSEL__SMCLK;        // SRC = SMCLK
    TB0CTL |= MC__UP;               // Mode = UP
    TB0CTL |= CNTL_0;               // Length = 16-bit
    TB0CTL |= ID__8;                // ste d1 to 8
    TB0EX0 |= TBIDEX__8;            // set d2 to 8
    TB0CCR0 = 16000;

    // TB1
//    TB1CTL |= TBCLR;                // Clear timer and divs
//    TB1CTL |= TBSSEL__SMCLK;        // SRC = SMCLK
//    TB1CTL |= MC__UP;               // Mode = UP
//    TB1CTL |= CNTL_0;               // Length = 16-bit
//    TB1CTL |= ID__8;                // ste d1 to 8
//    TB1EX0 |= TBIDEX__8;            // set d2 to 8
    return;
}

/*
 * Enables TB0 overflow interrupt with a compareValue defined by timerCompareValue.
 */
void enableTimerInterrupt(unsigned int timerCompareValue){
    // IRQs
    // Timer Compare IRQ
    TB0CCR0 = timerCompareValue;
    TB0CCTL0 |= CCIE;               // Enable TB0 CCR0 overflow IRQ
    TB0CCTL0 &= ~CCIFG;             // Clear CCR0 flag
    return;
}

/*
 * Disable TB0 overflow interrupt
 */
void disableTimerInterrupt() {
    TB0CCTL0 &= ~CCIE;              // Disable TB0 CCR0 overflow IRQ
    TB0CCTL0 &= ~CCIFG;             // Clear CCR0 flag
    return;
}

/*
 * Mirrors a nibble and returns it.
 * Adapted from StackOverflow's 'Mike DeSimone'
 * https://stackoverflow.com/questions/4245936/mirror-bits-of-a-32-bit-word
 */
unsigned int reverseFourBitInt(unsigned int fourBitInt) {
    unsigned int x = fourBitInt;
    x = ((x & 0x55555555) << 1) | ((x & 0xAAAAAAAA) >> 1); // Swap _<>_
    x = ((x & 0x33333333) << 2) | ((x & 0xCCCCCCCC) >> 2); // Swap __<>__
    x = ((x & 0x0F0F0F0F) << 4) | ((x & 0xF0F0F0F0) >> 4); // Swap ____<>____
    return x >> 4;
}

/*
 * Uses the mask to write the correct values to each pin for the LED bar on a 2310.
 */
void writeToLEDBar(unsigned int mask) {
    static const unsigned int P1Mask = 0b11110011;
    static const unsigned int P2Mask = 0b00001100;
    P1OUT = (mask & (P1Mask)) | (P1OUT & (P2Mask));
    unsigned int p2LED = P2OUT & 0x01;
    P2OUT = (mask & (P2Mask)) << 4;
    P2OUT |= p2LED;
    return;
}

/*
 * Handles the reset condition for this device.
 */
void reset(void) {
    disableTimerInterrupt();
    writeToLEDBar(0x00);
    currentPattern = 4;
    receivedData = 0x00;
    patternData = 0x00;
    lastPatternData = 0x00;
    patternDOn = 0;
    patternDDirection = 0;
    patternCMask = 0x07F;
    patternDMask = 0x08;
    passcodeEnteredCorrectly = 0;
    P2OUT &= ~BIT0;
    return;
}


int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer


    PM5CTL0 &= ~LOCKLPM5;       // Enable GPIO



//    P1DIR |= BIT0;        // P1.0
//    P1OUT &= ~BIT0;       // Init val = 0
//
//    P1DIR |= BIT1;        // P1.1
//    P1OUT &= ~BIT1;       // Init val = 0

//    P6DIR |= BIT6;
//    P6OUT &= ~BIT6;


    // LED Bar:
    P1DIR |= 0b11110011;
    P1OUT &= ~0b11110011;

    P2DIR |= 0b11000000;
    P2OUT &= ~0b11000000;

    P2DIR |= BIT0;
    P2OUT &= ~BIT0;

//    P3DIR |= 0x0FF;
//    P3OUT &= ~0x0FF;

    configI2C();

    configTimer();

    __enable_interrupt();
//
//    P1OUT |= BIT0;
//    P1OUT |= BIT1;

    writeToLEDBar(0x00);

    while(1) {
        if (passcodeEnteredCorrectly == 1) {

            // reset receivedData and store the value in a variable that wont be updated by I2C recive
//            if (receivedData != 0x00) {
//                patternData = receivedData; // may make patterns miss keypresses if they're to quick
//                receivedData = 0x00;
//            }
            patternData = receivedData;
            if (patternData != 0x00){
                if (patternData == 0x081) {
                    currentPattern = 0;
                }
                else if (patternData == 0x041) {
                    currentPattern = 1;
                }
                else if (patternData == 0x021) {
                    currentPattern = 2;
                }
                else if (patternData == 0x011) {
                    currentPattern = 3;
                }
                else if (patternData == 0x018) {
                    reset();
                }
            }

            switch (currentPattern) {
            case 0: // A
                disableTimerInterrupt();
                writeToLEDBar(0x0AA);
                break;
            case 1: // B
                enableTimerInterrupt(16425);

                if ((lastPatternData == 0x041) && (patternData == 0x00)) {
                    patternBFlag = 1;
                }

                else if ((lastPatternData == patternData) && (patternData == 0x041) && (patternBFlag == 1)) {
                    patternBFlag = 0;
                    patternBMask = 0x00;
                }

                else if ((lastPatternData != 0x41) || (patternData != 0)) {
                    patternBFlag = 0;
                }
                writeToLEDBar(patternBMask);
                break;
            case 2: // C
                enableTimerInterrupt(32850);
                writeToLEDBar(patternCMask);
                break;
            case 3: // D
                if (patternDOn == 0) {
                    enableTimerInterrupt(32850);
                    writeToLEDBar((reverseFourBitInt(patternDMask) << 4) + patternDMask);
                } else {
                    enableTimerInterrupt(4107);
                    writeToLEDBar(0x00);
                }
                break;
            }

            if (patternData != 0x00) {
                lastPatternData = patternData;
            }
            patternData = 0x00;

        }
    }

    return 0;
}


//-- Interrupt Service Routines -------------------------
//-- Service I2C
#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    receivedData = UCB0RXBUF;
    if (passcodeEnteredCorrectly == 0 && receivedData > 0x00) {
        passcodeEnteredCorrectly = 1;
        P2OUT |= BIT0;
//        TB1CCR0 = 16425;
//        TB1CCTL0 |= CCIE;               // Enable TB0 CCR0 overflow IRQ
//        TB1CCTL0 &= ~CCIFG;             // Clear CCR0 flag
    }
    // P1OUT ^= BIT0;

    UCB0CTLW1 &= ~UCRXIFG0;
    return;
}
//-- END EUSCI_B0_I2C_ISR

//-- Service TB0
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void) {
    switch (currentPattern) {
    case 0:
        break;
    case 1:
        patternBMask += 0x01;
        if (patternBMask == 0x0FF) {
            patternBMask = 0x00;
        }
        break;
    case 2:
        patternCMask = (patternCMask >> 1) | (patternCMask << 7);   // Adapted from GeeksForGeeks
        break;
    case 3:
        if (patternDOn == 0) {
            patternDOn = 1;

            if (patternDMask == 0x08){
                patternDDirection = 0;
            } else if (patternDMask == 0x01){
                patternDDirection = 1;
            }

            if (patternDDirection == 0){
                patternDMask >>= 1;
            } else {
                patternDMask <<= 1;
            }

        } else {
            patternDOn = 0;
        }
        break;
    }
    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 flag
}
//-- END TB0 ISR


////-- Service TB1
//#pragma vector = TIMER0_B1_VECTOR
//__interrupt void ISR_TB1_CCR0(void) {
//    P2OUT ^= BIT0;
//    TB1CCTL0 &= ~CCIFG;         // Clear CCR0 flag
//}
////-- END TB1 ISR
