#!/usr/bin/env python3
"""Loopback static server + same-origin API proxy; development only."""
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from http.client import HTTPConnection
from pathlib import Path
import json
import argparse
import signal
from urllib.parse import urlsplit
ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=8081)
parser.add_argument('--api-port', type=int, default=8080)
args = parser.parse_args()
MEMBER_ROUTES = {
    page['slug'] for page in json.loads((ROOT / 'web/pages.json').read_text(encoding='utf-8'))
    if page.get('access') == 'member' and not page.get('client_guard')
}
class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *a, **kw):
        self.member_route_request = False
        super().__init__(*a, directory=str(ROOT / 'build/client'), **kw)
    def send_error(self, code, message=None, explain=None):
        if code != 404:
            try:
                return super().send_error(code, message, explain)
            except BrokenPipeError:
                return
        # Keep genuine unknown URLs as 404s, but use the same helpful page as
        # the production site and avoid the stock handler's duplicate error line.
        page = ROOT / 'build/client/404.html'
        try:
            body = page.read_bytes()
        except OSError:
            return super().send_error(code, message, explain)
        self.send_response(404)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Cache-Control', 'no-store')
        self.end_headers()
        self.log_request(404)
        if self.command != 'HEAD':
            try:
                self.wfile.write(body)
            except BrokenPipeError:
                pass
    def end_headers(self):
        if self.member_route_request:
            self.send_header('Cache-Control', 'private, no-store')
        super().end_headers()
    def is_member_route(self):
        path = urlsplit(self.path).path.strip('/')
        return bool(path) and path.split('/', 1)[0] in MEMBER_ROUTES
    def is_api_status_route(self):
        return urlsplit(self.path).path.rstrip('/') == '/api-status'
    def send_api_status_page(self):
        page = ROOT / 'build/client/api-status.html'
        try:
            body = page.read_bytes()
        except OSError:
            return self.send_error(404, 'API status page has not been built')
        self.send_response(200)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Cache-Control', 'no-store')
        self.end_headers()
        if self.command != 'HEAD':
            try:
                self.wfile.write(body)
            except BrokenPipeError:
                pass
    def send_member_status_page(self, code, slug):
        page = ROOT / 'build/client' / slug / 'index.html'
        try:
            body = page.read_bytes()
        except OSError:
            return self.send_error(code, 'Member access could not be checked')
        self.send_response(code)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Cache-Control', 'private, no-store')
        self.end_headers()
        if self.command != 'HEAD':
            try:
                self.wfile.write(body)
            except BrokenPipeError:
                pass
    def authorize_member_route(self):
        self.member_route_request = True
        headers = {'X-Real-IP': self.client_address[0]}
        if self.headers.get('Cookie'):
            headers['Cookie'] = self.headers['Cookie']
        try:
            backend = HTTPConnection('127.0.0.1', args.api_port, timeout=10)
            try:
                backend.request('GET', '/api/page-access', headers=headers)
                response = backend.getresponse()
                response.read()
                if response.status == 204:
                    return True
                if response.status == 403:
                    self.send_member_status_page(403, 'member-access-unavailable')
                    return False
                if response.status != 401:
                    self.send_member_status_page(503, 'member-service-unavailable')
                    return False
            finally:
                backend.close()
        except OSError:
            self.send_member_status_page(503, 'member-service-unavailable')
            return False
        self.send_member_status_page(401, 'member-sign-in-required')
        return False
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
        except BrokenPipeError:
            # Browsers commonly cancel an in-flight API request during
            # navigation. The client is gone, so there is no 502 to send.
            return
        except (OSError, ValueError):
            self.send_error(502, 'Account service unavailable')
    def do_GET(self):
        if self.path == '/__codaris_dev_health':
            payload = b'{"member_route_guard":1}'
            self.send_response(200)
            self.send_header('Content-Type', 'application/json; charset=utf-8')
            self.send_header('Cache-Control', 'no-store')
            self.send_header('Content-Length', str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
        elif self.is_api_status_route():
            self.send_api_status_page()
        elif self.path.startswith('/api/'):
            self.proxy()
        elif self.is_member_route() and not self.authorize_member_route():
            return
        else:
            super().do_GET()
    def do_HEAD(self):
        if self.is_api_status_route():
            self.send_api_status_page()
            return
        elif self.is_member_route() and not self.authorize_member_route():
            return
        super().do_HEAD()
    def do_POST(self):
        if self.path.startswith('/api/'):
            self.proxy()
        else:
            self.send_error(405)
print(f'CODARIS development: http://127.0.0.1:{args.port}', flush=True)
server = ThreadingHTTPServer(('127.0.0.1', args.port), Handler)
def stop_server(signum, frame):
    raise KeyboardInterrupt
signal.signal(signal.SIGTERM, stop_server)
try:
    server.serve_forever()
except KeyboardInterrupt:
    pass
finally:
    server.server_close()
