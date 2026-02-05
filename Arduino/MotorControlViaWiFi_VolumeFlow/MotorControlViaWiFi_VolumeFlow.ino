#include <Stepper.h>
#include <WiFiNINA.h>



const int stepsPerRevolution = 1000; // Anzahl der Schritte pro Umdrehung
long stepCount = 0; // Zählt Schritte des Motors
float rotations = 0.0 // Geleistete Umdrehung des Motors seit Programmbeginn
float volumeLoss = 0.0; // ml 
const float initialVolume = 12.7; //ml
float currentVolume;

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

// Variablen zur Berechnung
float pi = PI;
long pi_long = long(pi);
long AreaCalc; // in mm^2
long pitch = 1; // in mm
float injektionDiameter = 14.960; // in mm
long injektionArea; // in µm^2
long injektionVolumeFlow ; // in µm^3/min
float injektionVolumeFlow_ml = 5.11; // in ml/h
float injektionSpeed; // in µm/min
long spindleRotationalSpeed; // in U/min
int motorSpeed =1; // in U/min
float volumePerRevolution;


void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN, INPUT);

    //  // Berechnungen durchführen
    AreaCalc = long(pi * injektionDiameter * injektionDiameter); // in mm^2
    injektionArea = AreaCalc / 4; // in mm^2
    injektionVolumeFlow = injektionVolumeFlow_ml * 1000; // mm^3/min
    injektionSpeed = float(injektionVolumeFlow  /float(injektionArea )); // in mm/h 
    spindleRotationalSpeed = injektionSpeed / pitch ; // in U/h
    motorSpeed = spindleRotationalSpeed; // in U/h

    volumePerRevolution = injektionArea * pitch * 0.001;  //in ml/U

    Serial.println("PI:");
    Serial.println(pi_long);
    
    Serial.println("Injektion Diameter:");
    Serial.println(injektionDiameter);
    Serial.println("AreaCalc:");
    Serial.println(AreaCalc);
    Serial.println("Injektion Area:");
    Serial.println(injektionArea);
  
    Serial.println("Injektion Volume Flow:");
    Serial.println(injektionVolumeFlow);
    Serial.println("Injektion Speed:");
    Serial.println(injektionSpeed);
    Serial.println("Spindle Speed");
    Serial.println(spindleRotationalSpeed);

    Serial.println("Current Motor Speed");
    Serial.println(motorSpeed);

     Serial.println("Volume per Revolution");
    Serial.println(volumePerRevolution);
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
                        // Eingabe für injektionVolumeFlow in ml/min
                        client.print("<p>Injection Volume Flow (ml/h): <input type='text' id='injektionVolumeFlow' value=''"); // Umrechnung in ml/min
                        client.print(injektionVolumeFlow_ml); // den letzten Wert hier verwenden
                        client.println("'></p>");
                        client.println("<button onclick='setVolumeFlow()'>Set Volume Flow</button>");
                        client.println("<p id='errorMsg' style='color:red;'></p>"); // Fehlernachricht

                         // Zeile für den letzten eingegebenen Wert
                        client.print("<p>Current Volume Flow (ml/h): ");
                        client.print(injektionVolumeFlow_ml); // Setze den zuletzt eingegebenen Wert in this line
                        client.println("</p>");
                        client.println("<p>Click <a href=\"/start\">here</a> to start the motor</p>");
                        client.println("<p>Click <a href=\"/stop\">here</a> to stop the motor</p>");
                        client.println("Current motor status: ");
                        client.print(motorRunning ? "Running" : "Stopped");
                        client.println("</p>");
                        
                        client.println("<script>");
                        client.println("function setVolumeFlow() {");
                        client.println("  var xhr = new XMLHttpRequest();");
                        client.println("  var volumeFlow = document.getElementById('injektionVolumeFlow').value;");

                        client.println("  // Überprüfen, ob die Eingabe nur Zahlen enthält");
                        client.println("  if (!/^[0-9]+([.][0-9]+)?$/.test(volumeFlow)) {"); // Regular Expression für nur Ziffern
                        client.println("    document.getElementById('errorMsg').innerText = 'Bitte nur Zahlen eingeben.';");
                        client.println("    return; // Abbrechen, keine Anfrage senden");
                        client.println("  } else {");
                        client.println("    document.getElementById('errorMsg').innerText = ''; // Fehlernachricht zurücksetzen");
                        client.println("  }");

                        client.println("  xhr.open('GET', '/setVolumeFlow?value=' + volumeFlow, true);");
                        client.println("  xhr.onreadystatechange = function () {");
                        client.println("    if (xhr.readyState == 4 && xhr.status == 200) {");
                        client.println("      // Optional: Zwinge die Seite, neu zu laden, um den neuen Wert anzuzeigen");
                        client.println("      location.reload();");
                        client.println("    }");
                        client.println("  };");
                        client.println("  xhr.send();");
                        client.println("}");
                        client.println("</script>");

                        client.println("</body></html>");

                        break; // Exit the while loop
                    } else {
                        
                        // Process requests to start or stop the motor
                        if (currentLine.endsWith("GET /start HTTP/1.1")) {
                            Serial.println("Read Start motor");
                            motorRunning = true; // Start the motor     
                        }
                        if (currentLine.endsWith("GET /stop HTTP/1.1")) {
                            Serial.println("Read Stop motor");
                            motorRunning = false; // Stop the motor
                        }
                        if (currentLine.startsWith("GET /setVolumeFlow?value=")) {
                            // Extrahieren Sie die injektionVolumeFlow aus der Anfrage
                            Serial.println();
                            Serial.println("Read Volume Flow");
                            motorRunning = false; // Stoppe den Motor vorübergehend
                            int valueStartIndex = currentLine.indexOf('=') + 1; // Erhalte den Index nach '='
                            int valueEndIndex = currentLine.indexOf(' ', valueStartIndex);
                            // Serial.println(valueStartIndex);
                            // Serial.println(valueEndIndex); 
    
                            // Wenn kein Leerzeichen gefunden wird, setzen Sie valueEndIndex auf die Länge von currentLine
                            if (valueEndIndex == -1) {
                                valueEndIndex = currentLine.length();
                            }
                            
                            // Hole den Wert zwischen den Indizes
                            String volumeFlowString = currentLine.substring(valueStartIndex, valueEndIndex);
                            Serial.println("Ausgelesener Substring (Volume Flow Wert):");
                            Serial.println(volumeFlowString);                      
                            // // Umwandeln der injektionVolumeFlow
                            injektionVolumeFlow_ml = volumeFlowString.toFloat(); // Konvertieren Sie den injektionVolumeFlow in eine ganze Zahl
                            injektionVolumeFlow = injektionVolumeFlow_ml * 1000; // mm^3/min
                            injektionSpeed = float(injektionVolumeFlow  /float(injektionArea )); // in mm/h und multipliziert mit 1.000.000, um zu vermeiden, dass Werte verloren gehen.
                            spindleRotationalSpeed = injektionSpeed / pitch ; // in U/h
                            motorSpeed = spindleRotationalSpeed; // in U/h

                            Serial.println("Berechneter MotorSpeed:");
                            Serial.println(motorSpeed);
                            myStepper.setSpeed(motorSpeed); // Setzen Sie die neue Geschwindigkeit für den Motor}
                        }
                        currentLine = ""; // Clear the current line
                    }
                } else if (c != '\r') {
                    currentLine += c; 
                }

                
            }
        }
        client.stop(); // Close the client connection
        Serial.println("Client disconnected");
    }

    // Motorsteuerung basierend auf motorRunning
    if (motorRunning) {
        myStepper.step(1); // Move the motor one step
       
        stepCount += 1;
        rotations = (float)stepCount / stepsPerRevolution; 
        volumeLoss = rotations * volumePerRevolution;
        currentVolume = initialVolume - volumeLoss;
         delay(5); // Delay for smoother movement

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