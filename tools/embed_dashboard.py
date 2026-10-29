"""Regenerate the ESP32 flash-resident page after changing dashboard.html."""
from pathlib import Path

root = Path(__file__).resolve().parent.parent
page = (root / 'firmware/esp32/dashboard.html').read_text(encoding='utf-8-sig')
(root / 'firmware/esp32/web_ui.h').write_text(
    '#pragma once\n#include <pgmspace.h>\n'
    'static const char WEB_UI[] PROGMEM=R"MEDBOX(' + page + ')MEDBOX";\n',
    encoding='utf-8',
)
