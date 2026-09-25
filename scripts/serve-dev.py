#!/usr/bin/env python3
"""Loopback static server + same-origin API proxy; development only."""
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from http.client import HTTPConnection
from pathlib import Path
import argparse
ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=8081)
parser.add_argument('--api-port', type=int, default=8080)
args = parser.parse_args()
class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *a, **kw):
        super().__init__(*a, directory=str(ROOT / 'build/client'), **kw)
    def proxy(self):
        try:
            length = int(self.headers.get('Content-Length', '0'))
            if length < 0 or length > 98304:
                self.send_error(413)
                return
            headers = {k: v for k, v in self.headers.items() if k.lower() not in ('host','connection','transfer-encoding','x-real-ip')}
            headers['X-Real-IP'] = self.client_address[0]
            backend = HTTPConnection('127.0.0.1', args.api_port, timeout=30)
            try:
                backend.request(self.command, self.path, self.rfile.read(length), headers)
                response = backend.getresponse()
                data = response.read()
                self.send_response(response.status)
                for k, v in response.getheaders():
                    if k.lower() not in ('transfer-encoding', 'connection', 'content-length'):
                        self.send_header(k, v)
                self.send_header('Content-Length', str(len(data)))
                self.end_headers()
                self.wfile.write(data)
            finally:
                backend.close()
        except (OSError, ValueError):
            self.send_error(502, 'Account service unavailable')
    def do_GET(self):
        if self.path.startswith('/api/'):
            self.proxy()
        else:
            super().do_GET()
    def do_POST(self):
        if self.path.startswith('/api/'):
            self.proxy()
        else:
            self.send_error(405)
print(f'CODARIS development: http://127.0.0.1:{args.port}', flush=True)
ThreadingHTTPServer(('127.0.0.1', args.port), Handler).serve_forever()
