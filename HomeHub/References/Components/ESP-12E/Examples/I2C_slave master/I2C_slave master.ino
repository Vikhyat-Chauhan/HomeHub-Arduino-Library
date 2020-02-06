#include <Wire.h>

unsigned long counter;

void setup() {
  Serial.begin(115200); /* begin serial for debug */
  Wire.begin(); /* join i2c bus with SDA=D1 and SCL=D2 of NodeMCU */
}

void loop() {
  Wire.beginTransmission(8);    /* begin with device address 8 */
  Wire.write("<RELAYSTATES>");  /* sends hello string */
  Wire.endTransmission();       /* stop transmitting */
  Wire.requestFrom(8, 4);       /* request & read data of size 13 from slave */
  //delay(200);
  counter = millis();
  while(Wire.available()){
    char c = Wire.read();
    Serial.print(c);
  }
  Serial.println();
  delay(1000);
}
