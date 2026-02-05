

///////////////////////////////////////////////////////////////////////////
// Funktionnierendes Minimalbeispiel
///////////////////////////////////////////////////////////////////////////////
#include <AccelStepper.h>
#include <WiFiNINA.h>

const int airSensorPin=A1;
const int heliumSensorPin=A0;

const int stepsPerRevolution = 4000; // Anzahl der Schritte pro Umdrehung
long stepCount = 0; // Zählt Schritte des Motors
float rotations = 0.0; // Geleistete Umdrehung des Motors seit Programmbeginn
float volumeLoss = 0.0; // ml 
const float initialVolume = 12.7; //ml
float currentVolume;
int led =  LED_BUILTIN;
#define EMPTY_BUTTON_PIN 12 // Taster-Pin
#define FULL_BUTTON_PIN 13 // Taster-Pin
// Definieren Sie die Pins für den Schrittmotor
//const int motorPins[4] = {8, 9, 10, 11};

#define DIR 4
#define STEP 3

#define MS1 5
#define MS2 6
#define MS3 7

// Erstellen Sie eine Instanz des Stepper-Objekts
//Stepper myStepper(stepsPerRevolution, motorPins[0], motorPins[1], motorPins[2], motorPins[3]);
AccelStepper myStepper(1, STEP, DIR);


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
float injektionVolumeFlow_ml = 0.0; // in ml/h
float injektionSpeed; // in µm/min
long spindleRotationalSpeed; // in U/h
int motorSpeed = 1; // in U/min
int motorSpeed_step;
float volumePerRevolution;
float airFlowRate;
float airSensorVoltage;
float airSensorValue;
float heliumFlowRate;
float heliumSensorVoltage;
float heliumSensorValue;
int empty_button_status = 1;
int full_button_status = 1;

void setup() {
    Serial.begin(9600);

    pinMode(led, OUTPUT);
    // pinMode(EMPTY_BUTTON_PIN, INPUT);
    // //pinMode(FULL_BUTTON_PIN, INPUT);
    // pinMode(A1, INPUT);
    // pinMode(A0, INPUT);
    myStepper.setMaxSpeed(1000);
    
    digitalWrite(MS1, LOW);
    digitalWrite(MS2, HIGH);
    digitalWrite(MS3, LOW);
    myStepper.setCurrentPosition(0);
    //  // Berechnungen durchführen
    AreaCalc = long(pi * injektionDiameter * injektionDiameter); // in mm^2
    injektionArea = AreaCalc / 4; // in mm^2
    injektionVolumeFlow = injektionVolumeFlow_ml * 1000; // mm^3/min
    injektionSpeed = float(injektionVolumeFlow  /float(injektionArea )); // in mm/h 
    spindleRotationalSpeed = injektionSpeed / pitch ; // in U/h
    motorSpeed = int(spindleRotationalSpeed / 60); // in U/min
    motorSpeed_step = motorSpeed *stepsPerRevolution / 60; // in steps/sec
    volumePerRevolution = injektionArea * pitch * 0.001;  //in ml/U
    

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

  if (motorRunning) {

    myStepper.runSpeed(); // Move the motor one step
    
    stepCount += 1;
    rotations = (float)stepCount / stepsPerRevolution; 
    volumeLoss = rotations * volumePerRevolution;
    currentVolume = initialVolume - volumeLoss;
    //Serial.println("Motor is running");
    //Serial.println(myStepper.speed());
    //delay(5); // Delay for smoother movement

  }


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
                if (requestLine.startsWith("GET /controlMotor?action=stop")) {
            
                    motorRunning = false;
                    injektionVolumeFlow_ml = 0.0;
                    Serial.println("Motor stopped");
                    
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: application/json");
                    client.println("Connection: close");
                    client.println();
                    client.print("{\"status\": \"");
                    client.print(motorRunning ? "Running" :"Stopped");
                    client.print("\", \"volumeFlow\": ");
                    client.print(injektionVolumeFlow_ml);
                    client.println("}");
                    client.flush();
                    delay(5);
                    client.stop();
                    return;
                }

                if (requestLine.startsWith("GET /controlMotor?action=start&volumeFlow=")) {
                    // Extract the volume flow value from the request
                    int actionStartIndex = requestLine.indexOf('=') + 1;
                    int actionEndIndex = requestLine.indexOf('&');
                    int volStartIndex = requestLine.indexOf('=', actionStartIndex) + 1;
                    int volEndIndex = requestLine.indexOf(' ', volStartIndex);
                    
                    if (volEndIndex == -1) {
                        volEndIndex = requestLine.length();
                    }

                    // Parse action and volume flow
                    String action = requestLine.substring(actionStartIndex, actionEndIndex); // "-13" accounts for "&volumeFlow="
                    String volumeFlowString = requestLine.substring(volStartIndex, volEndIndex);

                    Serial.println("Received start action with volume flow:");
                    Serial.println(volumeFlowString);

                    // Convert and set values
                    injektionVolumeFlow_ml = volumeFlowString.toFloat(); 
                    injektionVolumeFlow = injektionVolumeFlow_ml * 1000; 
                    injektionSpeed = float(injektionVolumeFlow  / float(injektionArea)); 
                    spindleRotationalSpeed = injektionSpeed / pitch; 
                    motorSpeed = int(spindleRotationalSpeed / 60); // U/min
                    motorSpeed_step = motorSpeed * stepsPerRevolution / 60;

                    Serial.println("Calculated motor speed:");
                    Serial.println(motorSpeed);
                    Serial.println("Calculated motor speed in steps/second:");
                    Serial.println(motorSpeed_step);

                    // Set motor speed and start the motor
                    myStepper.setSpeed(motorSpeed_step);
                    motorRunning = true;
                    Serial.println("Motor started");
                    Serial.println(injektionVolumeFlow_ml);
                    // Send response back to the client
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: application/json");
                // client.println("Connection: close");
                    client.println();
                    client.print("{\"status\": \"Running\", \"volumeFlow\": ");
                    client.print(injektionVolumeFlow_ml);
                    client.println("}");
                    client.flush();
                    delay(5);
                    client.stop();
                    return;
                }

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
                client.println("<h1>Step Motor Control</h1>");
                // Eingabe für injektionVolumeFlow in ml/min
                client.print("<p>Current Air Volume Flow (l/min):  <span id='currentAirflow'>");
                client.print(airFlowRate); 
                client.println("</span></p>");

                client.print("<p>Current Helium Volume Flow (l/min):  <span id='currentHeliumflow'>");
                client.print(heliumFlowRate); 
                client.println("</span></p>");

                client.print("<p>Injection Volume Flow (ml/h): <input type='text' id='injektionVolumeFlow' value=''"); // Umrechnung in ml/min
                client.print(injektionVolumeFlow_ml); // den letzten Wert hier verwenden
                client.println("'></p>");
                //client.println("<button onclick='setVolumeFlow()'>Set Volume Flow</button>");
                client.println("<p id='errorMsg' style='color:red;'></p>"); // Fehlernachricht
                //client.println("<p id='buttonMsg' style='color:red'></p>");

                    // Zeile für den letzten eingegebenen Wert
                client.print("<p>Current Volume Flow (ml/h): <span id='currentVolumeFlow'>");
                client.print(injektionVolumeFlow_ml); // Setze den zuletzt eingegebenen Wert in this line
                client.println("</span></p>");
                client.println("<button onclick=\"startMotor()\">Start Motor</button>");
                client.println("<button onclick=\"controlMotor('stop')\">Stop Motor</button>");
                client.println("<p");
                client.println(" id=\"motorStatus\">Current motor status: ");
                client.print(motorRunning ? "Running" : "Stopped");
                client.println("</p>");
                

                client.println("<script>");
            
                client.println("function startMotor() {");
                client.println("    var volumeFlow = document.getElementById('injektionVolumeFlow').value;");

                // Validate the volume input
                client.println("    if (!/^[0-9]+([.][0-9]+)?$/.test(volumeFlow)) {");
                client.println("        document.getElementById('errorMsg').innerText = 'Bitte nur Zahlen eingeben.';");
                client.println("        return;");
                client.println("    } else {");
                client.println("        document.getElementById('errorMsg').innerText = '';");
                client.println("    }");

                // Send a request to start the motor with the volume flow
                //client.println("    fetch('/controlMotor?action=start&volumeFlow=' + volumeFlow)");
                client.println("    fetch(`/controlMotor?action=start&volumeFlow=${volumeFlow}`)");
                client.println("        .then(response => response.json())");
                client.println("        .then(data => {");
                client.println("            updateMotorStatus(data.status);");
                client.println("            document.getElementById('currentVolumeFlow').innerText = data.volumeFlow;");
                client.println("        })");
                client.println("        .catch(error => console.error('Error controlling motor and saving volume flow:', error));");
                client.println("}");
                
                client.println("function controlMotor(action) {");
                client.println("    fetch(`/controlMotor?action=${action}`)");
                client.println("        .then(response => response.json())");
                client.println("        .then(data => {");   // Motorstatus aktualisieren, falls nötig
                client.println("            updateMotorStatus(data.status);");
                client.println("            document.getElementById('currentVolumeFlow').innerText = data.volumeFlow;");
                client.println("        })");
                client.println("        .catch(error => console.error('Error controlling motor:', error));");
                client.println("}");

                // Funktion zum Anzeigen des Motorstatus
                client.println("function updateMotorStatus(status) {");
                client.println("    const statusElement = document.getElementById('motorStatus');");
                client.println("    statusElement.textContent = 'Current motor status: ' +  status.charAt(0).toUpperCase() + status.slice(1);");
                client.println("}");

                client.println("</script>");

                client.println("<script>");
                client.println("function updateStatus(){");
                client.println("  fetch('/status').then(r => r.text()).then(t => {");
                client.println("    document.getElementById('ledStatus').innerText = t;");
                client.println("  });");
                client.println("}");
                client.println("setInterval(updateStatus, 10000);");
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

    // Motorsteuerung basierend auf motorRunning
    



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

void handleWiFiStatus(){
     // Update WiFi status
    if (status != WiFi.status()) {
        status = WiFi.status();
        if (status == WL_AP_CONNECTED) {
            Serial.println("Device connected to AP");
        } else {
            Serial.println("Device disconnected from AP");
        }
    }
}

void handleMotor(){
    // Motorsteuerung basierend auf motorRunning
    if (motorRunning) {
        myStepper.runSpeed(); // Move the motor one step
        stepCount += 1;
        // myStepper.run();
        // stepCount = myStepper.currentPosition();
        rotations = (float)stepCount / stepsPerRevolution; 
        volumeLoss = rotations * volumePerRevolution;
        currentVolume = initialVolume - volumeLoss;
        //Serial.println("Motor is running");
        //Serial.println(myStepper.speed());


    }
}

