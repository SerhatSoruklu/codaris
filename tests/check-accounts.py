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
credential_review_dir = os.environ.get('CODARIS_CREDENTIAL_REVIEW_DIR')
if credential_review_dir:
    Path(credential_review_dir).mkdir(parents=True, exist_ok=True)

def save_credential_review(name, payload):
    if credential_review_dir:
        (Path(credential_review_dir) / name).write_bytes(payload)

server = None
completed = False
messages = []
legacy_credentials = {}
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
def raw_get(path, cookie=''):
    conn=http.client.HTTPConnection('127.0.0.1',port,timeout=20)
    headers={}
    if cookie: headers['Cookie']=cookie
    conn.request('GET','/api/'+path,headers=headers)
    response=conn.getresponse();payload=response.read();status=response.status;content_type=response.getheader('Content-Type','')
    conn.close();return status,content_type,payload

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
        if migration.stem=='008_membership_credentials':
            psql("INSERT INTO app.users(username,display_name) VALUES('PREEXISTINGVERIFIEDID','Existing Verified'),('PREEXISTINGUNVERIFIEDID','Existing Unverified'); INSERT INTO app.accounts(user_id,email,password_hash,email_verified,country,role,motivation) SELECT u.id,CASE WHEN u.username='PREEXISTINGVERIFIEDID' THEN 'old-verified@example.test' ELSE 'old-unverified@example.test' END,'test-hash',u.username='PREEXISTINGVERIFIEDID','United Kingdom','Developer / Engineer','This is a legacy account created before credential consent.' FROM app.users u WHERE u.username LIKE 'PREEXISTING%'")
        subprocess.run(['psql','-X','-v','ON_ERROR_STOP=1','-f',str(migration)],env=env,check=True,stdout=subprocess.DEVNULL)
        if migration.stem=='008_membership_credentials':
            assert psql("SELECT count(*) FROM app.membership_credentials c JOIN app.users u ON u.id=c.user_id WHERE u.username LIKE 'PREEXISTING%' AND NOT c.public_enabled AND c.consented_at IS NULL").stdout.strip()=='2'
            assert psql("SELECT c.status || '|' || (c.issued_at IS NOT NULL)::text FROM app.membership_credentials c JOIN app.users u ON u.id=c.user_id WHERE u.username='PREEXISTINGVERIFIEDID'").stdout.strip()=='active|true'
            assert psql("SELECT c.status || '|' || (c.issued_at IS NULL)::text FROM app.membership_credentials c JOIN app.users u ON u.id=c.user_id WHERE u.username='PREEXISTINGUNVERIFIEDID'").stdout.strip()=='pending|true'
            rows=psql("SELECT u.username || '|' || c.membership_number || '|' || c.verification_id::text FROM app.membership_credentials c JOIN app.users u ON u.id=c.user_id WHERE u.username LIKE 'PREEXISTING%' ORDER BY u.username").stdout.splitlines()
            legacy_credentials={row.split('|')[0]:row.split('|')[1:] for row in rows}
        if migration.stem=='009_cred_numbers':
            assert len(legacy_credentials)==2
            repaired_rows=psql("SELECT u.username || '|' || c.membership_number || '|' || c.verification_id::text FROM app.membership_credentials c JOIN app.users u ON u.id=c.user_id WHERE u.username LIKE 'PREEXISTING%' ORDER BY u.username").stdout.splitlines()
            repaired_credentials={row.split('|')[0]:row.split('|')[1:] for row in repaired_rows}
            assert set(repaired_credentials)==set(legacy_credentials)
            for username,(old_number,old_verifier) in legacy_credentials.items():
                repaired=repaired_credentials[username]
                assert repaired[1]==old_verifier, (username,old_verifier,repaired)
                assert repaired[0]!=old_number, (username,old_number)
                assert repaired[0]!='CDR-'+old_verifier.replace('-','')[:16].upper(), (username,repaired[0])
            assert psql("SELECT count(*) FROM app.membership_credentials c JOIN app.users u ON u.id=c.user_id WHERE u.username LIKE 'PREEXISTING%' AND c.membership_number = 'CDR-' || upper(substr(replace(c.verification_id::text, '-', ''), 1, 16))").stdout.strip()=='0'
            psql("DELETE FROM app.users WHERE username LIKE 'PREEXISTING%'")
    server = subprocess.Popen([str(binary)], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    for _ in range(100):
        if server.poll() is not None:
            raise AssertionError(server.stderr.read().decode())
        try:
            health, _ = call('health')
            assert health.get('contact_api') == 1 and health.get('mail_sender_aligned') is True
            break
        except OSError:
            time.sleep(.05)
    else: raise AssertionError('API did not start')
    password = 'A unique testing password 123!'
    registration = dict(name='Test Üser',email='TEST@example.test',password=password,country='United Kingdom',role='Developer / Engineer',
                        motivation='I want to build responsible software with others.',credential_public_consent='true',linkedin='https://www.linkedin.com/in/test',github='',website='https://example.test')
    call('register',registration,expected=403,request_origin='https://evil.example')
    call('register',dict(registration,credential_public_consent=''),expected=400)
    call('register',dict(registration,linkedin='https://linkedin.com.evil.test/in/test'),expected=400)
    call('register',dict(registration,country='Atlantis'),expected=400)
    call('register',dict(registration,role='Administrator'),expected=400)
    for role, address in (('Product / UX Designer','product@example.test'), ('Educator / Technical Writer','educator@example.test')):
        call('register',dict(registration,email=address,role=role),expected=201)
        psql("DELETE FROM app.users WHERE id=(SELECT user_id FROM app.accounts WHERE email='"+address+"')")
    psql("CREATE FUNCTION app.test_reject_mail() RETURNS trigger LANGUAGE plpgsql AS $$ BEGIN RAISE EXCEPTION 'test queue failure'; END $$; CREATE TRIGGER test_reject_mail BEFORE INSERT ON app.mail_outbox FOR EACH ROW EXECUTE FUNCTION app.test_reject_mail()")
    call('register',registration,expected=500)
    assert psql('SELECT count(*) FROM app.accounts').stdout.strip()=='0'
    psql('DROP TRIGGER test_reject_mail ON app.mail_outbox; DROP FUNCTION app.test_reject_mail()')
    call('register',registration,expected=201)
    call('register',registration,expected=409)
    call('me',expected=401)
    assert raw_get('session')[0] == 401
    call('login',dict(identifier=registration['email'],password='wrong'),expected=401)
    _,cookie=call('login',dict(identifier=registration['email'],password=password))
    assert 'HttpOnly' in cookie and 'SameSite=Strict' in cookie
    assert raw_get('session',cookie)[0] == 204
    me,_=call('me',cookie=cookie)
    assert not me['email_verified'] and me['email']=='test@example.test'
    assert len(me['membership_id'])==24
    assert me['credential']['status']=='pending' and me['credential']['public_enabled']
    assert me['credential']['membership_number'].startswith('CDR-') and me['credential']['verification_id']
    # Non-BMP passwords are valid Unicode code points, not UTF-16 code units.
    unicode_password='😀'*65
    call('register',dict(registration,email='unicode@example.test',password=unicode_password,
                         country='Afghanistan',role='AI / ML Engineer'),expected=201)
    assert psql("SELECT country || '|' || role FROM app.accounts WHERE email='unicode@example.test'").stdout.strip() == 'Afghanistan|AI / ML Engineer'
    assert psql("SELECT public_enabled FROM app.membership_credentials c JOIN app.accounts a ON a.user_id=c.user_id WHERE a.email='unicode@example.test'").stdout.strip()=='t'
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
    contact_origin_failure, _ = call('contact', dict(name='Contact Test', email='contact@example.test',
        topic='General question', message='I have a question about the CODARIS learning library.'),
        expected=403, request_origin='https://evil.example')
    call('contact', dict(name='Contact Test', email='contact@example.test',
        topic='Invented topic', message='I have a question about the CODARIS learning library.'), expected=400)
    marker = 'CONTACT-PRIVATE-MARKER-' + secrets.token_hex(8)
    contact_result, _ = call('contact', dict(name='Contact Test', email='contact@example.test',
        topic='General question', message='I have a question about the CODARIS learning library. ' + marker), expected=202)
    assert '3 working days' in contact_result['message'] and 'not a guaranteed deadline' in contact_result['message']
    ciphertext = psql('SELECT encrypted_payload FROM app.contact_outbox').stdout.strip()
    assert marker not in ciphertext and 'contact@example.test' not in ciphertext
    before_contact_mail = len(messages)
    drain()
    contact_messages = messages[before_contact_mail:]
    admin_message = next(m for m in contact_messages if 'admin@coupyn.com' in str(m['To']))
    receipt_message = next(m for m in contact_messages if 'contact@example.test' in str(m['To']))
    assert 'contact@example.test' in str(admin_message['Reply-To'])
    admin_body = admin_message.get_body(preferencelist=('plain',)).get_content()
    assert marker in admin_body and 'General question' in admin_body and 'Contact Test' in admin_body
    receipt_body = receipt_message.get_body(preferencelist=('plain',)).get_content()
    assert '3 working days' in receipt_body and 'not a guaranteed deadline' in receipt_body
    assert marker not in receipt_body
    assert psql('SELECT count(*) FROM app.contact_outbox').stdout.strip() == '0'
    verification=token_for('test@example.test')
    call('verify',dict(token=verification))
    call('verify',dict(token=verification),expected=400)
    me,_=call('me',cookie=cookie);assert me['email_verified']
    assert me['credential']['status']=='active'
    credential_id=me['credential']['verification_id']
    verified,_=call('credential/verify?credential='+credential_id)
    assert verified=={'valid':True,'display_name':registration['name'],'membership_number':me['credential']['membership_number'],'role':registration['role'],'status':'active','issued_at':verified['issued_at']}
    assert set(verified)=={'valid','display_name','membership_number','role','status','issued_at'}
    assert me['membership_id'] not in json.dumps(verified) and me['email'] not in json.dumps(verified)
    unavailable,_=call('credential/verify?credential=00000000-0000-0000-0000-000000000000')
    assert unavailable=={'valid':False,'message':'Credential verification is not publicly available.'}
    call('credential/public',{'enabled':'false'},cookie)
    assert call('credential/verify?credential='+credential_id)[0]==unavailable
    call('credential/public',{'enabled':'true'},cookie)
    assert raw_get('credential/card?format=svg&side=front')[0]==401
    status,content_type,svg=raw_get('credential/card?format=svg&side=front',cookie)
    assert status==200 and content_type.startswith('image/svg+xml')
    save_credential_review('front-no-avatar.svg',svg)
    assert b'CDR-' in svg and b'membership_id' not in svg and b'Test &#' not in svg
    import xml.etree.ElementTree as ET
    ET.fromstring(svg)
    try:
        import cv2
        import numpy as np
        qr_match=re.search(rb'<path fill="#061018" transform="translate\([0-9.]+ [0-9.]+\) scale\([0-9.]+\)" d="([^"]+)"',svg)
        assert qr_match
        runs=re.findall(rb'M([0-9]+) ([0-9]+)h([0-9]+)v1h-([0-9]+)z',qr_match.group(1))
        side=max(max(int(run[0])+int(run[2]),int(run[1])+1) for run in runs)
        matrix=np.full((side+8,side+8),255,dtype=np.uint8)
        for x,y,width,back_width in runs:
            assert width==back_width
            matrix[int(y)+4,int(x)+4:int(x)+4+int(width)]=0
        raster=cv2.resize(matrix,None,fx=12,fy=12,interpolation=cv2.INTER_NEAREST)
        decoded,_,_=cv2.QRCodeDetector().detectAndDecode(raster)
        assert decoded==origin+'/verify/?credential='+credential_id,decoded
    except ImportError:
        pass
    try:
        import zxingcpp
        import cv2
        import numpy as np
        barcode_rects=re.findall(rb'<rect x="([0-9.]+)" y="526\.00" width="([0-9.]+)" height="52\.00"/>',svg)
        scale=5
        barcode=np.full((52*scale,658*scale),255,dtype=np.uint8)
        for x,width in barcode_rects:
            left=max(0,int(round((float(x)-76)*scale)));right=min(barcode.shape[1],int(round((float(x)+float(width)-76)*scale)))
            if right>left and not (left==0 and right==barcode.shape[1]):barcode[:,left:right]=0
        barcode=cv2.copyMakeBorder(barcode,40,40,40,40,cv2.BORDER_CONSTANT,value=255)
        decoded_barcode=zxingcpp.read_barcode(barcode)
        assert decoded_barcode and decoded_barcode.text==credential_id,(decoded_barcode.text if decoded_barcode else None,len(barcode_rects),int((barcode==0).sum()))
    except ImportError:
        pass
    status,back_content_type,back_svg=raw_get('credential/card?format=svg&side=back',cookie)
    assert status==200 and back_content_type.startswith('image/svg+xml') and b'MEMBERSHIP VERIFICATION' in back_svg
    save_credential_review('back.svg',back_svg)
    status,content_type,pdf=raw_get('credential/card?format=pdf',cookie)
    assert status==200 and content_type=='application/pdf' and pdf.startswith(b'%PDF-')
    assert len(re.findall(rb'/Type /Page\b',pdf))==2
    save_credential_review('credential.pdf',pdf)
    avatar=base64.b64encode(bytes([32,180,220,255])*10000).decode()
    call('profile',dict(name='Bad image',country='UK',role='Other',avatar='garbage'),cookie,400)
    profile=dict(avatar=avatar,name="O'Neil <script>",country='Türkiye',role='Researcher',linkedin='',github='https://github.com/test',website='https://example.test/path')
    call('profile',dict(profile,motivation='Replacement reason must fail'),cookie,400)
    call('profile',dict(profile,website='javascript:alert(1)'),cookie,400)
    call('profile',profile,cookie)
    me,_=call('me',cookie=cookie);assert me['name']==profile['name'] and me['country']=='Türkiye' and me['motivation']==registration['motivation']
    status,content_type,svg=raw_get('credential/card?format=svg&side=front',cookie)
    assert status==200 and b'data:image/png;base64,' in svg and b'<script>' not in svg, (status,content_type,svg[:150])
    save_credential_review('front-with-avatar.svg',svg)
    encoded=re.search(rb'data:image/png;base64,([A-Za-z0-9+/=]+)',svg).group(1)
    photo_png=base64.b64decode(encoded)
    assert photo_png.startswith(b'\x89PNG\r\n\x1a\n') and photo_png[12:16]==b'IHDR' and photo_png[16:24]==bytes([0,0,0,100,0,0,0,100])
    status,content_type,pdf=raw_get('credential/card?format=pdf',cookie)
    assert status==200 and content_type=='application/pdf' and b'/Subtype /Image' in pdf
    save_credential_review('credential-with-avatar.pdf',pdf)
    verified,_=call('credential/verify?credential='+credential_id)
    assert verified['display_name']==profile['name'] and verified['membership_number']==me['credential']['membership_number']
    assert base64.b64decode(me['avatar'])==bytes([32,180,220,255])*10000
    long_profile=dict(profile,name='A'*120)
    call('profile',long_profile,cookie)
    status,content_type,long_front=raw_get('credential/card?format=svg&side=front',cookie)
    assert status==200 and content_type.startswith('image/svg+xml')
    front_tree=ET.fromstring(long_front)
    assert sum(1 for item in front_tree.iter() if item.attrib.get('class')=='member-name')==4
    save_credential_review('front-long-name.svg',long_front)
    status,content_type,long_back=raw_get('credential/card?format=svg&side=back',cookie)
    assert status==200 and content_type.startswith('image/svg+xml')
    back_tree=ET.fromstring(long_back)
    assert sum(1 for item in back_tree.iter() if item.attrib.get('class')=='member-name')==4
    save_credential_review('back-long-name.svg',long_back)
    status,content_type,long_pdf=raw_get('credential/card?format=pdf',cookie)
    assert status==200 and content_type=='application/pdf' and long_pdf.startswith(b'%PDF-')
    save_credential_review('credential-long-name.pdf',long_pdf)
    call('profile',dict(profile,name='Qi'),cookie)
    status,content_type,short_front=raw_get('credential/card?format=svg&side=front',cookie)
    assert status==200 and content_type.startswith('image/svg+xml')
    short_tree=ET.fromstring(short_front)
    assert sum(1 for item in short_tree.iter() if item.attrib.get('class')=='member-name')==1
    save_credential_review('front-short-name.svg',short_front)
    call('profile',profile,cookie)
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
    assert call('credential/verify?credential='+credential_id)[0]['valid'] is False
    drain();new_token=token_for('new@example.test');assert any('test@example.test' in str(m['To']) and 'changed' in str(m['Subject']) for m in messages)
    call('verify',dict(token=new_token));me,_=call('me',cookie=cookie);assert me['email_verified']
    assert call('credential/verify?credential='+credential_id)[0]['valid'] is True
    call('resend',{},cookie)
    call('recover',dict(email='new@example.test'));drain();reset=token_for('new@example.test','reset-password')
    new_password='Another unique password 456!'
    call('reset',dict(token=reset,password=new_password))
    call('reset',dict(token=reset,password=new_password),expected=400)
    call('me',cookie=cookie,expected=401)
    call('login',dict(identifier='new@example.test',password=password),expected=401)
    _,cookie=call('login',dict(identifier='new@example.test',password=new_password))
    call('logout',{},cookie);call('me',cookie=cookie,expected=401)
    assert raw_get('session',cookie)[0] == 401
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
    assert all(m['Date'] for m in messages)
    account_messages = [m for m in messages if str(m['Subject']) not in
                        ('CODARIS contact message', 'We received your message to CODARIS')]
    assert all(m['Reply-To']=='contact@codaris.org' for m in account_messages)
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
    print('PASS: account flows, legacy credential backfill/consent, public projection, SVG/PDF/PNG data, QR decoding, optional Code 128 decoding, SMTP, CSRF, recovery and rate limits')
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
