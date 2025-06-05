#include <EEPROM.h>

#define MQ135_PIN 34     // Pin analog MQ-135
#define RL 10000.0       // Load resistor 10k ohm
#define EEPROM_ADDR 0    // Alamat EEPROM untuk menyimpan Ro

float R0 = 0;

void setup() {
  delay(1000); // Tambahkan delay 1 detik agar stabil
  Serial.begin(115200);
  EEPROM.begin(512); // Inisialisasi EEPROM

  Serial.println("Memanaskan sensor selama 5 menit...");
  delay(300000); // 5 menit (300.000 ms)

  Serial.println("Kalibrasi MQ-135 di udara bersih...");

  float Rs_total = 0;
  int samples = 50;

  for (int i = 0; i < samples; i++) {
    float voltage = analogRead(MQ135_PIN) * 3.3 / 4095.0;
    float Rs = (3.3 - voltage) * RL / voltage;
    Rs_total += Rs;
    delay(100); // jeda antar-sampel
  }

  float Rs_avg = Rs_total / samples;
  R0 = Rs_avg / 3.6; // Berdasarkan datasheet Rs/Ro ≈ 3.6 di udara bersih

  Serial.print("Nilai Ro (rata-rata): ");
  Serial.println(R0);

  // Simpan ke EEPROM
  EEPROM.put(EEPROM_ADDR, R0);
  EEPROM.commit();
  Serial.println("Ro disimpan ke EEPROM.");
}

void loop() {
  // Tidak ada apa-apa di loop
}
