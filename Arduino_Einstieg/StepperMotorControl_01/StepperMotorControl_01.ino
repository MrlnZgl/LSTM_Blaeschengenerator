#include <Stepper.h>

#define STEPS_PER_REVOLUTION 200  // Anzahl der Schritte pro Umdrehung 
#define BUTTON_PIN 12              // Taster-Pin

// Definieren Sie die Pins für den Schrittmotor
const int motorPins[4] = {8, 9, 10, 11};

// Erstellen Sie eine Instanz des Stepper-Objekts
Stepper myStepper(STEPS_PER_REVOLUTION, motorPins[0], motorPins[1], motorPins[2], motorPins[3]);

void setup() {
    Serial.begin(9600); // Serielle Kommunikation starten
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Taster mit internem Pull-Up-Widerstand festlegen
    myStepper.setSpeed(60); // Setzen Sie die maximale Geschwindigkeit in Umdrehungen pro Minute
}

void loop() {
    // Überprüfen Sie, ob der Taster gedrückt wird
    if (digitalRead(BUTTON_PIN) == LOW) {
        // Stoppen Sie den Motor, wenn der Taster gedrückt wird
        myStepper.step(0); // Halt-Befehl für den Motor
    } else {
        // Drehen Sie den Motor um 1 Umdrehung (in Schritten)
        myStepper.step(STEPS_PER_REVOLUTION);
        delay(10); // Wartezeit von 10 ms zwischen den Umdrehungen
    }
}