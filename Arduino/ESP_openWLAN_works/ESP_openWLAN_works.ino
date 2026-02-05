/*
    Go to thingspeak.com and create an account if you don't have one already.
    After logging in, click on the "New Channel" button to create a new channel for your data. This is where your data will be stored and displayed.
    Fill in the Name, Description, and other fields for your channel as desired, then click the "Save Channel" button.
    Take note of the "Write API Key" located in the "API keys" tab, this is the key you will use to send data to your channel.
    Replace the channelID from tab "Channel Settings" and privateKey with "Read API Keys" from "API Keys" tab.
    Replace the host variable with the thingspeak server hostname "api.thingspeak.com"
    Upload the sketch to your ESP32 board and make sure that the board is connected to the internet. The ESP32 should now send data to your Thingspeak channel at the intervals specified by the loop function.
    Go to the channel view page on thingspeak and check the "Field1" for the new incoming data.
    You can use the data visualization and analysis tools provided by Thingspeak to display and process your data in various ways.
    Please note, that Thingspeak accepts only integer values.

    You can later check the values at https://thingspeak.com/channels/2005329
    Please note that this public channel can be accessed by anyone and it is possible that more people will write their values.
 */

#include <WiFi.h>
#include <AccelStepper.h>

#define ARDUINO_SERIAL Serial1
#define SERIAL_BAUD 9600


const int airSensorPin=A0;
const int heliumSensorPin=A1;

const int stepsPerRevolution = 4000; // Anzahl der Schritte pro Umdrehung
long stepCount = 0; // Zählt Schritte des Motors
float rotations = 0.0; // Geleistete Umdrehung des Motors seit Programmbeginn
float volumeLoss = 0.0; // ml 
const float initialVolume = 12.7; //ml
float currentVolume;

#define EMPTY_BUTTON_PIN 12 // Taster-Pin
#define FULL_BUTTON_PIN 13 // Taster-Pin
// Definieren Sie die Pins für den Schrittmotor
//const int motorPins[4] = {8, 9, 10, 11};

#define DIR 4
#define STEP 3

#define MS1 5
#define MS2 6
#define MS3 7


AccelStepper myStepper(1, STEP, DIR);


const char *ssid = "@BayernWLAN";          // Change this to your WiFi SSID
const char *password = "";  // Change this to your WiFi password
WiFiServer server(80);

int field1 = 0;

int numberOfResults = 3;  // Number of results to be read
int fieldNumber = 1;      // Field number which will be read out


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
int empty_button_status = 1;
int full_button_status = 1;
bool buttonStatusChanged = false;
int button_status = 1;


void setup() {
  Serial.begin(115200);
  ARDUINO_SERIAL.begin(SERIAL_BAUD);
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println("******************************************************");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}



void loop(){

  handleClient();
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
                        // client.println("HTTP/1.1 200 OK");
                        // client.println("Content-type:text/html");
                        // client.println("Connection: close");
                        // client.println();
                        // client.println("<!DOCTYPE html><html>");
                        // client.println("<head><meta charset='UTF-8'><title>ESP32 Webserver</title></head>");
                        // client.println("<body>");
                        // client.println("<h1>Hallo vom ESP32!</h1>");
                        // client.println("<p>Dies ist eine Webseite, die direkt vom ESP32 kommt.</p>");
                        // client.println("</body></html>");
                        // client.println();
                        sendHTML(client);
                        break; // Exit the while loop
                    } else {
                        //processRequest(client, currentLine);
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
    client.print(empty_button_status ? 1: 0); 
    client.println(";");
    client.println("function startMotor() {");
    client.println("    console.log('Buttonstatus:', buttonStatus);"); // Debug
    client.println("    var volumeFlow = document.getElementById('injektionVolumeFlow').value;");
    client.println("    var num = parseFloat(volumeFlow);"); // String -> Zahl
    client.println("    if(buttonStatus == 0){");
    client.println("        if (!(num < 0)) {");
    client.println("            document.getElementById('errorMsg').innerText = 'Nur negative Zahlen erlaubt, wenn Button gedrueckt ist';");
    client.println("            return;");
    client.println("        } else {");
    client.println("            document.getElementById('errorMsg').innerText = '';");
    client.println("        }");
    client.println("    }");
    client.println("    else {");
    client.println("        if (!(num > 0)) {");
    client.println("            document.getElementById('errorMsg').innerText = 'Nur positive Zahlen erlaubt, wenn Button nicht gedrueckt ist!';");
    client.println("            return;");
    client.println("        } else {");
    client.println("            document.getElementById('errorMsg').innerText = '';"); 
    client.println("        }");
    client.println("    }");

    // Send a request to start the motor with the volume flow
    client.println("    fetch(`/controlMotor?action=start&volumeFlow=${volumeFlow}`)");
    client.println("        .then(response => response.json())");
    client.println("        .then(data => {");
    client.println("            updateMotorStatus(data.status);");
    client.println("            document.getElementById('currentVolumeFlow').innerText = data.volumeFlow;");
    client.println("        })");
    client.println("        .catch(error => console.error('Error controlling motor:', error));");
    client.println("}");
    

    // Buttonstatus regelmäßig abfragen
    // client.println("function checkEmptyButton(){");
    // client.println("  fetch('/empytButtonStatus').then(r=>r.text()).then(s=>{");
    // client.println("    empytButtonStatus = parseInt(s);");
    // client.println("    console.log('ButtonstatusInterval:', empytButtonStatus);"); // Debug
    // client.println("    if(empytButtonStatus == 0){");
    // client.println("      document.getElementById('errorMsg').innerText = 'Button pressed, Motor stopped!';");
    // client.println("    } else if (empytButtonStatus == 1) {");
    // client.println("      document.getElementById('errorMsg').innerText = '';"); 
    // client.println("    }");
    // client.println("  });");
    // client.println("}");
    // client.println("setInterval(checkEmptyButton, 10000);"); // jede Sekunde prüfen
    
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
    // client.println("setInterval(checkButton, 1000);"); // jede Sekunde prüfen
    
    //client.println("let lastButtonStatus = -1;");
    // client.println("function checkButton(){");
    // client.println("  fetch('/buttonStatus').then(r => r.text()).then(s => {");
    // client.println("    if(s == 'nochange') return;"); // Keine Änderung, nichts tun
    // client.println("    const newStatus = parseInt(s);");
    // client.println("    if(isNaN(newStatus)) return;");
    // client.println("    buttonStatus = newStatus;");
    // client.println("    console.log('Neuer Buttonstatus:', buttonStatus);");
    // client.println("    if(buttonStatus == 0){");
    // client.println("      document.getElementById('errorMsg').innerText = 'Button gedrückt, Motor gestoppt, Start gesperrt!';");
    // client.println("      updateMotorStatus('Stopped');"); // UI direkt anpassen
    // client.println("    } else {");
    // client.println("      document.getElementById('errorMsg').innerText = '';"); 
    // client.println("    }");
    // client.println("  }).catch(err => console.error('Buttonstatus Fehler:', err));");
    // client.println("}");
    // client.println("setInterval(checkButton, 1000);"); // alle 1s prüfen

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
    //client.println("setInterval(fetchMotorStatus, 5000); ");// Alle 5 Sekunden aktualisieren
    //client.println("setInterval(fetchVolumeFlow, 5000); // Alle 5 Sekunden abrufen");
    client.println("</script>");

    client.println("</body>");
    client.println("</html>");
    client.stop();

}

void processRequest(WiFiClient &client, String currentLine){
    // if (currentLine.startsWith("GET /controlMotor?action=stop")) {
        
    //     motorRunning = false;
    //     injektionVolumeFlow_ml = 0.0;
    //     Serial.println("Motor stopped");
        
    //     client.println("HTTP/1.1 200 OK");
    //     client.println("Content-Type: application/json");
    //     client.println();
    //     client.print("{\"status\": \"");
    //     client.print(motorRunning ? "Running" :"Stopped");
    //     client.print("\", \"volumeFlow\": ");
    //     client.print(injektionVolumeFlow_ml);
    //     client.println("}");
    //     client.stop();
    //     return;
    // }

    if (currentLine.startsWith("GET /controlMotor?action=stop")) {

        // Motor stoppen und VolumeFlow auf 0 setzen
        motorRunning = false;
        injektionVolumeFlow_ml = 0.0;
        Serial.println("Motor stopped");

        // An Arduino senden
        Serial1.println("STOP");  // Arduino empfängt Befehl

        // Antwort an Client
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println();
        client.print("{\"status\": \"Stopped\", \"volumeFlow\": ");
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

        // // Convert and set values
        // injektionVolumeFlow_ml = volumeFlowString.toFloat(); 
        // injektionVolumeFlow = injektionVolumeFlow_ml * 1000; 
        // injektionSpeed = float(injektionVolumeFlow  / float(injektionArea)); 
        // spindleRotationalSpeed = injektionSpeed / pitch; 
        // motorSpeed = int(spindleRotationalSpeed / 60); // U/min
        // motorSpeed_step = motorSpeed * stepsPerRevolution / 60;

        // Serial.println("Calculated motor speed:");
        // Serial.println(motorSpeed);
        // Serial.println("Calculated motor speed in steps/second:");
        // Serial.println(motorSpeed_step);

        // // Set motor speed and start the motor
        // myStepper.setSpeed(motorSpeed_step);
        // motorRunning = true;
        // Serial.println("Motor started");

        Serial1.println("VOLUME:" + String(injektionVolumeFlow_ml)); // Arduino bekommt den Wert
        Serial1.println("START");  // Startsignal für Arduino

        motorRunning = true;

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

    // if (currentLine.startsWith("GET /setVolumeFlow?value=")) {
    //     // Extrahieren Sie die injektionVolumeFlow aus der Anfrage
    //     Serial.println();
    //     Serial.println("Read Volume Flow");
    //     motorRunning = false; // Stoppe den Motor vorübergehend
    //     int valueStartIndex = currentLine.indexOf('=') + 1; // Erhalte den Index nach '='
    //     int valueEndIndex = currentLine.indexOf(' ', valueStartIndex);
    //     // Wenn kein Leerzeichen gefunden wird, setzen Sie valueEndIndex auf die Länge von currentLine
    //     if (valueEndIndex == -1) {
    //         valueEndIndex = currentLine.length();
    //     }
        
    //     // Hole den Wert zwischen den Indizes
    //     String volumeFlowString = currentLine.substring(valueStartIndex, valueEndIndex);
    //     Serial.println("Ausgelesener Substring (Volume Flow Wert):");
    //     Serial.println(volumeFlowString);                      
    //     // // Umwandeln der injektionVolumeFlow
    //     injektionVolumeFlow_ml = volumeFlowString.toFloat(); // Konvertieren Sie den injektionVolumeFlow in eine ganze Zahl
    //     injektionVolumeFlow = injektionVolumeFlow_ml * 1000; // mm^3/min
    //     injektionSpeed = float(injektionVolumeFlow  /float(injektionArea )); // in mm/h und multipliziert mit 1.000.000, um zu vermeiden, dass Werte verloren gehen.
    //     spindleRotationalSpeed = injektionSpeed / pitch ; // in U/h
    //     motorSpeed = int(spindleRotationalSpeed / 60); // in U/ min
    //     Serial.println("Berechneter MotorSpeed:");
    //     Serial.println(motorSpeed);
    //     myStepper.setSpeed(motorSpeed);
    //     client.println("HTTP/1.1 200 OK");
    //     client.println("Content-Type: text/plain");
    //     client.println("Connection: close");
    //     client.println();
    //     String responseMessage = "HTTP/1.1 200 OK\r\n";
    //     responseMessage += "Content-Type: text/plain\r\n";
    //     responseMessage += "Connection: close\r\n\r\n";
    //     responseMessage += String(injektionVolumeFlow_ml); 
    //     // Send the response back to the client
    //     client.println(injektionVolumeFlow_ml);
    //     client.stop();
    //     return;
    // }

    // if (currentLine.startsWith("GET /getVolumeFlow")) {

    //     client.println("HTTP/1.1 200 OK");
    //     client.println("Content-Type: application/json");
    //     client.println("Connection: close");
    //     client.println();
    //     client.print("{\"volumeFlow\": ");
    //     client.print(injektionVolumeFlow_ml); // Annahme: injektionVolumeFlow_ml ist der aktuelle Volumenfluss
    //     client.println("}");
    //     client.stop();
    //     Serial.println(injektionVolumeFlow_ml);
    //     return;
    // }

    // if (currentLine.startsWith("GET /getMotorStatus")) {

    //     client.println("HTTP/1.1 200 OK");
    //     client.println("Content-Type: application/json");
    //     client.println("Connection: close");
    //     client.println();
    //     client.print("{\"status\": \"");
    //     client.print(motorRunning ? "Running" :"Stopped");// Annahme: injektionVolumeFlow_ml ist der aktuelle Volumenfluss
    //     client.println("}");
    //     client.stop();
    //     Serial.println("Motor status ausgelesen");

    //     return;
    // }

    // if (currentLine.startsWith("GET /empytButtonStatus")) {
    //     client.println("HTTP/1.1 200 OK");
    //     client.println("Content-Type: text/plain");
    //     client.println("Connection: close");
    //     client.println();
    //     client.println(empty_button_status);   // sendet nur "0" oder "1"
    //     Serial.println(empty_button_status);
    //     return;
    // }

    // if (currentLine.startsWith("GET /buttonStatus")) {
    //     client.println("HTTP/1.1 200 OK");
    //     client.println("Content-Type: text/plain");
    //     client.println("Connection: close");
    //     client.println();

    //     if (buttonStatusChanged) {
    //         client.println(button_status);     // Nur senden, wenn sich was geändert hat
    //         buttonStatusChanged = false;       // Flag zurücksetzen
    //     } else {
    //         client.println("nochange");        // Browser weiß, dass nix Neues da ist
    //     }

    //     client.stop();
    // }

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

