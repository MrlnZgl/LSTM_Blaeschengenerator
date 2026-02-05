int Sensor = 7;

int Wert;

int LED = 6;

void setup()

{

  pinMode(Sensor, INPUT);

  pinMode(LED, OUTPUT);

}

void loop ()

{

  Wert = digitalRead(Sensor);

  if (Wert == 0)

  {

    // der Mikroschalter wurde gedrückt

    digitalWrite (LED, HIGH);

  }

  else

  {

    // der Mikroschalter wurde losgelassen

    digitalWrite (LED, LOW);

  }  

}