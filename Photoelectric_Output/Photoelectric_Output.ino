int inPin = 37;

void setup() {
  // put your setup code here, to run once:
  pinMode(inPin, INPUT);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(digitalRead(inPin),DEC);
  delay(500);
}
