#define BLYNK_TEMPLATE_ID "TMPL2VEcI8xU6"
#define BLYNK_TEMPLATE_NAME "AmoreSystem"
#define BLYNK_AUTH_TOKEN "6B0_1ZHuKtm9LiVLaL1XWhXJvJ2I5ApE"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <BlynkSimpleEsp32.h>

// Pins
#define DHTPIN     4
#define FLAME_PIN  32
#define SMOKE_PIN  34
#define RAIN_PIN   35
#define I2C_SDA    21
#define I2C_SCL    22
#define BUZZER     23
#define RESET_PIN  0

// DHT
#define DHTTYPE    DHT22
DHT dht(DHTPIN, DHTTYPE);

// LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Blynk Auth
char auth[] = BLYNK_AUTH_TOKEN;
BlynkTimer timer;
bool sentAlert = false;

void sendToBlynk() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int flame = analogRead(FLAME_PIN);
  int smoke = analogRead(SMOKE_PIN);
  int rain = analogRead(RAIN_PIN);

  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, hum);
  Blynk.virtualWrite(V2, flame);
  Blynk.virtualWrite(V3, smoke);
  Blynk.virtualWrite(V4, rain);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("--AMORE-SYSTEM---------");

  dht.begin();
  pinMode(BUZZER, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);

  // Reset WiFi settings if button held
  if (digitalRead(RESET_PIN) == LOW) {
    WiFiManager wm;
    wm.resetSettings();
    Serial.println("WiFi settings reset");
    lcd.setCursor(0, 1); lcd.print("WiFi Resetting...");
    delay(2000);
  }

  // WiFi Setup using WiFiManager
  WiFiManager wm;
  lcd.setCursor(0, 1); lcd.print("Connecting WiFi...");
  bool res = wm.autoConnect("Smart_System", "12345678");

  if (!res) {
    Serial.println("❌ WiFi connection failed");
    lcd.setCursor(0, 1); lcd.print("WiFi Connect Failed");
    delay(3000);
    ESP.restart();
  }

  Serial.println("✅ WiFi connected");
  lcd.setCursor(0, 1); lcd.print(" WiFi Connected     ");
  delay(1000);

  Blynk.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str());
  timer.setInterval(5000L, sendToBlynk);
}

void loop() {
  Blynk.run();
  timer.run();

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int flame = analogRead(FLAME_PIN);
  int smoke = analogRead(SMOKE_PIN);
  int rain = analogRead(RAIN_PIN);

  bool flameDetected = flame < 500;
  bool rainDetected = rain < 2000;

  digitalWrite(BUZZER, flameDetected ? HIGH : LOW);

  lcd.setCursor(0, 0); lcd.print("T:"); lcd.print(temp); lcd.print("C H:"); lcd.print(hum); lcd.print("%  ");
  lcd.setCursor(0, 1); lcd.print("Flame: "); lcd.print(flameDetected ? "YES" : "NO "); lcd.print("    ");
  lcd.setCursor(0, 2); lcd.print("Smoke: "); lcd.print(smoke); lcd.print("     ");
  lcd.setCursor(0, 3); lcd.print("Rain: "); lcd.print(rain); lcd.print("     ");

  // Optional alert logic (can be expanded to Blynk alerts or notification widget)
  if (flameDetected && rainDetected && !sentAlert) {
    Serial.println("🔥 Rain + Flame detected!");
    sentAlert = true;
  }

  if (!flameDetected && !rainDetected) {
    sentAlert = false;
  }

  delay(2000);
}
