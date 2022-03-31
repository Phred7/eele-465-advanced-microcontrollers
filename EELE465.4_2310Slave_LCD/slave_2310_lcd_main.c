#include <msp430.h>

/**
 * main.c
 *
 * Built for MSP 2310 board. Initializes LCD. Writes input value to LCD when the button is
 * pressed. In standby mode until a new character is pressed
 *
 *      Port Definitions
 *      P1.7 - DB7
 *      P1.6 - DB6
 *      P1.5 - DB5
 *      P1.4 - DB4
 *      P2.7 - RS       Data/Instruction Register Select
 *      P2.6 - E        Enable Signal
 */
int DelayFlag = 0;          // Delay flag {"1", Hold Delay; "0", Ignore Delay}
int InputStatus = 0;        // {"0", No new input; "1", New value from Master}
int CursorLocation = 0;     // Keep track of position on LCD
unsigned int InValue = 0x0;         // Button value from Keypad
unsigned int LastValue = 0x0;         // Hold value of last button press

int DataDelay(int TimerCount){
    DelayFlag = 1;

    TB0CCR0 = TimerCount;            // Seconds input times counter compare value for 1 second

    TB0CCTL0 |= CCIE;             // Enable TB0 Overflow IRQ
    TB0CCTL0 &= ~CCIFG;           // Clear CCR0 flag

    while(DelayFlag == 1){};         // Loop until delay has finished

    TB0CCTL0 &= ~CCIE;              // Disable TB0 Overflow IRQ

    return(0);
}

int LatchClock (void){
    P2OUT |= BIT6;          // Set P2.6 - Pin E
    DataDelay(15);
//    _delay_cycles(1000);    // Delay 1000 clock cycles
    P2OUT &= ~BIT6;         // Clear P2.6 - Pin E

    return(0);
}

int InitFunctionSet(void){
    P2OUT &= ~BIT7;         // Clear RS bit
    P1OUT |= BIT4 | BIT5;
    P1OUT &= -(BIT6|BIT7);

    return(0);
}

int DisplayClear(void){
    P2OUT &= ~BIT7;         // Clear RS bit

    P1OUT &= ~BIT4;
    P1OUT &= ~BIT5;
    P1OUT &= ~BIT6;
    P1OUT &= ~BIT7;

    LatchClock();

    P1OUT &= ~BIT7;
    P1OUT &= ~BIT6;
    P1OUT &= ~BIT5;
    P1OUT |= BIT4;          // Clear Display

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
      P2OUT |= BIT7;          // Set RS bit (Indicates Data send)
    } else {
        P2OUT &= ~BIT7;       // Clear RS bit (Indicates Instruction send)
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
        P2OUT &= ~BIT7;         // Clear RS bit
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
               CursorLocation = 0;
               SendDataLCD(0,0b00000001);  // Display clear, set DDRAM to 00h

               SendDataLCD(0,0b10000000);  // Set DDRAM Location to 00h
               break;

           default:
               break;
           }

    return(0);
}

int CheckCursorLocation(void){
    if (CursorLocation == 16){
        CursorLocation = CursorLocation + 48;
        SendDataLCD(0,0b11000000);      // Set DDRAM to second line, first position
    }
    if(CursorLocation == 80){
        CursorLocation = 0;
//        SendDataLCD(0,0b00000001);  // Display clear, set DDRAM to 00h
        DisplayClear();

        SendDataLCD(0,0b10000000);  // Set DDRAM Location to 00h
    }
    return 0;
}

int main(void)
{
//-- Initialization
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    //-- Setup I2C
    PM5CTL0 &= ~LOCKLPM5;       // Enable GPIO

    P1SEL1 &= ~BIT3;            // P1.3 = SCL
    P1SEL0 |= BIT3;

    P1SEL1 &= ~BIT2;            // P1.2 = SDA
    P1SEL0 |= BIT2;

    UCB0CTLW0 |= UCSWRST;

    UCB0CTLW0 |= UCSSEL_3;
    UCB0BRW = 1;
    UCB0CTLW0 |= UCMODE_3 + UCSYNC;           // I2C mode, sync mode (Do not set clock in slave mode)

    UCB0CTLW0 &= ~UCMST;
    UCB0I2COA0 = 0x69 | UCOAEN;               // Slave address is 0x69; enable it
    UCB0CTLW0 &= ~UCTR;     // set to receive
    UCB0CTLW1 &= ~UCSWACK;  // auto ACK

    UCB0CTLW0 &= ~UCSWRST;

    UCB0CTLW1 &= ~UCRXIFG0;

    UCB0IE |= UCRXIE0;
    UCB0IE |= UCTXIE0;
    __enable_interrupt();

    //-- Setup Ports
    P1DIR |= BIT7;              // Set P1.7 as an output
    P1OUT &= ~BIT7;             // Clear P1.7
    P1DIR |= BIT6;              // Set P1.6 as an output
    P1OUT &= ~BIT6;             // Clear P1.6
    P1DIR |= BIT5;              // Set P1.5 as an output
    P1OUT &= ~BIT5;             // Clear P1.5
    P1DIR |= BIT4;              // Set P1.4 as an output
    P1OUT &= ~BIT4;             // Clear P1.4
    P2DIR |= BIT7;              // Set P2.7 as an output
    P2OUT &= ~BIT7;             // Clear P2.7
    P2DIR |= BIT6;              // Set P2.6 as an output
    P2OUT &= ~BIT6;             // Clear P2.6

    PM5CTL0 &= ~LOCKLPM5;      // Turn on GPIO

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

    //-- Setup Variables
    int i = 0;                  // Loop Counter

//-- Initialize LCD
    DataDelay(820);                // Initial delay for LCD startup

    for (i = 3; i == 0; i--){
        InitFunctionSet();
        LatchClock();
        DataDelay(82);
    }

    SendDataLCD(0,0b00101100);  // Function Set - Change operation mode to 4-bit

    SendDataLCD(0,0b00001111);  // Display on, Turn display on and set cursor style

//    SendDataLCD(0,0b00000001);  // Display clear, set DDRAM to 00h
    DisplayClear();

    SendDataLCD(0,0b00000110);  // Set Entry Mode

    SendDataLCD(0,0b00000010);  // Move cursor to home position

    SendDataLCD(0,0b01000000);  //  SetCGRAM();

    SendDataLCD(0,0b10000000);  // Set DDRAM Location to 00h


//-- Main Loop
    while(1){

//        if (InputStatus == 1){
//            InputStatus = 0;
//            if((LastValue == 0)&&(InValue != 0)){
//                CheckCursorLocation();
//                CheckInputVal(InValue);
//                CursorLocation++;
//                DataDelay(1000);
//            }
//            LastValue = InValue;
//        }
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
#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    InValue = UCB0RXBUF;
    InputStatus = 1;
    UCB0CTLW1 &= ~UCRXIFG0;
    return;
}
//-- END EUSCI_B0_I2C_ISR
