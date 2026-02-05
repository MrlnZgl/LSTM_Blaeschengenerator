/*
    Reads voltage across 250 Ohm Resistor which is in series with 4-20mAmp Sensor
    www.circuits4you.com
*/
const int sensorPin=A1;



const float V_REF = 5.0;
// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(A1, INPUT);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  //analogReadResolution(10);
  float sensorValue = analogRead(sensorPin);
  Serial.print(sensorValue);
  Serial.println(" as int");
  float Result;
  float Voltage;
  float Air_Flow;
  
  //Result = ((sensorValue-204.0)*100.0)/(1024.0-204.0);  //204 is offset, 4mAmp is 0
  Voltage = sensorValue*(4.0)/ (1024.0-204.0);
  Air_Flow  = (Voltage - 1.0) *(25.0/4.0);
  // print out the value you read:
  Serial.print("Sensor Output:");
  Serial.print(Air_Flow); 
  Serial.println(" l/min");
  
 
  delay(5000);        // delay in between reads for stability    
}
