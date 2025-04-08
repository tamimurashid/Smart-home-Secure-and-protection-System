#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <time.h>
#include <secret.h>

// ==== Pins ====
#define DHTPIN     4
#define FLAME_PIN  32
#define SMOKE_PIN  34
#define RAIN_PIN   35
#define I2C_SDA    21
#define I2C_SCL    22
#define BUZZER     23
#define RESET_PIN  0

// ==== DHT Sensor ====
#define DHTTYPE    DHT22
DHT dht(DHTPIN, DHTTYPE);

// ==== LCD ====
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ==== Telegram Bot ====
// #define BOT_TOKEN "7982663980:AAFhpE02DBjD70e0FCjNQ2gAoc2otYXTDkE"
// #define CHAT_ID   "7982663980"

#define BOT_TOKEN TOKENS
#define CHAT_ID   ID

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// === Flags & Timing ===
bool sentAlert = false;
unsigned long lastCheck = 0;
const unsigned long checkInterval = 5000;

void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("⏳ Syncing time...");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("✅ Time synced!");
}

void testInternet() {
  Serial.print("🌐 Testing internet connection... ");
  WiFiClient client;
  if (client.connect("google.com", 80)) {
    Serial.println("✅ Internet OK!");
    client.stop();
  } else {
    Serial.println("❌ No Internet!");
  }
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

  if (digitalRead(RESET_PIN) == LOW) {
    WiFiManager wm;
    wm.resetSettings();
    Serial.println("WiFi settings reset");
    delay(1000);
  }

  WiFiManager wm;
  bool res = wm.autoConnect("Smart_System", "12345678");

  if (!res) {
    Serial.println("Failed to connect or portal timeout");
    lcd.setCursor(0, 1); lcd.print("WiFi Setup Failed!");
    delay(3000);
    ESP.restart();
  }

  Serial.println("✅ Connected to WiFi!");
  lcd.setCursor(0, 1); lcd.print(" WiFi Connected     ");
  delay(1000);

  secured_client.setInsecure(); // No cert validation
  syncTime();
  testInternet();
}

void handleTelegramMessages() {
  int newMessages = bot.getUpdates(bot.last_message_received + 1);
  while (newMessages) {
    for (int i = 0; i < newMessages; i++) {
      String chat_id = String(bot.messages[i].chat_id);
      String text = bot.messages[i].text;

      if (text == "/status") {
        float temp = dht.readTemperature();
        float hum = dht.readHumidity();
        int flame = analogRead(FLAME_PIN);
        int smoke = analogRead(SMOKE_PIN);
        int rain = analogRead(RAIN_PIN);

        String msg = "*📡 System Status*\n";
        msg += "🌡 Temp: " + String(temp) + "°C\n";
        msg += "💧 Humidity: " + String(hum) + "%\n";
        msg += "🔥 Flame: " + String(flame) + "\n";
        msg += "💨 Smoke: " + String(smoke) + "\n";
        msg += "🌧 Rain: " + String(rain);
        bot.sendMessage(chat_id, msg, "Markdown");
      }
    }
    newMessages = bot.getUpdates(bot.last_message_received + 1);
  }
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int flame = analogRead(FLAME_PIN);
  int smoke = analogRead(SMOKE_PIN);
  int rain = analogRead(RAIN_PIN);

  bool flameDetected = flame < 500;
  bool rainDetected = rain < 2000;

  digitalWrite(BUZZER, flameDetected ? HIGH : LOW);

  // Update LCD without clearing every loop
  lcd.setCursor(0, 0); lcd.print("T:"); lcd.print(temp); lcd.print("C H:"); lcd.print(hum); lcd.print("%  ");
  lcd.setCursor(0, 1); lcd.print("Flame: "); lcd.print(flameDetected ? "YES" : "NO "); lcd.print("    ");
  lcd.setCursor(0, 2); lcd.print("Smoke: "); lcd.print(smoke); lcd.print("     ");
  lcd.setCursor(0, 3); lcd.print("Rain: "); lcd.print(rain); lcd.print("     ");

  if (flameDetected && rainDetected && !sentAlert) {
    String alertMsg = "🚨 *Alert!*\n🔥 Flame & 🌧 Rain detected.\nCheck the environment.";
    bot.sendMessage(CHAT_ID, alertMsg, "Markdown");
    sentAlert = true;
  }

  if (!flameDetected && !rainDetected) {
    sentAlert = false;
  }

  if (millis() - lastCheck > checkInterval) {
    handleTelegramMessages();
    lastCheck = millis();
  }

  delay(2000);
}
