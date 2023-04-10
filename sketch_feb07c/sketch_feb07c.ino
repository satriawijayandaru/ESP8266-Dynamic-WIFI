#include <ESP8266WiFi.h>
#include <EEPROM.h>

char* ssid = "";
char* password = "";

const int EEPROM_SIZE = 512;
int wifi_ssid_address = 0;
int wifi_password_address = 32;

const char* ssidAp = "ESP8266 Hotspot";
const char* passwordAp = "password";

WiFiServer server(80);
String selectedSSID;
String selectedPassword;

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidAp, passwordAp);

  server.begin();
  Serial.println("Server started");
  Serial.print("HotSpot IP address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
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
        selectedSSID = req.substring(startIndex, endIndex);
        Serial.print("REQ RAW = ");
        Serial.println(req);
        Serial.print("Selected SSID: '");
        Serial.print(selectedSSID);
        Serial.println("'");
        int passStartIndex = req.indexOf("password=") + 9;
        int passEndIndex = req.indexOf("HTTP", passStartIndex);
//        int passEndIndex = req.length();
        selectedPassword = req.substring(passStartIndex, passEndIndex - 1);
        Serial.print("Selected password: '");
        Serial.print(selectedPassword);
        Serial.println("'");
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
