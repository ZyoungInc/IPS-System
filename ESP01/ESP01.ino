#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
String comdata = "";
const char* ssid     = "IPS Hotspot";
const char* password = "password";
const char* serverName = "http://gocarbonneutral.org/drivers/post-data.php";
void setup() {
  Serial.begin(74880);
  Serial.println();
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected");
  //Serial.println(WiFi.localIP());
  //Serial.print("MAC: ");
  //Serial.println(WiFi.macAddress());
}

void loop() {
  while (Serial.available() > 0) {
    char temp = Serial.read();
    if (temp == '\n') {
      break;
    } else {
      comdata += temp;
    }
    delay(2);
  }
  if (comdata.length() > 0) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      comdata += "&api_key=tPmAT6Ab3j7F9";
      int httpResponseCode = http.POST(comdata);
      Serial.println(httpResponseCode);
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    comdata = "";
  }
}
