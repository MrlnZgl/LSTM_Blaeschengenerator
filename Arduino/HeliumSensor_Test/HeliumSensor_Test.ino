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
  float Helium_Flow;
  float corr_Helium_Flow;
  
  //Result = ((sensorValue-204.0)*100.0)/(1024.0-204.0);  //204 is offset, 4mAmp is 0
  Voltage = sensorValue*(5.0)/ (1024.0);
  Helium_Flow  = (Voltage - 0.045) *.4;
  corr_Helium_Flow = Helium_Flow* 1.45;
  // print out the value you read:
  Serial.println("Voltage:");
  Serial.println(Voltage);
  Serial.println("Helium Flow:");
  Serial.print(Helium_Flow); 
  Serial.println(" l/min");

  Serial.println("Helium Flow with corrector:");
  Serial.print(corr_Helium_Flow); 
  Serial.println(" l/min");
  
 
  delay(5000);        // delay in between reads for stability    
}
