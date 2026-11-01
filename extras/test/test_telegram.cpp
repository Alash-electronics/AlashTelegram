// Тесты AlashTelegram без сети: разбор JSON ответов Telegram, кодирование запросов, обновления, ошибки HTTP.
//   g++ -std=c++17 -Wall -I mock -I ../../src test_telegram.cpp ../../src/AlashTelegram.cpp && ./a.out
#include <stdio.h>
#include "Arduino.h"
#include "Client.h"
#include "AlashTelegram.h"

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); fails++; } } while (0)
typedef AlashTelegram::Message Msg;
static std::vector<Msg> got; static std::vector<int64_t> ids;
static void collect(const Msg &m, int64_t id, void *) { got.push_back(m); ids.push_back(id); }
static void handler(const Msg &m) { got.push_back(m); }
static std::string body(const std::string &req) { size_t p = req.find("\r\n\r\n"); return p == std::string::npos ? "" : req.substr(p + 4); }

static const char *MSG = "{\"ok\":true,\"result\":[{\"update_id\":500,\"message\":{\"message_id\":5,\"from\":{\"id\":111,\"is_bot\":false,\"first_name\":\"Али\"},\"chat\":{\"id\":111,\"first_name\":\"Али\",\"type\":\"private\"},\"date\":1,\"text\":\"\\u0412\\u043a\\u043b\\u044e\\u0447\\u0438 \\u0441\\u0432\\u0435\\u0442\"}}]}";

int main() {
  char o[64];
  // 1) разбор строк: обычная, экранирование, юникод (кириллица), суррогатные пары (эмодзи), ключ внутри текста
  CHECK(AlashTelegram::jsonString("{\"text\":\"hello\"}", "text", o, sizeof o) && strcmp(o, "hello") == 0);
  CHECK(AlashTelegram::jsonString("{\"text\":\"a\\nb \\\"q\\\" \\\\ \\/\"}", "text", o, sizeof o) && strcmp(o, "a\nb \"q\" \\ /") == 0);
  CHECK(AlashTelegram::jsonString("{\"text\":\"\\u043f\\u0440\\u0438\\u0432\\u0435\\u0442\"}", "text", o, sizeof o) && strcmp(o, "привет") == 0);
  CHECK(AlashTelegram::jsonString("{\"text\":\"\\ud83d\\ude00\"}", "text", o, sizeof o) && strcmp(o, "\xF0\x9F\x98\x80") == 0);
  CHECK(AlashTelegram::jsonString("{\"text\":\"привет в UTF-8\"}", "text", o, sizeof o) && strcmp(o, "привет в UTF-8") == 0);
  CHECK(AlashTelegram::jsonString("{\"a\":\"x \\\"text\\\":\\\"fake\\\" y\",\"text\":\"real\"}", "text", o, sizeof o) && strcmp(o, "real") == 0);   // "text" внутри значения это не ключ
  char small[8]; AlashTelegram::jsonString("{\"text\":\"\\u043f\\u0440\\u0438\\u0432\\u0435\\u0442\"}", "text", small, sizeof small);
  CHECK(strlen(small) <= 7 && strlen(small) % 2 == 0 && strncmp(small, "при", 6) == 0);           // обрезано по границе символа, без битого байта
  CHECK(!AlashTelegram::jsonString("{\"x\":\"y\"}", "text", o, sizeof o));

  // 2) числа, в том числе отрицательные 64-битные идентификаторы групп
  int64_t n = 0;
  CHECK(AlashTelegram::jsonNumber("{\"id\":-1001234567890,\"x\":1}", "id", &n) && n == -1001234567890LL);
  CHECK(AlashTelegram::jsonNumber("{\"update_id\":42}", "update_id", &n) && n == 42);
  CHECK(!AlashTelegram::jsonNumber("{\"id\":\"str\"}", "id", &n));

  char idb[24];
  AlashTelegram::idToString(0, idb); CHECK(strcmp(idb, "0") == 0);
  AlashTelegram::idToString(6912345678LL, idb); CHECK(strcmp(idb, "6912345678") == 0);                  // больше 2^31
  AlashTelegram::idToString(-1001234567890LL, idb); CHECK(strcmp(idb, "-1001234567890") == 0);
  AlashTelegram::idToString(-5, idb); CHECK(strcmp(idb, "-5") == 0);

  // 3) экранирование исходящего текста
  char e[64];
  AlashTelegram::jsonEscape("a\"b\\c\nd\te", e, sizeof e); CHECK(strcmp(e, "a\\\"b\\\\c\\nd\\te") == 0);
  AlashTelegram::jsonEscape("Привет", e, sizeof e); CHECK(strcmp(e, "Привет") == 0);
  AlashTelegram::jsonEscape("x\x01y", e, sizeof e); CHECK(strcmp(e, "x\\u0001y") == 0);
  AlashTelegram::jsonEscape("abcdefghij", e, 6); CHECK(strlen(e) == 5);

  // 4) разбор списка обновлений: сообщение, кнопка, группа, обновление без чата
  got.clear(); ids.clear();
  CHECK(AlashTelegram::parseUpdates(MSG, collect, nullptr) == 1 && got.size() == 1);
  CHECK(got[0].chatId == 111 && strcmp(got[0].text, "Включи свет") == 0 && strcmp(got[0].from, "Али") == 0 && !got[0].isCallback && ids[0] == 500);
  const char *CB = "{\"ok\":true,\"result\":[{\"update_id\":501,\"callback_query\":{\"id\":\"2086\",\"from\":{\"id\":111,\"first_name\":\"Али\"},\"message\":{\"message_id\":9,\"chat\":{\"id\":-1001234567890,\"title\":\"Класс\",\"type\":\"supergroup\"},\"text\":\"Свет\"},\"data\":\"led_on\"}},{\"update_id\":502,\"edited_channel_post\":{\"x\":1}},{\"update_id\":503,\"message\":{\"chat\":{\"id\":7},\"from\":{\"first_name\":\"B\"},\"text\":\"/start\"}}]}";
  got.clear(); ids.clear();
  CHECK(AlashTelegram::parseUpdates(CB, collect, nullptr) == 2 && got.size() == 2);
  CHECK(got[0].isCallback && got[0].chatId == -1001234567890LL && strcmp(got[0].data, "led_on") == 0 && strcmp(got[0].callbackId, "2086") == 0 && strcmp(got[0].from, "Али") == 0 && got[0].text[0] == 0);
  CHECK(!got[1].isCallback && got[1].chatId == 7 && strcmp(got[1].text, "/start") == 0 && ids[1] == 503);
  CHECK(AlashTelegram::parseUpdates("{\"ok\":true,\"result\":[]}", collect, nullptr) == 0);

  // 5) update(): запрос getUpdates, обработчик, смещение растёт, интервал
  FakeTelegram net; AlashTelegram bot(net, "123:ABC");
  bot.onMessage(handler); bot.setInterval(2000);
  net.response = MSG; got.clear(); g_ms += 5000;
  bot.update();
  CHECK(net.host == "api.telegram.org" && net.port == 443 && got.size() == 1 && got[0].chatId == 111);
  CHECK(net.requests.size() == 1 && net.requests[0].rfind("POST /bot123:ABC/getUpdates HTTP/1.0\r\n", 0) == 0);
  CHECK(body(net.requests[0]) == "{\"offset\":0,\"timeout\":0,\"limit\":2,\"allowed_updates\":[\"message\",\"callback_query\"]}");
  g_ms += 500; bot.update(); CHECK(net.requests.size() == 1);                                      // рано
  g_ms += 2000; net.response = "{\"ok\":true,\"result\":[]}"; bot.update();
  CHECK(net.requests.size() == 2 && body(net.requests[1]).find("\"offset\":501") != std::string::npos);   // смещение = последний id + 1

  // 6) допуск только своих чатов: чужие не доходят до обработчика, но смещение всё равно растёт
  FakeTelegram n2; AlashTelegram b2(n2, "t:k"); b2.onMessage(handler); b2.allowChat(999); b2.allowChat(999); b2.allowChat(111);
  got.clear(); n2.response = MSG; g_ms += 5000; b2.update(); CHECK(got.size() == 1);
  FakeTelegram n3; AlashTelegram b3(n3, "t:k"); b3.onMessage(handler); b3.allowChat(555);
  got.clear(); n3.response = MSG; g_ms += 5000; b3.update(); CHECK(got.empty());                    // чужой чат: игнор
  g_ms += 3000; n3.response = "{}"; b3.update(); CHECK(body(n3.requests[1]).find("\"offset\":501") != std::string::npos);

  // 7) ошибки HTTP: неверный токен, запрет, лимит с ожиданием, плохой запрос, нет связи
  FakeTelegram n4; AlashTelegram b4(n4, "bad"); n4.nextStatus = "404 Not Found"; n4.response = "{\"ok\":false,\"error_code\":404}";
  g_ms += 5000; b4.update(); CHECK(b4.error() == AlashTelegram::ERR_BAD_TOKEN && b4.lastHttpStatus() == 404);
  n4.nextStatus = "403 Forbidden"; CHECK(!b4.send(1, "x") && b4.error() == AlashTelegram::ERR_FORBIDDEN);
  n4.nextStatus = "400 Bad Request"; CHECK(!b4.send(1, "x") && b4.error() == AlashTelegram::ERR_BAD_REQUEST);
  n4.nextStatus = "429 Too Many Requests"; n4.response = "{\"ok\":false,\"error_code\":429,\"parameters\":{\"retry_after\":10}}";
  g_ms += 3000; b4.update(); CHECK(b4.error() == AlashTelegram::ERR_TOO_MANY);
  size_t reqs = n4.requests.size(); g_ms += 5000; b4.update(); CHECK(n4.requests.size() == reqs);     // Telegram просил подождать 10 с: не лезем
  g_ms += 6000; b4.update(); CHECK(n4.requests.size() == reqs + 1);
  FakeTelegram n5; AlashTelegram b5(n5, "t:k"); n5.reachable = false; g_ms += 5000; b5.update(); CHECK(b5.error() == AlashTelegram::ERR_NO_CONNECTION);
  CHECK(!b5.send(1, "x") && b5.error() == AlashTelegram::ERR_NO_CONNECTION);

  // 8) отправка: точный JSON, кавычки, перевод строки, русский текст, 64-битный chat_id группы
  FakeTelegram n6; AlashTelegram b6(n6, "9:Z"); n6.response = "{\"ok\":true}";
  CHECK(b6.send(123, "Привет \"мир\"\nстрока 2"));
  CHECK(n6.requests[0].rfind("POST /bot9:Z/sendMessage HTTP/1.0\r\n", 0) == 0 && n6.requests[0].find("Content-Length: 61\r\n") != std::string::npos || n6.requests[0].find("Content-Length:") != std::string::npos);
  CHECK(body(n6.requests[0]) == "{\"chat_id\":123,\"text\":\"Привет \\\"мир\\\"\\nстрока 2\"}");
  CHECK(b6.send(-1001234567890LL, "x") && body(n6.requests[1]) == "{\"chat_id\":-1001234567890,\"text\":\"x\"}");
  size_t cl = body(n6.requests[0]).size(); char want[40]; snprintf(want, sizeof want, "Content-Length: %zu\r\n", cl);
  CHECK(n6.requests[0].find(want) != std::string::npos);                                            // длина тела считается верно (в байтах UTF-8)

  // 9) кнопки: ряд через |, ряды через ;, подпись и данные через последнее двоеточие
  CHECK(b6.sendButtons(5, "Свет", "Вкл:led_on|Выкл:led_off;Статус:status"));
  CHECK(body(n6.requests[2]) == "{\"chat_id\":5,\"text\":\"Свет\",\"reply_markup\":{\"inline_keyboard\":[[{\"text\":\"Вкл\",\"callback_data\":\"led_on\"},{\"text\":\"Выкл\",\"callback_data\":\"led_off\"}],[{\"text\":\"Статус\",\"callback_data\":\"status\"}]]}}");
  CHECK(b6.sendButtons(5, "T", "ok") && body(n6.requests[3]).find("{\"text\":\"ok\",\"callback_data\":\"ok\"}") != std::string::npos);   // без двоеточия данные равны подписи

  // 10) подтверждение нажатия и проверка бота
  Msg cbm; memset(&cbm, 0, sizeof cbm); strcpy(cbm.callbackId, "2086");
  CHECK(b6.answerCallback(cbm, "Готово") && body(n6.requests[4]) == "{\"callback_query_id\":\"2086\",\"text\":\"Готово\"}");
  n6.response = "{\"ok\":true,\"result\":{\"id\":1,\"is_bot\":true,\"first_name\":\"AlashBot\",\"username\":\"alash_bot\"}}";
  char nm[24]; CHECK(b6.getMe(nm, sizeof nm) && strcmp(nm, "AlashBot") == 0);
  n6.response = "{\"ok\":true,\"result\":{}}"; CHECK(!b6.getMe(nm, sizeof nm) && b6.error() == AlashTelegram::ERR_BAD_RESPONSE);

  // 11) диагностика на трёх языках
  FakeTelegram n7; AlashTelegram b7(n7, "123456789:AAHdqTcvCH1vGWJxfSeofSAs0K5PALDsaw"); n7.response = "{\"ok\":true,\"result\":{\"first_name\":\"AlashBot\"}}";
  Print out; b7.diagnose(out); CHECK(out.log.find("AlashBot") != std::string::npos && out.log.find("вид верный") != std::string::npos);
  n7.nextStatus = "404 Not Found"; Print o2; b7.diagnose(o2); CHECK(o2.log.find("@BotFather") != std::string::npos);
  b7.setLanguage(AlashTelegram::LANG_KK); CHECK(strlen((const char *)b7.errorText()) > 10);

  if (fails) { printf("FAILED: %d\n", fails); return 1; }
  printf("ALL TESTS PASSED\n");
  return 0;
}
