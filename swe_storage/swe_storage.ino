//RECIEVER WITH FILE UPLOADS
//Include Libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SD.h>
RF24 contr(4, 6);  // CE, CSN
RF24 postn(3, 5);
const byte address1[6] = "CONTR";
const byte address2[6] = "POSTN";
int chipSelect = 10;
File file;
boolean firstPositionReceived = false;
boolean send;
int numQueries = 0; // the number of lfdm responses received
int connectionCount = 0; // keep track of how many current lfdm connections there are
int connectionLedPin = 6; // the LED pin numbers for the corresponding lfdm connections, 6-9 corresponds to lfdm 1-4 respectively

struct empty_packet
{
  
};
empty_packet packet;

struct position_update
{
  float latitude;
  float longitude;
  float altitude;
  float timestamp;
};
position_update pos;

struct lfdm_detection_signal
{
  float intensity;
  float depth;
};
lfdm_detection_signal currentSignal;

void setup()
{
  while (!Serial);
    Serial.begin(9600);
  contr.begin();
  postn.begin();
  contr.openReadingPipe(0, address1);
  postn.openReadingPipe(0, address2);
  contr.startListening();
  postn.startListening();
  pinMode(chipSelect, OUTPUT);
  if(!SD.begin(chipSelect)){
    Serial.println("Could not initialize SD card.");
  }
  if (SD.exists("file.txt")) {
    Serial.println("File exists.");
    if (SD.remove("file.txt") == true) {
      Serial.println("Successfully removed file.");
    } else {
      Serial.println("Could not removed file.");
    }
  }
  pinMode(A1, OUTPUT); // lfdm connection
  pinMode(A0, OUTPUT); // position
}

void loop() 
{
  if (postn.available()) // read position updates from autonomy
  {
    digitalWrite(A0, HIGH);
    postn.read(&pos, sizeof(pos));
    firstPositionReceived = true;
    Serial.println("Position received");
    Serial.println(pos.latitude);
    Serial.println(pos.longitude);
    Serial.println(pos.altitude);
    Serial.println(pos.timestamp);
    digitalWrite(A0, LOW);

    if (contr.available()) {
      digitalWrite(A1, HIGH);
      contr.read(&currentSignal, sizeof(currentSignal));
      digitalWrite(A1, LOW);
    }
  } 
  add_position_entry(pos, currentSignal);
}

void add_position_entry(position_update entry, lfdm_detection_signal currentSignal)
{
  file = SD.open("file.txt", FILE_WRITE); // open "file.txt" to write data
  if (file) {
    file.print("NEW_ENTRY ");
    file.println(entry.timestamp);
    file.print("LOCATION ");
    file.print(entry.latitude);
    file.print(entry.longitude);
    file.println(entry.altitude);
    file.println(currentSignal.intensity);
    file.println(currentSignal.depth);
    file.close(); // close file
    Serial.print("Wrote number: "); // debug output: show written number in serial monitor
    Serial.println(entry.latitude);
    
  } else {
    Serial.println("Could not open file (writing).");
  }
  file = SD.open("file.txt", FILE_READ); // open "file.txt" to read data
  if (file) {
    Serial.println("- – Reading start – -");
    char number;
    while ((number = file.read()) != -1) { // this while loop reads data stored in "file.txt" and prints it to serial monitor
      Serial.print(number);
    }
    file.close();
    Serial.println("- – Reading end – -");
  } else {
    Serial.println("Could not open file (reading).");
  }
  delay(5000); // wait for 5000ms
}

////Include Libraries
//#include <SPI.h>
//#include <nRF24L01.h>
//#include <RF24.h>
//
////create an RF24 object
//RF24 radio(3, 5);  // CE, CSN
//
////address through which two modules communicate.
//const byte address[6] = "00013";
//
//void setup()
//{
//  while (!Serial);
//    Serial.begin(9600);
//  
//  radio.begin();
//  
//  //set the address
//  radio.openReadingPipe(0, address);
//  
//  //Set module as receiver
//  radio.startListening();
//}
//
//void loop()
//{
//  //Read the data if available in buffer
//  if (radio.available())
//  {
//    char text[32] = {0};
//    radio.read(&text, sizeof(text));
//    Serial.println(text);
//  }
//}
