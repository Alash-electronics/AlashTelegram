// [ru] AlashTelegram: самый простой пример. Бот отвечает на любое сообщение тем же текстом и называет ваш chat_id.
//
//      Как получить бота: в Telegram откройте @BotFather, команда /newbot, придумайте имя; он пришлёт токен вида 123456789:AAH...
//      Впишите токен и данные Wi-Fi, загрузите скетч, напишите боту. Wi-Fi только 2.4 ГГц.
// [kk] AlashTelegram: ең қарапайым мысал. Бот кез келген хабарламаға сол мәтінмен жауап береді және сіздің chat_id-ңызды атайды.
//
//      Ботты қалай алу: Telegram-да @BotFather ашыңыз, /newbot командасы, ат ойлап табыңыз; ол 123456789:AAH... түріндегі токен жібереді.
//      Токен мен Wi-Fi деректерін жазып, скетчті жүктеңіз, ботқа жазыңыз. Тек 2.4 ГГц Wi-Fi.
// [en] AlashTelegram: the simplest example. The bot answers any message with the same text and tells your chat_id.
//
//      How to get a bot: in Telegram open @BotFather, the /newbot command, invent a name; it sends a token like 123456789:AAH...
//      Put in the token and the Wi-Fi details, upload the sketch, write to the bot. 2.4 GHz Wi-Fi only.
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

void onMessage(const AlashTelegram::Message &m) {
  Serial.print(m.from);
  Serial.print(": ");
  Serial.println(m.text);
  char id[24], reply[200];
  AlashTelegram::idToString(m.chatId, id);                // chat_id бывает больше 2 миллиардов: обычным %ld на ESP он обрежется
  snprintf(reply, sizeof reply, "Вы написали: %s\nВаш chat_id: %s", m.text, id);
  bot.send(m.chatId, reply);                              // ответить в тот же чат
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_NAME, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(300);
  net.setInsecure();                                      // не проверять сертификат Telegram (проще для уроков; для серьёзных проектов добавьте корневой сертификат)
  bot.onMessage(onMessage);
}

void loop() {
  bot.update();                                           // раз в 2 секунды спрашивает Telegram о новых сообщениях
  if (bot.error() != AlashTelegram::ERR_OK) {
    static uint32_t last = 0;
    if (millis() - last > 10000) { last = millis(); Serial.println(bot.errorText()); }
  }
}
