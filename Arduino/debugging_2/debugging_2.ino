#include <WiFiNINA.h>


char ssid[] = "ArduinoAP";        // Your network SSID (name)
char pass[] = "testArduino25";      // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key Index number (needed only for WEP)

int led =  LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Access Point Web Server");

  pinMode(led, OUTPUT);      // set the LED pin mode

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }

  // wait 10 seconds for connection:
  delay(10000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();
}


void loop() {
  WiFiClient client = server.available();

  if (!client) return;

  Serial.println("new client");
  String requestLine = "";
  String currentLine = "";

  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      Serial.write(c);

      if (c == '\n') {

        // Erste Zeile merken
        if (requestLine.length() == 0 && currentLine.length() > 0) {
          requestLine = currentLine;
        }

        // Leere Zeile = Header zu Ende
        if (currentLine.length() == 0) {

          // ---------- /status ----------
          if (requestLine.startsWith("GET /status")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();

            client.print(digitalRead(led) ? "ON" : "OFF");

            client.flush();
            delay(5);
            client.stop();
            Serial.println("status sent");
            return;
          }

          // ---------- LED ON ----------
          if (requestLine.startsWith("GET /H")) {
            digitalWrite(led, HIGH);
          }

          // ---------- LED OFF ----------
          if (requestLine.startsWith("GET /L")) {
            digitalWrite(led, LOW);
          }

          // ---------- HTML Seite ----------
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();

          client.println("<html><body>");
          client.println("<h2>LED Control</h2>");

          client.println("Click <a href=\"/H\">here</a> to turn LED ON<br>");
          client.println("Click <a href=\"/L\">here</a> to turn LED OFF<br><br>");

          client.println("<h3>LED Status: <span id='ledStatus'>...</span></h3>");

          client.println("<script>");
          client.println("function updateStatus(){");
          client.println("  fetch('/status').then(r => r.text()).then(t => {");
          client.println("    document.getElementById('ledStatus').innerText = t;");
          client.println("  });");
          client.println("}");
          client.println("setInterval(updateStatus, 1000);");
          client.println("updateStatus();");
          client.println("</script>");

          client.println("</body></html>");
          client.println();

          client.flush();
          delay(5);
          client.stop();
          Serial.println("html sent");
          return;
        }

        currentLine = "";
      }
      else if (c != '\r') {
        currentLine += c;
      }
    }
  }

  client.stop();
  Serial.println("client disconnected");
}
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}