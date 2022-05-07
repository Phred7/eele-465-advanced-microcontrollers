//#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
//  #define SERIAL SerialUSB
//  #define SYS_VOL   3.3
//#else
//  #define SERIAL Serial
//  #define SYS_VOL   5
//#endif

// Wire Peripheral Receiver
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Receives data as an I2C/TWI Peripheral device
// Refer to the "Wire Master Writer" example for use with this

// Created 29 March 2006

// This example code is in the public domain.

#include <Wire.h>

const int ledPin = 13;

void setup()
{
  Wire.begin(31);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register receive event
  Wire.onRequest(requestEvent); // register request event
  Serial.begin(115200);           // start serial for output
  pinMode(ledPin, OUTPUT);
}

void loop()
{
  digitalWrite(ledPin, HIGH);   // set the LED on
  delay(500);
  digitalWrite(ledPin, LOW);    // set the LED off
  delay(500);
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int numberOfBytes)
{
  while(1 < Wire.available()) // loop through all but the last
  {
    char c = Wire.read(); // receive byte as a character
    Serial.print(c);         // print the character
  }
  int x = Wire.read();    // receive byte as an integer
  Serial.println(x);         // print the integer
}

void requestEvent() {
  Wire.write("wxyzabcd"); // 8-Bytes (4 for flywheel v, 3 for rotation, 1 for rotation deciaml)
  return;
}
