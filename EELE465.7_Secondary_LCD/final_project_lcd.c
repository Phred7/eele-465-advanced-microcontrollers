/**
 * W. Ward
 * 05/05/2022
 * I2C Secondary - LCD Controller - EELE465 Final Project
 *
 * Port Definitions (MSP2355)
 *  P1.7 - DB7 (Pin 14 on LCD)
 *  P1.6 - DB6 (Pin 13 on LCD)
 *  P1.5 - DB5 (Pin 12 on LCD)
 *  P1.4 - DB4 (Pin 11 on LCD)
 *  P3.7 - RS  (Pin 4 on LCD)     Data/Instruction Register Select
 *  P3.6 - E   (Pin 6 on LCD)     Enable Signal
 *
 *  P5.4 - System Reset
 *  P4.6 - SDA
 *  P4.7 - SCL
 */

#include <msp430.h> 

#define lcdAddress 0x069
#define lcdPackets 16

int delayFlag = 0;      //{"1", Hold Delay; "0", No delay}
unsigned int receivedData[lcdPackets] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // DO NOT CHANGE DATA TYPE
// {n, ambient integer, ambient decimal, mode, minutes,
// seconds, plant integer, plant decimal}
unsigned int convDecData[4] = {0,0,0,0};      // Decimal values of temp and time
unsigned int receiveDataCounter = 0;

void dataDelay(unsigned int timerCount){
    delayFlag = 1;

    TB0CCR0 = timerCount;       // Counter compare value

    TB0CCTL0 |= CCIE;           // Enable TB0 Overflow IRQ
    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 flag

    while(delayFlag == 1);      // Loop until delay has finished

    TB0CCTL0 &= ~CCIE;          // Disable TB0 Overflow IRQ

    return;
}

void latchClock(void){
    P3OUT |= BIT6;              // Set P3.6 - Pin E
    dataDelay(15);              // Delay for ~1000 clock cycles
    P3OUT &= ~BIT6;             // Clear P3.6 - Pin E

    return;
}

void initFunctionSet(void){
    P3OUT &= ~BIT7;             // Clear P3.7 - RS bit
    P1OUT |= BIT4 | BIT5;
    P1OUT &= -(BIT6|BIT7);

    return;
}

void sendDataLCD(int RSflag, unsigned int dataByte){
    // Initialize bit mask values
    int B7 = 0b1000;
    int B6 = 0b0100;
    int B5 = 0b0010;
    int B4 = 0b0001;
    int B3 = 0b1000;
    int B2 = 0b0100;
    int B1 = 0b0010;
    int B0 = 0b0001;
    int UpperByte = 0b0000;
    int LowerByte = 0b0000;

    // Toggle the RS bit accordingly
    if(RSflag == 1){
        P3OUT |= BIT7;              // Set RS bit (indicates data send)
    } else{
        P3OUT &= ~BIT7;             // Clear RS bit (indicates instruction send)
    }

    // Read data byte and set bit mask values
    if((dataByte & 0b10000000) == 0){
        B7 = 0b0000;
    }
    if((dataByte & 0b01000000) == 0){
        B6 = 0b0000;
    }
    if((dataByte & 0b00100000) == 0){
        B5 = 0b0000;
    }
    if((dataByte & 0b00010000) == 0){
        B4 = 0b0000;
    }
    if((dataByte & 0b00001000) == 0){
        B3 = 0b0000;
    }
    if((dataByte & 0b00000100) == 0){
        B2 = 0b0000;
    }
    if((dataByte & 0b00000010) == 0){
        B1 = 0b0000;
    }
    if((dataByte & 0b00000001) == 0){
        B0 = 0b0000;
    }

    // Set data nibbles
    UpperByte = (B7 | B6 | B5 | B4);
    UpperByte = UpperByte << 4;         // shift data into upper nibble
    LowerByte = (B3 | B2 | B1 | B0);
    LowerByte = LowerByte << 4;         // shift data into upper nibble

    // Push data to LCD
    P1OUT &= ~0b11110000;
    P1OUT |= UpperByte;
    latchClock();
    P1OUT &= ~0b11110000;
    P1OUT |= LowerByte;
    latchClock();

    // Toggle RS bit
    if(RSflag == 1){
        P3OUT &= ~BIT7;                 // Clear RS bit
    }

    dataDelay(80);

    return;
}

void setDDRAM(int row, int col){
    P3OUT &= ~BIT7;                 // Clear RS bit

    // Set upper nibble
    if(row == 0){
        P1OUT &= ~0b11110000;
        P1OUT |=  0b10000000;
    } else if(row == 1){
        P1OUT &= ~0b11110000;
        P1OUT |=  0b11000000;
    }

    latchClock();

    // Set lower nibble
    col = col << 4;
    P1OUT &= ~0b11110000;
    P1OUT |= col;
    latchClock();

    return;
}

void resetDisplay(void){
    sendDataLCD(0,0b00000001);      // Display clear, clear DDRAM

    // Velocity Target
    setDDRAM(0,0);
    sendDataLCD(1,0b01010110);      // V
    sendDataLCD(1,0b01010100);      // T
    sendDataLCD(1,0b00111010);      // :

    // Velocity Actual
    setDDRAM(0,8);
    sendDataLCD(1,0b01010110);      // V
    sendDataLCD(1,0b01000001);      // A
    sendDataLCD(1,0b00111010);      // :

    // Rotational Target Position
    setDDRAM(1,0);
    sendDataLCD(1,0b01010010);      // R
    sendDataLCD(1,0b01010100);      // T
    sendDataLCD(1,0b00111010);      // :

    // Rotational Actual Position
    setDDRAM(1,8);
    sendDataLCD(1,0b01010010);      // R
    sendDataLCD(1,0b01000001);      // A
    sendDataLCD(1,0b00111010);      // :

    setDDRAM(0,4);          // Set cursor to n location

    return;
}

// Converts keypad hex value to LCD binary value
unsigned int convKey2LCDhex(int buttonValue){
    unsigned int LCDvalue = 0;
    switch(buttonValue){
    case 0x81:                      // "A"
        LCDvalue = 0b01000001;
        break;
    case 0x41:                      // "B"
        LCDvalue = 0b01000010;
        break;
    case 0x21:                      // "C"
        LCDvalue = 0b01000011;
        break;
    case 0x11:                      // "D"
        LCDvalue = 0b01000100;
        break;
    case 0x14:                      // "0"
        LCDvalue = 0b00110000;
        break;
    case 0x88:                      // "1"
        LCDvalue = 0b00110001;
        break;
    case 0x84:                      // "2"
        LCDvalue = 0b00110010;
        break;
    case 0x82:                      // "3"
        LCDvalue = 0b00110011;
        break;
    case 0x48:                      // "4"
        LCDvalue = 0b00110100;
        break;
    case 0x44:                      // "5"
        LCDvalue = 0b00110101;
        break;
    case 0x42:                      // "6"
        LCDvalue = 0b00110110;
        break;
    case 0x28:                      // "7"
        LCDvalue = 0b00110111;
        break;
    case 0x24:                      // "8"
        LCDvalue = 0b00111000;
        break;
    case 0x22:                      // "9"
        LCDvalue = 0b00111001;
        break;
    case 0x18:                      // "*"
        LCDvalue = 0b00101010;
        break;
    case 0x12:                      // "#"
        sendDataLCD(0,0b00000001);  // Display clear, clears DDRAM
        sendDataLCD(0,0b10000000);  // Set DDRAM location to 00h
        sendDataLCD(0,0b00000010);  // Move cursor to home position
        sendDataLCD(0,0b00001111);  // Display on, set cursor style
        resetDisplay();             // Resets LCD display to input screen
        break;
    default:
        break;
    }
    return LCDvalue;
}

// converts decimal value to keypad hex value
unsigned char convDec2Hex(int number){
    unsigned char hexNumber = 0;

    switch(number){
       case 0:
           hexNumber = 0x14;
           break;
       case 1:
           hexNumber = 0x88;
           break;
       case 2:
           hexNumber = 0x84;
           break;
       case 3:
           hexNumber = 0x82;
           break;
       case 4:
           hexNumber = 0x48;
           break;
       case 5:
           hexNumber = 0x44;
           break;
       case 6:
           hexNumber = 0x42;
           break;
       case 7:
           hexNumber = 0x28;
           break;
       case 8:
           hexNumber = 0x24;
           break;
       case 9:
           hexNumber = 0x22;
           break;
       default:
           hexNumber = 0x7D;
           break;
    }
    return hexNumber;
}

int BCD2Hex(unsigned int hexValue){
    int decValue = ((hexValue & 0xF0) >> 4) * 10 + (hexValue & 0x0F);

    return decValue;
}

void convertReceivedData(void){
    P1OUT ^= BIT0;

    // Velocity Target
//    setDDRAM(0, 3);
//    sendDataLCD(1, 0b00110110);
//    //sendDataLCD(1, convDec2Hex(receivedData[54]));
//    sendDataLCD(1, convDec2Hex(receivedData[5]));
//    sendDataLCD(1, convDec2Hex(receivedData[6]));
//    sendDataLCD(1, convDec2Hex(receivedData[7]));

//    // Velocity Actual
//    setDDRAM(0, 11);
//    sendDataLCD(1, receivedData[0]);
//    sendDataLCD(1, receivedData[1]);
//    sendDataLCD(1, receivedData[2]);
//    sendDataLCD(1, receivedData[3]);
//
//    // Rotational Position Target
//    setDDRAM(1, 4);
//    sendDataLCD(1, receivedData[12]);
//    sendDataLCD(1, receivedData[13]);
//    sendDataLCD(1, receivedData[14]);
//    sendDataLCD(1, receivedData[15]);
//
//    // Rotational Position Actual
//    setDDRAM(1, 12);
//    sendDataLCD(1, receivedData[8]);
//    sendDataLCD(1, receivedData[9]);
//    sendDataLCD(1, receivedData[10]);
//    sendDataLCD(1, receivedData[11]);

//    // N value
//    setDDRAM(0,4);
//    sendDataLCD(1, convKey2LCDhex(receivedData[0]));
//
//    // First ambient temp digit
//    unsigned int A1 = 0;
//    A1 = receivedData[1] / 10;
//    convDecData[0] = A1;
//    A1 = convKey2LCDhex(convDec2Hex(A1));
//
//    // Second ambient temp digit
//    unsigned int A2 = 0;
//    A2 = receivedData[1] - (10 * convDecData[0]);
//    A2 = convKey2LCDhex(convDec2Hex(A2));
//
//    setDDRAM(0,10);
//    sendDataLCD(1, A1);
//    sendDataLCD(1, A2);
//
//    // Third ambient temp digit
//    unsigned int A3 = 0;
//    A3 = convKey2LCDhex(convDec2Hex(receivedData[2]));
//    setDDRAM(0,13);
//    sendDataLCD(1, A3);
//
//    // Mode digit
//    setDDRAM(1,0);
//    sendDataLCD(1, convKey2LCDhex(receivedData[3]));
//
//    // First time digit
//    unsigned int secondsBCD = 0;
//    secondsBCD = BCD2Hex(receivedData[5]);
//    unsigned int timeElap = 0;
//    timeElap = secondsBCD + (60 * receivedData[4]);
//    unsigned int T1 = 0;
//    T1 = timeElap/100;
//    convDecData[1] = T1;
//    T1 = convKey2LCDhex(convDec2Hex(T1));
//
//    // Second time digit
//    unsigned int T2 = 0;
//    T2 = (timeElap - (100 * convDecData[1]))/10;
//    convDecData[2] = T2;
//    T2 = convKey2LCDhex(convDec2Hex(T2));
//
//    // Third time digit
//    unsigned int T3 = 0;
//    T3 = (timeElap - (100 * convDecData[1]) - (10 * convDecData[2]));
//    T3 = convKey2LCDhex(convDec2Hex(T3));
//
//    setDDRAM(1,2);
//    sendDataLCD(1, T1);
//    sendDataLCD(1, T2);
//    sendDataLCD(1, T3);
//
//    // First plant temp digit
//    unsigned int P1 = 0;
//    P1 = receivedData[6]/10;
//    convDecData[3] = P1;
//    P1 = convKey2LCDhex(convDec2Hex(P1));
//
//    // Second plant temp digit
//    unsigned int P2 = 0;
//    P2 = receivedData[6] - (10 * convDecData[3]);
//    P2 = convKey2LCDhex(convDec2Hex(P2));
//
//    setDDRAM(1,10);
//    sendDataLCD(1, P1);
//    sendDataLCD(1, P2);
//
//    // Third Plant temp digit
//    unsigned int P3 = 0;
//    P3 = convKey2LCDhex(convDec2Hex(receivedData[7]));
//    setDDRAM(1,13);
//    sendDataLCD(1, P3);
//
//    setDDRAM(0,6);

    return;
}

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
    //-- Setup Ports
    P1DIR |= BIT7;              // Set P1.7 as an output
    P1OUT &= ~BIT7;             // Clear P1.7
    P1DIR |= BIT6;              // Set P1.6 as an output
    P1OUT &= ~BIT6;             // Clear P1.6
    P1DIR |= BIT5;              // Set P1.5 as an output
    P1OUT &= ~BIT5;             // Clear P1.5
    P1DIR |= BIT4;              // Set P1.4 as an output
    P1OUT &= ~BIT4;             // Clear P1.4
    P3DIR |= BIT7;              // Set P3.7 as an output
    P3OUT &= ~BIT7;             // Clear P3.7
    P3DIR |= BIT6;              // Set P3.6 as an output
    P3OUT &= ~BIT6;             // Clear P3.6

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

void initLCD(void){
    int i = 0;
    dataDelay(1000);            // Initial delay required for LCD startup

    for(i=3; i==0; i--){
        initFunctionSet();
        latchClock();
        dataDelay(82);
    }

    sendDataLCD(0,0b00101000);  // Function Set - Change operation mode to 4-bit
    sendDataLCD(0,0b00001111);  // Display on, Turn display on and set cursor style
    sendDataLCD(0,0b00000001);  // Display clear, set DDRAM to 00h
    sendDataLCD(0,0b00000110);  // Set Entry Mode
    sendDataLCD(0,0b00000010);  // Move cursor to home position
    sendDataLCD(0,0b01000000);  // SetCGRAM();
    setDDRAM(0,0);              // Set DDRAM Location to 00h

    resetDisplay();

    return;
}

int main(void){
     WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    configI2C();

    configPorts();

    PM5CTL0 &= ~LOCKLPM5;       // Turn on GPIO

    configTimerB0();

    initLCD();

    sendDataLCD(0,0b00001100);  // Turn off cursor

    // Main loop
    while(1){
        convertReceivedData();

        dataDelay(3170);       // 32790 is ~2 seconds
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

        // TODO: reset any counters and flags.
        receiveDataCounter = 0;
//
//        UCB1IFG &= ~UCSTPIFG; // TODO: these may cause problems...
//        UCB1STATW &= ~UCBBUSY;
        break;
    case 0x16:
        /*
         * Data Received;
         */
        receivedData[receiveDataCounter] = UCB1RXBUF;
        receiveDataCounter++;

        if (receiveDataCounter >= lcdPackets){
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
