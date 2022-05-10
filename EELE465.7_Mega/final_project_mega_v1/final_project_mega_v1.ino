#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup()
{
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
  // put your main code here, to run repeatedly:
  lcd.setCursor ( 0, 0 );            // go to the top left corner
  //lcd.print("VT:            0 RPM");
  lcd.print("VT:"); // write this string on the top row //"    Hello,world!    "
  lcd.setCursor ( 15, 0 );
  lcd.print("0");
  lcd.setCursor ( 17, 0 );
  lcd.print("RPM");
  
  lcd.setCursor ( 0, 1 );            // go to the 2nd row
  //lcd.print("VA:            0 RPM");
  lcd.print("VA: "); // pad string with spaces for centering
  lcd.setCursor ( 15, 1 );
  lcd.print("0");
  lcd.setCursor ( 17, 1 );
  lcd.print("RPM");
  
  lcd.setCursor ( 0, 2 );            // go to the third row
  //lcd.print("RT:            0 deg");
  lcd.print("RT: "); // pad with spaces for centering
  lcd.setCursor ( 15, 2 );
  lcd.print("0");
  lcd.setCursor ( 17, 2 );
  lcd.print("deg");
  
  lcd.setCursor ( 0, 3 );            // go to the fourth row
  //lcd.print("VA:            0 deg");
  lcd.print("RA: ");
  lcd.setCursor ( 15, 3 );
  lcd.print("0");
  lcd.setCursor ( 17, 3 );
  lcd.print("deg");

  delay(100);
}
