#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// ==== Pins ====
#define DHTPIN         4
#define FLAME_PIN      32
#define SMOKE_PIN      34
#define RAIN_PIN       35
#define I2C_SDA        21
#define I2C_SCL        22
#define BUZZER         23

// ==== DHT Sensor ====
#define DHTTYPE        DHT22
DHT dht(DHTPIN, DHTTYPE);

// ==== LCD ====
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ==== Telegram Config ====
#define BOT_TOKEN     "7982663980:AAFhpE02DBjD70e0FCjNQ2gAoc2otYXTDkE"
#define CHAT_ID       "YOUR_CHAT_ID_HERE"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// Alert flag
bool sentAlert = false;

// Telegram polling interval
unsigned long lastCheck = 0;
const unsigned long checkInterval = 2000;

void setup() {
  Serial.begin(9600);

  // Start LCD and I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" Smart Weather Sys ");

  // Start DHT
  dht.begin();

  // Set pin modes
  pinMode(BUZZER, OUTPUT);

  // ==== WiFiManager ====
  WiFiManager wm;
  wm.autoConnect("SmartWeatherAP");
  Serial.println("WiFi connected!");

  // HTTPS cert for Telegram
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
}

// === Telegram message handler ===
void handleTelegramMessages() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  while (numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
      String chat_id = String(bot.messages[i].chat_id);
      String text = bot.messages[i].text;

      if (text == "/status" || text == "status") {
        float temp = dht.readTemperature();
        float hum = dht.readHumidity();
        int flame = analogRead(FLAME_PIN);
        int smoke = analogRead(SMOKE_PIN);
        int rain = analogRead(RAIN_PIN);

        String statusMsg = "*📊 System Status:*\n";
        statusMsg += "🌡️ Temp: " + String(temp) + "°C\n";
        statusMsg += "💧 Humidity: " + String(hum) + "%\n";
        statusMsg += "🔥 Flame: " + String(flame) + "\n";
        statusMsg += "💨 Smoke: " + String(smoke) + "\n";
        statusMsg += "🌧️ Rain: " + String(rain) + "\n";

        bot.sendMessage(chat_id, statusMsg, "Markdown");
      }
    }
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
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

  // LCD display
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("T:"); lcd.print(temp); lcd.print("C H:"); lcd.print(hum); lcd.print("%");
  lcd.setCursor(0, 1); lcd.print("Flame:"); lcd.print(flameDetected ? "YES" : "NO");
  lcd.setCursor(0, 2); lcd.print("Smoke:"); lcd.print(smoke);
  lcd.setCursor(0, 3); lcd.print("Rain:"); lcd.print(rain);

  // Telegram auto-alert
  if (flameDetected && rainDetected && !sentAlert) {
    String message = "🚨 *ALERT!*\n🔥 Flame detected!\n🌧️ Rainfall detected too!\nPlease check the system.";
    bot.sendMessage(CHAT_ID, message, "Markdown");
    sentAlert = true;
  }

  if (!flameDetected && !rainDetected) {
    sentAlert = false;
  }

  // Check for Telegram commands
  if (millis() - lastCheck > checkInterval) {
    handleTelegramMessages();
    lastCheck = millis();
  }

  delay(2000);
}
