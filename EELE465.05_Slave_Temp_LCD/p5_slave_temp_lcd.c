#include <msp430.h> 
/**
 * main.c
 *
 * Built for MSP 2355 Dev board. Initializes LCD. Writes a set display to the LCD at startup.
 * Prints updated temperature values to LCD when received from Master.
 *
 *      Port Definitions
 *      P1.7 - DB7
 *      P1.6 - DB6
 *      P1.5 - DB5
 *      P1.4 - DB4
 *      P4.7 - RS       Data/Instruction Register Select
 *      P4.6 - E        Enable Signal
 */
int DelayFlag = 0;          // Delay flag {"1", Hold Delay; "0", Ignore Delay}
int InputStatus = 0;        // {"0", No new input; "1", New value from Master}
int LastStatus = 0;         // Keep track of last button press
unsigned int InValue = 0x22;         // "A" in ASCII -> Button value from Keypad
unsigned int recievedData = 0x00;
unsigned int recievedData2 = 0x00;
int recievedDataFlag = 0;

int LatchClock (void){
    P4OUT |= BIT6;          // Set P2.6 - Pin E
    _delay_cycles(1000);    // Delay 1000 clock cycles
    P4OUT &= ~BIT6;         // Clear P2.6 - Pin E

    return(0);
}

int DataDelay(int TimerCount){
    DelayFlag = 1;

    TB0CCR0 = TimerCount;            // Seconds input times counter compare value for 1 second

    TB0CCTL0 |= CCIE;             // Enable TB0 Overflow IRQ
    TB0CCTL0 &= ~CCIFG;           // Clear CCR0 flag

    while(DelayFlag == 1){};         // Loop until delay has finished

    TB0CCTL0 &= ~CCIE;              // Disable TB0 Overflow IRQ

    return(0);
}

int InitFunctionSet(void){
    P4OUT &= ~BIT7;         // Clear RS bit
    P1OUT |= BIT4 | BIT5;
    P1OUT &= -(BIT6|BIT7);

    return(0);
}

int SendDataLCD(int RSflag, int DataByte){

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

    if (RSflag == 1){
      P4OUT |= BIT7;          // Set RS bit (Indicates Data send)
    } else {
        P4OUT &= ~BIT7;       // Clear RS bit (Indicates Instruction send)
    }

    if ((DataByte & 0b10000000) == 0){
        B7 = 0b0000;}
    if((DataByte & 0b01000000) == 0){
        B6 = 0b0000;}
    if((DataByte & 0b00100000) == 0){
        B5 = 0b0000;}
    if((DataByte & 0b00010000) == 0){
        B4 = 0b0000;}
    if((DataByte & 0b00001000) == 0){
        B3 = 0b0000;}
    if((DataByte & 0b00000100) == 0){
        B2 = 0b0000;}
    if((DataByte & 0b00000010) == 0){
        B1 = 0b0000;}
    if((DataByte & 0b00000001) == 0){
        B0 = 0b0000;}

    UpperByte = (B7 | B6 | B5 | B4);
    UpperByte = UpperByte << 4;         // shift data into upper nibble
    LowerByte = (B3 | B2 | B1 | B0);
    LowerByte = LowerByte << 4;         // shift data into upper nibble

    P1OUT &= ~0b11110000;
    P1OUT |= UpperByte;
    LatchClock();
    P1OUT &= ~0b11110000;
    P1OUT |= LowerByte;
    LatchClock();

    if (RSflag == 1){
        P4OUT &= ~BIT7;         // Clear RS bit
    }

    DataDelay(800);

    return(0);
}

int CheckInputVal(int ButtonValue){
       switch(ButtonValue){
           case 0x81:                  // "A"
               SendDataLCD(1,0b01000001);
               break;
           case 0x41:                  // "B"
               SendDataLCD(1,0b01000010);
               break;
           case 0x21:                  // "C"
               SendDataLCD(1,0b01000011);
               break;
           case 0x11:                  // "D"
               SendDataLCD(1,0b01000100);
               break;
           case 0x14:                  // "0"
               SendDataLCD(1,0b00110000);
               break;
           case 0x88:                  // "1"
               SendDataLCD(1,0b00110001);
               break;
           case 0x84:                  // "2"
               SendDataLCD(1,0b00110010);
               break;
           case 0x82:                  // "3"
               SendDataLCD(1,0b00110011);
               break;
           case 0x48:                  // "4"
               SendDataLCD(1,0b00110100);
               break;
           case 0x44:                  // "5"
               SendDataLCD(1,0b00110101);
               break;
           case 0x42:                  // "6"
               SendDataLCD(1,0b00110110);
               break;
           case 0x28:                  // "7"
               SendDataLCD(1,0b00110111);
               break;
           case 0x24:                  // "8"
               SendDataLCD(1,0b00111000);
               break;
           case 0x22:                  // "9"
               SendDataLCD(1,0b00111001);
               break;
           case 0x18:                  // "*"
               SendDataLCD(1,0b00101010);
               break;
           case 0x12:                  // "#"
               SendDataLCD(1,0b00100011);
               break;

           default:
               break;
           }

    return(0);
}

int SetDDRAM(int Row, int Col){
    P4OUT &= ~BIT7;         // Clear RS bit

    // Set upper nibble
    if (Row == 0){
        P1OUT &= ~0b11110000;
        P1OUT |= 0b10000000;
    } else if (Row == 1){
        P1OUT &= ~0b11110000;
        P1OUT |= 0b11000000;
    }

    LatchClock();

    // Set lower nibble
    Col = Col << 4;
    P1OUT &= ~0b11110000;
    P1OUT |= Col;
    LatchClock();

    return(0);
}

int InitializeTempDisp(void){
    SetDDRAM(0,0);
    SendDataLCD(1,0b01000101);  // E
    SendDataLCD(1,0b01101110);  // n
    SendDataLCD(1,0b01110100);  // t
    SendDataLCD(1,0b01100101);  // e
    SendDataLCD(1,0b01110010);  // r
    SendDataLCD(1,0b00100000);  // Space
    SendDataLCD(1,0b01101110);  // n
    SendDataLCD(1,0b00111010);  // :

    SetDDRAM(1,0);
    SendDataLCD(1,0b01010100);  // T
    SendDataLCD(1,0b00100000);  // Space
    SendDataLCD(1,0b00111101);  // =
    SendDataLCD(1,0b00100000);  // Space

    SetDDRAM(1,7);
    SendDataLCD(1,0b11011111);  // Degree
    SendDataLCD(1,0b01001011);  // K

    SetDDRAM(1,12);
    SendDataLCD(1,0b00101110);  // .
    SendDataLCD(1,0b00100000);  // Space
    SendDataLCD(1,0b11011111);  // Degree
    SendDataLCD(1,0b01000011);  // C

    return 0;
}

unsigned int ConvDec2Hex(int Number){
    unsigned int HexNumber = 0;

    switch(Number){
       case 0:
           HexNumber = 0x14;
           break;
       case 1:
           HexNumber = 0x88;
           break;
       case 2:
           HexNumber = 0x84;
           break;
       case 3:
           HexNumber = 0x82;
           break;
       case 4:
           HexNumber = 0x48;
           break;
       case 5:
           HexNumber = 0x44;
           break;
       case 6:
           HexNumber = 0x42;
           break;
       case 7:
           HexNumber = 0x28;
           break;
       case 8:
           HexNumber = 0x24;
           break;
       case 9:
           HexNumber = 0x22;
           break;
       default:
           break;
    }
    return HexNumber;
}

int TestSend(float ADCval){
    int Digit1;
    int Digit2;
    int Digit3;
    int KelVal;
    int KelVal1;
    int KelVal2;
    int KelVal3;

    KelVal = ADCval + 273.15;

    // Most significant digit
    Digit1 = ADCval/10;
    KelVal1 = KelVal/100;

    // Middle digit
    ADCval = ADCval - 10*Digit1;
    Digit2 = ADCval;
    KelVal = KelVal - 100*KelVal1;
    KelVal2 = KelVal/10;

    // Least significant digit
    ADCval = ADCval - Digit2;
    Digit3 = 10 * ADCval;
    KelVal = KelVal - 10*KelVal2;
    KelVal3 = KelVal;


    // Push Values to LCD
    SetDDRAM(1,4);
    KelVal1 = ConvDec2Hex(KelVal1);
    CheckInputVal(KelVal1);
    KelVal2 = ConvDec2Hex(KelVal2);
    CheckInputVal(KelVal2);
    KelVal3 = ConvDec2Hex(KelVal3);
    CheckInputVal(KelVal3);

    SetDDRAM(1,10);
    Digit1 = ConvDec2Hex(Digit1);
    CheckInputVal(Digit1);          // Push value to LCD

    Digit2 = ConvDec2Hex(Digit2);
    SetDDRAM(1,11);
    CheckInputVal(Digit2);

    SetDDRAM(1,13);
    Digit3 = ConvDec2Hex(Digit3);
    CheckInputVal(Digit3);

    SetDDRAM(0,8);

    InputStatus = 0;

    return 0;
}

void configI2C(void) {
    P4SEL1 &= ~BIT7;            // P4.7 = SCL
    P4SEL0 |= BIT7;

    P4SEL1 &= ~BIT6;            // P4.6 = SDA
    P4SEL0 |= BIT6;

    UCB1CTLW0 |= UCSWRST;

    UCB1CTLW0 |= UCSSEL_3;
    UCB1BRW = 1;
    UCB1CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)

    UCB1CTLW0 &= ~UCMST;
    UCB1I2COA0 = 0x69 | UCOAEN;               // Slave address is 0x77; enable it
    UCB1CTLW0 &= ~UCTR;     // set to receive
    UCB1CTLW1 &= ~UCSWACK;  // auto ACK

    UCB1CTLW0 &= ~UCSWRST;

    UCB1CTLW1 &= ~UCRXIFG0;

    UCB1IE |= UCRXIE0;
    UCB1IE |= UCTXIE0;
}

int main(void)
{
//-- Initialization
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    //-- Setup Ports
    P1DIR |= BIT7;              // Set P1.7 as an output
    P1OUT &= ~BIT7;             // Clear P1.7
    P1DIR |= BIT6;              // Set P1.6 as an output
    P1OUT &= ~BIT6;             // Clear P1.6
    P1DIR |= BIT5;              // Set P1.5 as an output
    P1OUT &= ~BIT5;             // Clear P1.5
    P1DIR |= BIT4;              // Set P1.4 as an output
    P1OUT &= ~BIT4;             // Clear P1.4
    P4DIR |= BIT7;              // Set P2.7 as an output
    P4OUT &= ~BIT7;             // Clear P2.7
    P4DIR |= BIT6;              // Set P2.6 as an output
    P4OUT &= ~BIT6;             // Clear P2.6

    // Setup LEDs
    P1DIR |= BIT0;              // Set LED1 as an output
    P1OUT &= ~BIT0;             // Turn off LED1 initially
    P6DIR |= BIT6;              // Set LED2 as an output
    P6OUT &= ~BIT6;             // Turn off LED2 initially

    // Setup S1
    P4DIR &= ~BIT1;
    P4REN |= BIT1;
    P4OUT |= BIT1;
    P4IES |= BIT1;

    PM5CTL0 &= ~LOCKLPM5;      // Turn on GPIO

    configI2C();

    //-- Setup Timer
    TB0CTL |= TBCLR;            // Clear timer and dividers
    TB0CTL |= TBSSEL__SMCLK;     // Source = SMCLK
    TB0CTL |= MC__UP;           // Mode = Up
    TB0CTL |= ID__8;            // Divide by 8
    TB0EX0 |= TBIDEX__8;        // Divide clock by 8 again
    TB0CCR0 = 32791;            // CCR0 = 32791

    //-- Setup Timer Overflow IRQ
    TB0CCTL0 &= ~CCIE;           // Disable TB0 Overflow IRQ
    TB0CCTL0 &= ~CCIFG;           // Clear CCR0 flag
    __enable_interrupt();       // Enable Maskable IRQs

    //-- Setup Button IRQ
    P4IFG &= ~BIT1;
    P4IE |= BIT1;
    __enable_interrupt();

    //-- Setup Variables
    int i = 0;                  // Loop Counter

//    int CursorLocation = 0;     // Keep track of position on LCD

//-- Initialize LCD
    DataDelay(820);                // Initial delay for LCD startup

    for (i = 3; i == 0; i--){
        InitFunctionSet();
        LatchClock();
        DataDelay(82);
    }

    SendDataLCD(0,0b00101000);  // Function Set - Change operation mode to 4-bit

    SendDataLCD(0,0b00001111);  // Display on, Turn display on and set cursor style

    SendDataLCD(0,0b00000001);  // Display clear, set DDRAM to 00h

    SendDataLCD(0,0b00000110);  // Set Entry Mode

    SendDataLCD(0,0b00000010);  // Move cursor to home position

    SendDataLCD(0,0b01000000);  // SetCGRAM();

    SetDDRAM(0,0);  // Set DDRAM Location to 00h

    InitializeTempDisp();


//-- Main Loop
    while(1){

        if (InputStatus == 1){
            P1OUT &= ~BIT0;     // Turn off LED1
            TestSend(23.5);
 //           CursorLocation++;
            DataDelay(32000);           // 16395 per second
            TestSend(39.2);
            DataDelay(32000);
            TestSend(90.0);
            DataDelay(8000);
        } else {
            P1OUT |= BIT0;          // Turn on LED1
        }

    }
    return(0);
}

//-- Interrupt Service Routines
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void){

    DelayFlag = 0;
    TB0CCTL0 &= ~CCIFG;             // Clear CCR0 Flag

}

//-- Service I2C
#pragma vector = EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void) {
    if (recievedDataFlag == 0) {
        recievedData = UCB1RXBUF;
        recievedDataFlag = 1;
    } else {
        recievedData2 = UCB1RXBUF;
        recievedDataFlag = 0;
    }

    UCB1CTLW1 &= ~UCRXIFG0;
    return;
}
//-- END EUSCI_B0_I2C_ISR

#pragma vector = PORT4_VECTOR
__interrupt void ISR_Port4_S1(void){

    InputStatus = 1;

    P4IFG &= ~BIT1;
}
