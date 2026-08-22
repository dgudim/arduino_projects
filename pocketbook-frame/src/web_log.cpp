#include "web_log.h"
#include "config.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <rom/ets_sys.h>
#include <string.h>

WebLog Logger;
AsyncWebServer server(WEB_HTTP_PORT);

namespace {

portMUX_TYPE log_mux = portMUX_INITIALIZER_UNLOCKED;
char lines[WEB_LOG_LINES][WEB_LOG_LINE_MAX];
uint8_t line_len[WEB_LOG_LINES] = {};
uint8_t count = 0;
uint8_t head = 0;
char current[WEB_LOG_LINE_MAX];
uint8_t current_len = 0;

void IRAM_ATTR commit_line() {
    if (current_len == 0) {
        return;
    }
    uint8_t idx;
    if (count < WEB_LOG_LINES) {
        idx = count++;
    } else {
        idx = head;
        head = static_cast<uint8_t>((head + 1) % WEB_LOG_LINES);
    }
    memcpy(lines[idx], current, current_len);
    lines[idx][current_len] = '\0';
    line_len[idx] = current_len;
    current_len = 0;
}

void IRAM_ATTR write_char(char c) {
    if (c == '\r') {
        return;
    }
    if (c == '\n') {
        commit_line();
        return;
    }
    if (current_len + 1 >= WEB_LOG_LINE_MAX) {
        commit_line();
    }
    if (current_len + 1 < WEB_LOG_LINE_MAX) {
        current[current_len++] = c;
    }
}

void html_escape(String &out, const char *text, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        const char c = text[i];
        if (c == '&') {
            out += F("&amp;");
        } else if (c == '<') {
            out += F("&lt;");
        } else if (c == '>') {
            out += F("&gt;");
        } else if (c == '"') {
            out += F("&quot;");
        } else {
            out += c;
        }
    }
}

}  // namespace

void IRAM_ATTR web_log_putc(char c) {
    portENTER_CRITICAL(&log_mux);
    write_char(c);
    portEXIT_CRITICAL(&log_mux);
}

size_t WebLog::write(uint8_t ch) {
    web_log_putc(static_cast<char>(ch));
    return 1;
}

size_t WebLog::write(const uint8_t *buffer, size_t size) {
    portENTER_CRITICAL(&log_mux);
    for (size_t i = 0; i < size; i++) {
        write_char(static_cast<char>(buffer[i]));
    }
    portEXIT_CRITICAL(&log_mux);
    return size;
}

int web_log_vprintf(const char *fmt, va_list args) {
    char tmp[256];
    const int n = vsnprintf(tmp, sizeof(tmp), fmt, args);
    if (n > 0) {
        uint32_t len = static_cast<uint32_t>(n);
        if (len > sizeof(tmp) - 1) {
            len = sizeof(tmp) - 1;
        }
        Logger.write(reinterpret_cast<const uint8_t *>(tmp), len);
    }
    return n;
}

void web_log_install() {
    esp_log_set_vprintf(web_log_vprintf);
    ets_install_putc1(web_log_putc);
}

void web_log_append_html(String &out) {
    char snapshot[WEB_LOG_LINES][WEB_LOG_LINE_MAX];
    uint8_t snapshot_len[WEB_LOG_LINES];
    uint8_t snapshot_count = 0;
    uint8_t snapshot_head = 0;
    char tail[WEB_LOG_LINE_MAX];
    uint8_t tail_len = 0;

    portENTER_CRITICAL(&log_mux);
    snapshot_count = count;
    snapshot_head = head;
    for (uint8_t i = 0; i < count; i++) {
        snapshot_len[i] = line_len[i];
        memcpy(snapshot[i], lines[i], line_len[i] + 1);
    }
    tail_len = current_len;
    if (tail_len > 0) {
        memcpy(tail, current, tail_len);
        tail[tail_len] = '\0';
    }
    portEXIT_CRITICAL(&log_mux);

    auto emit = [&](const char *text, uint8_t len) {
        html_escape(out, text, len);
        out += '\n';
    };

    if (snapshot_count < WEB_LOG_LINES) {
        for (uint8_t i = 0; i < snapshot_count; i++) {
            emit(snapshot[i], snapshot_len[i]);
        }
    } else {
        for (uint8_t i = 0; i < WEB_LOG_LINES; i++) {
            const uint8_t idx = static_cast<uint8_t>((snapshot_head + i) % WEB_LOG_LINES);
            emit(snapshot[idx], snapshot_len[idx]);
        }
    }
    if (tail_len > 0) {
        html_escape(out, tail, tail_len);
    }
}
