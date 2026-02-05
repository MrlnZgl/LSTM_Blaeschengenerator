# include <Stepper.h>

const int stepsPerRevolution = 1000;

Stepper stepper = Stepper(stepsPerRevolution, 8, 9, 10 ,11);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  stepper.setSpeed(1);
  
}

void loop() {
    stepper.step(stepsPerRevolution);  
    delay(1000); // 1 Sekunde warten

    // Drehen Sie den Motor eine vollständige Umdrehung gegen den Uhrzeigersinn
    stepper.step(-stepsPerRevolution);
    delay(1000);
}
