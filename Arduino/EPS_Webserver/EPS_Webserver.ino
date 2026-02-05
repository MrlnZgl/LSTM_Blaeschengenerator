#include <WiFi.h>

// Netzwerkdaten für den Access Point
const char* ssid = "Marlene's iPhone";
const char* password = "qspxdv11dnfgd";

// Webserver auf Port 80 starten
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  //delay(30000);

  // if (WiFi.status() == WL_NO_MODULE) {
  //   Serial.println("Communication with WiFi module failed!");
  //   while (true);
  // }

  // // String fv = WiFi.firmwareVersion();
  // // if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
  // //   Serial.println("Please upgrade the firmware");
  // // }

  // Serial.print("Creating access point named: ");
  // Serial.println(ssid);

  // status = WiFi.begin(ssid, password);
  // if (status != WL_AP_LISTENING) {
  //   Serial.println("Creating access point failed");
  //   while (true);
  // }

  delay(10000); // wait for connection

  server.begin();
  Serial.println();
  Serial.println("Starte ESP32 Access Point...");

  // Access Point starten
  WiFi.mode(WIFI_AP);
  bool apStarted = WiFi.softAP(ssid, password,1 ,0,4);
  if (!apStarted) {
    Serial.println("Fehler: Access Point konnte nicht gestartet werden!");
    while (true);
  }

  // IP-Adresse ausgeben
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point gestartet. IP-Adresse: ");
  Serial.println(IP);
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Passwort: ");
  Serial.println(password);
  Serial.println("Webserver wird gestartet...");
  delay(30000);
  server.begin();
  Serial.println("Webserver läuft!");
  

}

void loop() {
  // Auf eingehende Clients warten
  WiFiClient client = server.available();
  // if (!client) {
  //   return;
  // }
  if(client){
    Serial.println("Neuer Client verbunden.");
    String request = "";

    // Warten bis Daten empfangen werden
    while (client.connected() && !client.available()) delay(1);

    // Anfrage lesen
    request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    // HTTP-Antwort senden
    String response = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32 Webserver</title></head>";
    response += "<body><h1>Hallo vom ESP32!</h1>";
    response += "<p>Du bist mit dem ESP32 Access Point verbunden.</p>";
    response += "</body></html>";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();
    client.println(response);
    client.println();

    delay(10);
    client.stop();
    Serial.println("Client getrennt.");
  }
}