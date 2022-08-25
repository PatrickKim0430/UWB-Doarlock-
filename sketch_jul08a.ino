#include <ArduinoJson.h>
#include <Servo.h>
#include "Adafruit_Keypad.h"
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
// define your specific keypad here via PID
#define KEYPAD_PID3844
// define your pins here
// can ignore ones that don't apply
#define R1    2
#define R2    3
#define R3    4
#define R4    5
#define C1    9
#define C2    10
#define C3    11
#define C4    12
#define SS_PIN 53
#define RST_PIN 8

#define FACTOR 0.9
#define FILT(filter, val) filter=filter*FACTOR+val*(1-FACTOR)
const int xPin = A0;                  // x-axis of the accelerometer

const int yPin = A1;                  // y-axis

const int zPin = A2;                  // z-axis (only on 3-axis models)
int minVal = 265;

int maxVal = 402;
double x, y, z;
double filterx=0, filtery=0,filterz=0;
int distFilter = 100;
// leave this import after the above configuration
#include "keypad_config.h"
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

MFRC522::MIFARE_Key key;
//initialize an instance of class NewKeypad
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);
Servo servo;
long long unsigned int time, sub;
String SSID = "jamcoding_2nd_TR";
String PASSWORD = "43214321";
bool isUnlock=false;
String count = "";
String r = "";
const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
DynamicJsonDocument doc(capacity);
String readFromWifi(int reading = 1000, char endWith = '\n') {
  String result = "";
  unsigned long long int time = millis();
  while ((millis() - time) < reading) {
    if (Serial1.available()) {
      result += Serial1.readStringUntil(endWith) + "\n";
      time = millis();
    }
  }
  return result;
}
String getHex(byte *buffer, byte bufferSize) {
  String result = "";
  for (byte i = 0; i < bufferSize; i++) {
    result += String(buffer[i], HEX);
  }
  return result;
}
String sendCommand(String cmd, String params[], int size, int reading = 1000) {
  String sender = "AT+" + cmd;
  for (int i = 0; i < size; ++i) {
    if (i == 0)sender += "=";
    else sender += ",";
    sender += params[i];
  }
  Serial.println("executing: " + sender);
  Serial1.println(sender + "\r\n");
  String result = readFromWifi(reading);
  Serial.println(result);
  return result;
}

void connectWifi() {
  String cmd[] = {"\"" + SSID + "\"", "\"" + PASSWORD + "\""};
  String result = sendCommand("CWJAP", cmd, sizeof(cmd) / sizeof(String), 2000);
  if (result.indexOf("OK") != -1) {
    Serial.println("Wifi connected");

  } else {
    Serial.println("Cannot connect to Wifi");

  }
}
void getAngle(){
     int xRead = analogRead(xPin);

  int yRead = analogRead(yPin);

  int zRead = analogRead(zPin);



  //convert read values to degrees -90 to 90 - Needed for atan2

  int xAng = map(xRead, minVal, maxVal, -90, 90);

  int yAng = map(yRead, minVal, maxVal, -90, 90);

  int zAng = map(zRead, minVal, maxVal, -90, 90);



  //Caculate 360deg values like so: atan2(-yAng, -zAng)

  //atan2 outputs the value of -π to π (radians)

  //We are then converting the radians to degrees

  x = RAD_TO_DEG * (atan2(-yAng, -zAng) + PI);

  y = RAD_TO_DEG * (atan2(-xAng, -zAng) + PI);

  z = RAD_TO_DEG * (atan2(-yAng, -xAng) + PI);
  FILT(filterx, x);
  FILT(filtery, y);
  FILT(filterz, z);
}
void httpGet(String server, String uri = "/", int port = 80) {
  String connect_server_cmd[] = {"\"TCP\"", "\"" + server + "\"", String(port)};
  sendCommand("CIPSTART", connect_server_cmd, sizeof(connect_server_cmd) / sizeof(String));
  String httpCmd = "GET " + uri;
  String cmd = "AT+CIPSEND=" + String(httpCmd.length() + 4);
  Serial1.println(cmd + "\r\n");
  Serial.println(readFromWifi(2000));
  Serial1.println(httpCmd + "\r\n\n\n");
  String result = readFromWifi(2000);
  int first = result.indexOf("{");
  int end = result.indexOf("}");
  result = result.substring(first, end + 1);
  DeserializationError error = deserializeJson(doc, result);
  Serial.println(doc["data"].as<String>());
} 
void message(String text, bool isClear=false) {
  if(isClear) lcd.clear();
   lcd.setCursor(0, 0);
  lcd.print(text);
  }
void inputPassword(String i) {
  message("Input Password:");
  lcd.setCursor(0, 1);
  lcd.print(i);
}
void setup() {
  Serial2.begin(9600);
  Serial1.begin(115200);
  Serial.begin(9600);
   lcd.init();   
   lcd.backlight();
   message("Input Password:");
  customKeypad.begin();
  servo.attach(24);
  servo.write(10);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  connectWifi();
}
void loop() {
  getAngle();
  customKeypad.tick();
  while (customKeypad.available()) {
    keypadEvent e = customKeypad.read();
    char c = (char)e.bit.KEY;
    Serial.print(c);
    if (e.bit.EVENT == KEY_JUST_PRESSED) Serial.println(" pressed");
    else if (e.bit.EVENT == KEY_JUST_RELEASED) {
      Serial.println(" released");
      if (c == '*') {
        if(count == "8745"){
          isUnlock=true;
        }else {
          message("Invalid Password", true);
          delay(2000);
          message("Input Password:", true);
       }
        count = "";
      }
      else {
        count += c;
        inputPassword(count);
      }
    }
  }
  if(r == "40618b7c"){
      isUnlock=true;
    }
  if(Serial2.available()){
    String val = Serial2.readStringUntil('\n');
    int dist = val.substring(0, val.indexOf(",")).toInt();
    double angle = val.substring(val.indexOf(",")+1, val.length()).toDouble();

    Serial.println(angle);
    FILT(distFilter, dist);
    Serial.println(distFilter);
    if(distFilter<=150) if(sub > 10000) isUnlock=true;
    } else sub = millis();
    if(isUnlock) {
    time = millis();
    message("unlocked", true);
    isUnlock = false;
    r = "";
    servo.write(100);
  } 
  if(millis()-time > 5000) {
    message("Input Password:");
    servo.write(10);
  }
  //httpGet("kimbros.kro.kr", "/?uid=test"+String(count++), 8000);
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

    // Store NUID into nuidPICC array

    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    r = getHex(rfid.uid.uidByte, rfid.uid.size);
    if(r == "40618b7c"){
      isUnlock=true;
    } else if(r!=""){
      message("Invalid RFID", true);
      delay(1000);
      r = "";
      message("Input Password:", true);
      }
    Serial.println();

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
  
  delay(10);

}
