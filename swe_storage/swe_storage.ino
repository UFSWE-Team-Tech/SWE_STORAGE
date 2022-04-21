//RECIEVER WITH FILE UPLOADS
//Include Libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SD.h>
RF24 contr(2, 3);  // CE, CSN
RF24 postn(4, 5);
const byte address1[6] = "CONTR";
const byte address2[6] = "POSTN";
int chipSelect = 10;
File file;
boolean firstPositionReceived = false;
boolean send;
int numQueries = 0; // the number of lfdm responses received
int connectionCount = 0; // keep track of how many current lfdm connections there are
int connectionLedPin = 6; // the LED pin numbers for the corresponding lfdm connections, 6-9 corresponds to lfdm 1-4 respectively

struct position_update
{
  float latitude;
  float longitude;
  float altitude;
  float timestamp;
};
position_update pos;

struct lfdm_identifier
{
  char id[4]; // 3-letter string (unique ID)
  byte address[6]; // 5-byte nRF24L01+ address
};
lfdm_identifier connections[4]; // contains the four potential lfdm connections
lfdm_identifier testConnection;

struct lfdm_detection_signal
{
  float intensity;
  float depth;
};
lfdm_detection_signal currentSignal;

struct lfdm_query
{
  lfdm_identifier identifier;
  lfdm_detection_signal signal;
};
lfdm_query query; // holds the response from the lfdm temporarily before being added into a new position_entry

struct position_entry
{
  position_update pos;
  int num_queries;
  lfdm_query queries[];
};
position_entry newPositionEntry;

void setup()
{
  while (!Serial);
    Serial.begin(9600);
  contr.begin();
  postn.begin();
  contr.enableAckPayload(); // allows contr module to accept acknowledge packets from lfdm
  contr.setRetries(0, 2); // (delay, count) retries sending data to lfdm twice without delay
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
  // lfdm connections:
  pinMode(6, OUTPUT); // 1
  pinMode(7, OUTPUT); // 2
  pinMode(8, OUTPUT); // 3
  pinMode(9, OUTPUT); // 4
  pinMode(A0, OUTPUT);
}

void loop() 
{
  if (postn.available()) // read position updates from autonomy
  {
    digitalWrite(A0, HIGH);
    char test[32] = {0};
    postn.read(&pos, sizeof(pos));
    firstPositionReceived = true;
    Serial.println("Position received");
    Serial.println(pos.latitude);
    Serial.println(pos.longitude);
    Serial.println(pos.altitude);
    Serial.println(pos.timestamp);
    
    digitalWrite(A0, LOW);
  } 
  
  
  if (true) // !firstPositionReceived while first position has not been received AND max connections (4) not yet reached, listen for lfdm connections
  {
    if (contr.available() && connectionCount < 4)
    {
      boolean connectionExists[4] = {(connectionCount>=1), (connectionCount>=2), (connectionCount>=3), (connectionCount>=4)};
      boolean ignore = false;
      contr.read(&testConnection, sizeof(testConnection)); // stores lfdm info in the lfdm_identifier struct 
      for (int i = 0; i < connectionCount; i++) {
        for (int j = 0; j < 3; j++) {
          if (testConnection.id[j] != connections[i].id[j]) {
            connectionExists[i] = false;
          }
        }
      }
      for (int i = 0; i < sizeof(connectionExists); i++) {
        if (connectionExists[i] == true) {
          ignore = true;
        }
      }
      if (!ignore) {
        for (int i = 0; i < sizeof(testConnection.id); i++) {
          connections[connectionCount].id[i] = testConnection.id[i];
        }
        for (int i = 0; i < sizeof(testConnection.address); i++) {
          connections[connectionCount].address[i] = testConnection.address[i];
        }
        connectionCount++;
        digitalWrite(connectionLedPin, HIGH); // turn on LED for each corresponding lfdm connection
        connectionLedPin++;
        Serial.print("Connection received: ");
        Serial.println(connectionCount);
        Serial.println(connections[connectionCount].id);
      }
    }
  }
  else
  {
    for (lfdm_identifier lfdm : connections) { // write an empty packet to each lfdm address
      contr.openWritingPipe(lfdm.address);
      contr.stopListening();
      send = contr.write(&pos, sizeof(pos)); // FIXME Figure out how to write an empty packet to lfdm
      delay(1000);
      if (send) { // if a packet is successfully sent, read the acknowledge packet that lfdm sends back, which contains the current detection signal
          if (contr.isAckPayloadAvailable()) {
              contr.read(&currentSignal, sizeof(currentSignal));
              numQueries++;
              query = {lfdm, currentSignal}; // store the response from each lfdm
              newPositionEntry.queries[numQueries - 1] = query; // adds the lfdm response to a new position entry
          }
          else {
              Serial.println("Acknowledge but no data");
          }
      }
      else {
          Serial.println("Tx failed");
      }
    }
    newPositionEntry.pos = pos;
    newPositionEntry.num_queries = numQueries;
    add_position_entry(newPositionEntry);
  }
}

void add_position_entry(position_entry entry)
{
    file = SD.open("file.txt", FILE_WRITE); // open "file.txt" to write data
  if (file) {
    file.print("NEW_ENTRY ");
    file.println(entry.pos.timestamp);
    file.print("LOCATION ");
    file.print(entry.pos.latitude);
    file.print(entry.pos.longitude);
    file.println(entry.pos.altitude);
    for (int i = 0; i < entry.num_queries; i++) {
      file.print("LFDM_SIGNAL ");
      file.print(entry.queries[i].identifier.id);
      file.print(entry.queries[i].signal.intensity);
    }
    file.close(); // close file
    Serial.print("Wrote number: "); // debug output: show written number in serial monitor
    Serial.println(entry.pos.latitude);
    
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
//RF24 radio(2, 3);  // CE, CSN
//
////address through which two modules communicate.
//const byte address[6] = "00006";
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
