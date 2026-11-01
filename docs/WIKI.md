# AlashTelegram: руководство / нұсқаулық / guide

[Русский](#русский) | [Қазақша](#қазақша) | [English](#english)

---

## Русский

Простая библиотека, которая превращает **ESP32 или ESP8266 в Telegram-бота**: принимает команды (`/on`, `/off`) и **нажатия кнопок** в чате, отвечает текстом и кнопками, присылает уведомления. Команды принимаются **только из разрешённых вами чатов**, поэтому чужой не включит вашу лампу. Работает с кириллицей и эмодзи, правильно хранит большие chat_id. **ArduinoJson и UniversalTelegramBot не нужны.** Сделана для уроков и наборов Alash Electronics.

### Установка
Arduino IDE: Скетч, Подключить библиотеку, Управлять библиотеками, найти **AlashTelegram** и нажать Установить. Или скачайте ZIP с GitHub: Скетч, Подключить библиотеку, Добавить .ZIP библиотеку.

### Платы
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 и другие, где работает Arduino-ядро.

### Быстрый старт
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

### Справочник по методам
| Метод | Что делает |
|---|---|
| `AlashTelegram(client, token)` | Бот с токеном из @BotFather |
| `onMessage(handler)` | Функция для входящих сообщений и нажатий кнопок |
| `update()` | В loop(): спросить Telegram |
| `send(chatId, text)` | Отправить текст |
| `sendButtons(chatId, text, buttons)` | Сообщение с кнопками |
| `answerCallback(m, toast)` | Ответить на нажатие кнопки |
| `allowChat(chatId)` | Принимать команды только из этого чата |
| `getMe(name, size)` | Проверить токен |
| `idToString(id, buf)` | chat_id в строку |
| `errorText() / diagnose(Serial)` | Причина / самопроверка |

### Если что-то не работает
**Бот не отвечает**

Запустите пример Diagnose_ESP: он проверит токен. Токен берут у @BotFather (/newbot), вид 123456789:AAH... без пробелов. Напишите боту первым сообщением /start. Если сеть блокирует Telegram (школа, офис), раздайте интернет с телефона.

**Как узнать свой chat_id**

Запустите Basic_ESP и напишите боту: он ответит вашим chat_id. Впишите его в `bot.allowChat(...)` (число с суффиксом LL).

**Ответ приходит с задержкой в несколько секунд**

Библиотека опрашивает Telegram раз в 2 секунды (`setInterval()`), и каждый запрос с шифрованием занимает 0.3-2 секунды. Для мгновенной реакции Telegram не подходит; не опрашивайте чаще секунды: он ограничит запросы.

### Самопроверка
В библиотеке есть `diagnose(Serial)`: она печатает, что видит плата, и подсказывает, что проверить. Язык подсказок: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Примеры
`Basic_ESP`, `LedControl_ESP`, `Buttons_ESP`, `Diagnose_ESP`

---

## Қазақша

**ESP32 не ESP8266-ны Telegram-ботқа** айналдыратын қарапайым кітапхана: чаттағы командаларды (`/on`, `/off`) және **түймелерді басуды** қабылдайды, мәтінмен және түймелермен жауап береді, хабарландыру жібереді. Командалар **тек рұқсат етілген чаттардан** қабылданады. Кирилл мен эмодзи, үлкен chat_id дұрыс жұмыс істейді. **ArduinoJson қажет емес.**

### Орнату
Arduino IDE: Sketch, Include Library, Manage Libraries, **AlashTelegram** табып, Install басыңыз. Немесе GitHub-тан ZIP жүктеп, Sketch, Include Library, Add .ZIP Library.

### Тақталар
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 және Arduino ядросы бар басқалар.

### Жылдам бастау
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

### Әдістер анықтамасы
| Әдіс | Не істейді |
|---|---|
| `AlashTelegram(client, token)` | @BotFather токені бар бот |
| `onMessage(handler)` | Кіріс хабарлар функциясы |
| `update()` | loop() ішінде |
| `send(chatId, text)` | Мәтін жіберу |
| `sendButtons(chatId, text, buttons)` | Түймелері бар хабар |
| `answerCallback(m, toast)` | Түйме басуына жауап |
| `allowChat(chatId)` | Тек осы чаттан командалар |
| `getMe(name, size)` | Токенді тексеру |
| `idToString(id, buf)` | chat_id жолға |
| `errorText() / diagnose(Serial)` | Себеп / өзін тексеру |

### Бірдеңе жұмыс істемесе
**Бот жауап бермейді**

Diagnose_ESP мысалын іске қосыңыз. Токенді @BotFather-ден алыңыз. Ботқа /start жазыңыз.

**Өз chat_id қалай білуге болады**

Basic_ESP іске қосып, ботқа жазыңыз: ол chat_id жібереді.

**Жауап бірнеше секундқа кешігеді**

Кітапхана Telegram-нан 2 секунд сайын сұрайды.

### Өзін-өзі тексеру
Кітапханада `diagnose(Serial)` бар: тақта не көргенін басып шығарады және нені тексеру керегін айтады. Кеңес тілі: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Мысалдар
`Basic_ESP`, `LedControl_ESP`, `Buttons_ESP`, `Diagnose_ESP`

---

## English

A simple library that turns an **ESP32 or ESP8266 into a Telegram bot**: it receives commands (`/on`, `/off`) and **button presses** in a chat, answers with text and buttons and sends notifications. Commands are accepted **only from chats you allow**, so a stranger cannot switch your lamp. It handles Cyrillic and emoji and stores large chat ids correctly. **No ArduinoJson or UniversalTelegramBot needed.** Made for classes and Alash Electronics kits.

### Installation
Arduino IDE: Sketch, Include Library, Manage Libraries, search **AlashTelegram** and click Install. Or download the ZIP from GitHub: Sketch, Include Library, Add .ZIP Library.

### Boards
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 and others with an Arduino core.

### Quick start
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

### Method reference
| Method | What it does |
|---|---|
| `AlashTelegram(client, token)` | A bot with a token from @BotFather |
| `onMessage(handler)` | Handler for incoming messages and button presses |
| `update()` | In loop(): ask Telegram |
| `send(chatId, text)` | Send text |
| `sendButtons(chatId, text, buttons)` | A message with buttons |
| `answerCallback(m, toast)` | Answer a button press |
| `allowChat(chatId)` | Accept commands only from this chat |
| `getMe(name, size)` | Verify the token |
| `idToString(id, buf)` | chat_id to text |
| `errorText() / diagnose(Serial)` | Reason / self-check |

### If something does not work
**The bot does not answer**

Run the Diagnose_ESP example: it checks the token. Get the token from @BotFather (/newbot), it looks like 123456789:AAH... with no spaces. Send the bot /start first. If the network blocks Telegram (school, office), use a phone hotspot.

**How to find my chat_id**

Run Basic_ESP and message the bot: it answers with your chat_id. Put it in `bot.allowChat(...)` (a number with the LL suffix).

**The answer comes a few seconds late**

The library polls Telegram every 2 seconds (`setInterval()`) and each encrypted request takes 0.3-2 seconds. Telegram is not for instant reactions; do not poll faster than a second: it will rate-limit you.

### Self-check
The library has `diagnose(Serial)`: it prints what the board sees and tells what to check. Tips language: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Examples
`Basic_ESP`, `LedControl_ESP`, `Buttons_ESP`, `Diagnose_ESP`

---

MIT License, Alash Electronics. https://github.com/Alash-electronics/AlashTelegram
