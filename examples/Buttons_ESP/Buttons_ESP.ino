// [ru] AlashTelegram: кнопки в чате. Команда /menu показывает сообщение с кнопками «Включить» и «Выключить»; нажатия приходят в тот же обработчик
//      (m.isCallback, данные в m.data). answerCallback() убирает «часики» с нажатой кнопки и может показать короткое уведомление.
// [kk] AlashTelegram: чаттағы батырмалар. /menu командасы «Қосу» және «Өшіру» батырмалары бар хабарлама көрсетеді; басулар сол өңдеушіге келеді
//      (m.isCallback, деректер m.data-да). answerCallback() басылған батырмадан «сағатты» алады және қысқа хабарландыру көрсете алады.
// [en] AlashTelegram: buttons in the chat. The /menu command shows a message with the buttons "On" and "Off"; the presses arrive in the same handler
//      (m.isCallback, the data in m.data). answerCallback() removes the "clock" from the pressed button and can show a short notification.
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

void showMenu(int64_t chat) {
  // "Подпись:данные", кнопки в одном ряду через |, ряды через ;
  bot.sendButtons(chat, "Что сделать со светом?", "Включить:led_on|Выключить:led_off;Узнать состояние:status");
}

void onMessage(const AlashTelegram::Message &m) {
  if (m.isCallback) {                                      // нажата кнопка
    if (strcmp(m.data, "led_on") == 0) { digitalWrite(LED_BUILTIN, HIGH); bot.answerCallback(m, "Включено"); }
    else if (strcmp(m.data, "led_off") == 0) { digitalWrite(LED_BUILTIN, LOW); bot.answerCallback(m, "Выключено"); }
    else if (strcmp(m.data, "status") == 0) bot.answerCallback(m, digitalRead(LED_BUILTIN) ? "Горит" : "Не горит");
    return;
  }
  showMenu(m.chatId);                                      // на любое сообщение показываем меню
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.begin(WIFI_NAME, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(300);
  net.setInsecure();
  bot.allowChat(MY_CHAT);
  bot.onMessage(onMessage);
  showMenu(MY_CHAT);
}

void loop() {
  bot.update();
}
