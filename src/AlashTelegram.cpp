// MIT License. Alash Electronics.
#include "AlashTelegram.h"

#define T(ru, kk, en) (_lang == LANG_KK ? F(kk) : _lang == LANG_EN ? F(en) : F(ru))

// ---- Разбор JSON: ровно столько, сколько нужно ответам Telegram ---------------------------------------------

static int hexv(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static size_t putUtf8(uint32_t cp, char *out, size_t pos, size_t size) {
  char tmp[4];
  size_t n;
  if (cp < 0x80) { tmp[0] = (char)cp; n = 1; }
  else if (cp < 0x800) { tmp[0] = (char)(0xC0 | (cp >> 6)); tmp[1] = (char)(0x80 | (cp & 0x3F)); n = 2; }
  else if (cp < 0x10000) { tmp[0] = (char)(0xE0 | (cp >> 12)); tmp[1] = (char)(0x80 | ((cp >> 6) & 0x3F)); tmp[2] = (char)(0x80 | (cp & 0x3F)); n = 3; }
  else { tmp[0] = (char)(0xF0 | (cp >> 18)); tmp[1] = (char)(0x80 | ((cp >> 12) & 0x3F)); tmp[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); tmp[3] = (char)(0x80 | (cp & 0x3F)); n = 4; }
  if (pos + n + 1 > size) return pos;                       // не помещается: обрезаем, не ломая символ
  for (size_t i = 0; i < n; i++) out[pos + i] = tmp[i];
  return pos + n;
}

bool AlashTelegram::jsonString(const char *json, const char *key, char *out, size_t size) {
  char pat[40];
  snprintf(pat, sizeof pat, "\"%s\":\"", key);
  const char *p = json;
  size_t plen = strlen(pat);
  while ((p = strstr(p, pat)) != nullptr) {
    // это ключ, а не часть текста: перед кавычкой должна стоять { или ,
    if (p == json || p[-1] == '{' || p[-1] == ',') break;
    p += plen;
  }
  if (!p) return false;
  p += plen;
  size_t o = 0;
  while (*p && *p != '"') {
    if (*p == '\\') {
      p++;
      char c = *p;
      if (c == 'n') o = putUtf8('\n', out, o, size);
      else if (c == 'r') o = putUtf8('\r', out, o, size);
      else if (c == 't') o = putUtf8('\t', out, o, size);
      else if (c == 'u' && hexv(p[1]) >= 0 && hexv(p[2]) >= 0 && hexv(p[3]) >= 0 && hexv(p[4]) >= 0) {
        uint32_t cp = (hexv(p[1]) << 12) | (hexv(p[2]) << 8) | (hexv(p[3]) << 4) | hexv(p[4]);
        p += 4;
        if (cp >= 0xD800 && cp < 0xDC00 && p[1] == '\\' && p[2] == 'u') {         // пара суррогатов: эмодзи
          uint32_t lo = (hexv(p[3]) << 12) | (hexv(p[4]) << 8) | (hexv(p[5]) << 4) | hexv(p[6]);
          if (lo >= 0xDC00 && lo < 0xE000) { cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00); p += 6; }
        }
        o = putUtf8(cp, out, o, size);
      } else if (c) o = putUtf8((uint8_t)c, out, o, size);                       // \" \\ \/ и прочее: сам символ
      if (c) p++;
      continue;
    }
    if (o + 2 > size) { p++; continue; }
    out[o++] = *p++;
  }
  out[o] = 0;
  return true;
}

bool AlashTelegram::jsonNumber(const char *json, const char *key, int64_t *out) {
  char pat[40];
  snprintf(pat, sizeof pat, "\"%s\":", key);
  const char *p = strstr(json, pat);
  if (!p) return false;
  p += strlen(pat);
  bool neg = false;
  if (*p == '-') { neg = true; p++; }
  if (*p < '0' || *p > '9') return false;
  int64_t v = 0;
  while (*p >= '0' && *p <= '9') v = v * 10 + (*p++ - '0');
  *out = neg ? -v : v;
  return true;
}

// Список обновлений: режем по "update_id", каждое разбираем отдельно
int AlashTelegram::parseUpdates(const char *json, void (*cb)(const Message &, int64_t, void *), void *ctx) {
  static const char tag[] = "{\"update_id\":";
  int count = 0;
  const char *p = strstr(json, tag);
  while (p) {
    const char *next = strstr(p + 1, tag);
    size_t len = next ? (size_t)(next - p) : strlen(p);
    if (len > 1400) len = 1400;
    char slice[1401];
    memcpy(slice, p, len);
    slice[len] = 0;

    int64_t id = 0;
    Message m;
    memset(&m, 0, sizeof m);
    bool ok = jsonNumber(slice, "update_id", &id);
    const char *cq = strstr(slice, "\"callback_query\":");
    const char *scope = cq ? cq : slice;
    const char *chat = strstr(scope, "\"chat\":{");
    ok = ok && chat && jsonNumber(chat + 7, "id", &m.chatId);
    const char *from = strstr(scope, "\"from\":{");
    if (from) jsonString(from, "first_name", m.from, sizeof m.from);
    if (cq) {
      m.isCallback = true;
      jsonString(cq, "data", m.data, sizeof m.data);
      jsonString(cq + 17, "id", m.callbackId, sizeof m.callbackId);      // первый строковый "id" внутри callback_query это идентификатор нажатия
    } else {
      jsonString(slice, "text", m.text, sizeof m.text);
    }
    if (ok) {
      count++;
      cb(m, id, ctx);
    }
    p = next;
  }
  return count;
}

size_t AlashTelegram::jsonEscape(const char *in, char *out, size_t size) {
  size_t o = 0;
  for (size_t i = 0; in[i]; i++) {
    unsigned char c = in[i];
    char tmp[8];
    size_t n = 0;
    if (c == '"') { tmp[0] = '\\'; tmp[1] = '"'; n = 2; }
    else if (c == '\\') { tmp[0] = '\\'; tmp[1] = '\\'; n = 2; }
    else if (c == '\n') { tmp[0] = '\\'; tmp[1] = 'n'; n = 2; }
    else if (c == '\r') { tmp[0] = '\\'; tmp[1] = 'r'; n = 2; }
    else if (c == '\t') { tmp[0] = '\\'; tmp[1] = 't'; n = 2; }
    else if (c < 0x20) { n = snprintf(tmp, sizeof tmp, "\\u%04x", c); }
    else { tmp[0] = (char)c; n = 1; }                       // байты UTF-8 (кириллица, эмодзи) остаются как есть
    if (o + n + 1 > size) break;
    memcpy(out + o, tmp, n);
    o += n;
  }
  out[o] = 0;
  return o;
}

// ---- Запросы -------------------------------------------------------------------------------------------------

// 64-битное число в строку (на ESP long 32-битный: chat_id групп вроде -1001234567890 и новых аккаунтов в него не помещаются)
void AlashTelegram::idToString(int64_t v, char *out) {
  char tmp[24];
  int n = 0;
  bool neg = v < 0;
  uint64_t u = neg ? (uint64_t)(-(v + 1)) + 1 : (uint64_t)v;
  do { tmp[n++] = (char)('0' + u % 10); u /= 10; } while (u);
  int o = 0;
  if (neg) out[o++] = '-';
  while (n) out[o++] = tmp[--n];
  out[o] = 0;
}

bool AlashTelegram::request(const char *method, const char *body, bool wantBody) {
  _http = 0;
  if (!_c->connect(_host, 443)) {
    _err = ERR_NO_CONNECTION;
    return false;
  }
  size_t bl = strlen(body);
  char head[200];
  int hl = snprintf(head, sizeof head, "POST /bot%s/%s HTTP/1.0\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %u\r\nConnection: close\r\n\r\n",
                    _token, method, _host, (unsigned)bl);
  if (hl <= 0 || (size_t)hl >= sizeof head) { _c->stop(); _err = ERR_TOO_BIG; return false; }
  _c->write((const uint8_t *)head, hl);
  _c->write((const uint8_t *)body, bl);

  size_t n = 0;
  uint32_t t0 = millis(), last = t0;
  while ((uint32_t)(millis() - t0) < 8000) {
    while (_c->available() && n < sizeof(_buf) - 1) { _buf[n++] = (char)_c->read(); last = millis(); }
    if (!_c->connected() && !_c->available()) break;
    if (n >= sizeof(_buf) - 1) break;
    if (n && (uint32_t)(millis() - last) > 2500) break;
    if (!_c->available()) delay(2);
  }
  _buf[n] = 0;
  _c->stop();
  if (n < 12 || strncmp(_buf, "HTTP/1.", 7) != 0) { _err = n ? ERR_BAD_RESPONSE : ERR_NO_CONNECTION; return false; }
  _http = atoi(_buf + 9);
  // Тело ответа начинается после пустой строки; переносим его в начало буфера
  char *b = strstr(_buf, "\r\n\r\n");
  if (b) memmove(_buf, b + 4, strlen(b + 4) + 1); else _buf[0] = 0;
  (void)wantBody;
  if (_http == 200) { _err = ERR_OK; return true; }
  if (_http == 401 || _http == 404) _err = ERR_BAD_TOKEN;
  else if (_http == 403) _err = ERR_FORBIDDEN;
  else if (_http == 429) {
    _err = ERR_TOO_MANY;
    int64_t wait = 0;
    if (jsonNumber(_buf, "retry_after", &wait)) _blockedUntil = millis() + (uint32_t)wait * 1000UL;   // не спрашиваем, пока Telegram просит подождать
  } else if (_http == 400) _err = ERR_BAD_REQUEST;
  else _err = ERR_BAD_RESPONSE;
  return false;
}

bool AlashTelegram::chatAllowed(int64_t id) const {
  if (_nAllowed == 0) return true;
  for (uint8_t i = 0; i < _nAllowed; i++)
    if (_allowed[i] == id) return true;
  return false;
}

void AlashTelegram::allowChat(int64_t chatId) {
  for (uint8_t i = 0; i < _nAllowed; i++)
    if (_allowed[i] == chatId) return;
  if (_nAllowed < 4) _allowed[_nAllowed++] = chatId;
}

void AlashTelegram::onUpdate(const Message &m, int64_t id, void *self) {
  AlashTelegram *t = (AlashTelegram *)self;
  if (id >= t->_offset) t->_offset = id + 1;               // подтверждаем получение, даже если чат чужой: иначе Telegram будет присылать одно и то же
  if (t->_h && t->chatAllowed(m.chatId)) t->_h(m);
}

void AlashTelegram::update() {
  uint32_t now = millis();
  if ((uint32_t)(now - _lastPoll) < _interval) return;
  if ((int32_t)(now - _blockedUntil) < 0) return;
  _lastPoll = now;
  char body[120];
  char off[24];
  idToString(_offset, off);
  snprintf(body, sizeof body, "{\"offset\":%s,\"timeout\":0,\"limit\":2,\"allowed_updates\":[\"message\",\"callback_query\"]}", off);
  if (!request("getUpdates", body)) return;
  parseUpdates(_buf, onUpdate, this);
}

bool AlashTelegram::send(int64_t chatId, const char *text) {
  char esc[700];
  jsonEscape(text, esc, sizeof esc);
  char body[820], id[24];
  idToString(chatId, id);
  snprintf(body, sizeof body, "{\"chat_id\":%s,\"text\":\"%s\"}", id, esc);
  return request("sendMessage", body);
}

// "Вкл:led_on|Выкл:led_off;Статус:status" -> inline_keyboard [[Вкл, Выкл],[Статус]]
bool AlashTelegram::sendButtons(int64_t chatId, const char *text, const char *buttons) {
  char esc[400];
  jsonEscape(text, esc, sizeof esc);
  char kb[420];
  size_t k = 0;
  k += snprintf(kb + k, sizeof kb - k, "[[");
  const char *p = buttons;
  while (*p && k + 80 < sizeof kb) {
    const char *end = p;
    while (*end && *end != '|' && *end != ';') end++;
    const char *colon = end;                                // последнее двоеточие в кнопке отделяет надпись от данных
    for (const char *q = p; q < end; q++) if (*q == ':') colon = q;
    char label[64], data[64], labelEsc[96], dataEsc[96];
    size_t ll = (colon == end ? end : colon) - p, dl = colon == end ? ll : (size_t)(end - colon - 1);
    if (ll > 40) ll = 40;
    if (dl > 40) dl = 40;
    memcpy(label, p, ll); label[ll] = 0;
    if (colon == end) memcpy(data, p, dl); else memcpy(data, colon + 1, dl);
    data[dl] = 0;
    jsonEscape(label, labelEsc, sizeof labelEsc);
    jsonEscape(data, dataEsc, sizeof dataEsc);
    k += snprintf(kb + k, sizeof kb - k, "{\"text\":\"%s\",\"callback_data\":\"%s\"}", labelEsc, dataEsc);
    if (*end == '|') { k += snprintf(kb + k, sizeof kb - k, ","); p = end + 1; }
    else if (*end == ';') { k += snprintf(kb + k, sizeof kb - k, "],["); p = end + 1; }
    else p = end;
  }
  k += snprintf(kb + k, sizeof kb - k, "]]");
  if (k >= sizeof kb - 4) { _err = ERR_TOO_BIG; return false; }
  char body[900], id[24];
  idToString(chatId, id);
  int n = snprintf(body, sizeof body, "{\"chat_id\":%s,\"text\":\"%s\",\"reply_markup\":{\"inline_keyboard\":%s}}", id, esc, kb);
  if (n <= 0 || (size_t)n >= sizeof body) { _err = ERR_TOO_BIG; return false; }
  return request("sendMessage", body);
}

bool AlashTelegram::answerCallback(const Message &m, const char *toast) {
  char body[200], esc[100] = "";
  if (toast) jsonEscape(toast, esc, sizeof esc);
  snprintf(body, sizeof body, "{\"callback_query_id\":\"%s\",\"text\":\"%s\"}", m.callbackId, esc);
  return request("answerCallbackQuery", body);
}

bool AlashTelegram::getMe(char *nameOut, size_t size) {
  if (!request("getMe", "{}")) return false;
  if (!jsonString(_buf, "first_name", nameOut, size)) {
    nameOut[0] = 0;
    _err = ERR_BAD_RESPONSE;
    return false;
  }
  return true;
}

// ---- Диагностика ----------------------------------------------------------------------------------------------

const __FlashStringHelper *AlashTelegram::errorText() {
  switch (_err) {
    case ERR_OK: return F("OK");
    case ERR_NO_CONNECTION:
      return T("Нет связи с Telegram: проверьте Wi-Fi и интернет (WiFi.status() == WL_CONNECTED). В некоторых сетях (школа, офис) Telegram заблокирован: раздайте интернет с телефона",
               "Telegram-мен байланыс жоқ: Wi-Fi мен интернетті тексеріңіз. Кейбір желілерде (мектеп, кеңсе) Telegram бұғатталған: телефоннан интернет таратыңыз",
               "No link to Telegram: check Wi-Fi and the internet (WiFi.status() == WL_CONNECTED). Some networks (school, office) block Telegram: use a phone hotspot");
    case ERR_BAD_TOKEN:
      return T("Неверный токен бота. Возьмите его у @BotFather (команда /newbot или /token), скопируйте целиком, без пробелов и лишних символов, вид: 123456789:AAH...",
               "Бот токені қате. Оны @BotFather-ден алыңыз (/newbot не /token), толығымен, бос орынсыз көшіріңіз: 123456789:AAH...",
               "The bot token is wrong. Get it from @BotFather (/newbot or /token), copy all of it with no spaces: 123456789:AAH...");
    case ERR_FORBIDDEN:
      return T("Telegram запретил отправку: пользователь заблокировал бота или бот не добавлен в группу. Откройте бота и нажмите Start",
               "Telegram жіберуге тыйым салды: пайдаланушы ботты бұғаттаған не бот топқа қосылмаған. Ботты ашып, Start басыңыз",
               "Telegram forbade sending: the user blocked the bot or the bot is not in the group. Open the bot and press Start");
    case ERR_TOO_MANY:
      return T("Слишком много запросов: Telegram просит подождать. Не отправляйте чаще раза в секунду в один чат", "Тым көп сұрау: Telegram күтуді сұрайды. Бір чатқа секундына бірден жиі жібермеңіз", "Too many requests: Telegram asks to wait. Do not send more than once a second to one chat");
    case ERR_BAD_REQUEST:
      return T("Telegram не принял запрос: проверьте chat_id (число, у групп отрицательное) и что текст не пустой. Чтобы узнать свой chat_id, напишите боту: он придёт в обработчик сообщения",
               "Telegram сұранысты қабылдамады: chat_id (сан) және мәтін бос емес екенін тексеріңіз. chat_id білу үшін ботқа жазыңыз",
               "Telegram rejected the request: check the chat_id (a number, negative for groups) and that the text is not empty. To learn your chat_id, message the bot: it arrives in the message handler");
    case ERR_BAD_RESPONSE:
      return T("Ответ Telegram не разобрался (сбой сети или прокси)", "Telegram жауабы талданбады", "The Telegram reply could not be parsed (network or proxy glitch)");
    case ERR_TOO_BIG:
      return T("Сообщение слишком длинное для буфера (до 600 символов)", "Хабар буферге тым ұзын (600 таңбаға дейін)", "The message is too long for the buffer (up to 600 characters)");
  }
  return F("");
}

void AlashTelegram::diagnose(Print &out) {
  out.println(T("AlashTelegram: диагностика", "AlashTelegram: диагностика", "AlashTelegram: diagnostics"));
  out.print(T("Токен: ", "Токен: ", "Token: "));
  size_t tl = strlen(_token);
  out.println(tl > 20 && strchr(_token, ':') ? T("вид верный", "түрі дұрыс", "looks right") : T("ПОДОЗРИТЕЛЬНЫЙ: должен быть вида 123456789:AAH... (около 46 символов)", "КҮДІКТІ: 123456789:AAH... түрінде болуы керек", "SUSPICIOUS: it should look like 123456789:AAH... (about 46 characters)"));
  char name[40];
  if (getMe(name, sizeof name)) {
    out.print(T("Бот найден: ", "Бот табылды: ", "Bot found: "));
    out.println(name);
    out.println(T("Напишите боту любое сообщение: его chat_id попадёт в обработчик onMessage (m.chatId). Только этому чату разрешайте команды через allowChat().",
                  "Ботқа кез келген хабар жазыңыз: оның chat_id onMessage өңдеушісіне түседі (m.chatId). Командаларды тек осы чатқа allowChat() арқылы рұқсат етіңіз.",
                  "Message the bot anything: its chat_id arrives in the onMessage handler (m.chatId). Allow commands only for that chat via allowChat()."));
  } else {
    out.print(T("HTTP-статус: ", "HTTP-күйі: ", "HTTP status: "));
    out.println(_http);
    out.println(errorText());
  }
}
