#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

void receiveEvent(int numberOfBytes);

const int ledPin = LED_BUILTIN;

int updateTargets = false;
int receiveDataCounter = 0;

int indexerState = false;
long actualFlywheelVelocity = 0;
long targetFlywheelVelocity = 0;
long maxFlywheelVelocity = 6000;
float actualRotationalPosition = 0.0;
float targetRotationalPosition = 0.0;

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

//float round(float d)
//{
//  return floor(d + 0.5);
//}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int numberOfBytes)
{
  receiveDataCounter = 0;
  indexerState = false;
  targetFlywheelVelocity = 0;
  targetRotationalPosition = 0.0;
  Serial.print("RecieveEvent:\t");
  Serial.print(numberOfBytes);
  Serial.print("Bytes\t");
  while (Wire1.available() > 1) { // loop through all but the last
    unsigned char c = Wire1.read();        // receive byte
    Serial.print(c);             // print the character
    if (receiveDataCounter < 4) {
      targetFlywheelVelocity += ((int)c) * pow(10, (3 - receiveDataCounter));
    } else if (receiveDataCounter < 8) {
      targetRotationalPosition += ((int)c) * pow(10, (6 - receiveDataCounter));
    }
    receiveDataCounter++;
  }
  unsigned char x = Wire1.read();           // receive last byte
  Serial.print(x);             // print the integer
  if ((int)x == 1) {
    indexerState = true;
  } else {
    indexerState = false;
  }
  receiveDataCounter++;

  if (targetFlywheelVelocity >= maxFlywheelVelocity) {
    targetFlywheelVelocity = maxFlywheelVelocity;
  }

  Serial.print("\tFwTV: ");
  Serial.print(targetFlywheelVelocity);
  Serial.print(targetFlywheelVelocity == maxFlywheelVelocity ? "(MAX)" : "");
  Serial.print("\tRTP: ");
  Serial.print(targetRotationalPosition);
  Serial.print("\tIndexer: ");
  Serial.println(indexerState == true ? "indexer on" : "indexer off");


}

void requestEvent()
{
  //  Serial.println("RequestEvent");

//  actualFlywheelVelocity = 2587;
//  actualRotationalPosition = 51.7;
  // unsigned char dataToSend[8] = {0x02, 0x05, 0x03, 0x05, 0x01, 0x07, 0x03, 0x08};
  
  unsigned char dataToSend[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

  // Construct actual flywheel velocity
  long tempAFV = actualFlywheelVelocity;
  dataToSend[0] = (unsigned char)(int)(tempAFV / 1000);
  tempAFV -= (tempAFV / 1000) * 1000;
  dataToSend[1] = (unsigned char)(tempAFV / 100);
  tempAFV -= (tempAFV / 100) * 100;
  dataToSend[2] = (unsigned char)(tempAFV / 10);
  tempAFV -= (tempAFV / 10) * 10;
  dataToSend[3] = (unsigned char)(tempAFV);

  // Construct actual rotational position
  float tempARP = actualRotationalPosition;
//  printf("a:%f\n", tempARP);
  dataToSend[4] = (unsigned char)(int)(tempARP / 100);
//  printf("x:%d\n", (unsigned char)(int)(tempARP / 100));
//  printf("a:%f\n", tempARP);
  tempARP -= ((int)tempARP / 100) * 100;
//  printf("b:%f\n", tempARP);

  dataToSend[5] = (unsigned char)(int)(tempARP / 10);
  tempARP -= ((int)tempARP / 10) * 10;
  dataToSend[6] = (unsigned char)((int)tempARP);
  tempARP -= ((int)tempARP);
  dataToSend[7] = (unsigned char)(int)(round(tempARP / 0.1));

  //  printf("afv:%d\t%d\t%d\t%d\n", dataToSend[0], dataToSend[1], dataToSend[2], dataToSend[3]);
  //  printf("arp:%d\t%d\t%d\t%d\n", dataToSend[4], dataToSend[5], dataToSend[6], dataToSend[7]);

  Serial.print("RequestEvent:\t8Bytes\t");
  int i;
  String str;
  for (i = 0; i < 8; i++) {
    str += dataToSend[i];
  }
  Serial.println(str);

  Wire1.write(dataToSend, 8); // 8-Bytes (4 for flywheel v, 3 for rotation, 1 for rotation deciaml) should be FwA=2535, RA=173.8
  // "25351738"
}
