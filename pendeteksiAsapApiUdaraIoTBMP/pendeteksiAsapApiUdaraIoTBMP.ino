#define BLYNK_TEMPLATE_ID "TMPL6Hii9W6Qu"
#define BLYNK_TEMPLATE_NAME "Kualitas Udara"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_BMP280.h>

#define FLAME_PIN 35
#define MQ135_PIN 34
#define LED_MERAH 18
#define LED_HIJAU 19
#define BUZZER 23

#define VREF 3.3
#define RL 10000.0
const float Ro = 13513.07;

// Ganti sesuai data dari Blynk
char auth[] = "7mzuKvPDPYfgRhgkZCga1KZLwiSdiv7u";
char ssid[] = "Syekang";
char pass[] = "elmariajin";

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_BMP280 bmp;

BlynkTimer timer;

float getRs(int adcValue);
float getRatio(float Rs);
float getCOppm(float ratio);
float getCO2ppm(float ratio);
void tampilkanLCD(String baris1, String baris2);
void indikatorBahaya(int tempo);
void indikatorNormal(float CO, float CO2);

void setup() {
  Serial.begin(115200);

  pinMode(FLAME_PIN, INPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  lcd.init();
  lcd.backlight();

  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 gagal!");
    tampilkanLCD("BMP280 GAGAL", "Periksa sensor!");
    while (1);
  }

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("\nWiFi terhubung");
  Blynk.config(auth);
  Blynk.connect(5000); // Tunggu max 5 detik


  // Timer kirim data ke Blynk tiap 2 detik
  timer.setInterval(2000L, kirimDataKeBlynk);
}

void loop() {
  Blynk.run();
  timer.run();

  int adcValue = analogRead(MQ135_PIN);
  float Rs = getRs(adcValue);
  float ratio = getRatio(Rs);
  float CO_ppm = getCOppm(ratio);
  float CO2_ppm = getCO2ppm(ratio);

  float pressure = bmp.readPressure() / 100.0;
  float temperature = bmp.readTemperature();

  Serial.print("CO: "); Serial.print(CO_ppm);
  Serial.print(" ppm | CO2: "); Serial.print(CO2_ppm);
  Serial.print(" ppm | Tekanan: "); Serial.print(pressure);
  Serial.print(" hPa | Suhu: "); Serial.println(temperature);

  bool apiTerdeteksi = digitalRead(FLAME_PIN) == LOW;
  bool asapTerdeteksi = (CO_ppm > 25 || CO2_ppm > 100);

  if (apiTerdeteksi && asapTerdeteksi) {
    tampilkanLCD("WASPADA", "KEBAKARAN!");
    indikatorBahaya(300);
    Blynk.logEvent("kebakaran", "KEBAKARAN TERDETEKSI!");
    Blynk.virtualWrite(V4, 255);  // LED Merah ON
  } else if (apiTerdeteksi) {
    tampilkanLCD("ADA API!", "");
    indikatorBahaya(500);
    Blynk.logEvent("api", "API TERDETEKSI!");
    Blynk.virtualWrite(V4, 255);  // LED Merah ON
  } else if (asapTerdeteksi) {
    tampilkanLCD("ADA ASAP!", "");
    indikatorBahaya(500);
    Blynk.logEvent("asap", "ASAP TERDETEKSI!");
    Blynk.virtualWrite(V4, 255);  // LED Merah ON
  } else {
    indikatorNormal(CO_ppm, CO2_ppm);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(pressure, 1); lcd.print(" hPa");
    lcd.setCursor(0, 1);
    lcd.print(temperature, 1); lcd.print(" C");
    delay(5000);
  }

  delay(200);
}

void kirimDataKeBlynk() {
  int adcValue = analogRead(MQ135_PIN);
  float Rs = getRs(adcValue);
  float ratio = getRatio(Rs);
  float CO_ppm = getCOppm(ratio);
  float CO2_ppm = getCO2ppm(ratio);

  float pressure = bmp.readPressure() / 100.0;
  float temperature = bmp.readTemperature();

  Blynk.virtualWrite(V0, CO_ppm);
  Blynk.virtualWrite(V1, CO2_ppm);
  Blynk.virtualWrite(V2, temperature);
  Blynk.virtualWrite(V3, pressure);
  Blynk.virtualWrite(V4, 0); // Merah mati
  Blynk.virtualWrite(V5, 0); // Hijau mati

}

float getRs(int adcValue) {
  float Vrl = adcValue * VREF / 4095.0;
  return ((VREF - Vrl) / Vrl) * RL;
}

float getRatio(float Rs) {
  return Rs / Ro;
}

float getCOppm(float ratio) {
  return pow(10, (-0.42 * log10(ratio) + 1.4));
}

float getCO2ppm(float ratio) {
  return pow(10, (-0.30 * log10(ratio) + 1.2));
}

void tampilkanLCD(String baris1, String baris2) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(baris1);
  lcd.setCursor(0, 1); lcd.print(baris2);
}

void indikatorBahaya(int tempo) {
  digitalWrite(LED_MERAH, HIGH);
  for (int i = 0; i < 5; i++) {
    tone(BUZZER, 1000); delay(tempo);
    tone(BUZZER, 1500); delay(tempo);
  }
  noTone(BUZZER);
  digitalWrite(LED_MERAH, LOW);
}

void indikatorNormal(float CO, float CO2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CO: "); lcd.print((int)CO); lcd.print(" ppm");
  lcd.setCursor(0, 1);
  lcd.print("CO2: "); lcd.print((int)CO2); lcd.print(" ppm");
  delay(5000);

  if (CO > 35 || CO2 > 1000) {
    tampilkanLCD("UDARA", "BURUK!");
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_MERAH, HIGH); delay(250);
      digitalWrite(LED_MERAH, LOW); delay(250);
    }
    for (int i = 0; i < 2; i++) {
      tone(BUZZER, 1000); delay(500);
      noTone(BUZZER); delay(500);
    }
    Blynk.virtualWrite(V4, 255); // LED merah nyala
    Blynk.virtualWrite(V5, 0);   // LED hijau mati
  } else {
    tampilkanLCD("UDARA", "BAIK!");
    digitalWrite(LED_HIJAU, HIGH);
    Blynk.virtualWrite(V5, 255); // LED hijau nyala
    Blynk.virtualWrite(V4, 0);   // LED merah mati
    delay(5000);
    digitalWrite(LED_HIJAU, LOW);
  }
}

