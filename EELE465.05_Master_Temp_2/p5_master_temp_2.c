#include <msp430.h> 

/** W. Ward and H. Ketteler
 *  03/29/2022
 *  I2C Main
 */

unsigned int keypadValue = 0x00;
int dataSent = 0;
unsigned int dataToSendI2C[2] = {0,0};
int n = 0;
int numberOfReadings = 0;
int newReading = 0;
unsigned int newestReading = 0;
float movingAverage = 0.0;
float celsiusTemp = 0.0;
int dataTypeFlag = 0;           // {"0": n value, "1": temp data}

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
    UCB1TBCNT = 2;              // # of Bytes in Packet
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

unsigned int checkKeypad() {
    int buttonValue = 0b0;

    buttonValue = P3IN;     // Move input to variable

    if (buttonValue == 0b0) {
        return 0x00;
    }

    P3DIR &= ~0b00001111;   // Set columns as input
    P3OUT &= ~0b00001111;   // Set pull-down resistors for rows
    P3DIR |=  0b11110000;   // Set rows as outputs
    P3OUT |=  0b11110000;   // Set rows high

    buttonValue = buttonValue & P3IN;   // Add both nibbles together

    configKeypad();

    return buttonValue;
}

int delay(int delay){
    int zzz;
    for(zzz=0; zzz<delay; zzz++){}
    return 0;
}

int send_i2c(void) {
    if (dataTypeFlag == 1){
        UCB1TBCNT = 3;              // # of Bytes in Packet
    } else if(dataTypeFlag == 0){
        UCB1TBCNT = 2;
    }

    UCB1CTLW0 |= UCTXSTT;
    while (UCB0CTLW0 & UCTXSTP);

 //   dataToSendI2C = 0x00;

    return 0;
}

float getMovingAverage(float averageArray[]) {
    float sumTotal = 0.0;
    int j;
    for(j=0; j<n; j++) {
        sumTotal += averageArray[j];
    }
    return sumTotal;
}

float convertTemp(float averageTemp){
    unsigned int val1;
    unsigned int val2;
    float voltConvert;
    voltConvert = (movingAverage - 2316.47)/14.5;

    // Above the decimal point
    val1 = voltConvert;
    dataToSendI2C[0] = val1;

    // Below the decimal point
    val2 = (voltConvert - val1) * 10;
    dataToSendI2C[1] = val2;

    return 0.0;
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    P1DIR |= BIT0;        // P1.0 LED 1
    P1OUT &= ~BIT0;        // Init val = 0
    P6DIR |= BIT6;        // P6.6 LED 2
    P6OUT &= ~BIT6;        // Init val = 0

    PM5CTL0 &= ~LOCKLPM5;       // disable LPM

    configI2C();

    configTimer();

    configADC();

    P3IE |= 0x0FF;
    P3IES &= ~0x0FF;
    P3IFG &= ~0x0FF;

    configKeypad();

    __enable_interrupt();

    while(n == 0) {
        unsigned int keypadValue = checkKeypad();
        switch (keypadValue) {
        case 0x088:
            n = 1;
            break;
        case 0x084:
            n = 2;
            break;
        case 0x082:
            n = 3;
            break;
        case 0x048:
            n = 4;
            break;
        case 0x044:
            n = 5;
            break;
        case 0x042:
            n = 6;
            break;
        case 0x028:
            n = 7;
            break;
        case 0x024:
            n = 8;
            break;
        case 0x022:
            n = 9;
            break;
        default:
            n = 0;
            break;
        }
    }

    send_i2c();

    float readings[n];
    int i;
    for (i = 0; i < n; i++) {
        readings[i] = 0.0;
    }

    enableTimerInterrupt(6244);

    while(numberOfReadings < n) {
        if(newReading == 1) {
            readings[numberOfReadings] = newestReading;
            newReading = 0;
        }
    }

    while(1) {
        if(newReading == 1) {
            int k;
            for(k=0;k<n;k++) {
                if(k == (n-1)) {
                    readings[k] = 0.0;
                } else {
                    readings[k] = readings[k+1];
                }
            }
            newReading = 0;
        }
        movingAverage = getMovingAverage(readings);
        convertTemp(movingAverage);
    }

    return 0;
}
//-- END main


//-- Interrupt Service Routines -------------------------
//-- Service I2C B1
#pragma vector=EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void){
    if (dataTypeFlag == 0){
        if (dataSent == 1) {
            UCB1IFG &= ~UCTXIFG0;
            UCB1CTL1 |= UCTXSTP;
            dataSent = 0;
        } else {
            UCB1TXBUF = n;
            dataSent++;
            UCB1IFG &= ~UCTXIFG0;
        }
    } else if (dataTypeFlag == 1){
        if (dataSent == 2) {
            UCB1IFG &= ~UCTXIFG0;
            UCB1CTL1 |= UCTXSTP;
            dataSent = 0;
        } else {
            UCB1TXBUF = dataToSendI2C[dataSent];
            dataSent++;
            UCB1IFG &= ~UCTXIFG0;
        }
    }
    return;
}
//-- END I2C B1 ISR

//-- Service TB0
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void) {
    if (numberOfReadings > n) {
        send_i2c();
    }

    ADCCTL0 |= ADCENC | ADCSC;          // Start ADC
    while((ADCIFG & ADCIFG0) == 0);     // Wait for conversion completion
    newestReading = ADCMEM0;                // Read ADC value

    newReading = 1;
    numberOfReadings++;
    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 flag
}
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

    configKeypad();

    P3IFG &= ~0x0FF;
}

