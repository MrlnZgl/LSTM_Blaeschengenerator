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

    pinMode(led, OUTPUT);      // set the LED pin mode
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
            // Status ändern, um die AJAX-Anfragen richtig zu bedienen
            while (digitalRead(button) == HIGH); // Kurze Verzögerung, um die Schaltlogik abzukapseln
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

    WiFiClient client = server.available();   // Listen for incoming clients

    if (client) {                             // If you get a client,
        Serial.println("new client");
        String currentLine = "";   
        //bool responseSent = false; // Track if response has been sent           // Hold incoming data from the client
        while (client.connected()) {          // Loop while the client is connected
            if (client.available()) {         // If there's bytes to read from the client,
                char c = client.read();       // Read a byte,
                Serial.write(c);              // Print it out the serial monitor
                if (c == '\n') {              // If the byte is a newline character

                    if (currentLine.length() == 0) {
                        // Send an HTTP response headers
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();

                        // Send HTML page
                        client.println("<!DOCTYPE html><html><body>");
                        client.print("Click <a href=\"/H\">here</a> to turn the LED on<br>");
                        client.print("Click <a href=\"/L\">here</a> to turn the LED off<br>");

                        // Display the current LED status
                        client.print("Current LED status: ");
                        client.print(ledState ? "On" : "Off");
                        client.println("<br>");

                         // Auto-refresh status every 2 seconds
                        client.println("<script>");
                        client.println("setInterval(function() {");
                        client.println("  var xhr = new XMLHttpRequest();");
                        client.println("  xhr.onreadystatechange = function() {");
                        client.println("    if (xhr.readyState == 4 && xhr.status == 200) {");
                        client.println("      document.getElementById('ledStatus').innerHTML = xhr.responseText;");
                        client.println("    }");
                        client.println("  };");
                        client.println("  xhr.open('GET', '/status', true);");
                        client.println("  xhr.send();");
                        client.println("}, 2000);"); // 2000 ms = 2 seconds
                        
                        client.println("<p id='ledStatus'></p>");
                        client.println("</script>");
                        client.println("</body></html>");
                        //responseSent = true; // Mark response as sent
                        client.println();
                        break; // Exit the while loop
                    } else {
                        currentLine = ""; // Clear the current line
                    }
                }
                else if (c != '\r') {
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
        
        client.stop();
        Serial.println("client disconnected");
    }

     // Serve status request if available
    if (client.connected()) {
      // Only check for status requests if client is still connected
      String request = client.readStringUntil('\n');
        if (request.startsWith("GET /status")) {
            client.print(ledState ? "On" : "Off"); 
            client.stop();
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