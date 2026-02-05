#include <WiFiNINA.h>

char ssid[] = "ArduinoAP";        // your network SSID (name)
char pass[] = "testArduino25";    // your network password (use for WPA, or use as key for WEP)

int led = 11;
int button = 12;

int status = WL_IDLE_STATUS;
WiFiServer server(80);
bool ledState = LOW;

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    Serial.println("Access Point Web Server");

    pinMode(led, OUTPUT);
    pinMode(button, INPUT);

    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        while (true);
    }

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        Serial.println("Please upgrade the firmware");
    }

    Serial.print("Creating access point named: ");
    Serial.println(ssid);

    status = WiFi.beginAP(ssid, pass);
    if (status != WL_AP_LISTENING) {
        Serial.println("Creating access point failed");
        while (true);
    }

    delay(10000); // wait for connection

    server.begin();
    printWiFiStatus();
}

void loop() {
    // Check the button state
    if (digitalRead(button) == HIGH) { // Button pressed
        delay(50); // debounce delay
        if (digitalRead(button) == HIGH) { // Check again after debounce
            ledState = !ledState; // Toggle LED state
            digitalWrite(led, ledState); // Set LED
            while (digitalRead(button) == HIGH);
        }
    }

    // Update WiFi status
    if (status != WiFi.status()) {
        status = WiFi.status();
        if (status == WL_AP_CONNECTED) {
            Serial.println("Device connected to AP");
        } else {
            Serial.println("Device disconnected from AP");
        }
    }

    WiFiClient client = server.available(); // Listen for incoming clients

    if (client) {                             // If you get a client,
      Serial.println("new client");
      String currentLine = "";              // Hold incoming data from the client
      while (client.connected()) {          // Loop while the client is connected
        if (client.available()) {         // If there's bytes to read from the client,
          char c = client.read();       // Read a byte,
          Serial.write(c);              // Print it out the serial monitor
          if (c == '\n') {              // If the byte is a newline character

            if (currentLine.length() == 0) {
            // Send an HTTP response header
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();

              // send HTML page
              client.println("<!DOCTYPE HTML><html>");
              client.println("<head><title>LED Controller</title>");
              client.println("<style>.modal { display: none; position: fixed; z-index: 1000; left: 0; top: 0; width: 100%; height: 100%; background-color: rgba(0,0,0,0.5); }");
              client.println(".modal-content { background-color: white; margin: auto; padding: 20px; border: 1px solid #888; width: 60%; }</style>");
              client.println("</head>");
              client.println("<body>");
              client.print("<h1>LED Controller</h1>");
              client.print("<p>Current LED status: <span id='ledStatus'>");
              client.print(ledState ? "On" : "Off");
              client.println("</span></p>");
              client.print("<a href=\"/H\">Turn LED On</a><br/>");
              client.print("<a href=\"/L\">Turn LED Off</a><br/>");
              client.println("<br>");
              client.println("</body></html>");
              // JavaScript for updating LED status
              client.println("<script>");
              client.println("setInterval(function() {");
              client.println("var xhr = new XMLHttpRequest();");
              client.println("xhr.onreadystatechange = function() {");
              client.println("if (xhr.readyState == 4 && xhr.status == 200) {");
              client.println("document.getElementById('ledStatus').innerHTML = xhr.responseText;");
              client.println("if (xhr.responseText == 'Off') {");
              //client.println("document.getElementById('myModal').style.display = 'block';");
              client.println("}}};");
              client.println("xhr.open('GET', '/status', true);");
              client.println("xhr.send();");
              client.println("}, 2000);");
              client.println("</script>");

              // // Modal HTML
              // client.println("<div id='myModal' class='modal'>");
              // client.println("<div class='modal-content'>");
              // client.println("<span onclick='document.getElementById(\"myModal\").style.display=\"none\"'>&times;</span>");
              // client.println("<p>The LED is OFF!</p>");
              // client.println("</div></div>");

              
              //client.println();
              break; // Exit the while loop
            } else {
                currentLine = ""; // Clear the current line
            }
          } else if (c != '\r') {
              currentLine += c; 
          }

          // Process requests to turn the LED on or off
          if (currentLine.endsWith("GET /H")) {
            digitalWrite(led, HIGH);              
            ledState = true;                       
          }
          if (currentLine.endsWith("GET /L")) {
            digitalWrite(led, LOW);               
            ledState = false;                      
          }
        }
      }
      client.stop();  // Close the client connection
      Serial.println("client disconnected");
    }

    // Serve status request if available
    if (client.available()) {
      String request = client.readStringUntil('\n');
      if (request.startsWith("GET /status")) {
        client.print(ledState ? "On" : "Off"); 
        client.stop(); // Close the client connection after responding
      }
    }
}

void printWiFiStatus() {
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    Serial.print("To see this page in action, open a browser to http://");
    Serial.println(ip);
}