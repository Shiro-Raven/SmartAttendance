#include <LCD5110_Basic.h>
#include <Time.h>
#include <TimeLib.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>

// SD defines
#define SDchipSelect 3

// Smoke defines
#define smokePin 0
#define buzzerPin 2
#define fireThreshold 140

// RFID defines
#define RFID_reset 9
#define RFID_select 10

// Logic defines
#define MAX_ID_LENGTH 40
#define TEACHER_ID "1B 56 81 63"
#define max_number_of_students 40

time_t start_timestamps[40];
unsigned int total_time_attended[40];
time_t session_start_time;
String student_IDs[40];
byte number_of_students_inside;

void init_session() {
  session_start_time = 0;
  number_of_students_inside = 0;
  for (byte i = 0; i < max_number_of_students; i++) {
    start_timestamps[i] = 0;
    total_time_attended[i] = 0;
    student_IDs[i] = "";
  }
}
void manage_attendence(String ID) {
  unsigned int current_time = (unsigned int) now();
  
  // Serial.println(current_time);
  
  if (ID == TEACHER_ID) { // Teacher activated the door
    if (session_start_time == 0) { // Teacher is entering
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
      get_attendence(current_time);
    }
  }
  else { // Student activated the door
    byte student_index = -1; // The student's index in the arrays
    for (byte i = 0; i < max_number_of_students; i++) { // Find the student index
      if (student_IDs[i] == ID) { // Student found among those in the session
        student_index = i;
        break;
      }
    }
    if (student_index == -1) { // Search for an empty slot to add a new student
      for (byte i = 0; i < max_number_of_students; i++) {
        if (student_IDs[i] == "") { // Empty slot was found
          student_index = i;
          student_IDs[i] = ID;
          break;
        }
      }
    }
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
  }
}
void get_attendence(unsigned int session_end_time) {
  unsigned int total_session_time = session_end_time - session_start_time;
  float percentage = 0.0;
  Serial.println(F("Attended: "));
  
  for (byte i = 0; i < max_number_of_students; i++) { // Calculate attendence
    if (student_IDs[i] != "") {
      percentage = ((float)total_time_attended[i]) / ((float)total_session_time); // Calculate attendance percentage
      if (percentage >= 0.75) { // Check if the student attended at least 75% of the session
        Serial.println(student_IDs[i]);
      }
    }
  }
  // print outputs
  Serial.print(F("Total Session Time: "));
  Serial.println(String(total_session_time));
  for (byte i = 0; i < max_number_of_students; i++) {
    if (student_IDs[i] != "") {
      Serial.print(F("Student: "));
      Serial.print(student_IDs[i]);
      Serial.print(F(" Total Time Spent: "));
      Serial.println(String(total_time_attended[i]));
    }
  }
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

LCD5110 myGLCD(5, 4, 6, 8, 7);
extern uint8_t SmallFont[];

void setup() {
  init_session();
  pinMode(buzzerPin, OUTPUT);
  Serial.begin(9600);
  while (!Serial);
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  myGLCD.InitLCD(60);
  myGLCD.setFont(SmallFont);
}

void loop() {
  myGLCD.clrScr();
  myGLCD.print("Count: " + String(number_of_students_inside), CENTER, 0);
  
  if (analogRead(smokePin) >= fireThreshold) {
    tone(buzzerPin, 1000);
  } else {
    noTone(buzzerPin);

    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
      return;
    }

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      return;
    }

    Serial.print(F("UID tag :"));
    String content = "";
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    Serial.println();
    content.trim();
    content.toUpperCase();

    manage_attendence(content);
    delay(1000);
  }
}

