// Pengukur Frekuensi Sinyal — ESP32 + LCD I2C 16x2
//
// Mengukur frekuensi sinyal gelombang kotak (0-3,3 V) yang masuk ke
// GPIO34. Setiap sisi naik sinyal membangkitkan interupsi yang menambah
// pencacah pulsa; tiap 1 detik (gate) jumlah pulsa dibaca dan ditampilkan
// sebagai frekuensi (Hz, atau kHz bila >= 1000 Hz) beserta periodenya
// dalam mikrodetik di LCD I2C alamat 0x27. Hasil yang sama dikirim juga
// ke Serial Monitor (115200 baud) untuk kalibrasi dan debug.
//
// Sumber sinyal yang cocok: generator fungsi, osilator NE555, keluaran
// PWM mikrokontroler lain, atau sensor berpulsa. JANGAN beri sinyal
// 5 V langsung ke GPIO34 — turunkan dulu dengan pembagi tegangan
// (mis. 10k seri + 10k ke GND) atau pakai sumber 3,3 V.
//
// Wiring:
//   Sinyal masuk -> GPIO34 (interupsi RISING, pull-down 10k ke GND)
//   LCD SDA      -> GPIO21
//   LCD SCL      -> GPIO22
//   LCD VCC      -> 5V, LCD GND -> GND
//   Catu         -> USB 5V ke VIN ESP32
//
// Board: ESP32 DevKit (esp32:esp32:esp32), ESP32 core 4.0.0.
// Library: LiquidCrystal_I2C 1.1.2 (lihat folder libraries/).

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- konfigurasi pin & konstanta ----------
const uint8_t PIN_SINYAL   = 34;    // masukkan sinyal gelombang kotak
const uint8_t PIN_SDA_LCD  = 21;    // SDA bus I2C
const uint8_t PIN_SCL_LCD  = 22;    // SCL bus I2C
const uint8_t ALAMAT_LCD   = 0x27;  // alamat I2C LCD 16x2
const uint8_t KOLOM_LCD    = 16;    // lebar LCD
const uint8_t BARIS_LCD    = 2;     // jumlah baris LCD
const uint32_t DURASI_GATE_MS = 1000;  // lama hitung pulsa (1 detik)
const uint32_t BATAS_KILOHERTZ = 1000; // >= nilai ini tampil dalam kHz
const uint32_t BAUD_SERIAL = 115200;   // kecepatan Serial Monitor

// ---------- state ----------
volatile uint32_t hitungPulsa = 0;   // pulsa sejak gate terakhir (ISR)
uint32_t frekuensiHz = 0;            // frekuensi hasil gate terakhir
uint32_t waktuGateSebelumnya = 0;    // millis() saat gate terakhir dibuka

LiquidCrystal_I2C lcd(ALAMAT_LCD, KOLOM_LCD, BARIS_LCD);

// ---------- interupsi: satu pulsa tiap sisi naik ----------
void IRAM_ATTR hitungPulsaInterupsi() {
  hitungPulsa++;
}

// ---------- baca & nolkan pencacah secara atomik ----------
uint32_t bacaFrekuensiGate() {
  noInterrupts();
  uint32_t pulsa = hitungPulsa;
  hitungPulsa = 0;
  interrupts();
  // Gate 1 detik: pulsa per detik = frekuensi dalam hertz.
  return pulsa;
}

// ---------- format frekuensi: "123 Hz" atau "1.23 kHz" ----------
void formatFrekuensi(uint32_t frekuensi, char *buf, size_t ukuranBuf) {
  if (frekuensi >= BATAS_KILOHERTZ) {
    float kiloHertz = frekuensi / 1000.0f;
    snprintf(buf, ukuranBuf, "%.2f kHz", kiloHertz);
  } else {
    snprintf(buf, ukuranBuf, "%u Hz", frekuensi);
  }
}

// ---------- tampilkan frekuensi di baris atas LCD ----------
void tampilkanFrekuensi(uint32_t frekuensi) {
  char teksFrekuensi[12];
  formatFrekuensi(frekuensi, teksFrekuensi, sizeof(teksFrekuensi));
  lcd.setCursor(0, 0);
  lcd.print("FREQ: ");
  lcd.print(teksFrekuensi);
  lcd.print("     ");  // hapus sisa angka lama yang lebih panjang
}

// ---------- tampilkan periode di baris bawah LCD ----------
void tampilkanPeriode(uint32_t frekuensi) {
  lcd.setCursor(0, 1);
  if (frekuensi == 0) {
    lcd.print("Tidak ada sinyal ");
  } else {
    uint32_t periodeUs = 1000000UL / frekuensi;
    lcd.print("Periode: ");
    lcd.print(periodeUs);
    lcd.print(" us   ");
  }
}

// ---------- kirim hasil ke Serial Monitor ----------
void kirimSerial(uint32_t frekuensi) {
  char teksFrekuensi[12];
  formatFrekuensi(frekuensi, teksFrekuensi, sizeof(teksFrekuensi));
  Serial.print("Frekuensi: ");
  Serial.print(teksFrekuensi);
  if (frekuensi > 0) {
    Serial.print(" | Periode: ");
    Serial.print(1000000UL / frekuensi);
    Serial.println(" us");
  } else {
    Serial.println(" | (tidak ada sinyal)");
  }
}

void setup() {
  Serial.begin(BAUD_SERIAL);
  pinMode(PIN_SINYAL, INPUT);  // pull-down 10k eksternal ke GND

  Wire.begin(PIN_SDA_LCD, PIN_SCL_LCD);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Pengukur Frekuensi");
  lcd.setCursor(0, 1);
  lcd.print("ESP32 + LCD I2C");
  delay(1500);
  lcd.clear();

  attachInterrupt(digitalPinToInterrupt(PIN_SINYAL),
                  hitungPulsaInterupsi, RISING);
  waktuGateSebelumnya = millis();
  Serial.println("Pengukur frekuensi siap - beri sinyal 0-3,3 V ke GPIO34");
}

void loop() {
  if (millis() - waktuGateSebelumnya >= DURASI_GATE_MS) {
    frekuensiHz = bacaFrekuensiGate();
    tampilkanFrekuensi(frekuensiHz);
    tampilkanPeriode(frekuensiHz);
    kirimSerial(frekuensiHz);
    waktuGateSebelumnya = millis();
  }
}
