#define POTMETER A0

void setup(){
  Serial.begin(9600); 
  pinMode(POTMETER, INPUT);
}

void loop(){
  int potmeterValue = analogRead(POTMETER);
  Serial.println(potmeterValue);
}