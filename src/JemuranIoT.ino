// ============================================================
// Jemuran Otomatis Berbasis IoT — REVISED
// Perubahan dari versi asli:
// 1. Tambah logic auto-extend saat cerah (tidak hujan + suhu tinggi)
// 2. Firebase pakai setFloat/setString (bukan push) supaya
//    database tidak membengkak tiap detik
// 3. Timer dipisah: rain-check lebih sering, upload sensor lebih jarang
//    -> tidak pakai delay() blocking di loop()
// 4. Variable "tempJemuran" diganti "jemuranOut" biar jelas artinya
//    (true = jemuran sedang di luar, false = sedang masuk)
// ============================================================

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

// SSID dan Password WiFi
char ssid[] = "SSID";
char pass[] = "PASSWORD";

// auth Blynk (isi sama dengan BLYNK_AUTH_TOKEN di atas)
char auth[] = "BLYNK_AUTH";

// Define PIN Number
#define DHTPIN 23
#define rainAnalog 35
#define rainDigital 34

// ULN2003 Motor Driver Pins
#define IN1 19
#define IN2 18
#define IN3 5
#define IN4 21
const int stepsPerRevolution = 2048;
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);

// Define Sensor
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

BlynkTimer timer;

// NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 21600; // GMT+7 (Jakarta)
const int   daylightOffset_sec = 3600;

// State jemuran: true = sedang di luar (extended), false = sedang masuk (retracted)
// Asumsi saat boot posisi fisik jemuran dalam keadaan MASUK.
// Kalau di lapangan belum tentu begitu, sesuaikan nilai awal ini
// atau simpan state ke EEPROM/Firebase supaya persist antar-restart.
bool jemuranOut = false;

// Ambang batas suhu (°C) untuk dianggap "cerah" -> auto extend
const float SUNNY_TEMP_THRESHOLD = 30.0;

void moveMotor(int direction, int rotation) {
  for (int i = 0; i < rotation; i++) {
    myStepper.step(direction * stepsPerRevolution);
  }
}

void extendJemuran() {
  Blynk.virtualWrite(V8, "Jemuran Di Luar");
  moveMotor(-1, 2);
  jemuranOut = true;
}

void retractJemuran() {
  Blynk.virtualWrite(V8, "Jemuran Masuk");
  moveMotor(1, 2);
  jemuranOut = false;
}

// Dipanggil sering (tiap 2 detik) — cuma cek hujan & auto-retract.
// Sengaja dipisah dari upload sensor supaya respons hujan tetap cepat
// walau interval upload ke Firebase diperlambat.
void checkRain() {
  int rainDigitalVal = digitalRead(rainDigital);
  Blynk.virtualWrite(V10, rainDigitalVal);

  if (rainDigitalVal == 1) {
    Blynk.virtualWrite(V7, "TIDAK HUJAN");
  } else {
    Blynk.virtualWrite(V7, "HUJAN");
    if (jemuranOut) {
      retractJemuran();
    }
  }
}

// Dipanggil lebih jarang (tiap 30 detik) — baca suhu/kelembapan,
// upload ke Firebase, dan auto-extend kalau cerah & jemuran belum di luar.
void uploadSensorData() {
  Blynk.syncAll();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  char timeLoc[50];
  strftime(timeLoc, 50, "%A, %B %d %Y %H:%M:%S", &timeinfo);

  // setString/setFloat menimpa nilai lama, jadi database tidak
  // bertambah terus tiap kali upload (beda dengan pushString/pushFloat)
  Firebase.setString(firebaseData, "/ESP32_APP/Time", timeLoc);
  Firebase.setFloat(firebaseData, "/ESP32_APP/Temperatur", t);
  Firebase.setFloat(firebaseData, "/ESP32_APP/Kelembapan", h);

  Blynk.virtualWrite(V11, timeLoc);
  Blynk.virtualWrite(V5, h);
  Blynk.virtualWrite(V6, t);

  // Auto-extend: cerah (suhu tinggi) & tidak hujan & jemuran belum di luar
  bool rainDetected = (digitalRead(rainDigital) == 0);
  if (!rainDetected && t >= SUNNY_TEMP_THRESHOLD && !jemuranOut) {
    extendJemuran();
  }
}

// Tombol manual "masuk" di app
BLYNK_WRITE(V0) {
  if (param.asInt() == 1) {
    retractJemuran();
    Blynk.virtualWrite(V0, 0);
  }
}

// Tombol manual "keluar" di app
BLYNK_WRITE(V9) {
  if (param.asInt() == 1) {
    extendJemuran();
    Blynk.virtualWrite(V9, 0);
  }
}

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  dht.begin();

  pinMode(rainDigital, INPUT);

  myStepper.setSpeed(15);

  Firebase.begin(FIREBASE_HOST, FIREBASE_Authorization_key);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  timer.setInterval(2000L, checkRain);
  timer.setInterval(30000L, uploadSensorData);
}

void loop() {
  Blynk.run();
  timer.run();
  // Tidak ada delay() blocking di sini supaya Blynk.run()/timer.run()
  // tetap responsif.
}