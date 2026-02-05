#include <AccelStepper.h>

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
  Serial.begin(115200);      // Debug auf PC
  Serial1.begin(9600);       // Kommunikation mit ESP
  pinMode(EMPTY_BUTTON_PIN, INPUT);
  pinMode(FULL_BUTTON_PIN, INPUT);
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
}

void loop() {
  Serial.println("Test");
  //readSensors();
  //handleEmptyButton();
  //handleFullButton();
  //handleButton();
  handleSerialCommands();
  handleMotor();
  
}


void handleSerialCommands() {
  if (Serial1.available()) {
    Serial.println("Test3");
    String command = Serial1.readStringUntil('\n');
    command.trim(); // Leerzeichen entfernen
    Serial.println(command);
    if (command == "STOP") {
      motorRunning = false;
      Serial.println("Motor gestoppt");
    }
    else if (command.startsWith("START")) {
      motorRunning = true;
      Serial.println("Motor gestartet");
    }
    else if (command.startsWith("VOLUME:")) {
      // z. B. "VOLUME:123.45"
      String valStr = command.substring(7);
      float volume_ml = valStr.toFloat();
      // Hier die Berechnung in Schritte umrechnen:
      motorSpeed_step = calculateMotorSpeed(volume_ml);
      myStepper.setSpeed(motorSpeed_step);
      Serial.print("Neuer VolumeFlow empfangen: ");
      Serial.println(volume_ml);
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

float calculateMotorSpeed(float volumeFlow_ml){
 // Convert and set values
  //volumeFlow_ml = volumeFlowString.toFloat(); 
  injektionVolumeFlow = volumeFlow_ml * 1000; 
  injektionSpeed = float(injektionVolumeFlow  / float(injektionArea)); 
  spindleRotationalSpeed = injektionSpeed / pitch; 
  motorSpeed = int(spindleRotationalSpeed / 60); // U/min
  motorSpeed_step = motorSpeed * stepsPerRevolution / 60;
  return motorSpeed_step;

}

