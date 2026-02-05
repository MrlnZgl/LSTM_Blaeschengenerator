#include <Stepper.h>
#include <WiFiNINA.h>

const int stepsPerRevolution = 1000; // Anzahl der Schritte pro Umdrehung
#define BUTTON_PIN 12 // Taster-Pin

// Definieren Sie die Pins für den Schrittmotor
const int motorPins[4] = {8, 9, 10, 11};

// Erstellen Sie eine Instanz des Stepper-Objekts
Stepper myStepper(stepsPerRevolution, motorPins[0], motorPins[1], motorPins[2], motorPins[3]);

char ssid[] = "ArduinoAP";        // Your network SSID (name)
char pass[] = "testArduino25";    // Your network password

int status = WL_IDLE_STATUS;
WiFiServer server(80);
bool motorRunning = false;
int motorSpeed = 1;

void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN, INPUT);
    myStepper.setSpeed(motorSpeed); // Maximalgeschwindigkeit in Schritten pro Sekunde
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
    if (digitalRead(BUTTON_PIN) == HIGH) { // Button pressed
        delay(50); // debounce delay
        if (digitalRead(BUTTON_PIN) == HIGH) { // Check again after debounce
             motorRunning = !motorRunning; 
            
            // Status ändern, um die AJAX-Anfragen richtig zu bedienen
            while (digitalRead(BUTTON_PIN) == HIGH); // Kurze Verzögerung, um die Schaltlogik abzukapseln
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

    if (client) { // If you get a client,
        Serial.println("New client");
        String currentLine = ""; // Hold incoming data from the client

        while (client.connected()) { // Loop while the client is connected
            if (client.available()) { // If there's bytes to read from the client,
                char c = client.read(); // Read a byte
                Serial.write(c); // Print it out the serial monitor

                if (c == '\n') { // If the byte is a newline character
                    // If currentLine is blank, send a response
                    if (currentLine.length() == 0) {
                        // Send an HTTP response header
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();
                        
                        // Send HTML page
                        client.println("<!DOCTYPE html><html><head><title>Step Motor Controller</title></head><body>");
                         client.println("<body>");
                        client.println("<h1>Step Motor Control</h1>");
                        client.println("<p>Set Motor Speed (steps/sec): <input type='text' id='motorSpeed' value='1'></p>");
                        client.println("<button onclick='setSpeed()'>Set Speed</button>");
                        client.println("<p id='errorMsg' style='color:red;'></p>"); // Fehlernachricht
                        client.println("<p>Click <a href=\"/start\">here</a> to start the motor</p>");
                        client.println("<p>Click <a href=\"/stop\">here</a> to stop the motor</p>");
                        client.println("Current motor status: ");
                        client.print(motorRunning ? "Running" : "Stopped");
                        client.println("</p>");
                        
                        client.println("<script>");
                        client.println("function setSpeed() {");
                        client.println("  var xhr = new XMLHttpRequest();");
                        client.println("  var speed = document.getElementById('motorSpeed').value;");

                        client.println("  // Überprüfen, ob die Eingabe nur Zahlen enthält");
                        client.println("  if (!/^[0-9]+$/.test(speed)) {"); // Regular Expression für nur Ziffern
                        client.println("    document.getElementById('errorMsg').innerText = 'Bitte nur Zahlen eingeben.';");
                        client.println("    return; // Abbrechen, keine Anfrage senden");
                        client.println("  } else {");
                        client.println("    document.getElementById('errorMsg').innerText = ''; // Fehlernachricht zurücksetzen");
                        client.println("  }");

                        client.println("  xhr.open('GET', '/setSpeed?value=' + speed, true);");
                        client.println("  xhr.send();");
                        client.println("}");
                        client.println("</script>");

                        client.println("</body></html>");

                        break; // Exit the while loop
                    } else {
                        currentLine = ""; // Clear the current line
                    }
                } else if (c != '\r') {
                    currentLine += c; 
                }

                // Process requests to start or stop the motor
                if (currentLine.endsWith("GET /start")) {
                    Serial.println("Read Start motor");
                    motorRunning = true; // Start the motor
                    // myStepper.step(1); // Move the motor one step
                    // delay(10); // Delay for smoother movement
                }
                if (currentLine.endsWith("GET /stop")) {
                    motorRunning = false; // Stop the motor
                }
                if (currentLine.startsWith("GET /setSpeed?value=")) {
                    // Extrahieren Sie die Geschwindigkeit aus der Anfrage
                    Serial.println("Read speed");
                    motorRunning = false;
                    int valueStartIndex = currentLine.indexOf('=') + 1; // Erhalte den Index nach '='
                    String speedString = currentLine.substring(valueStartIndex); // Hole den Rest der Zeile
                    Serial.println("ausgelesener Substring:");
                    Serial.println(speedString);
                    motorSpeed = speedString.toInt(); // Konvertieren Sie die Geschwindigkeit in eine ganze Zahl
                    Serial.println("MotorSpeed:");
                    Serial.println(motorSpeed);
                    myStepper.setSpeed(motorSpeed); // Setzen Sie die neue Geschwindigkeit für den Motor
                }
            }
        }
        client.stop(); // Close the client connection
        Serial.println("Client disconnected");
    }

    // Motorsteuerung basierend auf motorRunning
    if (motorRunning) {
        myStepper.step(1); // Move the motor one step
        delay(10); // Delay for smoother movement
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