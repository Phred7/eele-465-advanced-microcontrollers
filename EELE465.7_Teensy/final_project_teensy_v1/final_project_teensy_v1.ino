#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

void receiveEvent(int numberOfBytes);

const int ledPin = LED_BUILTIN;

void setup()
{
  Wire1.begin(0x31);                // join i2c bus with address #4
  Wire1.onReceive(receiveEvent); // register receive event
  Wire1.onRequest(requestEvent); // register request event
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
  Serial.print("RecieveEvent:\t");
  Serial.print(numberOfBytes);
  Serial.print("Bytes\t");
  while(Wire1.available() > 1) {  // loop through all but the last
    unsigned char c = Wire1.read();        // receive byte
    Serial.print(c);             // print the character
  }
  unsigned char x = Wire1.read();           // receive last byte
  Serial.println(x);             // print the integer
}

void requestEvent()
{
//  Serial.println("RequestEvent");
  unsigned char dataToSend[8] = {0x02, 0x05, 0x03, 0x05, 0x01, 0x07, 0x03, 0x08};
  Wire1.write(dataToSend, 8); // 8-Bytes (4 for flywheel v, 3 for rotation, 1 for rotation deciaml) should be FwA=2535, RA=173.8
  // "25351738"
}
