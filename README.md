# Pengukur Frekuensi Sinyal — ESP32 + LCD I2C

Osiloskop mahal, padahal frekuensi sinyal bisa diukur alat sekecil ini?
Alat ini menghitung berapa kali sinyal naik-turun tiap detik, lalu
menampilkan angkanya di LCD 16x2: frekuensi dalam Hz (otomatis jadi kHz
bila di atas 1000 Hz) beserta periodenya dalam mikrodetik. Cocok buat
mengecek osilator NE555, keluaran PWM, generator fungsi, atau sensor
berpulsa — tanpa osiloskop.

Video penjelasan (60 detik):
link menyusul setelah upload

## Komponen

| Bagian | Keterangan |
|---|---|
| ESP32 DevKit | Otak alat, menghitung pulsa lewat interupsi |
| LCD 16x2 I2C (0x27) | Tampil frekuensi + periode |
| Generator sinyal / NE555 | Sumber gelombang kotak 0–3,3 V |
| Resistor 10k | Pull-down pin sinyal ke GND |
| Adaptor USB 5V 2A | Catu daya |

## Wiring

| Dari | Ke |
|---|---|
| Generator OUT | ESP32 GPIO34 (interupsi RISING) |
| Resistor 10k | GPIO34 ke GND (pull-down) |
| LCD SDA | ESP32 GPIO21 |
| LCD SCL | ESP32 GPIO22 |
| LCD VCC | 5V |
| LCD GND | GND |
| Adaptor USB 5V | ESP32 VIN |
| Semua GND | Digabung |

Skema lengkap: [schematic.png](schematic.png)

## Cara kerja

Setiap sisi naik sinyal di GPIO34 membangkitkan interupsi yang menambah
pencacah pulsa. Tiap 1 detik (gate), ESP32 membaca pencacah itu dan
menolkannya — karena gate-nya tepat 1 detik, jumlah pulsa per detik
langsung sama dengan frekuensi dalam hertz, dengan resolusi 1 Hz.
Periode dihitung dari frekuensi: periode (mikrodetik) = 1.000.000 /
frekuensi. Hasil tampil di LCD dan dikirim juga ke Serial Monitor
(115200 baud).

```
frekuensi (Hz) = pulsa yang terhitung selama gate 1 detik
periode (us)   = 1.000.000 / frekuensi
```

Contoh angka: sinyal 440 Hz tampil "440 Hz" dengan periode 2272 us;
sinyal 1234 Hz tampil "1.23 kHz" dengan periode 810 us; tanpa sinyal,
LCD menampilkan "Tidak ada sinyal".

## Batas ukur

| Hal | Nilai |
|---|---|
| Resolusi | 1 Hz (gate 1 detik) |
| Tampil Hz | 1–999 Hz |
| Tampil kHz | 1000 Hz ke atas (2 desimal) |
| Bentuk sinyal | Gelombang kotak, 0–3,3 V |
| Pin masuk | GPIO34 (hanya input, interupsi RISING) |

Perhatian: JANGAN beri sinyal 5 V langsung ke GPIO34 — pin ESP32 tidak
tahan 5 V. Turunkan dulu dengan pembagi tegangan (mis. resistor 10k seri
+ 10k ke GND) atau pakai sumber sinyal 3,3 V. Sinyal sinus atau sinyal
yang naiknya landai bisa terhitung ganda — bentuk dulu jadi kotak dengan
komparator atau gerbang Schmitt bila perlu.

## Konfigurasi di sketch

| Konstanta | Nilai bawaan | Arti |
|---|---|---|
| `PIN_SINYAL` | 34 | Masukkan sinyal (interupsi RISING) |
| `PIN_SDA_LCD` | 21 | SDA bus I2C |
| `PIN_SCL_LCD` | 22 | SCL bus I2C |
| `ALAMAT_LCD` | 0x27 | Alamat I2C LCD 16x2 |
| `DURASI_GATE_MS` | 1000 | Lama hitung pulsa (1 detik) |
| `BATAS_KILOHERTZ` | 1000 | Di atas ini tampil dalam kHz |
| `BAUD_SERIAL` | 115200 | Kecepatan Serial Monitor |

## Compile (terverifikasi)

- Board: ESP32 DevKit (esp32:esp32:esp32), ESP32 core 4.0.0, arduino-cli 1.6.0
- Sketch: 318.293 byte (24%) dari 1.310.720 byte flash
- RAM: 25.112 byte (7%) dari 327.680 byte
- Library: LiquidCrystal_I2C 1.1.2 (lihat folder `libraries/`)
