// Library Blynk
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPLATE_ID"
#define BLYNK_DEVICE_NAME "DEVICE_NAME"
#define BLYNK_AUTH_TOKEN "BLYNK_AUTH"

// Library Firebase
#include <FirebaseESP32.h>
#define FIREBASE_HOST "FIREBASE_HOST_LINK"
#define FIREBASE_Authorization_key "FIREBASE_KEY"
FirebaseData firebaseData;
FirebaseJson json;

// Library Things
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Stepper.h>
#include "time.h"

// SSID dan Password WiFI
char ssid[] = "SSID";
char pass[] = "PASSWORD";

// auth Blynk
char auth[] = "BLYNK_AUTH";

// Define PIN Number
#define DHTPIN 23 //PIN
#define rainAnalog 35 //PIN
#define rainDigital 34 //PIN

// ULN2003 Motor Driver Pins
#define IN1 19
#define IN2 18
#define IN3 5
#define IN4 21
bool motor;
const int stepsPerRevolution = 2048;
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);

// Define Sensor
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

BlynkTimer timer;

// NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 21600; //GMT+7 (Jakarta)
const int   daylightOffset_sec = 3600;

// helper
bool tempJemuran;

void sendSensor(){
  Blynk.syncAll();

  // DHT
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Time
    struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  char timeLoc[50];
  strftime(timeLoc,50, "%A, %B %d %Y %H:%M:%S", &timeinfo);
  //Serial.println(timeLoc);

  // Upload
  Firebase.pushString(firebaseData, "/ESP32_APP/Time", timeLoc);
  Firebase.pushFloat(firebaseData, "/ESP32_APP/Temperatur", t);
  Firebase.pushFloat(firebaseData, "/ESP32_APP/Kelembapan", h);
  sensor_hujan();
  Blynk.virtualWrite(V11, timeLoc);
  Blynk.virtualWrite(V5, h);
  Blynk.virtualWrite(V6, t);
  
}

void sensor_hujan(){
  int rainAnalogVal = analogRead(rainAnalog);
  int rainDigitalVal = digitalRead(rainDigital);
  Blynk.virtualWrite(V10, rainDigitalVal);
  if (rainDigitalVal == 1){  
     Blynk.virtualWrite(V7, "TIDAK HUJAN");
  }else{
     Blynk.virtualWrite(V7, "HUJAN");
     if (tempJemuran == 1){
        Blynk.virtualWrite(V8, "Jemuran Masuk");
        stepmotor(1, 2);
        tempJemuran = 0;
     }
  }
} 


BLYNK_WRITE(V0){
  if(param.asInt() == 1){
      motor = param.asInt();
      Blynk.virtualWrite(V8, "Jemuran Masuk");
      stepmotor(1, 2);
      Blynk.virtualWrite(V0, 0);
  }
}

BLYNK_WRITE(V9){
  if(param.asInt() == 1){
      motor = param.asInt();
      Blynk.virtualWrite(V8, "Jemuran Di Luar");
      stepmotor(-1, 2);
      Blynk.virtualWrite(V9, 0);
      tempJemuran = 1;
  }
}

void stepmotor (int Direction, int Rotation){       
  for (int i = 0; i < Rotation; i++){               
    myStepper.step(Direction * stepsPerRevolution);     
  }
}

void setup()
{
  // Debug console
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  dht.begin();
  timer.setInterval(1000L, sendSensor);

  // RAINDROP
  pinMode(rainDigital,INPUT);

  // Step Motor
  myStepper.setSpeed(15);

  //Firebase
  Firebase.begin(FIREBASE_HOST,FIREBASE_Authorization_key);

  //NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop()
{
  Blynk.run();
  timer.run();
  delay(2000);
}
