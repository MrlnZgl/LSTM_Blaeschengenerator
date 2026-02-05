int LED_PIN = 11;
int BUTTON_PIN = 12;

int buttonstatus = 0;
int buttonZaehler = 0;
int letzterStatus = 0;

void setup() {

  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

}

void loop() {

  buttonstatus = digitalRead(BUTTON_PIN);

  if(buttonstatus != letzterStatus)
  {
    if (buttonstatus == 1) 
    {
      buttonZaehler++;
    }

    else 
    {
      digitalWrite(LED_PIN, HIGH); 
    }
  }

  if (buttonZaehler % 2 == 0)
  {
    digitalWrite(LED_PIN, LOW);
    buttonZaehler = 0;
  }

  else 
  {
    digitalWrite(LED_PIN, HIGH);
  }
  letzterStatus = buttonstatus; 
  Serial.println(buttonZaehler);
  delay(10);
}