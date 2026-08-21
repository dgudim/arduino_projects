# Fetch Espressif's prebuilt esp_new_jpeg (ESP32-S3) into .pio/.
# lib_deps cannot consume this: it is an IDF component with a .a, not an Arduino library.

from pathlib import Path
from urllib.request import Request, urlopen

# Pin to a commit
ESP_ADF_LIBS_REF = "67b8d0e98f58c774b8652480893037273190e8dc"
FILES = (
    "include/esp_jpeg_common.h",
    "include/esp_jpeg_enc.h",
    "include/esp_jpeg_version.h",
    "lib/esp32s3/libesp_new_jpeg.a",
    "LICENSE",
)


try:
    Import("env")  # type: ignore[name-defined]
    PROJECT_DIR = Path(env["PROJECT_DIR"])  # noqa: F821
except NameError:
    PROJECT_DIR = Path(__file__).resolve().parents[1]


def _download(url, dest):
    dest.parent.mkdir(parents=True, exist_ok=True)
    req = Request(url, headers={"User-Agent": "pocketbook-frame-fetch"})
    with urlopen(req, timeout=60) as response:
        dest.write_bytes(response.read())


def fetch(project_dir):
    dest_root = project_dir / ".pio" / "esp_new_jpeg"
    stamp = dest_root / ".stamp"
    if stamp.is_file() and stamp.read_text(encoding="utf-8").strip() == ESP_ADF_LIBS_REF:
        if all((dest_root / rel).is_file() for rel in FILES):
            print(f"esp_new_jpeg already fetched ({ESP_ADF_LIBS_REF[:12]})")
            return dest_root

    print(f"Fetching esp_new_jpeg from esp-adf-libs@{ESP_ADF_LIBS_REF[:12]}")
    base = f"https://github.com/espressif/esp-adf-libs/raw/{ESP_ADF_LIBS_REF}/esp_new_jpeg"
    for rel in FILES:
        print(f"  {rel}")
        _download(f"{base}/{rel}", dest_root / rel)
    stamp.write_text(ESP_ADF_LIBS_REF + "\n", encoding="utf-8")
    return dest_root


fetch(PROJECT_DIR)
