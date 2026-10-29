"""Check documentation links, generated web page and public-source hygiene."""
from pathlib import Path
import re
import sys
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parent.parent
EXCLUDED = {'.git', '.pio', '.vscode', '__pycache__', '.env', 'secrets.h'}


def main():
    failures = []
    files = [p for p in ROOT.rglob('*') if p.is_file() and not any(part in EXCLUDED for part in p.relative_to(ROOT).parts)]
    for path in files:
        if path.suffix.lower() not in {'.md', '.c', '.cpp', '.h', '.py', '.mjs', '.html', '.yml', '.ini'}:
            continue
        text = path.read_text(encoding='utf-8-sig')
        if path.suffix == '.md':
            for target in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)', text):
                target = target.strip().split(' "')[0].strip('<>')
                if not target or target.startswith(('#', 'http:', 'https:', 'mailto:', 'data:')):
                    continue
                target = unquote(target.split('#')[0])
                if not (path.parent / target).is_file():
                    failures.append(f'{path.relative_to(ROOT)}: missing link target {target}')
        # Match actual credential formats, not documented empty/default fields.
        patterns = [r'gh[pousr]_[A-Za-z0-9]{30,}', r'github_pat_[A-Za-z0-9_]{30,}', r'AKIA[0-9A-Z]{16}', r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----']
        if any(re.search(pattern, text) for pattern in patterns):
            failures.append(f'{path.relative_to(ROOT)}: credential-shaped value requires review')
        if re.search(r'(?:C:[/\\]Users[/\\]|/Users/)[A-Za-z0-9_-]+[/\\]', text):
            failures.append(f'{path.relative_to(ROOT)}: private machine path requires removal')
    page = (ROOT / 'firmware/esp32/dashboard.html').read_text(encoding='utf-8-sig')
    expected = '#pragma once\n#include <pgmspace.h>\nstatic const char WEB_UI[] PROGMEM=R"MEDBOX(' + page + ')MEDBOX";\n'
    actual = (ROOT / 'firmware/esp32/web_ui.h').read_text(encoding='utf-8-sig')
    if expected != actual:
        failures.append('web_ui.h is stale; run python tools/embed_dashboard.py')
    if failures:
        print('\n'.join(failures), file=sys.stderr)
        return 1
    print(f'PASS: {len(files)} project files; Markdown links, embedded dashboard and public-source hygiene')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
