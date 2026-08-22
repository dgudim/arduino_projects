#!/usr/bin/env python3
"""Gzip + brotli compress webserial/index.html into include/webserial_page.h."""

from pathlib import Path

import gzip

ROOT = Path(__file__).resolve().parents[1]
HTML = ROOT / "webserial" / "index.html"
HEADER = ROOT / "include" / "webserial_page.h"


def c_array(name: str, data: bytes) -> str:
    lines = [f"const uint8_t {name}[{len(data)}] PROGMEM = {{"]
    for i in range(0, len(data), 16):
        chunk = ",".join(str(b) for b in data[i : i + 16])
        suffix = "," if i + 16 < len(data) else ""
        lines.append(f"  {chunk}{suffix}")
    lines.append("};")
    return "\n".join(lines)


def main() -> None:
    html = HTML.read_bytes()
    gzip_data = gzip.compress(html, compresslevel=9)
    HEADER.write_text(
        "\n".join(
            [
                "#pragma once",
                "",
                "#include <Arduino.h>",
                "#include <ESPAsyncWebServer.h>",
                "",
                "// Compressed from webserial/index.html by scripts/compress_webserial.py",
                c_array("WEBSERIAL_HTML_GZIP", gzip_data),
                "",
                "inline void serve_webserial_page(AsyncWebServerRequest *request) {",
                '  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", WEBSERIAL_HTML_GZIP,',
                "                                                           sizeof(WEBSERIAL_HTML_GZIP));",
                '  response->addHeader("Content-Encoding", "gzip");',
                "  request->send(response);",
                "}",
                "",
            ]
        )
        + "\n"
    )
    print(f"gzip {len(gzip_data)}")
    print(f"wrote {HEADER.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
