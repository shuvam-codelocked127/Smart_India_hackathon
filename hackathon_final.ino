//GSM communication sms
#include <SoftwareSerial.h>
SoftwareSerial GPRS(8,9); //Pins 8 and 9 to Rx Tx
int c;

#include <dht.h>    //DHT11 library
dht DHT;  //DHT object

#include<LiquidCrystal.h>
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#define motor 4
#define acidvalve 5
#define basevalve 6
#define mainvalve 13

//Water level sensor Ultrasonic sensor
#define trigger 11
#define echo 10
int duration, cm;



int ch; //For crop type
int reset = LOW; //Initially no reset
void setup() {
  Serial.begin(9600);
  GPRS.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("WELCOME");
  pinMode(A0, INPUT); //Moisture sensor
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT); //Water level US sensor
  pinMode(7, INPUT); //DHT-Humidity Temp
  pinMode(A3, INPUT); //pH sensor
  pinMode(1, INPUT); // Button for Rice
  pinMode(2, INPUT); //Button for Tea
  pinMode(3, INPUT); //Reset button
  pinMode(4, OUTPUT); // Motor output
  pinMode(5, OUTPUT); //Acid valve
  pinMode(6, OUTPUT); //Base valve
  pinMode(12,OUTPUT); //Fertilizer valve
  pinMode(13,OUTPUT); //Main release valve

  GPRS.println("AT");
 
  delay(1000);
  

  init();



}

void init()
{
  lcd.setCursor(0, 0);
  lcd.print("Select crop");
  lcd.setCursor(1, 0);
  lcd.print("1:Cotton 2:Tea");
  if (digitalRead(1) == HIGH) {
    ch = 1;
    digitalWrite(1, LOW);
  }
  else if (digitalWrite(2) == HIGH) {
    ch = 2;
    digitalWrite(2, LOW);
  }
}

void loop() {
  reset = digitalRead(3);
  if (reset == HIGH) {
    init();
    digitalWrite(3, LOW);
  }
  wl = Level(); //Returns 0 if LOW, 1 if Medium and 2 if HIGH
  switch (ch) {
    case 1: if (wl == 0)
        digitalWrite(motor, LOW);
        
      else if (wl == 1)   //Water level is medium
      {
      switch(env()){   //env() is environment function that returns values ranging from 1-3 as per temperature and humidity(high or low). 1: Case when most water is req
          case 1:
          if (moisture() == 0) { //moisture() function returns 1 if high, 0 if low
            motorOn(5400, 27000); //motorOn(t1, t2) switches the motor on for t1 ms (filling time of treatment chamber)and off for t2 ms(emptying time of chamber)... Let the best case be 1.5 mins and 7.5 mins
          }
          else {
            digitalWrite(motor, LOW);
          }
        break;
        
         case 2:
          if (moisture() == 0) { 
            motorOn(3600,18000); //1min , 5 mins
          }
          else {
            digitalWrite(motor, LOW);
          }
        break;
        
         case 3:
          if (moisture() == 0) { 
            motorOn(1800,9000);  //0.5min, 2.5 mins
          }
          else {
            digitalWrite(motor, LOW);
          }
        break;
         }
      }
      
      else {  //Water level is maximum

         switch(env()){   //env() is environment function that returns values ranging from 1-3 as per temperature and humidity(high or low). 1: Case when most water is req
          case 1:
          if (moisture() == 0) { //moisture() function returns 1 if high, 0 if low
            motorOn(7200, 36000); //motorOn(t1, t2) switches the motor on for t1 ms (filling time of Treatment chamber)and off for t2 ms(emptying time)... Let the best case be 2 mins and 10 mins
          }
          else {
            digitalWrite(motor, LOW);
          }
        break;
        
         case 2:
          if (moisture() == 0) { 
            motorOn(5400,27000); //1.5mins , 7.5 mins
          }
          else {
            digitalWrite(motor, LOW);
          }
        break;
        
         case 3:
          if (moisture() == 0) { 
            motorOn(3600,18000);  //1 min, 5 mins
          }
          else {
            digitalWrite(motor, LOW);
          }
        break;
         }
        
      break;      //Break from cotton case
   
    
    //Tea case
    case 2:
      break;
  }


}

motorOn(int t1, int t2) {
  digitalWrite(motor, HIGH);
  delay(t1);
  digitalWrite(motor, LOW);

  digitalWrite(12,HIGH);  //12 is connected to fertilizer tank
  delay(0.1*t1);  //Fertilizer valve opens for 10% of t1 time.
  digitalWrite(12,LOW);

  if (ph() > 7) {     //ph() returns ph value of sample
    digitalWrite(acidvalve, HIGH);
    delay(int((ph() - 7) * 600)); // Acid valve opens for (ph()-7)*10 seconds... For 8 it opens for 10s, for 9 for 20s etc
    digitalWrite(acidvalve, LOW);
  }
  else {
    digitalWrite(basevalve, HIGH);
    delay(int((7 - ph()) * 600)); // Base valve opens for (7-ph())*10 seconds... For 6 it opens for 10s, for 5 for 20s etc
    digitalWrite(basevalve, LOW);
  }

  delay(3600);  //wait 1 min for mixing of componenents.

  digitalWrite(mainvalve,HIGH);
  delay(t2);
  digitalWrite(mainvalve,LOW);
}

int Level() {
  int wl;
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigger, LOW);
  duration = pulseIn(echo, HIGH); //pulseIn gives time value after which echo pin becomes high ie sound signal reaches back
  cm = (duration / 2) / 29;   //Here cm gives the depth of the tank upto which water is available, so water level is inversely prop to cm
  if (cm < 10){
    wl= 2; c=1;
  }
  else if (cm >= 10 && cm < 50){  
    wl= 1; c=1;
    }
  else{
    wl= 0;
    if(c==1&&wl==0){     //global variable c ensures that for one cycle only one sms is sent to the farmer, otherwise SMS will be sent infinitely.
    sendSMS();
    c=0;
    }
    return wl;
  }
}



sendSMS(){
  Serial.println("Alert: Water Level is Low");
  GPRS.println("AT+CMGF=1");
  delay(500);
  GPRS.println("AT+CMGS=\"+919531711838\"");
  delay(500);
  GPRS.println("Alert: Water Level is Low.");
  GPRS.write(0x1a); //hexadecimal for Ctrl+Z sequence to denote end of message.
  delay(500);
}


int env() {
  float T, H;  //T:in deg C, H:in %
  T = temp();
  H = hum();
  if(T>25 && H<50)
  return 1;
  else if(T>25 && H>50|| T<25 && H<50)
  return 2;
  else
  return 3;

}



float temp(){
  int chk = DHT.read11(7);  //DHT connected to pin 7
  return DHT.temperature; //in deg C
  
}


float hum(){
  int chk = DHT.read11(7);  //DHT connected to pin 7
  return DHT.humidity;  //in %
  
}


int moisture(){
  int mois= analogRead(A0);
  if(mois>700)
  return 1;
  else
  return 0; 
}


float ph(){
  const int analogInPin = A3; //pH probe connected to pin A3 
int sensorValue = 0; 
unsigned long int avgValue; 
float b;
int buf[10],temp;
 
 for(int i=0;i<10;i++) 
 { 
  buf[i]=analogRead(analogInPin);
  delay(10);
 }
 for(int i=0;i<9;i++)
 {
  for(int j=i+1;j<10;j++)
  {
   if(buf[i]>buf[j])
   {
    temp=buf[i];
    buf[i]=buf[j];
    buf[j]=temp;
   }
  }
 }
 avgValue=0;
 for(int i=2;i<8;i++)
 avgValue+=buf[i];
 float pHVol=(float)avgValue*5.0/1024/6;
 float phValue = -5.70 * pHVol + 21.34;
 return phValue;
 
}

