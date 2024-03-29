#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include "ClickButton.h"
#include <PubSubClient.h>

WiFiServer server(80);
ClickButton btn1(btn, LOW, CLICKBTN_PULLUP);
PubSubClient client(espClient);
char* ssid = "";
char* password = "";

const int EEPROM_SIZE = 512;
int wifi_ssid_address = 0;
int wifi_password_address = 32;

const char* ssidAp = "ESP8266 Hotspot";
const char* passwordAp = "password";


String selectedSSID;
String selectedPassword;
String storedSSID = "";
String storedPassword = "";

int btn = 0;
int btnFunc = 0;

const int ledPin = LED_BUILTIN;
int ledState = LOW;
unsigned long previousMillis = 0;
long interval;
int eepromStat;

const char* mqtt_server = "192.168.88.200";

void setup() {
  pinMode(btn, INPUT);
  pinMode(ledPin, OUTPUT);
  btn1.debounceTime   = 20;   // Debounce timer in ms
  btn1.multiclickTime = 250;
  btn1.longClickTime = 2000;

  Serial.begin(115200);
  EEPROM.begin(64);
  delay(100);
  Serial.println();
  
  // Check if stored WiFi credentials are present
  for (int i = 0; i < 32; ++i) {
    storedSSID += char(EEPROM.read(wifi_ssid_address + i));
  }
  for (int i = 0; i < 32; ++i) {
    storedPassword += char(EEPROM.read(wifi_password_address + i));
  }
  Serial.print("Boot Stored SSID = "); Serial.println(storedSSID);
  Serial.print("Boot Stored Pass = "); Serial.println(storedPassword);

  // If stored WiFi credentials are present, attempt to connect to the network
  if (storedSSID != "" && storedPassword != "") {
    eepromStat = 0;
    WiFi.begin(storedSSID.c_str(), storedPassword.c_str());
    int i = 0;
    while (WiFi.status() != WL_CONNECTED && i < 10) {
      delay(1000);
      Serial.println("Connecting to stored WiFi network...");
      i++;
    }
  }
  // If the ESP8266 is not connected to a network, start an Access Point
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssidAp, passwordAp);

    server.begin();
    Serial.println("Server started");
    Serial.print("HotSpot IP address: ");
    Serial.println(WiFi.softAPIP());
    eepromStat = 1;
  }
  Serial.print("EEPROM STAT = ");
  Serial.println(eepromStat);
}

void loop() {
  ledStatus();
  btn1.Update();
  if (btn1.clicks != 0) btnFunc = btn1.clicks;
  if (btnFunc == -1) {
    btnFunc = 0;
    Serial.println("Clearing EEPROM");
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
      Serial.println(i);
    }
    EEPROM.commit();
  }

  // If not connected to a network, listen for incoming clients
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  Serial.println("New client");
  while (client.connected()) {
    if (client.available()) {
      String req = client.readStringUntil('\r');
      Serial.println(req);

      if (req.indexOf("/select") != -1) {
        int startIndex = req.indexOf("ssid=") + 5;
        int endIndex = req.indexOf("&", startIndex);
        int passStartIndex = req.indexOf("password=") + 9;
        int passEndIndex = req.indexOf("HTTP", passStartIndex);
        selectedSSID = req.substring(startIndex, endIndex);
        selectedPassword = req.substring(passStartIndex, passEndIndex - 1);
        int ssidLen = selectedSSID.length() + 1;
        int passLen = selectedPassword.length() + 1;
        char charSSID[ssidLen];
        char charPass[passLen];

        selectedSSID.toCharArray(charSSID, ssidLen);
        selectedPassword.toCharArray(charPass, passLen);

        Serial.print("REQ RAW = ");
        Serial.println(req);
        Serial.print("Selected SSID: '");
        Serial.print(selectedSSID);
        Serial.println("'");
        Serial.print("Selected password: '");
        Serial.print(selectedPassword);
        Serial.println("'");
        Serial.println("Clearing EEPROM");
        for (int i = 0 ; i < EEPROM.length() ; i++) {
          EEPROM.write(i, 0);
          Serial.println(i);
        }
        Serial.println("EEPROM Cleared");
        Serial.println("Writing SSID");

        for (int i = 0; i < 32; ++i) {
          if (i < selectedSSID.length()) {
            EEPROM.write(wifi_ssid_address + i, selectedSSID[i]);
            Serial.println(selectedSSID[i]);
            //            EEPROM.write(wifi_ssid_address + i, charSSID[i]);
          }
          else {
            EEPROM.write(wifi_ssid_address + i, 0);
          }
        }
        Serial.println("Writing PASSWORD");
        for (int i = 0; i < 32; ++i) {
          if (i < selectedPassword.length()) {
            EEPROM.write(wifi_password_address + i, charPass[i]);
          }
          else {
            EEPROM.write(wifi_password_address + i, 0);
          }
        }
        EEPROM.commit();


        for (int i = 0; i < 32; ++i) {
          storedSSID += EEPROM.read(wifi_ssid_address + i);
          //          storedSSID += char(EEPROM.read(wifi_ssid_address + i));
        }
        for (int i = 0; i < 32; ++i) {
          storedPassword += char(EEPROM.read(wifi_password_address + i));
        }
        Serial.print("Stored SSID = "); Serial.println(storedSSID);
        Serial.print("Stored Pass = "); Serial.println(storedPassword);
      }

      client.flush();

      String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE html><html><head><title>WiFi Scan</title></head><body><h1>WiFi Networks</h1><form action='/select' method='get'><table><tr><th>SSID</th><th>Signal</th><th>Encryption</th><th>Select</th></tr>";
      int n = WiFi.scanNetworks();
      for (int i = 0; i < n; ++i) {
        s += "<tr><td>";
        s += WiFi.SSID(i);
        s += "</td><td>";
        s += WiFi.RSSI(i);
        s += " dBm</td><td>";
        s += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "Open" : "Encrypted";
        s += "</td><td>";
        s += "<input type='radio' name='ssid' value='" + WiFi.SSID(i) + "'>";
        s += "</td></tr>";
      }
      s += "</table><br><label for='password'>Password:</label><input type='text' id='password' name='password'><br><br><input type='submit' value='Select'></form></body></html>\r\n";

      client.print(s);
      break;
    }
  }
  client.stop();
  Serial.println("Client disconnected");
}

void ledStatus(){
  if (eepromStat == 1){
    interval = 250;
  }
  if (eepromStat == 0){
    interval = 1000;
  }
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    digitalWrite(ledPin, LOW);
    delay(5);
    digitalWrite(ledPin, HIGH);
  }
}
