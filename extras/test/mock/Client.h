// Имитация защищённого соединения с Telegram: запоминает отправленный запрос, отдаёт заготовленный ответ.
#pragma once
#include "Arduino.h"
#include <deque>
#include <vector>
class Client {
 public:
  virtual ~Client() {}
  virtual int connect(const char *host, uint16_t port) = 0;
  virtual size_t write(const uint8_t *buf, size_t size) = 0;
  virtual int available() = 0;
  virtual int read() = 0;
  virtual void stop() = 0;
  virtual uint8_t connected() = 0;
};
struct FakeTelegram : public Client {
  bool reachable = true, open = false;
  std::string request, response; std::string host; uint16_t port = 0; int connects = 0;
  std::deque<char> in; std::vector<std::string> requests;
  std::string nextStatus = "200 OK";
  int connect(const char *h, uint16_t p) override { served = false; in.clear(); connects++; host = h; port = p; if (!reachable) return 0; open = true; request.clear(); return 1; }
  size_t write(const uint8_t *b, size_t n) override {
    request.append((const char *)b, n);
    return n;
  }
  void stop() override { if (!request.empty()) { requests.push_back(request); request.clear(); } open = false; }
  uint8_t connected() override { return open || !in.empty(); }
  int available() override { prepare(); return (int)in.size(); }
  int read() override { prepare(); if (in.empty()) return -1; char c = in.front(); in.pop_front(); if (in.empty()) open = false; return (unsigned char)c; }
  void prepare() { if (open && in.empty() && !served) { std::string r = "HTTP/1.0 " + nextStatus + "\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n" + response; in.assign(r.begin(), r.end()); served = true; } }
  bool served = false;
  void reset() { served = false; }
};
