#include <LCD5110_Basic.h>
#include <Time.h>
#include <TimeLib.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>

// Door defines
#define LEDpin 15
#define OpenDoor digitalWrite(LEDpin, HIGH)
#define CloseDoor digitalWrite(LEDpin, LOW)

// Timer default value
#define defaultTime 1543622400 // Dec 1 2018 00:00:00 GMT

// SD defines
#define SDchipSelect 3

// Smoke defines
#define smokePin 0
#define buzzerPin 2

// RFID defines
#define RFID_reset 9
#define RFID_select 10

// Logic defines
#define MAX_ID_LENGTH 40
#define TEACHER_ID "BEST TA EVER"
#define max_number_of_students 40

unsigned int start_timestamps[40];
unsigned int total_time_attended[40];
time_t session_start_time;
String student_IDs[40];
byte number_of_students_inside;

bool fire = false;
byte fireThreshold = 140;

String nameOnCard;

File logFile;

void init_session() {
  session_start_time = 0;
  number_of_students_inside = 0;
  for (byte i = 0; i < max_number_of_students; i++) {
    start_timestamps[i] = 0;
    total_time_attended[i] = 0;
    student_IDs[i] = "";
  }
}
void manage_attendence() {
  time_t current_time = now();

  //Serial.println(nameOnCard);
  //Serial.println(current_time);
  //Serial.println(number_of_students_inside, BIN);
  
  if (nameOnCard == TEACHER_ID) { // Teacher activated the door
    if (session_start_time == 0) { // Teacher is entering
      
      //Serial.println(F("---------------------"));
      //Serial.print(F("Session started at: "));
      //digitalClockDisplaySerial(current_time);
      
      logFile = SD.open(F("embdproj/attLog.txt"), FILE_WRITE);

      if (!SD.exists(F("embdproj/attLog.txt"))) {
          Serial.println(F("Log file oops"));
      }
      logFile.println(F("---------------------"));
      logFile.print(F("Session started at: "));
      digitalClockDisplayFile(current_time);
      
      session_start_time = current_time;
      for (byte i = 0; i < max_number_of_students; i++) { // Set start time for all students inside the room
        if (start_timestamps[i] != 0) {
          start_timestamps[i] = current_time;
        }
      }
    }
    else { // Teacher is leaving the room
      for (byte i = 0; i < max_number_of_students; i++) { // Add the time from the last entrance into the room till the end of the session
        if (start_timestamps[i] != 0) {
          total_time_attended[i] += (current_time - start_timestamps[i]);
        }
      }

      //Serial.print(F("Session ended at: "));
      //digitalClockDisplaySerial(current_time);

      logFile.print(F("Session ended at: "));
      digitalClockDisplayFile(current_time);

      get_attendence(current_time);
    }
  }
  else { // Student activated the door
    int student_index = -1; // The student's index in the arrays
    
    for (byte i = 0; i < max_number_of_students; i++) { // Find the student index
      if (student_IDs[i] == nameOnCard) { // Student found among those in the session
        student_index = i;
        break;
      }
    }
    
    if (student_index == -1) { // Search for an empty slot to add a new student
      for (byte i = 0; i < max_number_of_students; i++) {
        if (student_IDs[i] == "") { // Empty slot was found
          student_index = i;
          student_IDs[i] = nameOnCard;
          break;
        }
      }
    }

    //Serial.print("Start Timestamp: ");
    //Serial.println(start_timestamps[student_index], DEC);
    
    if (session_start_time == 0) { // Session did not start yet
      if (start_timestamps[student_index] == 0) { // Student is entering the classroom
        start_timestamps[student_index] = current_time;
        number_of_students_inside++;
      }
      else { // Student is leaving the classroom before the session started
        start_timestamps[student_index] = 0;
        student_IDs[student_index] = "";
        total_time_attended[student_index] = 0;
        number_of_students_inside--;
      }
    }
    else { // Session had already started
      if (start_timestamps[student_index] == 0) { // Student is entering the classroom
        start_timestamps[student_index] = current_time;
        number_of_students_inside++;
      }
      else { // Student is leaving the classroom before the session ends
        total_time_attended[student_index] += (current_time - start_timestamps[student_index]);
        start_timestamps[student_index] = 0;
        number_of_students_inside--;
      }
    }

    //Serial.println("Students inside " + String(number_of_students_inside));
  }
}
void get_attendence(time_t session_end_time) {
  time_t total_session_time = session_end_time - session_start_time;
  double percentage = 0.0;
  
  //Serial.println(F("Attendees: "));
  logFile.println(F("Attendees: "));
  
  for (byte i = 0; i < max_number_of_students; i++) { // Calculate attendence
    if (student_IDs[i] != "") {
      percentage = ((double)total_time_attended[i]) / ((double)total_session_time); // Calculate attendance percentage
      if (percentage >= 0.75) { // Check if the student attended at least 75% of the session
        //Serial.print(F("Student: "));
        //Serial.print(student_IDs[i]);
        //Serial.print(F(" - "));
        //Serial.print(F(" Total Time Spent: "));
        //Serial.println(String(percentage * 100) + "%");

        logFile.print(F("Student: "));
        logFile.print(student_IDs[i]);
        logFile.print(F(" - "));
        logFile.print(F(" Total Time Spent: "));
        logFile.println(String(percentage * 100.0) + "%");
      }
    }
  }
  logFile.close();
  
  // reset variables after session ends
  session_start_time = 0;
  for (byte i = 0; i < max_number_of_students; i++) {
    if (start_timestamps[i] == 0) {
      student_IDs[i] = "";
    }
    total_time_attended[i] = 0;
  }
}

MFRC522 mfrc522(RFID_select, RFID_reset);
MFRC522::MIFARE_Key key;

LCD5110 myGLCD(5, 4, 6, 8, 7);
extern uint8_t SmallFont[];

void setup() {
  setTime(defaultTime);
  
  pinMode(buzzerPin, OUTPUT);

  pinMode(LEDpin, OUTPUT);

  pinMode(SDchipSelect, OUTPUT);
  digitalWrite(SDchipSelect, HIGH);

  Serial.begin(9600);
  while (!Serial);
  
  SPI.begin();

  mfrc522.PCD_Init();
  
  if(!SD.begin(SDchipSelect)){
    Serial.println(F("SD Oops"));
  }

  for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
  }
  
  myGLCD.InitLCD(62);
  myGLCD.setFont(SmallFont);

  fireThreshold = analogRead(smokePin) + 15;
  
  init_session();
}

void loop() {
  if (analogRead(smokePin) >= fireThreshold | fire) {
    myGLCD.clrScr();
    myGLCD.print(F("FIRE!!!"), CENTER, 0);
    fire = true;
    OpenDoor;
    tone(buzzerPin, 1000);
  } else {
    myGLCD.clrScr();
    myGLCD.print("Count: " + String(number_of_students_inside), CENTER, 0);

    if(session_start_time != 0){
      myGLCD.print(F("Tutorial in"), CENTER, 16);
      myGLCD.print(F("session"), CENTER, 24);
    }

    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
      return;
    }

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      return;
    }
    
    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);

    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, 4, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(4, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    nameOnCard = readCard(buffer);
    
    nameOnCard.trim();

    tone(buzzerPin, 2000);
    delay(100);
    noTone(buzzerPin);
    
    manage_attendence();

    OpenDoor;
    delay(1500);
    CloseDoor;
    
    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
  }
}

String readCard(byte *buffer) {
    String result = buffer;
    return result;
}

void digitalClockDisplaySerial(time_t t) {
  // digital clock display of the time
  Serial.print(hour(t));
  printDigitsSerial(minute(t));
  printDigitsSerial(second(t));
  Serial.print(" ");
  Serial.print(day(t));
  Serial.print("/");
  Serial.print(month(t));
  Serial.print("/");
  Serial.print(year(t));
  Serial.println();
}

void digitalClockDisplayFile(time_t t) {
  // digital clock display of the time
  logFile.print(hour(t));
  printDigitsFile(minute(t));
  printDigitsFile(second(t));
  logFile.print(" ");
  logFile.print(day(t));
  logFile.print("/");
  logFile.print(month(t));
  logFile.print("/");
  logFile.print(year(t));
  logFile.println();
}

void printDigitsSerial(int digits) {
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void printDigitsFile(int digits) {
  logFile.print(":");
  if (digits < 10)
    logFile.print('0');
  logFile.print(digits);
}

