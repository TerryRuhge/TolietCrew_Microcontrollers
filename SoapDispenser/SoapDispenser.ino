const byte interruptPin = 2; //Pin Connected to Soap Dispensor motor
volatile byte state = LOW;
void setup() {
  // Set up Printing
  Serial.begin(115200);
  // Set up interrupt Pin
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), dispensed, FALLING);
}

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println(state);
}

void dispensed() {
  state = !state;
  Serial.println("Detected!");
}
