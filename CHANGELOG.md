# Changelog

## 1.0.0
- Первый выпуск: Telegram Bot API по HTTPS для ESP32 и ESP8266 без ArduinoJson и UniversalTelegramBot.
- Приём сообщений и нажатий кнопок (`onMessage`), отправка текста и inline-кнопок, подтверждение нажатия, проверка токена `getMe()`.
- `allowChat()`: команды только из разрешённых чатов. Корректные 64-битные chat_id и кириллица/эмодзи в JSON (`\uXXXX`, пары суррогатов).
- Ожидание при лимите Telegram (429, `retry_after`), понятные ошибки по кодам HTTP и `diagnose()` на русском, казахском и английском.
