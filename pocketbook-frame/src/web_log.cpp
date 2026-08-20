#include "web_log.h"
#include "config.h"

#include <WebSerial.h>

WebLog Logger;
AsyncWebServer server(WEB_HTTP_PORT);

size_t WebLog::write(uint8_t ch) {
    return WebSerial.write(ch);
}

size_t WebLog::write(const uint8_t *buffer, size_t size) {
    return WebSerial.write(buffer, size);
}

int web_log_vprintf(const char *fmt, va_list args) {
    char tmp[256];
    const int32_t n = vsnprintf(tmp, sizeof(tmp), fmt, args);
    if (n > 0) {
        uint32_t len = n;
        if (len > sizeof(tmp) - 1) {
            len = sizeof(tmp) - 1;
        }
        Logger.write(reinterpret_cast<const uint8_t *>(tmp), len);
    }
    return n;
}
