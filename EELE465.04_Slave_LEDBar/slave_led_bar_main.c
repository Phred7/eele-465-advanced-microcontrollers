#include <msp430.h> 


unsigned int recievedData = 0x00;
unsigned int patternData = 0x00;
unsigned int lastPatternData = 0x00;
int patternBFlag = 0;
int currentPattern = 0;
int passcodeEnteredCorrectly = 0;
unsigned int patternBMask = 0x00;
unsigned int patternCMask = 0x07F;
unsigned int patternDMask = 0x018;


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
    return;
}


void disableTimerInterrupt() {
    TB0CCTL0 &= ~CCIE;
    return;
}


int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer


    PM5CTL0 &= ~LOCKLPM5;       // Enable GPIO

    P1DIR |= BIT0;        // P1.0
    P1OUT &= ~BIT0;       // Init val = 0

    P1DIR |= BIT1;        // P1.1
    P1OUT &= ~BIT1;       // Init val = 0

    P3DIR |= 0x0FF;
    P3OUT &= ~0x0FF;

    configI2C();

    __enable_interrupt();

    P1OUT |= BIT0;
    P1OUT |= BIT1;

    int i;
    while(1) {
        if (passcodeEnteredCorrectly == 1) {

            if (recievedData != 0x00) {
                patternData = recievedData;
                recievedData = 0x00;
            }

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
            }

            switch (currentPattern) {
            case 0: // A
                disableTimerInterrupt();
                P3OUT |= 0x0AA;
                break;
            case 1: // B
                enableTimerInterrupt(16425);
                if (lastPatternData != 0x041) {
                    patternBFlag = 0;
                } else {
                    if (patternData == 0x041 && patternBFlag == 1) {
                        patternBMask = 0x00;
                    }
                    if (patternData == 0x00) {
                        patternBFlag = 1;
                    }
                }
                P3OUT |= patternBMask;
                break;
            case 2: // C
                break;
            case 3: // D
                break;
            }

            if (patternData != 0x00) {
                lastPatternData = patternData;
            }
            patternData = 0x00;

        } else {
            passcodeEnteredCorrectly = 1;
        }
    }

    return 0;
}


//-- Interrupt Service Routines -------------------------
//-- Service I2C
#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    recievedData = UCB0RXBUF;
    P1OUT &= ~BIT0;
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
        break;
    case 2:
        break;
    case 3:
        break;
    }
    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 flag
}
//-- END TB0 ISR
