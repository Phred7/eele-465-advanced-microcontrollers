#include <Servo.h>
#include <FlexiTimer2.h>

#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

// method signitures
void receiveEvent(int numberOfBytes);
void hallSwitchISR(void);
void indexerEncoderAISR(void);
void indexerEncoderBISR(void);
void rotationEncoderAISR(void);
void rotationEncoderBISR(void);
void flywheelEncoderAISR(void);
void flywheelEncoderBISR(void);
void timerISR(void);

// other DIO
const int ledPin = LED_BUILTIN;
int ledState = LOW;

// i2c
int updateTargets = false;
int receiveDataCounter = 0;
int i2cConnected = false;
int i2cDisconnectedCounter = 0;

// indexer
const int indexerMotorControllerPin = 14;
const int indexerEncoderA = 2;
const int indexerEncoderB = 3;
const int indexerEncoderCPR = 12;
int indexerState = false;
Servo indexerMotorController;

// rotation
const int rotationMotorControllerPin = 15;
const int rotationEncoderA = 4;
const int rotationEncoderB = 5;
const int rotationEncoderCPR = 172;
float actualRotationalPosition = 0.0;
float targetRotationalPosition = 0.0;
int rotationDirection = 1;  // 1 is CCW, 0 is CW
Servo rotationMotorController;

// flywheel
const int flywheelMotorControllerPin = 33;
const int flywheelEncoderA = 0;
const int flywheelEncoderB = 1;
const int flywheelEncoderCPR = 4096;
long actualFlywheelVelocity = 0;
long targetFlywheelVelocity = 0;
long flywheelEncoderCounts = 0;
long maxFlywheelVelocity = 5500;
Servo flywheelMotorController;

// hall switch
const int hallSwitchPin = 9;
int hallSwitch = false;

// timer
int timerDelayMS = 250;
float deltaTime = (timerDelayMS / 1000.0);

void setup()
{
  // Setup I2C
  Wire1.begin(0x31);                // join i2c bus with address #0x31, #49d
  Wire1.onReceive(receiveEvent);    // register receive event
  Wire1.onRequest(requestEvent);    // register request event

  // Start serial connection
  Serial.begin(115200);             // start serial for output

  // config ports
  pinMode(ledPin, OUTPUT);

  // indexer interrupts
  attachInterrupt(digitalPinToInterrupt(indexerEncoderA), indexerEncoderAISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(indexerEncoderB), indexerEncoderBISR, CHANGE);

  // rotation interrupts
  attachInterrupt(digitalPinToInterrupt(rotationEncoderA), rotationEncoderAISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rotationEncoderB), rotationEncoderBISR, CHANGE);

  // flywheel interrupts
  attachInterrupt(digitalPinToInterrupt(flywheelEncoderA), flywheelEncoderAISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(flywheelEncoderB), flywheelEncoderBISR, CHANGE);

  // hall switch interrupts
  attachInterrupt(digitalPinToInterrupt(hallSwitchPin), hallSwitchISR, RISING);

  // enable timer, timer interrupt and start the timer
  FlexiTimer2::set(int(timerDelayMS), timerISR);
  FlexiTimer2::start();
  
  // Attach servos
  indexerMotorController.attach(indexerMotorControllerPin, 1500, 2000);
  rotationMotorController.attach(rotationMotorControllerPin, 1000, 2000); //TODO: this could be used to change the direction in hall effect interrupt???
  flywheelMotorController.attach(flywheelMotorControllerPin, 1500, 1000);    
}

void loop()
{

  if (i2cConnected == true) {
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
  } else {
    flywheelMotorController.write(0);
    rotationMotorController.write(90);
    indexerMotorController.write(0);
    Serial.println("I2C Bus Not Connected");
    delay(500);
  }
  
}

void receiveEvent(int numberOfBytes)
{
//  Serial.print("RecieveEvent:\t");
//  Serial.print(numberOfBytes);
//  Serial.print("Bytes\t");

  if (numberOfBytes != 9) {
    while (Wire1.available() > 1) { // loop through all but the last
      unsigned char c = Wire1.read(); 
    }
    Serial.println("Refused Bytes");
    return;
  } else {

    i2cConnected = true;
    i2cDisconnectedCounter = 0;
    
    receiveDataCounter = 0;
    indexerState = false;
    targetFlywheelVelocity = 0;
    targetRotationalPosition = 0.0;
    while (Wire1.available() > 1) { // loop through all but the last
      unsigned char c = Wire1.read();        // receive byte
      if (receiveDataCounter < 4) {
        targetFlywheelVelocity += ((int)c) * pow(10, (3 - receiveDataCounter));
      } else if (receiveDataCounter < 8) {
        targetRotationalPosition += ((int)c) * pow(10, (6 - receiveDataCounter));
      }
      receiveDataCounter++;
    }
    unsigned char x = Wire1.read();           // receive last byte
    if ((int)x == 1) {
      indexerState = true;
    } else {
      indexerState = false;
    }
    receiveDataCounter++;
  
    if (targetFlywheelVelocity >= maxFlywheelVelocity) {
      targetFlywheelVelocity = maxFlywheelVelocity;
    }

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
//
//  Serial.print("RequestEvent:\t8Bytes\t");

  Wire1.write(dataToSend, 8); // 8-Bytes (4 for flywheel v, 3 for rotation, 1 for rotation deciaml)
}

void hallSwitchISR() {
//  rotationMotorController.write(90);
//  hallSwitch = true;
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
  flywheelEncoderCounts++;
}

void flywheelEncoderBISR() {
  flywheelEncoderCounts++;
}

void timerISR() {

  if (i2cConnected == true) {
    if (flywheelEncoderCounts > 0) {
      actualFlywheelVelocity = ((flywheelEncoderCounts / deltaTime) * 60) / flywheelEncoderCPR;
      flywheelEncoderCounts = 0;
    } else {
      actualFlywheelVelocity = 0;
    }
    Serial.print("Flywheel Velocity:\t");
    Serial.print(actualFlywheelVelocity);
    Serial.println(" RPM");
  }
  
  i2cDisconnectedCounter++;
  if (i2cDisconnectedCounter >= 8) {
    i2cConnected = false;
  }
}
