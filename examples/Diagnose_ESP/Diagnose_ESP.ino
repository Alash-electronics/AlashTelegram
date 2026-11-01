// AlashTelegram: бот не отвечает? Скетч проверит токен (спросит Telegram, как зовут бота) и подскажет, что не так
// (токен, Wi-Fi, блокировка Telegram в сети).
#include <AlashTelegram.h>
#if defined(ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>
#else
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#endif

const char *WIFI_NAME = "ИМЯ_СЕТИ";
const char *WIFI_PASS = "ПАРОЛЬ";
const char *TOKEN = "123456789:ВАШ_ТОКЕН";

WiFiClientSecure net;
AlashTelegram bot(net, TOKEN);

void setup() {
  Serial.begin(115200);
  delay(500);
  WiFi.begin(WIFI_NAME, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(300);
  Serial.println(WiFi.status() == WL_CONNECTED ? "Wi-Fi подключён" : "Wi-Fi НЕ подключён: сначала исправьте его");
  net.setInsecure();
  bot.setLanguage(AlashTelegram::LANG_RU);                 // LANG_KK: қазақша, LANG_EN: English
  bot.diagnose(Serial);
}

void loop() {}
