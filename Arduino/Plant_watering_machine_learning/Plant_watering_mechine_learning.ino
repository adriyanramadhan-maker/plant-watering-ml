
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>

// =====================================================
// WIFI ACCESS POINT + NTP
// =====================================================
const char* WIFI_SSID = "Halloguys";
const char* WIFI_PASSWORD = "1234567890";

// Wi-Fi router untuk sinkronisasi NTP
const char* ROUTER_SSID = "Kelompok 5";
const char* ROUTER_PASSWORD = "1234567890";

const char* NTP_SERVER_1 = "pool.ntp.org";
const char* NTP_SERVER_2 = "time.google.com";

const long GMT_OFFSET_SEC = 7 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

WebServer server(80);

// =====================================================
// PIN TFT
// =====================================================
#define TFT_SCK   12
#define TFT_MISO  13
#define TFT_MOSI  11
#define TFT_CS    10
#define TFT_DC     9
#define TFT_RST    8

// =====================================================
// PIN TOUCH
// =====================================================
#define TOUCH_CS   7
#define TOUCH_IRQ  6

// =====================================================
// PIN SENSOR
// =====================================================
#define SOIL_PIN      1
#define DHT_PIN       4
#define DS18B20_PIN   5
#define DHTTYPE       DHT11

// =====================================================
// SPI, TFT, TOUCH
// =====================================================
SPIClass spi(FSPI);
Adafruit_ST7789 tft(&spi, TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// =====================================================
// SENSOR DAN PENYIMPANAN
// =====================================================
DHT dht(DHT_PIN, DHTTYPE);
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
Preferences preferences;

// =====================================================
// WARNA RGB565
// =====================================================
#define COLOR_BG       0x1082
#define COLOR_CARD     0x2104
#define COLOR_HEADER   0x001F
#define COLOR_GREEN    0x07E0
#define COLOR_YELLOW   0xFFE0
#define COLOR_RED      0xF800
#define COLOR_WHITE    0xFFFF
#define COLOR_GRAY     0x8410
#define COLOR_CYAN     0x07FF
#define COLOR_DARK     0x0000

// =====================================================
// KONFIGURASI TANAMAN
// =====================================================
const char* plantName = "PAKCOY";

const uint16_t INITIAL_AGE_DAYS = 7;
const uint16_t VEGETATIVE_DAYS = 21;
const uint16_t HARVEST_TARGET_DAYS = 45;

// =====================================================
// VARIABEL SENSOR
// =====================================================
float soilMv = 0;
float airTemp = NAN;
float airHum = NAN;
float waterTemp = DEVICE_DISCONNECTED_C;

const unsigned long UPDATE_INTERVAL = 1500;
unsigned long lastUpdate = 0;

const int SAMPLES = 15;
const float EMA_ALPHA = 0.15;
float emaSoil = 0;

// =====================================================
// VARIABEL WAKTU TFT
// =====================================================
unsigned long lastClockUpdate = 0;
const unsigned long CLOCK_INTERVAL = 1000;

// =====================================================
// VARIABEL UMUR TANAMAN
// =====================================================
uint32_t plantAgeDays = INITIAL_AGE_DAYS;
uint32_t lastSavedAge = INITIAL_AGE_DAYS;
unsigned long dayTick = 0;

const unsigned long DAY_MS = 86400000UL;

// =====================================================
// WAKTU REAL-TIME WIB UNTUK WEB DAN CSV
// =====================================================
String getRealtime() {
  struct tm timeInfo;

  if (!getLocalTime(&timeInfo, 10)) {
    return "Belum tersinkronisasi";
  }

  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);

  return String(buffer);
}

// =====================================================
// JAM HEADER TFT
// =====================================================
String getClockTime() {
  struct tm timeInfo;

  if (!getLocalTime(&timeInfo, 10)) {
    return "--:--:--";
  }

  char buffer[9];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeInfo);

  return String(buffer);
}

// =====================================================
// DASHBOARD WEB + PENGUMPULAN DATASET ML
// =====================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Kebun Pintar - Kelompok 5</title>
<style>
*{box-sizing:border-box}
body{margin:0;font-family:Arial,sans-serif;background:#f1f5f9;color:#172033}
header{background:#14532d;color:white;padding:22px 16px}
header h1{margin:0 0 7px;font-size:24px}
header p{margin:0;font-size:14px}
main{max-width:1000px;margin:auto;padding:16px}
.status{padding:12px;background:white;border-radius:10px;margin-bottom:15px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:12px}
.card{background:white;border-radius:12px;padding:18px;box-shadow:0 2px 7px #0000000d}
.label{font-size:13px;color:#64748b}
.value{font-size:28px;font-weight:bold;margin-top:9px;color:#166534;overflow-wrap:anywhere}
.unit{font-size:13px;color:#64748b}
h2{font-size:18px;margin:24px 0 12px}
.controls{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:12px}
input,select,button{width:100%;padding:11px;border:1px solid #cbd5e1;border-radius:8px;font-size:14px}
button{border:0;cursor:pointer;font-weight:bold;margin-top:8px}
.start{background:#15803d;color:white}
.stop{background:#b91c1c;color:white}
.download{background:#1d4ed8;color:white}
.secondary{background:#e2e8f0;color:#172033}
.actions{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:8px;margin-top:14px}
.tablewrap{overflow-x:auto;background:white;border-radius:10px}
table{border-collapse:collapse;width:100%;font-size:12px}
th,td{padding:10px;border-bottom:1px solid #e2e8f0;text-align:left;white-space:nowrap}
th{background:#e2e8f0}
small{color:#64748b}
</style>
</head>
<body>
<header>
  <h1>KEBUN PINTAR</h1>
  <p>Monitoring Sensor dan Pengumpulan Dataset ML — Kelompok 5</p>
</header>

<main>
  <div class="status">
    <b>Status perangkat:</b>
    <span id="connection">Menghubungkan...</span><br>
    <small>ESP32 Access Point | 192.168.4.1</small>
  </div>

  <div class="status">
    <b>Waktu real-time WIB:</b>
    <span id="realtime">Menunggu sinkronisasi NTP...</span>
  </div>

  <div class="grid">
    <div class="card">
      <div class="label">Tegangan Sensor Tanah</div>
      <div class="value" id="soil">--</div>
      <div class="unit">mV (bukan persen kelembapan)</div>
    </div>
    <div class="card">
      <div class="label">Suhu Udara — DHT11</div>
      <div class="value" id="air">--</div>
      <div class="unit">°C</div>
    </div>
    <div class="card">
      <div class="label">Kelembapan Udara</div>
      <div class="value" id="humidity">--</div>
      <div class="unit">%</div>
    </div>
    <div class="card">
      <div class="label">Suhu Air — DS18B20</div>
      <div class="value" id="water">--</div>
      <div class="unit">°C</div>
    </div>
  </div>

  <h2>Pengaturan Pengumpulan Dataset ML</h2>
  <div class="controls">
    <div class="card">
      <label for="plant">Nama tanaman</label>
      <input id="plant" value="Pakcoy">
    </div>

    <div class="card">
      <label for="age">Umur tanaman (hari)</label>
      <input id="age" type="number" min="0" value="7">
    </div>

    <div class="card">
      <label for="interval">Interval rekam (ms)</label>
      <select id="interval">
        <option value="1500">1500 ms</option>
        <option value="2000">2000 ms</option>
        <option value="5000" selected>5000 ms</option>
        <option value="10000">10000 ms</option>
        <option value="60000">60000 ms (1 menit)</option>
      </select>
    </div>

    <div class="card">
      <label for="soil_condition">Kondisi tanah (soil_condition)</label>
      <select id="soil_condition">
        <option value="belum_diamati">Belum diamati</option>
        <option value="kering">Kering</option>
        <option value="cukup">Cukup</option>
        <option value="basah">Basah</option>
        <option value="lainnya">Lainnya</option>
      </select>
    </div>

    <div class="card">
      <label for="pump_target">Target penyiraman (pump_target)</label>
      <select id="pump_target">
        <option value="belum_ditentukan">Belum ditentukan</option>
        <option value="PERLU_DISIRAM">PERLU_DISIRAM</option>
        <option value="BELUM_PERLU_DISIRAM">BELUM_PERLU_DISIRAM</option>
      </select>
    </div>
  </div>

  <div class="actions">
    <button class="start" id="startBtn" onclick="startRecording()">Mulai Rekam</button>
    <button class="stop" onclick="stopRecording()">Berhenti</button>
    <button class="download" onclick="downloadCSV()">Unduh CSV</button>
    <button class="secondary" onclick="clearData()">Hapus Data</button>
  </div>

  <p>
    Status rekaman: <b id="recordStatus">Belum merekam</b><br>
    Jumlah baris dataset: <b id="count">0</b>
  </p>
  <small>
    Dataset tersimpan sementara di browser. Unduh CSV sebelum menutup atau
    menyegarkan halaman. Pilih label sesuai pengamatan nyata.
  </small>

  <h2>Pratinjau Dataset</h2>
  <div class="tablewrap">
    <table>
      <thead>
        <tr>
          <th>Waktu WIB</th>
          <th>Tanah (mV)</th>
          <th>Suhu Udara</th>
          <th>Kelembapan</th>
          <th>Suhu Air</th>
          <th>Tanaman</th>
          <th>Umur</th>
          <th>Soil Condition</th>
          <th>Pump Target</th>
        </tr>
      </thead>
      <tbody id="preview">
        <tr><td colspan="9">Belum ada data rekaman.</td></tr>
      </tbody>
    </table>
  </div>
</main>

<script>
let latest = null;
let rows = [];
let recording = false;
let lastRecordedAt = 0;

function display(id, value, digits=1) {
  document.getElementById(id).textContent =
    value === null || value === undefined || !Number.isFinite(Number(value))
      ? "--"
      : Number(value).toFixed(digits);
}

async function refreshData() {
  try {
    const response = await fetch("/data", {cache:"no-store"});
    if (!response.ok) throw new Error("HTTP " + response.status);

    latest = await response.json();

    display("soil", latest.soil_mV, 0);
    display("air", latest.air_C, 1);
    display("humidity", latest.humidity_pct, 1);
    display("water", latest.water_C, 2);

    document.getElementById("realtime").textContent =
      latest.realtime || "Belum tersinkronisasi";

    document.getElementById("connection").textContent = "Terhubung";
    document.getElementById("connection").style.color = "#15803d";

    const now = Date.now();
    const interval = Number(document.getElementById("interval").value);

    if (recording && now - lastRecordedAt >= interval) {
      recordRow();
      lastRecordedAt = now;
    }
  } catch (e) {
    document.getElementById("connection").textContent = "Koneksi terputus";
    document.getElementById("connection").style.color = "#b91c1c";
  }
}

function recordRow() {
  if (!latest) return;

  rows.push({
    time: latest.realtime || "Belum tersinkronisasi",
    soil: latest.soil_mV,
    air: latest.air_C,
    humidity: latest.humidity_pct,
    water: latest.water_C,
    plant: document.getElementById("plant").value.trim(),
    age: document.getElementById("age").value,
    soil_condition: document.getElementById("soil_condition").value,
    pump_target: document.getElementById("pump_target").value
  });

  document.getElementById("count").textContent = rows.length;
  updatePreview();
}

function startRecording() {
  if (!latest) {
    alert("Tunggu sampai ESP32 terhubung terlebih dahulu.");
    return;
  }

  recording = true;
  lastRecordedAt = 0;
  document.getElementById("recordStatus").textContent = "Sedang merekam";
  document.getElementById("startBtn").textContent = "Rekaman Aktif";
}

function stopRecording() {
  recording = false;
  document.getElementById("recordStatus").textContent = "Rekaman berhenti";
  document.getElementById("startBtn").textContent = "Mulai Rekam";
}

function csvValue(value) {
  if (value === null || value === undefined) return "";
  const s = String(value);
  return '"' + s.replace(/"/g, '""') + '"';
}

function downloadCSV() {
  if (!rows.length) {
    alert("Belum ada data. Tekan Mulai Rekam terlebih dahulu.");
    return;
  }

  const header = [
    "timestamp_WIB",
    "soil_mV",
    "air_C",
    "humidity_pct",
    "water_C",
    "plant",
    "age_days",
    "soil_condition",
    "pump_target"
  ];

  const lines = [header.join(",")];

  for (const r of rows) {
    lines.push([
      r.time,
      r.soil,
      r.air,
      r.humidity,
      r.water,
      r.plant,
      r.age,
      r.soil_condition,
      r.pump_target
    ].map(csvValue).join(","));
  }

  const blob = new Blob(["\uFEFF" + lines.join("\r\n")], {
    type:"text/csv;charset=utf-8;"
  });

  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = "dataset_kebun_pintar.csv";
  a.click();

  setTimeout(() => URL.revokeObjectURL(url), 1000);
}

function clearData() {
  if (!confirm("Hapus semua data rekaman di halaman ini?")) return;

  rows = [];
  document.getElementById("count").textContent = "0";
  updatePreview();
}

function updatePreview() {
  const body = document.getElementById("preview");
  body.innerHTML = "";

  if (!rows.length) {
    body.innerHTML =
      '<tr><td colspan="9">Belum ada data rekaman.</td></tr>';
    return;
  }

  for (const r of rows.slice(-10).reverse()) {
    const tr = document.createElement("tr");
    const vals = [
      r.time, r.soil, r.air, r.humidity, r.water,
      r.plant, r.age, r.soil_condition, r.pump_target
    ];

    for (const v of vals) {
      const td = document.createElement("td");
      td.textContent = v === null || v === undefined ? "" : v;
      tr.appendChild(td);
    }

    body.appendChild(tr);
  }
}

refreshData();
setInterval(refreshData, 1500);
</script>
</body>
</html>
)rawliteral";

// =====================================================
// PROTOTIPE FUNGSI
// =====================================================
float readSoilMedian();
void readSensors();
void updatePlantAge();
void drawHeader();
void updateClockDisplay();
void drawCards();
void updateDisplay();
void handleRoot();
void handleData();

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // TFT
  spi.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.init(240, 320);
  tft.setRotation(1);
  tft.fillScreen(COLOR_BG);
  touch.begin(spi);

  // Sensor
  analogReadResolution(12);
  dht.begin();
  ds18b20.begin();

  // Umur tanaman
  preferences.begin("kebunpintar", false);
  plantAgeDays = preferences.getUInt("umur", INITIAL_AGE_DAYS);
  lastSavedAge = plantAgeDays;
  dayTick = millis();

  emaSoil = readSoilMedian();

  drawHeader();
  drawCards();
  readSensors();
  updateDisplay();

  // ===================================================
  // WIFI AP + STA
  // ===================================================
  WiFi.mode(WIFI_AP_STA);

  bool apStarted = WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

  if (apStarted) {
    Serial.println();
    Serial.println("Wi-Fi Access Point aktif");
    Serial.print("SSID: ");
    Serial.println(WIFI_SSID);
    Serial.print("IP dashboard: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Gagal membuat Wi-Fi Access Point!");
  }

  if (strlen(ROUTER_SSID) > 0 &&
      strcmp(ROUTER_SSID, "NAMA_WIFI_ROUTER") != 0) {

    WiFi.begin(ROUTER_SSID, ROUTER_PASSWORD);
    Serial.println("Menghubungkan ke router untuk NTP...");

    unsigned long startAttempt = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttempt < 10000) {
      delay(250);
    }

    if (WiFi.status() == WL_CONNECTED) {
      configTime(
        GMT_OFFSET_SEC,
        DAYLIGHT_OFFSET_SEC,
        NTP_SERVER_1,
        NTP_SERVER_2
      );

      Serial.println("Router terhubung; sinkronisasi NTP dimulai.");
      Serial.print("IP router: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("Router tidak tersedia; NTP belum tersinkronisasi.");
    }
  } else {
    Serial.println("Atur SSID router agar waktu NTP dapat disinkronkan.");
  }

  // Web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.begin();

  Serial.println("Web server berjalan.");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  server.handleClient();

  unsigned long now = millis();

  // Pembacaan sensor setiap 1,5 detik
  if (now - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = now;
    readSensors();
    updateDisplay();
  }

  // Jam header TFT diperbarui setiap 1 detik
  if (now - lastClockUpdate >= CLOCK_INTERVAL) {
    lastClockUpdate = now;
    updateClockDisplay();
  }

  updatePlantAge();
}

// =====================================================
// HALAMAN DASHBOARD
// =====================================================
void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

// =====================================================
// KIRIM DATA SENSOR + WAKTU NTP DALAM JSON
// Perhatikan: escape JSON harus memakai \" bukan \\"
// =====================================================
void handleData() {
  String json = "{";

  json += "\"soil_mV\":";
  json += String(soilMv, 0);

  json += ",\"air_C\":";
  if (isfinite(airTemp)) {
    json += String(airTemp, 2);
  } else {
    json += "null";
  }

  json += ",\"humidity_pct\":";
  if (isfinite(airHum)) {
    json += String(airHum, 2);
  } else {
    json += "null";
  }

  json += ",\"water_C\":";
  if (waterTemp != DEVICE_DISCONNECTED_C &&
      isfinite(waterTemp) &&
      waterTemp != 85.0) {
    json += String(waterTemp, 2);
  } else {
    json += "null";
  }

  json += ",\"plant\":\"";
  json += plantName;
  json += "\",\"age_days\":";
  json += String(plantAgeDays);

  json += ",\"realtime\":\"";
  json += getRealtime();
  json += "\"";

  json += "}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// =====================================================
// UPDATE UMUR TANAMAN
// =====================================================
void updatePlantAge() {
  unsigned long now = millis();

  if (now - dayTick >= DAY_MS) {
    uint32_t elapsedDays = (now - dayTick) / DAY_MS;

    plantAgeDays += elapsedDays;
    dayTick += elapsedDays * DAY_MS;

    if (plantAgeDays != lastSavedAge) {
      preferences.putUInt("umur", plantAgeDays);
      lastSavedAge = plantAgeDays;
    }

    updateDisplay();
  }
}

// =====================================================
// BACA SENSOR
// =====================================================
void readSensors() {
  float raw = readSoilMedian();

  emaSoil = (EMA_ALPHA * raw) +
            ((1.0 - EMA_ALPHA) * emaSoil);
  soilMv = emaSoil;

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t)) airTemp = t;
  if (!isnan(h)) airHum = h;

  ds18b20.requestTemperatures();
  float wt = ds18b20.getTempCByIndex(0);

  if (wt != DEVICE_DISCONNECTED_C &&
      wt != 85.0 &&
      wt >= -55.0 &&
      wt <= 125.0) {
    waterTemp = wt;
  }
}

// =====================================================
// MEDIAN FILTER SOIL
// =====================================================
float readSoilMedian() {
  int buffer[SAMPLES];

  for (int i = 0; i < SAMPLES; i++) {
    buffer[i] = analogReadMilliVolts(SOIL_PIN);
    delayMicroseconds(300);
  }

  for (int i = 1; i < SAMPLES; i++) {
    int key = buffer[i];
    int j = i - 1;

    while (j >= 0 && buffer[j] > key) {
      buffer[j + 1] = buffer[j];
      j--;
    }

    buffer[j + 1] = key;
  }

  return buffer[SAMPLES / 2];
}

// =====================================================
// HEADER TFT
// KEBUN PINTAR | HH:MM:SS | ONLINE
// =====================================================
void drawHeader() {
  tft.fillRect(0, 0, 320, 34, COLOR_HEADER);

  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 9);
  tft.print("KEBUN PINTAR");

  tft.setTextSize(1);
  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(154, 13);
  tft.print("|");

  updateClockDisplay();

  tft.fillCircle(270, 17, 4, COLOR_GREEN);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(278, 13);
  tft.print("ONLINE");
}

// =====================================================
// UPDATE JAM SAJA
// =====================================================
void updateClockDisplay() {
  tft.fillRect(164, 7, 101, 20, COLOR_HEADER);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(166, 13);
  tft.print(getClockTime());
}

// =====================================================
// KERANGKA DUA PANEL TFT
// =====================================================
void drawCards() {
  tft.fillRoundRect(5, 40, 153, 194, 7, COLOR_CARD);
  tft.fillRoundRect(163, 40, 152, 194, 7, COLOR_CARD);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_WHITE);

  tft.setCursor(12, 47);
  tft.print("PEMBACAAN SENSOR");

  tft.setCursor(171, 47);
  tft.print("TANAMAN SAYA");

  tft.drawFastHLine(12, 60, 139, COLOR_GRAY);
  tft.drawFastHLine(170, 60, 137, COLOR_GRAY);

  tft.setTextColor(COLOR_GRAY);

  tft.setCursor(12, 66);
  tft.print("KELEMBAPAN TANAH");

  tft.setCursor(12, 107);
  tft.print("SUHU UDARA (DHT11)");

  tft.setCursor(12, 141);
  tft.print("KELEMBAPAN UDARA");

  tft.setCursor(12, 175);
  tft.print("SUHU AIR (DS18B20)");

  tft.setCursor(171, 67);
  tft.print("UMUR TANAMAN");

  tft.setCursor(171, 100);
  tft.print("FASE PERTUMBUHAN");

  tft.setCursor(171, 145);
  tft.print("COOLDOWN FASE");

  tft.setCursor(171, 180);
  tft.print("TARGET PANEN");

  // Posisi tulisan tetap seperti TFT awal
  tft.setCursor(171, 207);
  tft.print("PREDIKSI ML");
}

// =====================================================
// UPDATE TAMPILAN TFT
// =====================================================
void updateDisplay() {
  String soilStatus;
  uint16_t soilColor;

  if (soilMv > 2400) {
    soilStatus = "KERING";
    soilColor = COLOR_RED;
  } else if (soilMv > 1800) {
    soilStatus = "CUKUP";
    soilColor = COLOR_YELLOW;
  } else {
    soilStatus = "BASAH";
    soilColor = COLOR_GREEN;
  }

  tft.fillRect(12, 77, 139, 25, COLOR_CARD);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(12, 80);
  tft.printf("%.0f", soilMv);

  tft.setTextSize(1);
  tft.setTextColor(soilColor);
  tft.setCursor(91, 86);
  tft.print(soilStatus);

  tft.drawFastHLine(12, 104, 139, COLOR_GRAY);

  tft.fillRect(12, 118, 139, 20, COLOR_CARD);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(12, 120);

  if (isnan(airTemp)) tft.print("--.- C");
  else tft.printf("%.1f C", airTemp);

  tft.fillRect(12, 151, 139, 20, COLOR_CARD);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(12, 154);

  if (isnan(airHum)) tft.print("-- %");
  else tft.printf("%.0f %%", airHum);

  tft.fillRect(12, 185, 139, 20, COLOR_CARD);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(12, 188);

  if (waterTemp == DEVICE_DISCONNECTED_C) tft.print("--.- C");
  else tft.printf("%.1f C", waterTemp);

  // Informasi umur tanaman
  tft.fillRect(171, 64, 137, 32, COLOR_CARD);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(171, 78);
  tft.printf("%lu hari", (unsigned long)plantAgeDays);

  // Fase pertumbuhan
  const char* phaseName;
  uint16_t phaseColor;

  if (plantAgeDays <= VEGETATIVE_DAYS) {
    phaseName = "Vegetatif";
    phaseColor = COLOR_GREEN;
  } else {
    phaseName = "Lanjut";
    phaseColor = COLOR_CYAN;
  }

  tft.fillRect(171, 111, 137, 20, COLOR_CARD);
  tft.setTextSize(2);
  tft.setTextColor(phaseColor);
  tft.setCursor(171, 113);
  tft.print(phaseName);

  // Sisa hari fase vegetatif
  tft.fillRect(171, 156, 137, 19, COLOR_CARD);

  uint32_t remainingDays = 0;
  if (plantAgeDays < VEGETATIVE_DAYS) {
    remainingDays = VEGETATIVE_DAYS - plantAgeDays;
  }

  tft.setTextSize(2);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(171, 157);
  tft.printf("%lu hari", (unsigned long)remainingDays);

  // Progress bar
  tft.fillRect(171, 174, 137, 4, COLOR_GRAY);

  uint32_t phaseProgress = plantAgeDays;
  if (phaseProgress > VEGETATIVE_DAYS) {
    phaseProgress = VEGETATIVE_DAYS;
  }

  int barWidth = (phaseProgress * 137) / VEGETATIVE_DAYS;
  tft.fillRect(171, 174, barWidth, 4, COLOR_GREEN);

  // Target panen
  tft.fillRect(171, 190, 137, 15, COLOR_CARD);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(171, 193);

  if (plantAgeDays >= HARVEST_TARGET_DAYS) {
    tft.print("Target umur tercapai");
  } else {
    tft.printf(
      "Sisa %lu hari",
      (unsigned long)(HARVEST_TARGET_DAYS - plantAgeDays)
    );
  }

  // Prediksi ML belum aktif sebelum model terlatih diintegrasikan.
  tft.fillRect(171, 218, 137, 12, COLOR_CARD);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(171, 220);
  tft.print("MENUNGGU MODEL");
}
