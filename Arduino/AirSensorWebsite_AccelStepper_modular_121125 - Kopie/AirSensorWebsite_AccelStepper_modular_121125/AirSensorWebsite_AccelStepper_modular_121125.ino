
#include <AccelStepper.h>
#include <WiFiNINA.h>

const int airSensorPin=A0;
const int heliumSensorPin=A1;

const int stepsPerRevolution = 4000; // Anzahl der Schritte pro Umdrehung
long stepCount = 0; // Zählt Schritte des Motors
float rotations = 0.0; // Geleistete Umdrehung des Motors seit Programmbeginn
float volumeLoss = 0.0; // ml 
const float initialVolume = 12.7; //ml
float currentVolume;

#define BUTTON_PIN 12 // Taster-Pin

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
float heliumFlowRate;
float heliumSensorVoltage;
int button_status = 1;

void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(A1, INPUT);
    pinMode(A0, INPUT);
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
    
    Serial.println("Injektion Diameter:");
    Serial.println(injektionDiameter);
    // Serial.println("AreaCalc:");
    // Serial.println(AreaCalc);
    // Serial.println("Injektion Area:");
    // Serial.println(injektionArea);
  
    // Serial.println("Injektion Volume Flow:");
    // Serial.println(injektionVolumeFlow);
    // Serial.println("Injektion Speed:");
    // Serial.println(injektionSpeed);
    // Serial.println("Spindle Speed");
    // Serial.println(spindleRotationalSpeed);

    // Serial.println("Current Motor Speed");
    // Serial.println(motorSpeed);

    // Serial.println("Volume per Revolution");
    // Serial.println(volumePerRevolution);
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

    readSensors();
    handleButton();
    handleWiFiStatus();

    handleClient();
    handleMotor();
  
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

void readSensors() {
    //read the input on analog pin 0 from air sensor:
    float airSensorValue = analogRead(airSensorPin);
    airSensorVoltage = airSensorValue*(4.0)/ (1024.0-204.0); // offset 1-5 V
    airFlowRate  = (airSensorVoltage - 1.0) *(25.0/4.0); // VolFlow 0-25 l/min

    float heliumSensorValue = analogRead(heliumSensorPin);
    heliumSensorVoltage = heliumSensorValue*(5.0)/ (1024.0);
    heliumFlowRate  = 1.45 * (heliumSensorVoltage - 0.045) *.4; // VolFlow 0 - 2 l/min 
}

void handleButton(){
    int buttonReading = digitalRead(BUTTON_PIN);
    if (buttonReading == LOW && button_status != 0) { 
        button_status = 0;
        motorRunning = false;
        Serial.println("Button gedrückt -> Motor gestoppt, Start gesperrt");

    } else if(buttonReading == HIGH && button_status != 1){  // Button losgelassen
        button_status = 1;
        Serial.println("Button losgelassen -> Start wieder erlaubt");
    }
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

void handleClient(){
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
                        sendHTML(client);
                        break; // Exit the while loop
                    } else {
                        processRequest(client, currentLine);
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
}

void sendHTML(WiFiClient &client){
    // Send an HTTP response header
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
    
    // Send HTML page
    client.println("<!DOCTYPE html><html><head><title>Step Motor Controller</title>");
    client.println("<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>");
    client.println("</head>");
        client.println("<body>");
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
    
    //   // Funktion zum Abrufen des Luftstromwerts
    // client.println("<script>");
    // client.println("function fetchAirflow() {");
    // client.println("    fetch('/getAirflow')"); 
    // client.println("        .then(response => response.json())");
    // client.println("        .then(data => {");
    // client.println("            document.getElementById('currentAirflow').innerText = data.airflow;");
    // client.println("        })");
    // client.println("        .catch(error => console.error('Error fetching airflow:', error));");
    // client.println("}");
    
    // Regelmäßiger Abruf des Luftstromwerts
    //client.println("setInterval(fetchAirflow, 5000);"); // Alle 5 Sekunden aktualisieren
    // client.println("</script>");

    //   // Funktion zum Abrufen des Heliumstromwerts
    // client.println("<script>");
    // client.println("function fetchHeliumflow() {");
    // client.println("    fetch('/getHeliumflow')"); 
    // client.println("        .then(response => response.json())");
    // client.println("        .then(data => {");
    // client.println("            document.getElementById('currentHeliumflow').innerText = data.heliumflow;");
    // client.println("        })");
    // client.println("        .catch(error => console.error('Error fetching heliumflow:', error));");
    // client.println("}");
    
    // // Regelmäßiger Abruf des Heliumstromwerts
    // //client.println("setInterval(fetchHeliumflow, 5000);"); // Alle 5 Sekunden aktualisieren
    // client.println("</script>");


    client.println("<script>");
    client.print("var buttonStatus = "); 
    client.print(button_status ? 1: 0); 
    client.println(";");
    client.println("function startMotor() {");
    client.println("    console.log('Buttonstatus:', buttonStatus);"); // Debug
    client.println("    var volumeFlow = document.getElementById('injektionVolumeFlow').value;");
    //client.println("    var num = parseFloat(volumeFlow);"); // String -> Zahl
    //client.println("    if(buttonStatus == 0){");
    client.println("     if (!/^-?[0-9]+([.][0-9]+)?$/.test(volumeFlow)) {");
    client.println("        document.getElementById('errorMsg').innerText = 'Bitte nur Zahlen eingeben.';");
    client.println("        return;");
    client.println("      } else {");
    client.println("        document.getElementById('errorMsg').innerText = '';");
    client.println("      }");


    // client.println("        if (!(num < 0)) {");
    // client.println("            document.getElementById('errorMsg').innerText = 'Nur negative Zahlen erlaubt, wenn Button gedrueckt ist';");
    // client.println("            return;");
    // client.println("        } else {");
    // client.println("            document.getElementById('errorMsg').innerText = '';");
    // client.println("        }");
    // client.println("    }");
    // client.println("    else {");
    // client.println("        if (!(num > 0)) {");
    // client.println("            document.getElementById('errorMsg').innerText = 'Nur positive Zahlen erlaubt, wenn Button nicht gedrueckt ist!';");
    // client.println("            return;");
    // client.println("        } else {");
    // client.println("            document.getElementById('errorMsg').innerText = '';"); 
    // client.println("        }");
    //client.println("    }");

    // // Send a request to start the motor with the volume flow
    // client.println("    fetch(`/controlMotor?action=start&volumeFlow=${volumeFlow}`)");
    // client.println("        .then(response => response.json())");
    // client.println("        .then(data => {");
    // client.println("            updateMotorStatus(data.status);");
    // client.println("            document.getElementById('currentVolumeFlow').innerText = data.volumeFlow;");
    // client.println("        })");
    // client.println("        .catch(error => console.error('Error controlling motor:', error));");
    // client.println("}");
    
    client.println("    fetch(`/controlMotor?action=start&volumeFlow=${volumeFlow}`)");
    client.println("        .then(response => response.json())");
    client.println("        .then(data => {");
    client.println("            console.log('Server Response:', data);"); // Debug

    // Wenn eine Warnung vom ESP kommt
    client.println("            if (data.status === 'Warning') {");
    client.println("                document.getElementById('errorMsg').innerText = data.message;");
    client.println("                document.getElementById('motorStatus').innerText = 'Blocked';");
    client.println("                return; // Stoppt hier, Motor wird nicht gestartet");
    client.println("            }");

    // Wenn keine Warnung – normaler Ablauf
    client.println("            document.getElementById('errorMsg').innerText = '';"); 
    client.println("            updateMotorStatus(data.status);");
    client.println("            if (data.volumeFlow !== undefined) {");
    client.println("                document.getElementById('currentVolumeFlow').innerText = data.volumeFlow;");
    client.println("            }");
    client.println("        })");
    client.println("        .catch(error => console.error('Error controlling motor:', error));");
    //client.println("        .catch(error => {");
   // client.println("            console.error('Error controlling motor:', error);");
    //client.println("            document.getElementById('errorMsg').innerText = 'Verbindungsfehler zum ESP!';");
    //client.println("        });");
    client.println("}");

    // // Buttonstatus regelmäßig abfragen
    // client.println("function checkButton(){");
    // client.println("  fetch('/buttonStatus').then(r=>r.text()).then(s=>{");
    // client.println("    buttonStatus = parseInt(s);");
    // client.println("    console.log('ButtonstatusInterval:', buttonStatus);"); // Debug
    // client.println("    if(buttonStatus == 0){");
    // client.println("      document.getElementById('errorMsg').innerText = 'Button pressed, Motor stopped!';");
    // client.println("    } else if (buttonStatus == 1) {");
    // client.println("      document.getElementById('errorMsg').innerText = '';"); 
    // client.println("    }");
    // client.println("  });");
    // client.println("}");
    // client.println("setInterval(checkButton, 10000);"); // jede Sekunde prüfen
    
    
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

    //Funktion zum regelmäßigen Abrufen des Motorstatus
    client.println("function fetchMotorStatus() {");
    client.println("    fetch('/getMotorStatus')"); // Endpunkt, der den Motorstatus zurückgibt
    client.println("        .then(response => response.json())");
    client.println("        .then(data => {");
    client.println("            updateMotorStatus(data.status);");
    client.println("        })");
    client.println("        .catch(error => console.error('Error fetching motor status:', error));");
    client.println("}");
    client.println("</script>");

    // Canvas für das Diagramm
    client.println("<canvas id='volumeChart' width='400' height='200'></canvas>");
    
    // JavaScript zur Erstellung und Aktualisierung des Diagramms
    client.println("<script>");
    client.println("let initialVolume = 10;");
    client.println("let remainingVolume = initialVolume;");
    client.println("let volumeData = [];");
    client.println("let timeData = [];");
    client.println("let volumeChart = new Chart(document.getElementById('volumeChart'), {");
    client.println("  type: 'line',");
    client.println("  data: {");
    client.println("    labels: timeData,");
    client.println("    datasets: [{");
    client.println("      label: 'Remaining Volume (ml)',");
    client.println("      data: volumeData,");
    client.println("      borderColor: 'rgba(75, 192, 192, 1)',");
    client.println("      fill: false");
    client.println("    }]");    
    client.println("  },");
    client.println("  options: {");
    client.println("    scales: {");
    client.println("      y: {");
    client.println("        beginAtZero: true");
    client.println("      }");
    client.println("    }");
    client.println("  }");
    client.println("});");

    client.println("function updateChart(remainingVolume) {");
    client.println("  const now = new Date().toLocaleTimeString();");
    client.println("  timeData.push(now);");
    client.println("  volumeData.push(remainingVolume);");
    client.println("  if (timeData.length > 60) { // Behalte nur die letzten 10 Werte");
    client.println("    timeData.shift();");
    client.println("    volumeData.shift();");
    client.println("  }");
    client.println("  volumeChart.update();");
    client.println("}");

    client.println("function fetchVolumeFlow() {");
    client.println("  fetch('/getVolumeFlow') // Server-Endpunkt zum Abrufen des aktuellen Volumenflusses");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("        let volumeFlowPerSecond = data.volumeFlow / 3600"); // Umrechnung von ml/h in ml/s
    client.println("        remainingVolume -= volumeFlowPerSecond * 5"); // alle 5 Sekunden aktualisieren
    client.println("        remainingVolume = Math.max(remainingVolume, 0)");  // Sicherstellen, dass es nicht negativ wird
    client.println("        updateChart(remainingVolume);");
    client.println("    });");
    client.println("}");

    // Regelmäßig den Volumenfluss abrufen
    client.println("setInterval(fetchMotorStatus, 5000); ");// Alle 5 Sekunden aktualisieren
    client.println("setInterval(fetchVolumeFlow, 5000); // Alle 5 Sekunden abrufen");
    client.println("</script>");

    client.println("</body>");
    client.println("</html>");
    client.stop();

}

void processRequest(WiFiClient &client, String currentLine){
    if (currentLine.startsWith("GET /controlMotor?action=stop")) {
        
        motorRunning = false;
        injektionVolumeFlow_ml = 0.0;
        Serial.println("Motor stopped");
        
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println();
        client.print("{\"status\": \"");
        client.print(motorRunning ? "Running" :"Stopped");
        client.print("\", \"volumeFlow\": ");
        client.print(injektionVolumeFlow_ml);
        client.println("}");
        client.stop();
        return;
    }

    if (currentLine.startsWith("GET /controlMotor?action=start&volumeFlow=")) {
        // Extract the volume flow value from the request
        int actionStartIndex = currentLine.indexOf('=') + 1;
        int actionEndIndex = currentLine.indexOf('&');
        int volStartIndex = currentLine.indexOf('=', actionStartIndex) + 1;
        int volEndIndex = currentLine.indexOf(' ', volStartIndex);
        
        if (volEndIndex == -1) {
            volEndIndex = currentLine.length();
        }

        // Parse action and volume flow
        String action = currentLine.substring(actionStartIndex, actionEndIndex); // "-13" accounts for "&volumeFlow="
        String volumeFlowString = currentLine.substring(volStartIndex, volEndIndex);

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

        // if (button_status == 0) {  
        //     // Button ist aktiv → Motor nicht starten, Warnung senden
        //     Serial.println("WARNUNG: Button aktiv, Motorstart blockiert!");

        //     client.println("HTTP/1.1 200 OK");
        //     client.println("Content-Type: application/json");
        //     client.println();
        //     client.print("{\"status\": \"Warning\", \"message\": \"Motorstart blockiert – Button aktiv!\", \"buttonStatus\": ");
        //     client.print(button_status);
        //     client.println("}");
        //     client.stop();
        //     return;  // ⛔ Motorstart abbrechen
        // }

        if (button_status == 0) {
        // Button gedrückt → Warnung IMMER anzeigen
            Serial.println("Button gedrückt – nur negative Werte erlaubt!");
            
            if (injektionVolumeFlow_ml >= 0) {
                // Positiver Wert → blockieren
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: application/json");
                client.println();
                client.println("{\"status\":\"Warning\",\"message\":\"Button gedrückt – nur negative Werte erlaubt!\"}");
                client.stop();
                return;
            }

            // Negativer Wert → Motor starten
        Serial.println("Negativer Wert erkannt – Motor darf starten (Button gedrückt).");

        } else {
            // Button nicht gedrückt → nur positive Werte erlaubt
            if (injektionVolumeFlow_ml <= 0) {
                Serial.println("WARNUNG: Button nicht gedrückt, aber negativer Wert!");
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: application/json");
                client.println();
                client.println("{\"status\":\"Warning\",\"message\":\"Nur positive Werte erlaubt, wenn Button nicht gedrückt ist!\"}");
                client.stop();
                return;
            }
        }
        

        // Set motor speed and start the motor
        myStepper.setSpeed(motorSpeed_step);
        motorRunning = true;
        Serial.println("Motor started");

        // Send response back to the client
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println();
        client.print("{\"status\": \"Running\", \"volumeFlow\": ");
        client.print(injektionVolumeFlow_ml);
        client.println("}");
        client.stop();
        return;
    }

    if (currentLine.startsWith("GET /setVolumeFlow?value=")) {
        // Extrahieren Sie die injektionVolumeFlow aus der Anfrage
        Serial.println();
        Serial.println("Read Volume Flow");
        motorRunning = false; // Stoppe den Motor vorübergehend
        int valueStartIndex = currentLine.indexOf('=') + 1; // Erhalte den Index nach '='
        int valueEndIndex = currentLine.indexOf(' ', valueStartIndex);
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
        motorSpeed = int(spindleRotationalSpeed / 60); // in U/ min
        Serial.println("Berechneter MotorSpeed:");
        Serial.println(motorSpeed);
        myStepper.setSpeed(motorSpeed);
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        String responseMessage = "HTTP/1.1 200 OK\r\n";
        responseMessage += "Content-Type: text/plain\r\n";
        responseMessage += "Connection: close\r\n\r\n";
        responseMessage += String(injektionVolumeFlow_ml); 
        // Send the response back to the client
        client.println(injektionVolumeFlow_ml);
        client.stop();
        return;
    }

    if (currentLine.startsWith("GET /getVolumeFlow")) {

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();
        client.print("{\"volumeFlow\": ");
        client.print(injektionVolumeFlow_ml); // Annahme: injektionVolumeFlow_ml ist der aktuelle Volumenfluss
        client.println("}");
        client.stop();
        Serial.println(injektionVolumeFlow_ml);
        return;
    }

    if (currentLine.startsWith("GET /getMotorStatus")) {

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();
        client.print("{\"status\": \"");
        client.print(motorRunning ? "Running" :"Stopped");// Annahme: injektionVolumeFlow_ml ist der aktuelle Volumenfluss
        client.println("}");
        client.stop();
        Serial.println("Motor status ausgelesen");

        return;
    }

    if (currentLine.startsWith("GET /buttonStatus")) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println(button_status);   // sendet nur "0" oder "1"
        Serial.println(button_status);
        return;
    }

    // if (currentLine.startsWith("GET /getAirflow")) {
    // // Antwort auf die Anfrage nach dem Luftstrom Wert
    // client.println("HTTP/1.1 200 OK");
    // client.println("Content-type: application/json");
    // client.println();
    // client.print("{\"airflow\":");
    // client.print(airFlowRate); // Den aktuellen Luftstromwert zurückgeben
    // client.println("}");
    // client.stop();
    // Serial.println(airFlowRate);
    // Serial.println(airSensorVoltage);
    // Serial.println(airSensorValue);
    // return;
    // }

    // if (currentLine.startsWith("GET /getHeliumflow")) {
    // // Antwort auf die Anfrage nach dem Heliumstrom Wert
    // client.println("HTTP/1.1 200 OK");
    // client.println("Content-type: application/json");
    // client.println();
    // client.print("{\"heliumflow\":");
    // client.print(heliumFlowRate); // Den aktuellen Heliumstromwert zurückgeben
    // client.println("}");
    // client.stop();
    // Serial.println(heliumFlowRate);
    // Serial.println(heliumSensorVoltage);
    // Serial.println(heliumSensorValue);
    // return;
    // }

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
