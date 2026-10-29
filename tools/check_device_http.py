"""Exercise the real ESP32 HTTP server through its USB loopback console.
No simulated STM32; POSTs traverse HTTP, UART and actual flash storage.
"""
import argparse
import json
import time
from urllib.parse import urlencode
import serial


class Device:
    def __init__(self, port, reset=False):
        self.serial = serial.Serial(port, 115200, timeout=.1)
        if reset:
            self.serial.dtr = False
            self.serial.rts = True
            time.sleep(.2)
            self.serial.rts = False
        self.serial.dtr = True
        if reset:
            time.sleep(2)

    def http(self, path, body=None):
        self.serial.reset_input_buffer()
        request = 'GET ' + path if body is None else 'POST ' + path + ' ' + urlencode(body)
        self.serial.write((request + '\n').encode())
        received = b''
        deadline = time.monotonic() + 7
        while time.monotonic() < deadline:
            received += self.serial.read(4096)
            if b'USBHTTP END' in received:
                break
        received = received.replace(b'\r\r\n', b'\r\n')
        start = received.find(b'HTTP/1.1 ')
        if start < 0:
            raise RuntimeError(received.decode(errors='replace'))
        response = received[start:received.index(b'USBHTTP END')].strip()
        headers, data = response.split(b'\r\n\r\n', 1)
        return int(headers.split()[1]), json.loads(data)

    def state(self):
        status, state = self.http('/api/state')
        assert status == 200
        return state

    def online(self):
        for _ in range(12):
            state = self.state()
            if state['online']:
                return state
            time.sleep(1)
        raise RuntimeError('No fresh STM32 state')

    def change(self, path, values):
        state = self.online()
        status, result = self.http(path, dict(values, revision=state['revision']))
        assert status == 202, (status, result)
        operation = result['operation']
        for _ in range(14):
            state = self.state()
            if state['operation'] == operation and not state['pending']:
                assert state['result'] == 0, state
                if state['revision'] == state['resultRevision']:
                    return state
            time.sleep(.5)
        raise RuntimeError('No confirmed/read-back change')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', default='COM20')
    parser.add_argument('--reset', action='store_true')
    args = parser.parse_args()
    device = Device(args.port, args.reset)
    try:
        code, network = device.http('/api/network')
        print('NETWORK', code, network)
        state = device.online()
        state.pop('token', None)
        print('STATE', json.dumps(state))
    finally:
        device.serial.close()
