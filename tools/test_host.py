"""Compile and run the hardware-independent C tests on Windows/Linux/macOS."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent.parent
TESTS = {
    'protocol': ([], ['tests/protocol_test.c']),
    'menu': (['menu'], ['tests/menu_test.c']),
    'alarm': (['menu', 'calendar', 'alarm', 'rtc_health', 'persistence'], ['tests/alarm_test.c']),
    'control': (['menu', 'calendar', 'remote'], ['tests/control_test.c']),
    'screen': (['menu', 'calendar', 'alarm', 'upcoming', 'screen'], ['tests/screen_test.c']),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default=os.environ.get('CC', 'gcc'), help='GCC-compatible compiler executable')
    args = parser.parse_args()
    compiler = shutil.which(args.cc)
    if compiler is None:
        parser.error('Compiler not found. Install GCC/Clang or pass --cc with its executable path.')
    output = ROOT / '.pio' / 'host-tests'
    output.mkdir(parents=True, exist_ok=True)
    for name, (modules, tests) in TESTS.items():
        binary = output / (name + ('.exe' if os.name == 'nt' else ''))
        sources = ['firmware/stm32/' + module + '.c' for module in modules] + tests
        print('Building ' + name, flush=True)
        subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-Ishared', '-Ifirmware/stm32', *sources, '-o', str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)
    print('All five host C test suites passed.', flush=True)


if __name__ == '__main__':
    main()
