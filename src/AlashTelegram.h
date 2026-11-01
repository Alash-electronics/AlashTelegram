// AlashTelegram: Telegram-бот на ESP32 и ESP8266: принять команду, отправить сообщение или кнопки. Только разрешённые чаты,
// понятные причины ошибок. Без ArduinoJson и без UniversalTelegramBot. MIT License. Alash Electronics, https://github.com/Alash-electronics/AlashTelegram
#ifndef ALASH_TELEGRAM_H
#define ALASH_TELEGRAM_H

#include <Arduino.h>
#include <Client.h>

class AlashTelegram {
 public:
  // Входящее событие: сообщение или нажатая кнопка
  struct Message {
    int64_t chatId;            // куда отвечать (у групп отрицательный)
    char text[160];            // текст сообщения ("/start", "включи свет"); для кнопки пусто
    char from[32];             // имя отправителя
    char data[48];             // для кнопки: её callback_data; для сообщения пусто
    char callbackId[24];       // для кнопки: идентификатор (нужен answerCallback)
    bool isCallback;
  };
  typedef void (*Handler)(const Message &m);

  enum Error : uint8_t {
    ERR_OK = 0,
    ERR_NO_CONNECTION,      // нет связи с api.telegram.org (Wi-Fi, интернет, DNS, сеть блокирует Telegram)
    ERR_BAD_TOKEN,          // токен неверный (ответ 401 / 404)
    ERR_FORBIDDEN,          // бот заблокирован пользователем или не добавлен в группу (403)
    ERR_TOO_MANY,           // слишком частые запросы (429): подождите
    ERR_BAD_REQUEST,        // Telegram не принял запрос (400): например, пустой текст или неверный chat_id
    ERR_BAD_RESPONSE,       // ответ не разобрался
    ERR_TOO_BIG             // сообщение не помещается в буфер
  };
  enum Language : uint8_t { LANG_RU = 0, LANG_KK = 1, LANG_EN = 2 };

  // client: защищённое соединение (WiFiClientSecure с setInsecure() или сертификатом); token: из @BotFather
  AlashTelegram(Client &client, const char *token) : _c(&client), _token(token) {}

  // ---- Настройка -----------------------------------------------------------------
  void onMessage(Handler h) { _h = h; }
  void allowChat(int64_t chatId);                  // принимать команды только из этого чата (можно несколько); без разрешённых принимаются из всех
  void setInterval(uint16_t ms) { _interval = ms; }   // как часто спрашивать новые сообщения (по умолчанию 2000 мс; запрос блокирует на 0.3-2 с)
  void setHost(const char *host) { _host = host; }    // для своего прокси; по умолчанию api.telegram.org

  // ---- Работа -------------------------------------------------------------------------
  void update();                                   // в loop(): раз в интервал спрашивает Telegram и вызывает обработчик для новых сообщений
  bool send(int64_t chatId, const char *text);
  bool sendButtons(int64_t chatId, const char *text, const char *buttons);   // "Вкл:led_on|Выкл:led_off" (по "|" в одном ряду); ряды разделяются ";"
  bool answerCallback(const Message &m, const char *toast = nullptr);        // убрать "часики" с нажатой кнопки
  bool getMe(char *nameOut, size_t size);          // проверка токена: имя бота

  // ---- Диагностика -----------------------------------------------------------------
  Error error() const { return _err; }
  int lastHttpStatus() const { return _http; }
  void setLanguage(Language lang) { _lang = lang; }
  const __FlashStringHelper *errorText();
  void diagnose(Print &out);

  // Разбор и кодирование (static: проверяются тестами без сети)
  static int parseUpdates(const char *json, void (*cb)(const Message &, int64_t updateId, void *), void *ctx);   // число обновлений
  static void idToString(int64_t id, char *out);   // chat_id числом в строку (минимум 21 байт); long на ESP только 32-битный, а chat_id бывает больше 2 миллиардов
  static size_t jsonEscape(const char *in, char *out, size_t size);
  static bool jsonString(const char *json, const char *key, char *out, size_t size);   // значение строки по ключу, с разбором \u и экранирования
  static bool jsonNumber(const char *json, const char *key, int64_t *out);

 private:
  Client *_c;
  const char *_token;
  const char *_host = "api.telegram.org";
  Handler _h = nullptr;
  int64_t _allowed[4] = {0};
  uint8_t _nAllowed = 0;
  int64_t _offset = 0;
  uint16_t _interval = 2000;
  uint32_t _lastPoll = 0, _blockedUntil = 0;
  Error _err = ERR_OK;
  Language _lang = LANG_RU;
  int _http = 0;
  char _buf[1536];
  bool request(const char *method, const char *body, bool wantBody = true);
  bool chatAllowed(int64_t id) const;
  static void onUpdate(const Message &m, int64_t id, void *self);
};

#endif  // ALASH_TELEGRAM_H
