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
long indexerEncoderCounts = 0;
long actualIndexerVelocity = 0;
int indexerState = false;
Servo indexerMotorController;

// rotation
const int rotationMotorControllerPin = 15;
const int rotationEncoderA = 4;
const int rotationEncoderB = 5;
const int rotationEncoderCPR = 172;
float actualRotationalPosition = 0.0;
float lastActualRotationalPosition = 0.0;
float targetRotationalPosition = 0.0;
int rotationDirection = 1;  // 1 is CCW, 0 is CW
int rotationEncoderChannelAState = LOW;
int rotationEncoderChannelBState = LOW;
long rotationEncoderCounts = 0;
long rotationLocalEncoderPosition = 0;
float rotationVelocity = 0;
float actualRotationalPositionError = 0;
float actualRotationalPositionLastError = 0;
int rotationHomed = true;
int rotationInitialHome = false;
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
int pidOffset = 0;
Servo flywheelMotorController;

// hall switch
const int hallSwitchPin = 9;
int hallSwitch = false;

// timer
int timerDelayMS = 250;
float deltaTime = (timerDelayMS / 1000.0);

int servoAngle = 0;

float curveVals[] = {0.0016,0.00639,0.01433,0.02538,0.03947,0.0565,0.0764,
    0.0989,0.1241,0.1512,0.1814,0.2132,0.2469,0.2822,0.3188,0.3566,0.3954,
    0.4348,0.4746,0.5146,0.5545,0.594,0.633,0.6711,0.7081,0.7437,0.7778,
    0.8102,0.8405,0.8687,0.8945,0.9178,0.9384,0.9562,0.9711,0.9829,0.9918,
    0.9974};
int steps = 38;
int currentStep = 0;
int delayLength = 150;
long rampCurrentTarget = 0;
int rampComplete = true;
int rampOffset = 0;

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
  attachInterrupt(digitalPinToInterrupt(hallSwitchPin), hallSwitchISR, FALLING);

  // enable timer, timer interrupt and start the timer
  FlexiTimer2::set(int(timerDelayMS), timerISR);
  
  // Attach servos
  indexerMotorController.attach(indexerMotorControllerPin, 1500, 2000);
  rotationMotorController.attach(rotationMotorControllerPin, 1000, 2000); //TODO: this could be used to change the direction in hall effect interrupt???
  flywheelMotorController.attach(flywheelMotorControllerPin, 1500, 1000);    

  servoAngle = mapIndexerPercentToAngle(-12);
}

void loop()
{
  while(rotationInitialHome == false) {
    hallSwitch = false;
    Serial.println("Homing rotation");
//    int servoA = mapIndexerPercentToAngle(14);
//    while(hallSwitch == false) {
//      rotationMotorController.write(servoA);
//    }
    rotationMotorController.write(90);
    actualRotationalPosition = 0.0;
    lastActualRotationalPosition = 0.0;
    rotationDirection = 1;
    rotationEncoderCounts = 0;
    rotationLocalEncoderPosition = 0;
    rotationVelocity = 0;
//    rotationMotorController.write(mapIndexerPercentToAngle(-12));
//    delay(100);
    rotationMotorController.write(90);
    rotationInitialHome = true;
    Serial.println("Homing rotation complete");
    FlexiTimer2::start();
    hallSwitch = false; 
  }

  if (i2cConnected == true) {

    // indexer
    if (indexerState == true) {
      indexerMotorController.write(60);
    } else {
      indexerMotorController.write(0);
    }

    // rotation
//    if (hallSwitch == true) {
//      if (rotationDirection == 0) { // ccw (-)
//        actualRotationalPosition = 360.0;
//        lastActualRotationalPosition = 360.0;
//        rotationEncoderCounts = rotationEncoderCPR;
//        rotationLocalEncoderPosition = rotationEncoderCPR;
//        servoAngle = mapIndexerPercentToAngle(0);
//        rotationVelocity = 0;
//        hallSwitch = false;
//      } else if (rotationDirection == 1) { // cw (+)
//        actualRotationalPosition = 0.0;
//        lastActualRotationalPosition = 0.0;
//        rotationEncoderCounts = 0;
//        rotationLocalEncoderPosition = 0;
//        rotationVelocity = 0;
//        servoAngle = mapIndexerPercentToAngle(12);
//        hallSwitch = false;
//      }
//    } else {
//      
      actualRotationalPosition = ((float)360 / ((float)rotationEncoderCPR)) * (float)(rotationLocalEncoderPosition);
      actualRotationalPositionLastError = actualRotationalPositionError;
      actualRotationalPositionError = targetRotationalPosition - actualRotationalPosition;
//      if (actualRotationalPosition > 360) {
//        rotationMotorController.write(90);    // good test percentage is ~14% - use map
//      } else if (actualRotationalPosition < 0) {
//        rotationMotorController.write(90);    // good test percentage is ~14% - use map
//      } else {
//        rotationMotorController.write(servoAngle);    // good test percentage is ~14% - use map
//      }
//    }
    rotationMotorController.write(90);

    // flywheel
    if (targetFlywheelVelocity > 0) {
      if (targetFlywheelVelocity != rampCurrentTarget) {
        if (targetFlywheelVelocity > actualFlywheelVelocity) {
          Serial.println("Ramping Flywheel");
          rampCurrentTarget = targetFlywheelVelocity;
          rampComplete = false;
          rampOffset = actualFlywheelVelocity;
          currentStep = 0;
        }

      }

      if (rampComplete == false) {
        float mappedServoValue = map(((rampCurrentTarget * curveVals[currentStep]) + rampOffset), 0, maxFlywheelVelocity, 0, 180);
        flywheelMotorController.write(mappedServoValue); 
        currentStep++;
        if (currentStep > steps) {
          rampComplete = true;
          rampOffset = 0;
          
          Serial.println("Ramping Flywheel Complete");
        }
        delay(delayLength);
      } else {
        // Serial.println(pidOffset);
        float mappedServoValue = map(targetFlywheelVelocity + pidOffset, 0, maxFlywheelVelocity, 0, 180);
        flywheelMotorController.write(mappedServoValue); // min of ~15
        if ((targetFlywheelVelocity + pidOffset < maxFlywheelVelocity) && (targetFlywheelVelocity + pidOffset > 0)) {
          if (actualFlywheelVelocity < (targetFlywheelVelocity - 30)) {
            pidOffset = pidOffset + ((targetFlywheelVelocity - actualFlywheelVelocity) * .1);
          } else if (actualFlywheelVelocity > (targetFlywheelVelocity + 30)) {
            pidOffset = pidOffset - ((actualFlywheelVelocity - targetFlywheelVelocity) * .1);
;
          }
          delay(delayLength);
        }
        
      }

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
  hallSwitch = true;
  rotationMotorController.write(90);
}

void indexerEncoderAISR() {
  indexerEncoderCounts++;
}

void indexerEncoderBISR() {
  indexerEncoderCounts++;
}

void rotationEncoderAISR() {
  rotationEncoderChannelAState = digitalRead(rotationEncoderA);
  if (rotationEncoderChannelAState == HIGH && rotationEncoderChannelBState == LOW) {
    rotationDirection = 0;
  } else if (rotationEncoderChannelAState == HIGH && rotationEncoderChannelBState == HIGH) {
    rotationDirection = 1;
  } else if (rotationEncoderChannelAState == LOW && rotationEncoderChannelBState == LOW) {
    rotationDirection = 1;
  } else if (rotationEncoderChannelAState == LOW && rotationEncoderChannelBState == HIGH) {
    rotationDirection = 0;
  }
  rotationEncoderCounts++;
  if (rotationDirection == 1) {
    rotationLocalEncoderPosition--;
  } else if (rotationDirection == 0) {
    rotationLocalEncoderPosition++;
  }
} 

void rotationEncoderBISR() {
  rotationEncoderChannelBState = digitalRead(rotationEncoderB);
  if (rotationEncoderChannelBState == HIGH && rotationEncoderChannelAState == LOW) {
    rotationDirection = 1;
  } else if (rotationEncoderChannelBState == HIGH && rotationEncoderChannelAState == HIGH) {
    rotationDirection = 0;
  } else if (rotationEncoderChannelBState == LOW && rotationEncoderChannelAState == LOW) {
    rotationDirection = 0;
  } else if (rotationEncoderChannelBState == LOW && rotationEncoderChannelAState == HIGH) {
    rotationDirection = 1;
  }
  rotationEncoderCounts++;
  if (rotationDirection == 1) {
    rotationLocalEncoderPosition--;
  } else if (rotationDirection == 0) {
    rotationLocalEncoderPosition++;
  }
}

void flywheelEncoderAISR() {
  flywheelEncoderCounts++;
}

void flywheelEncoderBISR() {
  flywheelEncoderCounts++;
}

void timerISR() {

  if (i2cConnected == true) {

    // indexer
    if (indexerEncoderCounts > 0) {
//      Serial.println(indexerEncoderCounts);
      actualIndexerVelocity = ((indexerEncoderCounts / deltaTime) * 60) / indexerEncoderCPR;
      indexerEncoderCounts = 0;
    } else {
      actualIndexerVelocity = 0;
    }

    // rotation
    rotationVelocity = (actualRotationalPosition - lastActualRotationalPosition) /  deltaTime;
    lastActualRotationalPosition = actualRotationalPosition;

    // flywheel
    if (flywheelEncoderCounts > 0) {
      actualFlywheelVelocity = ((flywheelEncoderCounts / deltaTime) * 60) / flywheelEncoderCPR;
      flywheelEncoderCounts = 0;
    } else {
      actualFlywheelVelocity = 0;
    }
    
//    Serial.print("Indexer Velocity:");
//    Serial.print(actualIndexerVelocity);
//    Serial.print(" RPM\t");
//    Serial.print("Rotational Position:");
//    Serial.print(actualRotationalPosition);
//    Serial.print(" deg\t");
////    Serial.print("Rotational Error:");
////    Serial.print(actualRotationalPositionError);
////    Serial.print(" deg\t");
////    Serial.print("Rotational Velocity:");
////    Serial.print(rotationVelocity);
////    Serial.print(" deg/sec\t");
//    Serial.print("Flywheel Velocity:");
//    Serial.print(actualFlywheelVelocity);
//    Serial.println(" RPM");

  }
  
  i2cDisconnectedCounter++;
  if (i2cDisconnectedCounter >= 8) {
    i2cConnected = false;
  }
}

int mapIndexerPercentToAngle(float percent) {
  int value = map(percent, -100, 100, 0, 180); // 0, 180
  if(value > 110) {
    value = 110;
  } else if (value < 70) {
    value = 70;
  }
  return value;
}
