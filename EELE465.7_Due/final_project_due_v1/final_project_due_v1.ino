#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// method signitures
void receiveEvent(int numberOfBytes);
void timerISR(void);

const int ledPin = LED_BUILTIN;

int integer = 0;
int ledState = LOW;
int updateLCD = false;

// i2c
int receiveDataCounter = 0;
int i2cConnected = false;
int i2cDisconnectedCounter = 0;

long actualFlywheelVelocity = 0;
long targetFlywheelVelocity = 0;
long maxFlywheelVelocity = 5500;
float actualRotationalPosition = 0.0;
float targetRotationalPosition = 0.0;

String actualFlywheelString;
String targetFlywheelString;
String actualRotationalString;
String targetRotationalString;

// timer
int timerDelayMS = 250;
float deltaTime = (timerDelayMS / 1000.0);
float currentTimeMS = 0;
float lastTimeMS = 0;

void setup()
{
  Wire1.begin(0x69);
  Wire1.onReceive(receiveEvent); // register receive event
  Serial.begin(115200); 

  pinMode(ledPin, OUTPUT);
  
  lcd.init();  //initialize the lcd
  lcd.backlight();  //open the backlight =
   
  lcd.setCursor ( 0, 0 );            // go to the top left corner
  lcd.print("VelT:");                //"    Hello,world!    "
  lcd.setCursor ( 15, 0 );
  lcd.print("0");
  lcd.setCursor ( 17, 0 );
  lcd.print("RPM");
  
  lcd.setCursor ( 0, 1 );            // go to the 2nd row
  lcd.print("VelA: ");
  lcd.setCursor ( 15, 1 );
  lcd.print("0");
  lcd.setCursor ( 17, 1 );
  lcd.print("RPM");
  
  lcd.setCursor ( 0, 2 );            // go to the third row
  lcd.print("RotT: ");
  lcd.setCursor ( 15, 2 );
  lcd.print("0");
  lcd.setCursor ( 17, 2 );
  lcd.print("deg");
  
  lcd.setCursor ( 0, 3 );            // go to the fourth row
  lcd.print("RotA: ");
  lcd.setCursor ( 15, 3 );
  lcd.print("0");
  lcd.setCursor ( 17, 3 );
  lcd.print("deg");
}

void loop() {

  if (i2cConnected == true) {
    if (updateLCD == true) {
      //updateLCD = false;
  
      // Write target flywheel velocity to LCD
      if (targetFlywheelVelocity >= maxFlywheelVelocity) {
        lcd.setCursor ( 5, 0 );
        lcd.print("MAX"); 
        targetFlywheelVelocity = maxFlywheelVelocity;
      } else {
        lcd.setCursor ( 5, 0 );
        lcd.print("   ");
      }
      lcd.setCursor ( 12, 0 );
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
  
      // Write target flywheel velocity to LCD
      if (actualFlywheelVelocity >= maxFlywheelVelocity) {
        lcd.setCursor ( 5, 1 );
        lcd.print("MAX"); 
      } else {
        lcd.setCursor ( 5, 1 );
        lcd.print("   ");
      }
      lcd.setCursor ( 12, 1 );
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
  
      // Write target rotational position to LCD
      lcd.setCursor ( 10, 2 );
      if (targetRotationalPosition > 99) {
        lcd.print("");
      } else if (targetRotationalPosition > 9) {
        lcd.print(" ");
      } else {
        lcd.print("  ");
      }
      lcd.print(targetRotationalPosition);
  
      // Write actual rotational position to LCD
      lcd.setCursor ( 10, 3 );
      if (actualRotationalPosition > 99) {
        lcd.print("");
      } else if (actualRotationalPosition > 9) {
        lcd.print(" ");
      } else {
        lcd.print("  ");
      }
      lcd.print(actualRotationalPosition);
  
      
    }
    delay(200);

  } else {
    Serial.println("I2C Bus Not Connected");
    lcd.setCursor ( 5, 0 );
    lcd.print("No"); 
    lcd.setCursor ( 5, 1 );
    lcd.print("I2C"); 
    delay(400);
    lcd.setCursor ( 5, 0 );
    lcd.print("   "); 
    lcd.setCursor ( 5, 1 );
    lcd.print("   "); 
    delay(400);
  }

  currentTimeMS = millis();
  if (currentTimeMS - lastTimeMS > 2000) {
    i2cConnected = false;
  } else {
    i2cConnected = true;
  }  
}

void receiveEvent(int numberOfBytes)
{
  
  Serial.print("RecieveEvent:\t");
  Serial.print(numberOfBytes);
  Serial.print("Bytes\t");

  if (numberOfBytes != 16) {
    while (Wire1.available() > 1) { // loop through all but the last
      unsigned char c = Wire1.read(); 
    }
    Serial.println("Refused Bytes");
    return;
  }
  receiveDataCounter = 0;
  actualFlywheelVelocity = 0;
  targetFlywheelVelocity = 0;
  actualRotationalPosition = 0.0;
  targetRotationalPosition = 0.0;
  
  while(Wire1.available() > 1) {  // loop through all but the last
    unsigned char c = Wire1.read();        // receive byte
    Serial.print(c);             // print the character
    if (receiveDataCounter < 4) {
      actualFlywheelVelocity += ((int)c) * pow(10, (3 - receiveDataCounter));
    } else if (receiveDataCounter < 8) {
      targetFlywheelVelocity += ((int)c) * pow(10, (7 - receiveDataCounter));
    } else if (receiveDataCounter < 12) {
      actualRotationalPosition += ((int)c) * pow(10, (10 - receiveDataCounter));
    } else if (receiveDataCounter < 16) {
      targetRotationalPosition += ((int)c) * pow(10, (14 - receiveDataCounter)); 
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

  lastTimeMS = millis();
  i2cConnected = true;
}

void timerISR() {
  i2cDisconnectedCounter++;
  if (i2cDisconnectedCounter >= 8) {
    i2cConnected = false;
  }
}
