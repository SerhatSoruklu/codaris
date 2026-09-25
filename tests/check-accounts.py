#!/usr/bin/env python3
"""Real PostgreSQL + native API + loopback SMTP integration. Isolated database."""
import email
import base64
import email.policy
import http.client
import json
import os
from pathlib import Path
import re
import secrets
import socket
import socketserver
import subprocess
import threading
import time

ROOT = Path(__file__).resolve().parents[1]
env = dict(os.environ)
env.setdefault('PGHOST', str(ROOT / '.local/postgres/socket'))
env.setdefault('PGPORT', '55432')
env.setdefault('PGUSER', 'codaris_app')
env.setdefault('PGCONNECT_TIMEOUT', '5')
database = 'codaris_test_' + secrets.token_hex(6)
server = None
completed = False
messages = []
class SMTP(socketserver.StreamRequestHandler):
    def handle(self):
        self.wfile.write(b'220 localhost ESMTP\r\n')
        while line := self.rfile.readline():
            command = line.decode().strip().upper()
            if command.startswith('EHLO'):
                self.wfile.write(b'250-localhost\r\n250 8BITMIME\r\n')
            elif command == 'DATA':
                self.wfile.write(b'354 Send message\r\n')
                parts = []
                while (line := self.rfile.readline()) != b'.\r\n':
                    if not line: return
                    parts.append(line[1:] if line.startswith(b'..') else line)
                messages.append(email.message_from_bytes(b''.join(parts), policy=email.policy.default))
                self.wfile.write(b'250 queued\r\n')
            elif command == 'QUIT':
                self.wfile.write(b'221 bye\r\n')
                return
            else:
                self.wfile.write(b'250 OK\r\n')
smtp = socketserver.ThreadingTCPServer(('127.0.0.1', 0), SMTP)
threading.Thread(target=smtp.serve_forever, daemon=True).start()
with socket.socket() as s:
    s.bind(('127.0.0.1', 0))
    port = s.getsockname()[1]
origin = 'http://127.0.0.1:8081'
env.update(PGDATABASE=database, CODARIS_DATABASE_URL='', CODARIS_ENV='development',
           CODARIS_PORT=str(port), CODARIS_ORIGIN=origin, CODARIS_MAIL_KEY=secrets.token_hex(32),
           CODARIS_SMTP_URL=f'smtp://127.0.0.1:{smtp.server_address[1]}', CODARIS_SMTP_USER='', CODARIS_SMTP_PASSWORD='')
binary = Path(os.environ.get('CODARIS_TEST_BINARY', ROOT / 'build/accounts/codaris_server'))
def psql(sql, db=None, check=True):
    e = dict(env)
    if db: e['PGDATABASE'] = db
    return subprocess.run(['psql','-X','-At','-v','ON_ERROR_STOP=1','-c',sql], env=e, capture_output=True, text=True, check=check)
def call(path, data=None, cookie='', expected=200, request_origin=origin):
    conn = http.client.HTTPConnection('127.0.0.1', port, timeout=20)
    headers = {'Content-Type':'application/json','Origin':request_origin}
    if cookie: headers['Cookie'] = cookie
    conn.request('GET' if data is None else 'POST', '/api/'+path, None if data is None else json.dumps(data), headers)
    response = conn.getresponse()
    body = json.loads(response.read())
    received = response.getheader('Set-Cookie')
    status = response.status
    conn.close()
    assert status == expected, (path,status,expected,body)
    return body, received

def drain():
    subprocess.run([str(binary),'--mail-once'],env=env,check=True,stdout=subprocess.DEVNULL)
def token_for(address, kind='verify-email'):
    for message in reversed(messages):
        if address in str(message['To']):
            plain = message.get_body(preferencelist=('plain',)).get_content()
            match = re.search('/'+kind+r'/#([a-f0-9]{64})', plain)
            if match:
                assert message.get_body(preferencelist=('html',)) is not None
                assert 'CODARIS' in str(message['From'])
                return match.group(1)
    raise AssertionError('Expected action mail not captured')
try:
    psql('CREATE DATABASE '+database, 'postgres')
    for migration in sorted((ROOT/'db/migrations').glob('*.sql')):
        subprocess.run(['psql','-X','-v','ON_ERROR_STOP=1','-f',str(migration)],env=env,check=True,stdout=subprocess.DEVNULL)
    server = subprocess.Popen([str(binary)], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    for _ in range(100):
        if server.poll() is not None:
            raise AssertionError(server.stderr.read().decode())
        try:
            call('health')
            break
        except OSError:
            time.sleep(.05)
    else: raise AssertionError('API did not start')
    password = 'A unique testing password 123!'
    registration = dict(name='Test Üser',email='TEST@example.test',password=password,country='United Kingdom',role='Developer / Engineer',
                        motivation='I want to build responsible software with others.',linkedin='https://www.linkedin.com/in/test',github='',website='https://example.test')
    call('register',registration,expected=403,request_origin='https://evil.example')
    call('register',dict(registration,linkedin='https://linkedin.com.evil.test/in/test'),expected=400)
    psql("CREATE FUNCTION app.test_reject_mail() RETURNS trigger LANGUAGE plpgsql AS $$ BEGIN RAISE EXCEPTION 'test queue failure'; END $$; CREATE TRIGGER test_reject_mail BEFORE INSERT ON app.mail_outbox FOR EACH ROW EXECUTE FUNCTION app.test_reject_mail()")
    call('register',registration,expected=500)
    assert psql('SELECT count(*) FROM app.accounts').stdout.strip()=='0'
    psql('DROP TRIGGER test_reject_mail ON app.mail_outbox; DROP FUNCTION app.test_reject_mail()')
    call('register',registration,expected=201)
    call('register',registration,expected=409)
    call('me',expected=401)
    call('login',dict(identifier=registration['email'],password='wrong'),expected=401)
    _,cookie=call('login',dict(identifier=registration['email'],password=password))
    assert 'HttpOnly' in cookie and 'SameSite=Strict' in cookie
    me,_=call('me',cookie=cookie)
    assert not me['email_verified'] and me['email']=='test@example.test'
    assert len(me['membership_id'])==24
    # Non-BMP passwords are valid Unicode code points, not UTF-16 code units.
    unicode_password='😀'*65
    call('register',dict(registration,email='unicode@example.test',password=unicode_password),expected=201)
    call('login',dict(identifier='unicode@example.test',password=unicode_password))
    psql("DELETE FROM app.users WHERE id=(SELECT user_id FROM app.accounts WHERE email='unicode@example.test')")
    assert psql("SELECT password_hash LIKE '$argon2id$%' FROM app.accounts").stdout.strip()=='t'
    assert psql("SELECT length(encrypted_token) FROM app.mail_outbox").stdout.strip()=='208'
    # SMTP failure keeps a durable retry; successful delivery erases its payload.
    failed_env=dict(env,CODARIS_SMTP_URL='smtp://127.0.0.1:1')
    failed=subprocess.run([str(binary),'--mail-once'],env=failed_env,capture_output=True)
    assert failed.returncode!=0
    assert psql('SELECT attempts FROM app.mail_outbox').stdout.strip()=='1'
    psql('UPDATE app.mail_outbox SET available_at=now()')
    drain()
    verification=token_for('test@example.test')
    call('verify',dict(token=verification))
    call('verify',dict(token=verification),expected=400)
    me,_=call('me',cookie=cookie);assert me['email_verified']
    avatar=base64.b64encode(bytes([32,180,220,255])*10000).decode()
    call('profile',dict(name='Bad image',country='UK',role='Other',avatar='garbage'),cookie,400)
    profile=dict(avatar=avatar,name="O'Neil <script>",country='Türkiye',role='Researcher',linkedin='',github='https://github.com/test',website='https://example.test/path')
    call('profile',dict(profile,motivation='Replacement reason must fail'),cookie,400)
    call('profile',dict(profile,website='javascript:alert(1)'),cookie,400)
    call('profile',profile,cookie)
    me,_=call('me',cookie=cookie);assert me['name']==profile['name'] and me['country']=='Türkiye' and me['motivation']==registration['motivation']
    assert base64.b64decode(me['avatar'])==bytes([32,180,220,255])*10000
    call('progress',dict(topic='4',read='1'),cookie)
    me,_=call('me',cookie=cookie);assert me['progress']==16
    call('progress',dict(topic='22',read='1'),cookie,400)
    call('progress',dict(topic='4',read='0'),cookie)
    me,_=call('me',cookie=cookie);assert me['progress']==0
    stats,_=call('community');assert stats==dict(members=1,countries=1)
    assert psql("UPDATE app.accounts SET motivation='Changed original application reason'",check=False).returncode!=0
    _,other_cookie=call('login',dict(identifier=me['membership_id'],password=password))
    call('email',dict(email='new@example.test',current_password='wrong'),cookie,403)
    call('email',dict(email='new@example.test',current_password=password),cookie)
    me,_=call('me',cookie=cookie);assert me['email']=='new@example.test' and not me['email_verified']
    call('me',cookie=other_cookie,expected=401)
    call('verify',dict(token=verification),expected=400)
    stats,_=call('community');assert stats['members']==0
    drain();new_token=token_for('new@example.test');assert any('test@example.test' in str(m['To']) and 'changed' in str(m['Subject']) for m in messages)
    call('verify',dict(token=new_token));me,_=call('me',cookie=cookie);assert me['email_verified']
    call('resend',{},cookie)
    call('recover',dict(email='new@example.test'));drain();reset=token_for('new@example.test','reset-password')
    new_password='Another unique password 456!'
    call('reset',dict(token=reset,password=new_password))
    call('reset',dict(token=reset,password=new_password),expected=400)
    call('me',cookie=cookie,expected=401)
    call('login',dict(identifier='new@example.test',password=password),expected=401)
    _,cookie=call('login',dict(identifier='new@example.test',password=new_password))
    call('logout',{},cookie);call('me',cookie=cookie,expected=401)
    assert psql('SELECT count(*) FROM app.users').stdout.strip()=='1'
    assert psql("SELECT count(*) FROM app.mail_outbox WHERE sent_at IS NOT NULL AND encrypted_token<>''").stdout.strip()=='0'
    # Recovery queue failures must not disclose whether an account exists.
    psql("CREATE FUNCTION app.test_recovery_mail_failure() RETURNS trigger LANGUAGE plpgsql AS $$ BEGIN RAISE EXCEPTION 'test queue failure'; END $$; CREATE TRIGGER test_recovery_mail_failure BEFORE INSERT ON app.mail_outbox FOR EACH ROW EXECUTE FUNCTION app.test_recovery_mail_failure()")
    found_failure,_=call('recover',dict(email='new@example.test'))
    missing_failure,_=call('recover',dict(email='absent@example.test'))
    assert found_failure==missing_failure
    psql('DROP TRIGGER test_recovery_mail_failure ON app.mail_outbox; DROP FUNCTION app.test_recovery_mail_failure()')
    invalid_email,_=call('recover',dict(email='a@b'),expected=400)
    assert 'email address' in invalid_email['message'] and 'profile' not in invalid_email['message']
    # Recovery is generic and account-scoped throttles survive requests.
    found,_=call('recover',dict(email='new@example.test'))
    missing,_=call('recover',dict(email='missing@example.test'));assert found==missing
    for _ in range(15): call('login',dict(identifier='limit@example.test',password=password),expected=401)
    call('login',dict(identifier='limit@example.test',password=password),expected=429)
    assert all(m['Date'] and m['Reply-To']=='contact@codaris.org' for m in messages)
    # Production cookie/origin policy is exercised without sending external mail.
    server.terminate();server.wait(timeout=5)
    diagnostics=server.stderr.read().decode()
    assert server.returncode==0 and 'Sanitizer' not in diagnostics, diagnostics
    prod_env=dict(env,CODARIS_ENV='production',CODARIS_ORIGIN='https://codaris.org',
                  CODARIS_SMTP_USER='test-user',CODARIS_SMTP_PASSWORD='local-test-only')
    server=subprocess.Popen([str(binary)],env=prod_env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    for _ in range(100):
        if server.poll() is not None: raise AssertionError(server.stderr.read().decode())
        try:
            call('health');break
        except OSError: time.sleep(.05)
    else: raise AssertionError('Production API did not start')
    call('login',dict(identifier='new@example.test',password=new_password),expected=403)
    _,secure_cookie=call('login',dict(identifier='new@example.test',password=new_password),request_origin='https://codaris.org')
    assert '; Secure' in secure_cookie and 'HttpOnly' in secure_cookie
    completed=True
    print('PASS: account persistence, SMTP multipart delivery, tokens, settings, immutable reason, sessions, CSRF, URL validation, recovery and rate limits')
finally:
    if server:
        server.terminate()
        try: server.wait(timeout=5)
        except subprocess.TimeoutExpired: server.kill(); server.wait()
        if completed:
            diagnostics=server.stderr.read().decode()
            assert server.returncode==0 and 'Sanitizer' not in diagnostics, diagnostics
    smtp.shutdown();smtp.server_close()
    psql('DROP DATABASE IF EXISTS '+database+' WITH (FORCE)', 'postgres', check=False)
