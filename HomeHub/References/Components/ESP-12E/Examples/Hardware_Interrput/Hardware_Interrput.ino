const byte interruptPin_1 = 13;
const byte interruptPin_2 = 12;
 
void setup(){
  Serial.begin(115200);
  pinMode(interruptPin_1, INPUT_PULLUP);
  pinMode(interruptPin_2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin_1), handleInterrupt_1, FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptPin_2), handleInterrupt_2, FALLING);
}
 
void handleInterrupt_1() {
  Serial.println("ONE");
}

void handleInterrupt_2() {
  Serial.println("TWO");
}
 
void loop() {
 
}

