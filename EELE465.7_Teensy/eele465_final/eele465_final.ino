#include <PWMServo.h>

int indexer = 14;
int hallSwitch = 9;

int percent = 0;
int stopFlag = 0;

PWMServo pwmServo;

void setup() {
  pwmServo.attach(indexer);
  pinMode(hallSwitch, INPUT);
  Serial.begin(115200);
  Serial.println("Beginning");
}

void loop() {
  if (digitalRead(hallSwitch) == HIGH && stopFlag == 0) {
    percent = 14;
  } else {
    percent = 0;
    stopFlag = 1;
  }
  driveIndexer(percent);
  delay(100);
}

void driveIndexer(float percent) {
  int pwmMap = map(percent, -100, 100, 0, 180);
  pwmServo.write(pwmMap);
  Serial.println(pwmMap);
}
