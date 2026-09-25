#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <stdarg.h>

class WebLog : public Print {
public:
    size_t write(uint8_t ch) override;
    size_t write(const uint8_t *buffer, size_t size) override;
};

extern WebLog Logger;
extern AsyncWebServer server;

void web_log_install();
void web_log_append_html(String &out);
int web_log_vprintf(const char *fmt, va_list args);
