#include <PWMServo.h>

PWMServo pwmServo;

int indexer = 14;
int pos = 0;

void setup() {
  // put your setup code here, to run once:
  pwmServo.attach(indexer);
  Serial.begin(115200);
  Serial.println("Beginning");
}

void loop() {
  // put your main code here, to run repeatedly:
  for(pos = 0; pos < 180; pos += 1) { // goes from 0 degrees to 180 degrees, 1 degree steps
    pwmServo.write(pos);              // tell servo to go to position in variable 'pos'
    Serial.println(pos);
    delay(50);                       // waits 15ms for the servo to reach the position
  }
  for(pos = 180; pos>=1; pos-=1) {   // goes from 180 degrees to 0 degrees
    pwmServo.write(pos);              // tell servo to go to position in variable 'pos'
    Serial.println(pos);
    delay(50);                       // waits 15ms for the servo to reach the position
  }
}
