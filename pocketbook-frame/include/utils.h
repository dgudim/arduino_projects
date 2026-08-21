#pragma once

#include <Arduino.h>
#include <http_parser.h>
#include <time.h>

// ESP32 time starts at the Unix epoch until NTP succeeds.
inline bool clock_is_set() {
    return time(nullptr) >= 1700000000;
}

// Accepts "http://host:port", "https://host", or similar absolute URLs.
inline bool parse_http_url(const char *url, String &host, uint16_t &port, uint16_t default_port = 80) {
    if (url == nullptr || url[0] == '\0') {
        return false;
    }

    http_parser_url parsed;
    http_parser_url_init(&parsed);
    if (http_parser_parse_url(url, strlen(url), 0, &parsed) != 0) {
        return false;
    }
    if ((parsed.field_set & (1 << UF_HOST)) == 0) {
        return false;
    }

    host = String(url + parsed.field_data[UF_HOST].off, parsed.field_data[UF_HOST].len);
    if (parsed.field_set & (1 << UF_PORT)) {
        port = parsed.port;
    } else if ((parsed.field_set & (1 << UF_SCHEMA)) && parsed.field_data[UF_SCHEMA].len == 5 &&
               strncmp(url + parsed.field_data[UF_SCHEMA].off, "https", 5) == 0) {
        port = 443;
    } else {
        port = default_port;
    }
    return host.length() > 0;
}
