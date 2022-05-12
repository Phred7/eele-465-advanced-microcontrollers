#include <Servo.h>

#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

void receiveEvent(int numberOfBytes);

const int ledPin = LED_BUILTIN;
int ledState = LOW;

int updateTargets = false;
int receiveDataCounter = 0;

int indexerState = false;
long actualFlywheelVelocity = 0;
long targetFlywheelVelocity = 0;
long maxFlywheelVelocity = 6000;
float actualRotationalPosition = 0.0;
float targetRotationalPosition = 0.0;

const int indexerMotorControllerPin = 14;
const int indexerEncoderA = 2;
const int indexerEncoderB = 3;
const int indexerEncoderCPR = 12;
Servo indexerMotorController;

const int rotationMotorControllerPin = 15;
const int rotationEncoderA = 4;
const int rotationEncoderB = 5;
const int rotationEncoderCPR = 172;
int rotationDirection = 1;  // 1 is CCW, 0 is CW
Servo rotationMotorController;

const int flywheelMotorControllerPin = 33;
const int flywheelEncoderA = 0;
const int flywheelEncoderB = 1;
const int flywheelEncoderCPR = 4096;
Servo flywheelMotorController;

const int hallSwitchPin = 9;
int hallSwitch = false;

void setup()
{
  Wire1.begin(0x31);                // join i2c bus with address #4
  Wire1.onReceive(receiveEvent); // register receive event
  Wire1.onRequest(requestEvent); // register request event
  Serial.begin(115200);           // start serial for output
  pinMode(ledPin, OUTPUT);
  
  attachInterrupt(digitalPinToInterrupt(indexerEncoderA), indexerEncoderAISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(indexerEncoderB), indexerEncoderBISR, CHANGE);
  
  attachInterrupt(digitalPinToInterrupt(rotationEncoderA), rotationEncoderAISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rotationEncoderB), rotationEncoderBISR, CHANGE);

  attachInterrupt(digitalPinToInterrupt(flywheelEncoderA), flywheelEncoderAISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(flywheelEncoderB), flywheelEncoderBISR, CHANGE);
  
  attachInterrupt(digitalPinToInterrupt(hallSwitchPin), hallSwitchISR, RISING);

  indexerMotorController.attach(indexerMotorControllerPin, 1500, 2000);
  rotationMotorController.attach(rotationMotorControllerPin, 1000, 2000); //TODO: this could be used to change the direction in hall effect interrupt???
  flywheelMotorController.attach(flywheelMotorControllerPin, 1500, 1000);    
}

void loop()
{
  if (indexerState == true) {
    indexerMotorController.write(60);
  } else {
    indexerMotorController.write(0);
  }

  
  rotationMotorController.write(90);

  if (targetFlywheelVelocity > 0) {
    float mappedServoValue = map(targetFlywheelVelocity, 0, maxFlywheelVelocity, 0, 180);
    flywheelMotorController.write(mappedServoValue); // min of ~15
  } else {
    flywheelMotorController.write(0);
  }
  
}

void receiveEvent(int numberOfBytes)
{
  Serial.print("RecieveEvent:\t");
  Serial.print(numberOfBytes);
  Serial.print("Bytes\t");

  if (numberOfBytes != 9) {
    while (Wire1.available() > 1) { // loop through all but the last
      unsigned char c = Wire1.read(); 
    }
    Serial.println("Refused Bytes");
    return;
  } else {
    receiveDataCounter = 0;
    indexerState = false;
    targetFlywheelVelocity = 0;
    targetRotationalPosition = 0.0;
    while (Wire1.available() > 1) { // loop through all but the last
      unsigned char c = Wire1.read();        // receive byte
      //Serial.print(c);             // print the character
      if (receiveDataCounter < 4) {
        targetFlywheelVelocity += ((int)c) * pow(10, (3 - receiveDataCounter));
      } else if (receiveDataCounter < 8) {
        targetRotationalPosition += ((int)c) * pow(10, (6 - receiveDataCounter));
      }
      receiveDataCounter++;
    }
    unsigned char x = Wire1.read();           // receive last byte
    //Serial.print(x);             // print the integer
    if ((int)x == 1) {
      indexerState = true;
    } else {
      indexerState = false;
    }
    receiveDataCounter++;
  
    if (targetFlywheelVelocity >= maxFlywheelVelocity) {
      targetFlywheelVelocity = maxFlywheelVelocity;
    }
  
//    Serial.print("\tFwTV: ");
//    Serial.print(targetFlywheelVelocity);
//    Serial.print(targetFlywheelVelocity == maxFlywheelVelocity ? "(MAX)" : "");
//    Serial.print("\tRTP: ");
//    Serial.print(targetRotationalPosition);
//    Serial.print("\tIndexer: ");
//    Serial.println(indexerState == true ? "indexer on" : "indexer off");
//  
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(ledPin, ledState); 
  }
}

void requestEvent()
{
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
  dataToSend[4] = (unsigned char)(int)(tempARP / 100);
  tempARP -= ((int)tempARP / 100) * 100;
  dataToSend[5] = (unsigned char)(int)(tempARP / 10);
  tempARP -= ((int)tempARP / 10) * 10;
  dataToSend[6] = (unsigned char)((int)tempARP);
  tempARP -= ((int)tempARP);
  dataToSend[7] = (unsigned char)(int)(round(tempARP / 0.1));

  Serial.print("RequestEvent:\t8Bytes\t");
//  int i;
//  String str;
//  for (i = 0; i < 8; i++) {
//    str += dataToSend[i];
//  }
//  Serial.println(str);

  Wire1.write(dataToSend, 8); // 8-Bytes (4 for flywheel v, 3 for rotation, 1 for rotation deciaml)
}

void hallSwitchISR() {
  rotationMotorController.write(90);
  hallSwitch = true;
}

void indexerEncoderAISR() {
  
}

void indexerEncoderBISR() {
  
}

void rotationEncoderAISR() {
  
}

void rotationEncoderBISR() {
  
}

void flywheelEncoderAISR() {
  
}

void flywheelEncoderBISR() {
  
}
