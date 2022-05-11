#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void receiveEvent(int numberOfBytes);

const int ledPin = LED_BUILTIN;

int integer = 0;
int ledState = LOW;
int updateLCD = false;
int receiveDataCounter = 0;

long actualFlywheelVelocity = 0;
long targetFlywheelVelocity = 0;
float actualRotationalPosition = 0.0;
float targetRotationalPosition = 0.0;

String actualFlywheelString;
String targetFlywheelString;
String actualRotationalString;
String targetRotationalString;

void setup()
{
  Wire1.begin(0x69);
  Wire1.onReceive(receiveEvent); // register receive event
  Serial.begin(115200); 

  pinMode(ledPin, OUTPUT);
  
  lcd.init();  //initialize the lcd
  lcd.backlight();  //open the backlight =
   
  lcd.setCursor ( 0, 0 );            // go to the top left corner
  lcd.print("VT:"); // write this string on the top row //"    Hello,world!    "
  lcd.setCursor ( 15, 0 );
  lcd.print("0");
  lcd.setCursor ( 17, 0 );
  lcd.print("RPM");
  
  lcd.setCursor ( 0, 1 );            // go to the 2nd row
  lcd.print("VA: "); // pad string with spaces for centering
  lcd.setCursor ( 15, 1 );
  lcd.print("0");
  lcd.setCursor ( 17, 1 );
  lcd.print("RPM");
  
  lcd.setCursor ( 0, 2 );            // go to the third row
  lcd.print("RT: "); // pad with spaces for centering
  lcd.setCursor ( 15, 2 );
  lcd.print("0");
  lcd.setCursor ( 17, 2 );
  lcd.print("deg");
  
  lcd.setCursor ( 0, 3 );            // go to the fourth row
  lcd.print("RA: ");
  lcd.setCursor ( 15, 3 );
  lcd.print("0");
  lcd.setCursor ( 17, 3 );
  lcd.print("deg");

}

void loop() {

  if (updateLCD == true) {
    updateLCD = false;
    lcd.setCursor ( 12, 0 );
    if (actualFlywheelVelocity > 999) {
      lcd.print("");
    } else if (actualFlywheelVelocity > 99) {
      lcd.print(" ");
    } else if (actualFlywheelVelocity > 9) {
      lcd.print("  ");
    } else {
      lcd.print("   ");
    }
    lcd.print(actualFlywheelVelocity);

    lcd.setCursor ( 12, 1 );
    if (targetFlywheelVelocity > 999) {
      lcd.print("");
    } else if (targetFlywheelVelocity > 99) {
      lcd.print(" ");
    } else if (targetFlywheelVelocity > 9) {
      lcd.print("  ");
    } else {
      lcd.print("   ");
    }
    lcd.print(targetFlywheelVelocity);

    lcd.setCursor ( 10, 2 );
    if (actualRotationalPosition > 99) {
      lcd.print("");
    } else if (actualRotationalPosition > 9) {
      lcd.print(" ");
    } else {
      lcd.print("  ");
    }
    lcd.print(actualRotationalPosition);

    lcd.setCursor ( 10, 3 );
    if (targetRotationalPosition > 99) {
      lcd.print("");
    } else if (targetRotationalPosition > 9) {
      lcd.print(" ");
    } else {
      lcd.print("  ");
    }
    lcd.print(targetRotationalPosition);
    
  }

//  lcd.setCursor ( 11, 3 );
//  lcd.print(integer);
//
//  integer++;
//
//  if (integer > 999) {
//    integer = 0;
//  }
  
  delay(10);
}

void receiveEvent(int numberOfBytes)
{
  receiveDataCounter = 0;
  actualFlywheelVelocity = 0;
  targetFlywheelVelocity = 0;
  actualRotationalPosition = 0.0;
  targetRotationalPosition = 0.0;
  
  Serial.print("RecieveEvent:\t");
  Serial.print(numberOfBytes);
  Serial.print("Bytes\t");
  while(Wire1.available() > 1) {  // loop through all but the last
    unsigned char c = Wire1.read();        // receive byte
    Serial.print(c);             // print the character
    if (receiveDataCounter < 4) {
      targetFlywheelVelocity += ((int)c) * pow(10, (3 - receiveDataCounter));
    } else if (receiveDataCounter < 8) {
      actualFlywheelVelocity += ((int)c) * pow(10, (7 - receiveDataCounter));
    } else if (receiveDataCounter < 12) {
      targetRotationalPosition += ((int)c) * pow(10, (10 - receiveDataCounter));
    } else if (receiveDataCounter < 16) {
      actualRotationalPosition += ((int)c) * pow(10, (14 - receiveDataCounter)); 
    }
    receiveDataCounter++;
  }
  unsigned char x = Wire1.read();           // receive last byte
  Serial.println(x);             // print the integer
  targetRotationalPosition += ((int)x) * pow(10, (-1));
  receiveDataCounter++;

  updateLCD = true;

  if (ledState == LOW) {
    ledState = HIGH;
  } else {
    ledState = LOW;
  }
  digitalWrite(ledPin, ledState);
}
