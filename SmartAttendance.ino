// Use defines for easier debugging
#define smokePin 0
#define buzzerPin 7

int fireThreshold = 150;

void setup() {
  // NOTE: USE THIS WHEN YOU WANT TO DEBUG, 
  // THEN USE Serial.println() to print and use the serial monitor from Tools -> Serial Monitor 
  // Serial.begin(9600); 

  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  if(analogRead(smokePin) >= fireThreshold){
    tone(buzzerPin, 1000);
  } else {
    noTone(buzzerPin);
  }
}
