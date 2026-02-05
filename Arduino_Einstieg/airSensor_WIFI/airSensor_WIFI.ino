#include <Stepper.h>
#include <WiFiNINA.h>

const int airSensorPin=A1;

const int stepsPerRevolution = 1000; // Anzahl der Schritte pro Umdrehung
long stepCount = 0; // Zählt Schritte des Motors
float rotations = 0.0; // Geleistete Umdrehung des Motors seit Programmbeginn
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
float injektionVolumeFlow_ml = 20.8; // in ml/h
float injektionSpeed; // in µm/min
long spindleRotationalSpeed; // in U/h
int motorSpeed =1; // in U/min
float volumePerRevolution;
float airFlowRate;
float airSensorValue;
unsigned long previousMillis = 0; // Store the last time sensor was read
const long interval = 1000; // Interval at which to read the sensor (milliseconds)


void setup() {
    Serial.begin(9600);
    delay(10);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(A1, INPUT);

    //  // Berechnungen durchführen
    AreaCalc = long(pi * injektionDiameter * injektionDiameter); // in mm^2
    injektionArea = AreaCalc / 4; // in mm^2
    injektionVolumeFlow = injektionVolumeFlow_ml * 1000; // mm^3/min
    injektionSpeed = float(injektionVolumeFlow  /float(injektionArea )); // in mm/h 
    spindleRotationalSpeed = injektionSpeed / pitch ; // in U/h
    motorSpeed = int(spindleRotationalSpeed / 60); // in U/min

    volumePerRevolution = injektionArea * pitch * 0.001;  //in ml/U

    // Serial.println("PI:");
    // Serial.println(pi_long);
    
    // Serial.println("Injektion Diameter:");
    // Serial.println(injektionDiameter);
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

    //  Serial.println("Volume per Revolution");
    // Serial.println(volumePerRevolution);
    // myStepper.setSpeed(motorSpeed); // Maximalgeschwindigkeit in Schritten pro Sekunde
   
    initializeWiFi();
    delay(10000); // wait for connection

    server.begin();
    //printWiFiStatus();

}

void loop() {

    // read the input on analog pin 0 from air sensor:
     unsigned long currentMillis = millis();

    // Check if a second has passed
    if (currentMillis - previousMillis >= interval) {
        // Save the last time you read the sensor
        previousMillis = currentMillis;

        // Read the analog value from the air sensor
        float airSensorValue_loop = analogRead(airSensorPin);
        
        // Calculate the air flow rate. Adjust the conversion range as needed.
        float airFlowRate_loop = airSensorValue_loop * (25.0 - 0.0) / (1023.0 - 0.0); 

        // Print the sensor values to the Serial Monitor
        Serial.print("Air Sensor Value: ");
        Serial.println(airSensorValue_loop);
        Serial.print("Air Flow Rate: ");
        Serial.println(airFlowRate_loop);
    }
}

void initializeWiFi(){
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
}