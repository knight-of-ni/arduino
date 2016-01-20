
// Define constants as macros to conserve memory
#define PROX_SENSOR 0  // Pin number the proximity sensor is wired to
#define MOSFET 1  // Pin number the mosfet is wired to
#define BUTTON 2 // Pin number the override button is wired to

#define FADESPEED 5  // make this higher to slow down the ramping effect of the output
#define DELAY 10000  // Time delay in milliseconds for output to stay on

// Define global variables
unsigned long previousMillis = 0;
bool stayOn = false;
bool isOn = false;

// Runs one time when the device is powered up
void setup() {
  pinMode(MOSFET, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
}

// As the name implies, this function gets called contunally
void loop() {

  if ( !digitalRead(BUTTON) ) { // The button was pressed
    if ( isOn ) {  // The button was pressed and the output is on
      rampDown();
      stayOn = false;
    } else { // The button was pressed and the output is NOT on
      rampUp();
      stayOn = true;
    }
  } else if ( !stayOn ) { // The button was not pressed, nor was it pressed previously
    if ( !digitalRead(PROX_SENSOR) ) {  // An object was detected
      previousMillis = millis();
      if ( !isOn ) {
        rampUp();
      }
    } else if ( isOn && timeExpired() ) { // An object was NOT detected, the output is on, and the timeout has expired
      rampDown();
//    previousMillis = 0;
    }
  }
}

void rampUp() {    // Ramps up the Mosfet output, rather than immediately turning on
  int r;
  isOn = true;
  
  for (r = 0; r < 256; r++) {
    analogWrite(MOSFET, r);
    delay(FADESPEED);
  } 
}

void rampDown() {    // Ramps down the Mosfet output, rather than immediately turning off
  int r;
  isOn = false;
  
  for (r = 255; r > -1; r--) {
    analogWrite(MOSFET, r);
    delay(FADESPEED);
  }
}

bool timeExpired() {   // Returns true when we've reached the timeout
  return( millis() - previousMillis > DELAY );
}
