# AlashTelegram

[Русский](#русский) | [Қазақша](#қазақша) | [English](#english)

---

## Русский

Простая библиотека, которая превращает **ESP32 или ESP8266 в Telegram-бота**: принимает команды (`/on`, `/off`) и **нажатия кнопок** в чате, отвечает текстом и кнопками, присылает уведомления. Команды принимаются **только из разрешённых вами чатов**, поэтому чужой не включит вашу лампу. Работает с кириллицей и эмодзи, правильно хранит большие chat_id. **ArduinoJson и UniversalTelegramBot не нужны.** Сделана для уроков и наборов Alash Electronics.

```cpp
#include <AlashTelegram.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
WiFiClientSecure net;
AlashTelegram bot(net, "123456789:ВАШ_ТОКЕН");

void onMessage(const AlashTelegram::Message &m) {
  bot.send(m.chatId, m.text);               // эхо / echo
}
void setup() {
  WiFi.begin("СЕТЬ", "ПАРОЛЬ");
  while (WiFi.status() != WL_CONNECTED) delay(300);
  net.setInsecure();
  bot.onMessage(onMessage);
}
void loop() { bot.update(); }
```

### Что умеет
- **Команды и кнопки:** обработчик `onMessage` получает текст, имя отправителя, `chatId` и данные нажатой кнопки (`isCallback`, `data`).
- **Отправка:** `send(чат, "текст")` и `sendButtons(чат, "Что делать?", "Вкл:led_on|Выкл:led_off;Статус:status")` (кнопки в ряд через `|`, ряды через `;`), `answerCallback()` убирает «часики» с нажатой кнопки.
- **Безопасность:** `allowChat(ваш_chat_id)` пропускает команды только из ваших чатов; из чужих игнорируются (но Telegram об этом уведомляется, чтобы не присылать их снова).
- **Правильные числа:** chat_id групп вроде -1001234567890 и новых аккаунтов больше 2 миллиардов; `long` на ESP их обрезает, поэтому внутри 64 бита и `idToString()`.
- **Русский и эмодзи:** разбор `\\u043f` и пар суррогатов, исходящий текст без порчи; длина запроса считается в байтах UTF-8.
- **Понятные ошибки:** неверный токен, бот заблокирован, лимит запросов (библиотека сама ждёт), нет связи (Telegram заблокирован сетью); `diagnose()` проверяет токен через `getMe()`.
- **Без лишних библиотек:** только `WiFiClientSecure`; свой разбор JSON ровно под ответы Telegram.
- Для **ESP32 и ESP8266** (и любых плат с `Client`).

### Чем отличается от других библиотек
Мы посмотрели UniversalTelegramBot (Brian Lough), AsyncTelegram2 и CTBot:
- UniversalTelegramBot требует ArduinoJson (определённой версии: 6, не 7), и ученики путаются в ошибках установки; у нас нет зависимостей.
- chat_id там хранится как `String`, и ошибки при сравнении и большими идентификаторами обычны. У нас 64-битные числа.
- Нет защиты от чужих: любой, кто найдёт бота, управляет платой. У нас `allowChat()`.
- Ошибка токена и блокировка Telegram сетью выглядят одинаково («ничего не происходит»). У нас разные тексты и `diagnose()`.

### Примеры
`Basic_ESP`, `LedControl_ESP`, `Buttons_ESP`, `Diagnose_ESP`

### Тесты
Разбор JSON Telegram (экранирование, `\\uXXXX`, кириллица, эмодзи парой суррогатов, ключ внутри текста, отрицательные 64-битные числа), сообщения, кнопки и группы в списке обновлений, точные тела запросов `sendMessage`, `getUpdates`, `answerCallbackQuery` и клавиатуры, длина запроса в байтах, смещение обновлений, интервал, допуск чатов, ошибки HTTP (401/404, 403, 400, 429 с ожиданием), нет связи проверены без сети на имитации сервера: `extras/test`.

Подробное руководство: [docs/WIKI.md](docs/WIKI.md).

---

## Қазақша

**ESP32 не ESP8266-ны Telegram-ботқа** айналдыратын қарапайым кітапхана: чаттағы командаларды (`/on`, `/off`) және **түймелерді басуды** қабылдайды, мәтінмен және түймелермен жауап береді, хабарландыру жібереді. Командалар **тек рұқсат етілген чаттардан** қабылданады. Кирилл мен эмодзи, үлкен chat_id дұрыс жұмыс істейді. **ArduinoJson қажет емес.**

```cpp
#include <AlashTelegram.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
WiFiClientSecure net;
AlashTelegram bot(net, "123456789:ВАШ_ТОКЕН");

void onMessage(const AlashTelegram::Message &m) {
  bot.send(m.chatId, m.text);               // эхо / echo
}
void setup() {
  WiFi.begin("СЕТЬ", "ПАРОЛЬ");
  while (WiFi.status() != WL_CONNECTED) delay(300);
  net.setInsecure();
  bot.onMessage(onMessage);
}
void loop() { bot.update(); }
```

### Мүмкіндіктері
- **Командалар және түймелер:** `onMessage` мәтінді, жіберуші атын, `chatId` және басылған түйме деректерін алады.
- **Жіберу:** `send(чат, "мәтін")` және `sendButtons(...)`, `answerCallback()`.
- **Қауіпсіздік:** `allowChat(chat_id)` командаларды тек сіздің чаттардан өткізеді.
- **Дұрыс сандар:** chat_id 64 битті.
- **Кирилл және эмодзи.**
- **Түсінікті қателер:** токен, бұғат, лимит, байланыс; `diagnose()` токенді тексереді.
- **Артық кітапхана жоқ:** тек `WiFiClientSecure`.
- **ESP32 және ESP8266**.

### Басқа кітапханалардан айырмашылығы
UniversalTelegramBot, AsyncTelegram2 және CTBot қарадық:
- UniversalTelegramBot-қа ArduinoJson керек; бізде тәуелділік жоқ.
- chat_id `String`; бізде 64 биттік сандар.
- Бөтеннен қорғаныс жоқ; бізде `allowChat()`.
- Токен қатесі мен бұғат ұқсас көрінеді; бізде әр түрлі мәтін.

### Мысалдар
`Basic_ESP`, `LedControl_ESP`, `Buttons_ESP`, `Diagnose_ESP`

### Тесттер
Telegram JSON талдауы, хабарлар, түймелер, сұраныс денелері, ығысу, рұқсат етілген чаттар, HTTP қателері тексерілген: `extras/test`.

Толық нұсқаулық: [docs/WIKI.md](docs/WIKI.md).

---

## English

A simple library that turns an **ESP32 or ESP8266 into a Telegram bot**: it receives commands (`/on`, `/off`) and **button presses** in a chat, answers with text and buttons and sends notifications. Commands are accepted **only from chats you allow**, so a stranger cannot switch your lamp. It handles Cyrillic and emoji and stores large chat ids correctly. **No ArduinoJson or UniversalTelegramBot needed.** Made for classes and Alash Electronics kits.

```cpp
#include <AlashTelegram.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
WiFiClientSecure net;
AlashTelegram bot(net, "123456789:ВАШ_ТОКЕН");

void onMessage(const AlashTelegram::Message &m) {
  bot.send(m.chatId, m.text);               // эхо / echo
}
void setup() {
  WiFi.begin("СЕТЬ", "ПАРОЛЬ");
  while (WiFi.status() != WL_CONNECTED) delay(300);
  net.setInsecure();
  bot.onMessage(onMessage);
}
void loop() { bot.update(); }
```

### Features
- **Commands and buttons:** the `onMessage` handler receives the text, the sender name, `chatId` and the data of a pressed button (`isCallback`, `data`).
- **Sending:** `send(chat, "text")` and `sendButtons(chat, "What to do?", "On:led_on|Off:led_off;Status:status")` (buttons in a row with `|`, rows with `;`), `answerCallback()` removes the spinner from a pressed button.
- **Security:** `allowChat(your_chat_id)` lets commands through only from your chats; others are ignored (while Telegram is still told they were received so they are not sent again).
- **Correct numbers:** group chat ids like -1001234567890 and ids of newer accounts exceed 2 billion; `long` on ESP truncates them, so the library uses 64 bits and `idToString()`.
- **Cyrillic and emoji:** parses `\\u043f` and surrogate pairs, outgoing text is not corrupted; the request length is counted in UTF-8 bytes.
- **Clear errors:** wrong token, bot blocked, rate limit (the library waits by itself), no connection (Telegram blocked by the network); `diagnose()` checks the token via `getMe()`.
- **No extra libraries:** only `WiFiClientSecure`; its own JSON parsing exactly for Telegram replies.
- For **ESP32 and ESP8266** (and any board with a `Client`).

### How it differs from other libraries
We looked at UniversalTelegramBot (Brian Lough), AsyncTelegram2 and CTBot:
- UniversalTelegramBot needs ArduinoJson (a particular version: 6, not 7), and students get lost in install errors; we have no dependencies.
- chat_id is stored as a `String` there, and comparison bugs and large ids are common. We use 64-bit numbers.
- No protection from strangers: anyone who finds the bot controls the board. We have `allowChat()`.
- A wrong token and a network blocking Telegram look the same ("nothing happens"). We have different texts and `diagnose()`.

### Examples
`Basic_ESP`, `LedControl_ESP`, `Buttons_ESP`, `Diagnose_ESP`

### Tests
Telegram JSON parsing (escapes, `\\uXXXX`, Cyrillic, emoji as surrogate pairs, a key inside text, negative 64-bit numbers), messages, buttons and groups in an update list, exact bodies of `sendMessage`, `getUpdates`, `answerCallbackQuery` and keyboards, request length in bytes, update offset, interval, chat filtering, HTTP errors (401/404, 403, 400, 429 with waiting) and a lost connection are checked without a network on a server simulator: `extras/test`.

Full guide: [docs/WIKI.md](docs/WIKI.md).

---

MIT License, Alash Electronics.
