// [ru] AlashTelegram: управление светодиодом (или реле) командами /on, /off, /status. Команды принимаются ТОЛЬКО из вашего чата:
//      узнайте chat_id в примере Basic_ESP и впишите в MY_CHAT. Без этого любой, кто найдёт бота, сможет включать вашу лампу.
// [kk] AlashTelegram: жарықдиодты (немесе релені) /on, /off, /status командаларымен басқару. Командалар ТЕК сіздің чатыңыздан қабылданады:
//      chat_id-ді Basic_ESP мысалынан біліп, MY_CHAT-қа жазыңыз. Онсыз ботты тапқан кез келген адам шамыңызды қоса алады.
// [en] AlashTelegram: controlling an LED (or a relay) with the commands /on, /off, /status. Commands are accepted ONLY from your chat:
//      find the chat_id in the Basic_ESP example and put it into MY_CHAT. Without this anyone who finds the bot can switch your lamp on.
#include <AlashTelegram.h>
#if defined(ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>
#else
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#endif
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

const char *WIFI_NAME = "ИМЯ_СЕТИ";
const char *WIFI_PASS = "ПАРОЛЬ";
const char *TOKEN = "123456789:ВАШ_ТОКЕН";
const int64_t MY_CHAT = 123456789LL;                       // ваш chat_id (суффикс LL обязателен для больших чисел)

WiFiClientSecure net;
AlashTelegram bot(net, TOKEN);
bool ledOn = false;

void onMessage(const AlashTelegram::Message &m) {
  if (strcmp(m.text, "/on") == 0) { ledOn = true; bot.send(m.chatId, "Свет включён"); }
  else if (strcmp(m.text, "/off") == 0) { ledOn = false; bot.send(m.chatId, "Свет выключен"); }
  else if (strcmp(m.text, "/status") == 0) bot.send(m.chatId, ledOn ? "Свет включён" : "Свет выключен");
  else bot.send(m.chatId, "Команды: /on /off /status");
  digitalWrite(LED_BUILTIN, ledOn);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.begin(WIFI_NAME, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(300);
  net.setInsecure();
  bot.allowChat(MY_CHAT);                                  // только этот чат
  bot.onMessage(onMessage);
  bot.send(MY_CHAT, "Плата включилась");                   // уведомление при запуске
}

void loop() {
  bot.update();
}
