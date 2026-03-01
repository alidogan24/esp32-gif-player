#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

#define SD_CS    5
#define SD_SCK   14
#define SD_MISO  19
#define SD_MOSI  13

#define TFT_BL    4
#define BTN_NEXT  15

TFT_eSPI tft = TFT_eSPI();
SPIClass spiSD(HSPI);

const int W = 240;
const int H = 240;

#define CHUNK_LINES 20
uint8_t chunkBuf[W * CHUNK_LINES * 2];

String binFiles[32];
int totalBins = 0;
int currentBin = 0;

// -------------------------
// BUTON FONKSİYON: Basılı bırakma algılama
// -------------------------
bool wasPressed = false;

bool nextPressed() {
  bool pressed = (digitalRead(BTN_NEXT) == LOW);

  if (!pressed && wasPressed) {
    wasPressed = false;
    return true;          // buton bırakıldı → sonraki
  }

  wasPressed = pressed;
  return false;
}

// ============================
// SD karttan .bin dosyalarını bul
// ============================
void scanBinFiles() {
  File root = SD.open("/");

  while (true) {
    File f = root.openNextFile();
    if (!f) break;

    String name = f.name();
    name.toLowerCase();

    if (name.endsWith(".bin")) {
      binFiles[totalBins++] = "/" + String(f.name());
      Serial.printf("📁 Bulundu: %s\n", f.name());
    }
    f.close();
  }

  if (totalBins == 0) {
    Serial.println("❌ SD kartta .bin dosyası yok!");

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(20, 110);
    tft.print("SD'de .bin yok!");

    while (1);
  }

  Serial.printf("✅ Toplam %d adet .bin bulundu\n", totalBins);
}

// ============================
// Bir animasyonu oynat
// ============================
void playBin(const char *path) {
  File f = SD.open(path);
  if (!f) {
    Serial.printf("❌ Dosya açılamadı: %s\n", path);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(20, 110);
    tft.print("Dosya acilamadi!");

    delay(1000);
    return;
  }

  int frameSize = W * H * 2;
  int totalFrames = f.size() / frameSize;

  while (true) {
    for (int frame = 0; frame < totalFrames; frame++) {

      for (int y = 0; y < H; y += CHUNK_LINES) {
        int readLines = CHUNK_LINES;
        if (y + readLines > H) readLines = H - y;

        uint32_t offset = frame * frameSize + (y * W * 2);
        f.seek(offset);
        f.read(chunkBuf, readLines * W * 2);

        tft.pushImage(0, y, W, readLines, (uint16_t*)chunkBuf);

        if (nextPressed()) {
          f.close();
          return;   // sadece RELEASE olduğunda sonraki
        }
      }

      delay(10);
    }
  }

  f.close();
}

// ============================
// SETUP
// ============================
void setup() {
  Serial.begin(115200);
  delay(400);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  pinMode(BTN_NEXT, INPUT_PULLUP);

  tft.init();
  tft.setRotation(3);
  tft.setSwapBytes(!tft.getSwapBytes());
  tft.fillScreen(TFT_BLACK);

  // -------------------------
  // BAŞLANGIÇ EKRANI
  // -------------------------
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(40, 110);
  tft.print("Baslatiliyor...");
  delay(1500);

  spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, spiSD, 15000000)) {
    Serial.println("❌ SD baslatilamadi!");

    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED);
    tft.setCursor(20, 110);
    tft.print("SD baslatilamadi!");

    while (1);
  }

  scanBinFiles();
}

// ============================
// LOOP
// ============================
void loop() {
  playBin(binFiles[currentBin].c_str());
  currentBin = (currentBin + 1) % totalBins;
}
