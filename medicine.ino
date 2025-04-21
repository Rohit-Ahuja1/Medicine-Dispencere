#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

// WiFi credentials
const char* ssid = "Sagar";
const char* password = "12345678";

// Google Apps Script Web App (original deployment URL)
const char* host = "script.google.com";
const int httpsPort = 443;
const String url = "/macros/s/AKfycbxzOpC9NpWfGVgtJsfup1UCJvR5Yl7mS6yBcwzxY-0ioBJSg_Sp-prE4MUoPcj3jxEejQ/exec";

// RFID pins
#define SS_PIN  D4
#define RST_PIN D0

// Servo pins
#define SERVO1_PIN D3
#define SERVO2_PIN D8

MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo1;
Servo servo2;
    
byte card1[4] = {0x71, 0x10, 0xF6, 0x7B }; // Rohit
byte card2[4] = {0x33, 0x87, 0xCD, 0x28}; // Kayam

int angles[] = {0, 30, 60, 90, 120, 150, 180};
const int angleCount = sizeof(angles) / sizeof(angles[0]);

int index1 = 0;
int index2 = 0;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  servo1.attach(SERVO1_PIN, 500, 2500);
  servo2.attach(SERVO2_PIN, 500, 2500);

  servo1.write(0);
  servo2.write(0);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

bool compareUID(byte *uid1, byte *uid2) {
  for (byte i = 0; i < 4; i++) {
    if (uid1[i] != uid2[i]) return false;
  }
  return true;
}

void sendToGoogleSheet(String uid, String name, String medicine) {
  WiFiClientSecure client;
  client.setInsecure(); // Ignore SSL certificate

  if (!client.connect(host, httpsPort)) {
    Serial.println("❌ Connection to Google Sheets failed.");
    return;
  }

  String postData = "{\"uid\":\"" + uid + "\",\"name\":\"" + name + "\",\"medicine\":\"" + medicine + "\"}";
  Serial.print("📤 Sending data: ");
  Serial.println(postData);

  String request = String("POST ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Content-Type: application/json\r\n" +
                   "Content-Length: " + postData.length() + "\r\n\r\n" +
                   postData;

  client.print(request);

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
    Serial.println(line);
  }

  String response = client.readString();
  Serial.println(" Server Response: " + response);
}

void rotateNext(Servo &servo, int &currentIndex, const char* label) {
  currentIndex = (currentIndex + 1) % angleCount;
  int angle = angles[currentIndex];
  servo.write(angle);
  Serial.print(" ");
  Serial.print(label);
  Serial.print(" moved to angle: ");
  Serial.println(angle);
}

String uidToString(byte *uid, byte length) {
  String uidStr = "";
  for (byte i = 0; i < length; i++) {
    uidStr += String(uid[i], HEX);
  }
  return uidStr;
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uidStr = uidToString(rfid.uid.uidByte, rfid.uid.size);
  Serial.print("UID Detected: ");
  Serial.println(uidStr);

  if (compareUID(rfid.uid.uidByte, card1)) {
    rotateNext(servo1, index1, "Servo 1");
    sendToGoogleSheet("0x7110F67B", "Rohit", "Paracetamol");
  } else if (compareUID(rfid.uid.uidByte, card2)) {
    rotateNext(servo2, index2, "Servo 2");
    sendToGoogleSheet("0x3387CD28", "Kayam", "Cetirizine");
  } else {
    Serial.println("Unknown card. No action.");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(1000);
}
