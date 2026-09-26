#include "codaris/service.h"
#include "codaris/version.h"
#include "../shared/membership_options.h"
#include "../shared/contact_options.h"
#include "../shared/contact_policy.h"
#include "credential_render.h"
#include <microhttpd.h>
#include <json-c/json.h>
#include <curl/curl.h>
#include <sodium.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <time.h>
#endif
#define BODY_MAX 98304
/* One MHD internal polling thread serializes requests; each request owns its
 * connection, parsed JSON, body and PGresults. No PGconn crosses threads. */
typedef struct {
    char body[BODY_MAX + 1];
    size_t length;
    int overflow;
} Request;
static volatile sig_atomic_t stopping;
static void stop_service(int sig) {
    (void)sig;
    stopping = 1;
}
static PGresult *query(PGconn *db, const char *sql, int count, const char **values) {
    PGresult *r = PQexecParams(db, sql, count, NULL, values, NULL, NULL, 0);
    if (!r)
        return NULL;
    ExecStatusType status = PQresultStatus(r);
    if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
        PQclear(r);
        return NULL;
    }
    return r;
}
static int command(PGconn *db, const char *sql, int n, const char **v) {
    PGresult *r = query(db, sql, n, v);
    if (!r)
        return 0;
    PQclear(r);
    return 1;
}
static void digest(const char *raw, char hash[65]) {
    unsigned char bytes[crypto_hash_sha256_BYTES];
    crypto_hash_sha256(bytes, (const unsigned char *)raw, strlen(raw));
    sodium_bin2hex(hash, 65, bytes, sizeof(bytes));
    sodium_memzero(bytes, sizeof(bytes));
}
static void new_token(char raw[65]) {
    unsigned char bytes[32];
    randombytes_buf(bytes, sizeof(bytes));
    sodium_bin2hex(raw, 65, bytes, sizeof(bytes));
    sodium_memzero(bytes, sizeof(bytes));
}
static enum MHD_Result respond(struct MHD_Connection *connection, unsigned status, const char *json,
                               const char *cookie) {
    struct MHD_Response *r =
        MHD_create_response_from_buffer(strlen(json), (void *)json, MHD_RESPMEM_MUST_COPY);
    if (!r)
        return MHD_NO;
    int ok = MHD_add_response_header(r, "Content-Type", "application/json; charset=utf-8") &&
             MHD_add_response_header(r, "Cache-Control", "no-store") &&
             MHD_add_response_header(r, "X-Content-Type-Options", "nosniff");
    if (cookie)
        ok = ok && MHD_add_response_header(r, "Set-Cookie", cookie);
    enum MHD_Result result = ok ? MHD_queue_response(connection, status, r) : MHD_NO;
    MHD_destroy_response(r);
    return result;
}
static const char *field(json_object *body, const char *name) {
    json_object *v = NULL;
    if (!body || !json_object_object_get_ex(body, name, &v) ||
        !json_object_is_type(v, json_type_string))
        return "";
    const char *s = json_object_get_string(v);
    if (!s || strlen(s) != (size_t)json_object_get_string_len(v))
        return "";
    return s;
}
static size_t characters(const char *s) {
    size_t count = 0;
    for (; *s; s++)
        if (((unsigned char)*s & 0xc0) != 0x80)
            count++;
    return count;
}
static int length_ok(const char *s, size_t min, size_t max) {
    size_t n = characters(s);
    return n >= min && n <= max;
}
static int email_normalize(const char *input, char out[255]) {
    size_t n = strlen(input);
    if (n < 3 || n > 254)
        return 0;
    const char *at = strchr(input, '@');
    if (!at || at == input || at - input > 64 || strchr(at + 1, '@') || !strchr(at + 1, '.') ||
        at[1] == '.' || input[n - 1] == '.')
        return 0;
    if (input[0] == '.' || at[-1] == '.' || strstr(input, ".."))
        return 0;
    size_t label = 0;
    for (const char *p = at + 1; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c == '.') {
            if (!label || p[-1] == '-')
                return 0;
            label = 0;
            continue;
        }
        if (!((c < 128 && isalnum(c)) || c == '-') || (!label && c == '-') || ++label > 63)
            return 0;
    }
    if (!label || input[n - 1] == '-')
        return 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)input[i];
        if (!(isalnum(c) && c < 128) && !strchr(".!#$%&'*+-/=?^_`{|}~@", c))
            return 0;
        out[i] = (char)tolower(c);
    }
    out[n] = 0;
    return 1;
}
static int password_ok(const char *s) {
    return length_ok(s, 15, 128) && strlen(s) <= 512;
}
static int role_ok(const char *s) {
    return membership_role_allowed(s);
}
static int url_normalize(const char *input, const char *provider, char out[2049]) {
    out[0] = 0;
    if (!*input)
        return 1;
    if (strlen(input) > 2048 || strpbrk(input, "\r\n\t \\"))
        return 0;
    CURLU *u = curl_url();
    char *scheme = NULL, *host = NULL, *user = NULL, *pass = NULL, *port = NULL, *normalized = NULL;
    int ok = u && !curl_url_set(u, CURLUPART_URL, input, 0) &&
             !curl_url_get(u, CURLUPART_SCHEME, &scheme, 0) && !strcmp(scheme, "https") &&
             !curl_url_get(u, CURLUPART_HOST, &host, 0) &&
             curl_url_get(u, CURLUPART_USER, &user, 0) == CURLUE_NO_USER &&
             curl_url_get(u, CURLUPART_PASSWORD, &pass, 0) == CURLUE_NO_PASSWORD;
    if (ok) {
        for (char *p = host; *p; p++)
            *p = (char)tolower((unsigned char)*p);
        ok = !curl_url_set(u, CURLUPART_HOST, host, 0);
    }
    if (ok && *provider)
        ok = !strcmp(host, provider) || (!strncmp(host, "www.", 4) && !strcmp(host + 4, provider));
    if (ok && !curl_url_get(u, CURLUPART_PORT, &port, 0))
        ok = !strcmp(port, "443");
    if (ok)
        ok = !curl_url_get(u, CURLUPART_URL, &normalized, 0) && strlen(normalized) <= 2048;
    if (ok)
        strcpy(out, normalized);
    curl_free(scheme);
    curl_free(host);
    curl_free(user);
    curl_free(pass);
    curl_free(port);
    curl_free(normalized);
    curl_url_cleanup(u);
    return ok;
}
/* Avatars are exactly 100x100 RGBA pixels, never uploaded file formats. */
static int avatar_ok(const char *value) {
    if (!*value)
        return 1;
    if (strlen(value) != 53336)
        return 0;
    unsigned char pixels[40000];
    size_t length = 0;
    return !sodium_base642bin(pixels, sizeof(pixels), value, strlen(value), NULL, &length, NULL,
                              sodium_base64_VARIANT_ORIGINAL) &&
           length == sizeof(pixels);
}
static int limited(PGconn *db, const char *bucket, int max) {
    const char *v[] = {bucket};
    PGresult *r =
        query(db,
              "INSERT INTO app.rate_limits(bucket) VALUES($1) ON CONFLICT(bucket) DO UPDATE SET "
              "attempts=CASE WHEN app.rate_limits.window_start < now()-interval '15 minutes' THEN "
              "1 ELSE app.rate_limits.attempts+1 END, window_start=CASE WHEN "
              "app.rate_limits.window_start < now()-interval '15 minutes' THEN now() ELSE "
              "app.rate_limits.window_start END RETURNING attempts",
              1, v);
    if (!r)
        return 1;
    int n = atoi(PQgetvalue(r, 0, 0));
    PQclear(r);
    return n > max;
}
static int queue_mail(PGconn *db, const Config *c, const char *id, const char *email,
                      const char *kind) {
    char token[65] = "", hash[65] = "", cipher[209] = "";
    int action = !strcmp(kind, "verify") || !strcmp(kind, "reset");
    if (action) {
        new_token(token);
        digest(token, hash);
        if (!token_encrypt(c, token, cipher, sizeof(cipher))) {
            sodium_memzero(token, sizeof(token));
            return 0;
        }
        sodium_memzero(token, sizeof(token));
        const char *v[] = {id, kind, email, hash};
        if (!command(db, "DELETE FROM app.action_tokens WHERE user_id=$1::bigint AND kind=$2", 2,
                     v) ||
            !command(db,
                     "DELETE FROM app.mail_outbox WHERE user_id=$1::bigint AND kind=$2 AND sent_at "
                     "IS NULL",
                     2, v) ||
            !command(db,
                     "INSERT INTO app.action_tokens(user_id,kind,email,token_hash,expires_at) "
                     "VALUES($1::bigint,$2,$3,$4,now()+CASE WHEN $2='reset' THEN interval '30 "
                     "minutes' ELSE interval '24 hours' END)",
                     4, v))
            return 0;
    }
    const char *v[] = {id, email, kind, cipher};
    int ok = command(db,
                     "INSERT INTO app.mail_outbox(user_id,recipient,kind,encrypted_token) "
                     "VALUES($1::bigint,$2,$3,$4)",
                     4, v);
    sodium_memzero(cipher, sizeof(cipher));
    return ok;
}
static int contact_text_ok(const char *s, size_t min, size_t max, int multiline,
                           size_t max_bytes) {
    size_t bytes = strlen(s);
    if (bytes > max_bytes || !length_ok(s, min, max))
        return 0;
    int non_space = 0;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        if (*p < 0x20 && !(multiline && (*p == '\n' || *p == '\r' || *p == '\t')))
            return 0;
        if (*p == 0x7f)
            return 0;
        if (*p >= 0x80 || !isspace(*p))
            non_space = 1;
    }
    return non_space;
}
static char *contact_payload_encrypt(const Config *c, const char *plain) {
    size_t length = strlen(plain);
    if (!length || length > 18000 || length > SIZE_MAX - crypto_secretbox_NONCEBYTES -
                                                    crypto_secretbox_MACBYTES)
        return NULL;
    size_t blob_size = crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES + length;
    unsigned char *blob = malloc(blob_size);
    char *hex = malloc(blob_size * 2 + 1);
    if (!blob || !hex) {
        free(blob);
        free(hex);
        return NULL;
    }
    randombytes_buf(blob, crypto_secretbox_NONCEBYTES);
    int ok = !crypto_secretbox_easy(blob + crypto_secretbox_NONCEBYTES,
                                    (const unsigned char *)plain, (unsigned long long)length, blob,
                                    c->mail_key);
    if (ok)
        sodium_bin2hex(hex, blob_size * 2 + 1, blob, blob_size);
    sodium_memzero(blob, blob_size);
    free(blob);
    if (!ok) {
        sodium_memzero(hex, blob_size * 2 + 1);
        free(hex);
        return NULL;
    }
    return hex;
}
static int authenticate(PGconn *db, struct MHD_Connection *connection, char id[32],
                        char session_hash[65]) {
    const char *raw = MHD_lookup_connection_value(connection, MHD_COOKIE_KIND, "codaris_session");
    if (!raw || strlen(raw) != 64)
        return 0;
    digest(raw, session_hash);
    const char *v[] = {session_hash};
    PGresult *r = query(
        db, "SELECT user_id FROM app.sessions WHERE token_hash=$1 AND expires_at>now()", 1, v);
    int ok = r && PQntuples(r) == 1;
    if (ok)
        snprintf(id, 32, "%s", PQgetvalue(r, 0, 0));
    if (r)
        PQclear(r);
    return ok;
}
static void credential_random(char public_number[21], char verification_id[37]) {
    static const char alphabet[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
    unsigned char random[16];
    randombytes_buf(random, sizeof(random));
    memcpy(public_number, "CDR-", 4);
    for (size_t i=0;i<16;i++) public_number[4+i]=alphabet[random[i]&31u];
    public_number[20]=0;
    randombytes_buf(random, sizeof(random));
    random[6]=(unsigned char)((random[6]&0x0f)|0x40);
    random[8]=(unsigned char)((random[8]&0x3f)|0x80);
    sodium_bin2hex(verification_id,9,random,4);
    verification_id[8]='-'; sodium_bin2hex(verification_id+9,5,random+4,2);
    verification_id[13]='-'; sodium_bin2hex(verification_id+14,5,random+6,2);
    verification_id[18]='-'; sodium_bin2hex(verification_id+19,5,random+8,2);
    verification_id[23]='-'; sodium_bin2hex(verification_id+24,13,random+10,6);
    sodium_memzero(random,sizeof(random));
}
static int credential_create(PGconn *db, const char *user_id, int active) {
    for (int attempt=0;attempt<4;attempt++) {
        char number[21], uuid[37]; credential_random(number,uuid);
        const char *v[]={user_id,number,uuid,active?"active":"pending"};
        PGresult *r=query(db,"INSERT INTO app.membership_credentials(user_id,membership_number,verification_id,status,issued_at,public_enabled,consented_at) VALUES($1::bigint,$2,$3::uuid,$4,CASE WHEN $4='active' THEN now() ELSE NULL END,true,now()) ON CONFLICT DO NOTHING RETURNING user_id",4,v);
        if (!r) return 0;
        int inserted=PQntuples(r)==1; PQclear(r);
        if(inserted)return 1;
    }
    return 0;
}
static int uuid_ok(const char *s) {
    if(strlen(s)!=36)return 0;
    for(size_t i=0;i<36;i++) {
        if(i==8||i==13||i==18||i==23){if(s[i]!='-')return 0;}
        else if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')||(s[i]>='A'&&s[i]<='F')))return 0;
    }
    return 1;
}
static enum MHD_Result public_credential(PGconn *db, struct MHD_Connection *conn, const char *raw) {
    const char *generic="{\"valid\":false,\"message\":\"Credential verification is not publicly available.\"}";
    if(!uuid_ok(raw))return respond(conn,200,generic,NULL);
    char bucket[96];
    const char *peer=MHD_lookup_connection_value(conn,MHD_HEADER_KIND,"X-Real-IP");
    if(!peer||strlen(peer)>64)peer="local";
    snprintf(bucket,sizeof(bucket),"credential:%s",peer);
    if(limited(db,bucket,120))return respond(conn,429,"{\"message\":\"Please try again later.\"}",NULL);
    const char *v[]={raw};
    PGresult *r=query(db,"SELECT json_build_object('valid',true,'display_name',u.display_name,'membership_number',c.membership_number,'role',a.role,'status','active','issued_at',to_char(c.issued_at AT TIME ZONE 'UTC','FMMonth YYYY'))::text FROM app.membership_credentials c JOIN app.accounts a ON a.user_id=c.user_id JOIN app.users u ON u.id=c.user_id WHERE c.verification_id=$1::uuid AND c.public_enabled AND c.status='active' AND a.email_verified",1,v);
    if(r&&PQntuples(r)==1){enum MHD_Result result=respond(conn,200,PQgetvalue(r,0,0),NULL);PQclear(r);return result;}
    if(r)PQclear(r);
    return respond(conn,200,generic,NULL);
}
static enum MHD_Result route(const Config *c, struct MHD_Connection *conn, const char *path,
                             const char *method, Request *req) {
    char endpoint[128];
    const char *query_start=strchr(path,'?');
    size_t endpoint_length=query_start?(size_t)(query_start-path):strlen(path);
    if(endpoint_length>=sizeof(endpoint))return respond(conn,404,"{\"message\":\"Not found\"}",NULL);
    memcpy(endpoint,path,endpoint_length);endpoint[endpoint_length]=0;path=endpoint;
    if (!strcmp(path, "/api/health") && !strcmp(method, "GET")) {
        PGconn *health = PQconnectdb(c->database);
        int ok = health && PQstatus(health) == CONNECTION_OK;
        if (ok) {
            PGresult *ready = PQexec(health, "SELECT a.avatar_rgba,c.verification_id FROM app.accounts a JOIN app.membership_credentials c ON c.user_id=a.user_id LIMIT 0");
            ok = ready && PQresultStatus(ready) == PGRES_TUPLES_OK;
            if (ready)
                PQclear(ready);
        }
        if (health)
            PQfinish(health);
        if (ok) {
            char body[192];
            int sender_aligned = !*c->smtp_user || !strcmp(c->mail_from, c->smtp_user);
            int length = snprintf(body, sizeof(body),
                                  "{\"message\":\"Ready\",\"version\":\"%s\",\"contact_api\":1,\"credential_api\":2,\"mail_sender_aligned\":%s}",
                                  CODARIS_VERSION_STRING, sender_aligned ? "true" : "false");
            if (length < 0 || (size_t)length >= sizeof(body))
                return respond(conn, 503, "{\"message\":\"Unavailable\"}", NULL);
            return respond(conn, 200, body, NULL);
        }
        return respond(conn, 503, "{\"message\":\"Unavailable\"}", NULL);
    }
    int post = !strcmp(method, "POST");
    if (!post && strcmp(method, "GET"))
        return respond(conn, 405, "{\"message\":\"Method not allowed\"}", NULL);
    if (post) {
        const char *origin = MHD_lookup_connection_value(conn, MHD_HEADER_KIND, "Origin");
        const char *type = MHD_lookup_connection_value(conn, MHD_HEADER_KIND, "Content-Type");
        if (!origin || strcmp(origin, c->origin) || !type || strcmp(type, "application/json"))
            return respond(conn, 403, "{\"message\":\"Request origin or content type rejected\"}",
                           NULL);
    }
    if (req->overflow)
        return respond(conn, 413, "{\"message\":\"Request too large\"}", NULL);
    json_object *body = NULL;
    if (post) {
        json_tokener *tok = json_tokener_new_ex(16);
        if (!tok)
            return MHD_NO;
        json_tokener_set_flags(tok, JSON_TOKENER_STRICT | JSON_TOKENER_VALIDATE_UTF8);
        body = json_tokener_parse_ex(tok, req->body, (int)req->length);
        int valid = json_tokener_get_error(tok) == json_tokener_success && body &&
                    json_object_is_type(body, json_type_object);
        json_tokener_free(tok);
        if (!valid) {
            if (body)
                json_object_put(body);
            return respond(conn, 400, "{\"message\":\"Invalid JSON\"}", NULL);
        }
    }
    PGconn *db = PQconnectdb(c->database);
    unsigned status = 500;
    const char *message = "{\"message\":\"Service temporarily unavailable\"}";
    char cookie[256] = "", id[32] = "", session_hash[65] = "";
    PGresult *r = NULL;
    int transaction = 0;
    if (!db || PQstatus(db) != CONNECTION_OK)
        goto done;
    if (!strcmp(path, "/api/session") && !post) {
        unsigned session_status = authenticate(db, conn, id, session_hash) ? 204u : 401u;
        enum MHD_Result result = respond(conn, session_status,
                                         session_status == 204 ? "" : "{\"message\":\"Please sign in.\"}",
                                         NULL);
        if (body)
            json_object_put(body);
        PQfinish(db);
        return result;
    }
    if (post) {
        char bucket[256];
        const char *peer = MHD_lookup_connection_value(conn, MHD_HEADER_KIND, "X-Real-IP");
        if (!peer || strlen(peer) > 64)
            peer = "local";
        snprintf(bucket, sizeof(bucket), "request:%s:%s", peer, path);
        if (limited(db, bucket, !strcmp(path, "/api/resend") ? 3 : 60)) {
            status = 429;
            message = "{\"message\":\"Too many requests. Try again later.\"}";
            goto done;
        }
    }
    if (!strcmp(path, "/api/community") && !post) {
        r = query(db,
                  "SELECT json_build_object('members',count(*),'countries',count(DISTINCT "
                  "lower(country)))::text FROM app.accounts WHERE email_verified",
                  0, NULL);
        if (r && PQntuples(r) == 1) {
            enum MHD_Result result = respond(conn, 200, PQgetvalue(r, 0, 0), NULL);
            PQclear(r);
            PQfinish(db);
            return result;
        }
        goto done;
    }
    if (!strcmp(path,"/api/credential/verify") && !post) {
        const char *credential=MHD_lookup_connection_value(conn,MHD_GET_ARGUMENT_KIND,"credential");
        enum MHD_Result result=public_credential(db,conn,credential?credential:"");PQfinish(db);if(body)json_object_put(body);return result;
    }
    if (!strcmp(path, "/api/contact") && post) {
        const char *name = field(body, "name"), *topic = field(body, "topic"),
                   *message_text = field(body, "message");
        char email[255];
        if (!contact_text_ok(name, 2, 120, 0, 512) || !email_normalize(field(body, "email"), email) ||
            !codaris_contact_topic_allowed(topic) ||
            !contact_text_ok(message_text, 20, 4000, 1, 16000)) {
            status = 400;
            message = "{\"message\":\"Check your name, email, topic and message.\"}";
            goto done;
        }
        const char *peer = MHD_lookup_connection_value(conn, MHD_HEADER_KIND, "X-Real-IP");
        if (!peer || !*peer || strlen(peer) > 64)
            peer = "local";
        char ip_bucket[96], email_bucket[300];
        int ip_n = snprintf(ip_bucket, sizeof(ip_bucket), "contact-ip:%s", peer);
        int email_n = snprintf(email_bucket, sizeof(email_bucket), "contact-email:%s", email);
        if (ip_n < 0 || (size_t)ip_n >= sizeof(ip_bucket) || email_n < 0 ||
            (size_t)email_n >= sizeof(email_bucket) || limited(db, ip_bucket, 4) ||
            limited(db, email_bucket, 3)) {
            status = 429;
            message = "{\"message\":\"Too many messages. Please try again later.\"}";
            goto done;
        }
        json_object *payload = json_object_new_object();
        if (!payload) {
            goto done;
        }
        const char *payload_keys[] = {"name", "email", "topic", "message"};
        const char *payload_values[] = {name, email, topic, message_text};
        int payload_ok = 1;
        for (size_t i = 0; i < sizeof(payload_keys) / sizeof(payload_keys[0]); ++i) {
            json_object *value = json_object_new_string(payload_values[i]);
            if (!value || json_object_object_add(payload, payload_keys[i], value)) {
                if (value)
                    json_object_put(value);
                payload_ok = 0;
                break;
            }
        }
        if (!payload_ok) {
            json_object_put(payload);
            goto done;
        }
        const char *serialized = json_object_to_json_string_ext(payload, JSON_C_TO_STRING_PLAIN);
        char *encrypted = serialized ? contact_payload_encrypt(c, serialized) : NULL;
        if (serialized)
            sodium_memzero((void *)serialized, strlen(serialized));
        for (size_t i = 0; i < sizeof(payload_keys) / sizeof(payload_keys[0]); ++i) {
            json_object *value = NULL;
            if (json_object_object_get_ex(payload, payload_keys[i], &value) &&
                json_object_is_type(value, json_type_string)) {
                const char *text = json_object_get_string(value);
                sodium_memzero((void *)text, (size_t)json_object_get_string_len(value));
            }
        }
        json_object_put(payload);
        if (!encrypted)
            goto done;
        const char *values[] = {encrypted};
        int queued = command(db, "INSERT INTO app.contact_outbox(encrypted_payload) VALUES($1)", 1,
                             values);
        sodium_memzero(encrypted, strlen(encrypted));
        free(encrypted);
        if (!queued)
            goto done;
        status = 202;
        message = "{\"message\":\"Thanks, your message is queued. We aim to reply within "
                  CODARIS_CONTACT_REPLY_TARGET ". This is an aim, not a guaranteed deadline.\"}";
        goto done;
    }
    if (!strcmp(path, "/api/register") && post) {
        char email[255], linkedin[2049], github[2049], website[2049], hash[crypto_pwhash_STRBYTES],
            membership[25];
        const char *name = field(body, "name"), *country = field(body, "country"),
                   *role = field(body, "role"), *reason = field(body, "motivation"),
                   *password = field(body, "password");
        if (strcmp(field(body,"credential_public_consent"),"true") || !avatar_ok(field(body, "avatar")) || !length_ok(name, 2, 120) ||
            !length_ok(country, 2, 120) || !membership_country_allowed(country) ||
            !role_ok(role) || !length_ok(reason, 20, 2000) ||
            !password_ok(password) || !email_normalize(field(body, "email"), email) ||
            !url_normalize(field(body, "linkedin"), "linkedin.com", linkedin) ||
            !url_normalize(field(body, "github"), "github.com", github) ||
            !url_normalize(field(body, "website"), "", website))
            goto invalid;
        char bucket[300];
        snprintf(bucket, sizeof(bucket), "register:%s", email);
        if (limited(db, bucket, 5))
            goto throttled;
        if (crypto_pwhash_str(hash, password, strlen(password), crypto_pwhash_OPSLIMIT_INTERACTIVE,
                              crypto_pwhash_MEMLIMIT_INTERACTIVE))
            goto done;
        unsigned char bytes[24];
        randombytes_buf(bytes, sizeof(bytes));
        for (size_t i = 0; i < 24; i++)
            membership[i] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"[bytes[i] & 31u];
        membership[24] = 0;
        if (!command(db, "BEGIN", 0, NULL))
            goto done;
        transaction = 1;
        const char *uv[] = {membership, name};
        r = query(db, "INSERT INTO app.users(username,display_name) VALUES($1,$2) RETURNING id", 2,
                  uv);
        if (!r)
            goto done;
        snprintf(id, sizeof(id), "%s", PQgetvalue(r, 0, 0));
        PQclear(r);
        r = NULL;
        const char *av[] = {id, email, hash, country, role, reason, linkedin, github, website};
        int inserted =
            command(db,
                    "INSERT INTO "
                    "app.accounts(user_id,email,password_hash,country,role,motivation,linkedin,"
                    "github,website) VALUES($1::bigint,$2,$3,$4,$5,$6,$7,$8,$9)",
                    9, av);
        sodium_memzero(hash, sizeof(hash));
        if (!inserted) {
            status = 409;
            message = "{\"message\":\"Unable to create account. Try signing in or recovering your "
                      "account.\"}";
            goto done;
        }
        const char *image_values[] = {id, field(body, "avatar")};
        if (!command(db,
                     "UPDATE app.accounts SET avatar_rgba=decode(NULLIF($2,''),'base64') WHERE "
                     "user_id=$1::bigint",
                     2, image_values))
            goto done;
        if (!credential_create(db,id,0)) goto done;
        if (!queue_mail(db, c, id, email, "verify") || !command(db, "COMMIT", 0, NULL))
            goto done;
        transaction = 0;
        status = 201;
        message = "{\"message\":\"Account created. Check your email to verify it, then sign in "
                  "using your email and password.\"}";
        goto done;
    }
    if (!strcmp(path, "/api/login") && post) {
        const char *identifier = field(body, "identifier"), *password = field(body, "password");
        if (strlen(identifier) > 254 || strlen(password) > 512)
            goto invalid;
        char bucket[320];
        snprintf(bucket, sizeof(bucket), "login:%s", identifier);
        for (char *p = bucket; *p; p++)
            *p = (char)tolower((unsigned char)*p);
        if (limited(db, bucket, 15))
            goto throttled;
        const char *v[] = {identifier};
        r = query(db,
                  "SELECT a.user_id,a.password_hash FROM app.accounts a JOIN app.users u ON "
                  "u.id=a.user_id WHERE lower(a.email)=lower($1) OR lower(u.username)=lower($1)",
                  1, v);
        if (!r)
            goto done;
        /* Hash even nonexistent accounts to avoid a cheap username timing oracle. */
        char dummy[crypto_pwhash_STRBYTES];
        int verified = 0;
        if (PQntuples(r) == 1)
            verified = !crypto_pwhash_str_verify(PQgetvalue(r, 0, 1), password, strlen(password));
        else {
            if (crypto_pwhash_str(dummy, password, strlen(password),
                                  crypto_pwhash_OPSLIMIT_INTERACTIVE,
                                  crypto_pwhash_MEMLIMIT_INTERACTIVE))
                goto done;
            sodium_memzero(dummy, sizeof(dummy));
        }
        if (!verified) {
            status = 401;
            message = "{\"message\":\"Email, membership ID or password is incorrect.\"}";
            goto done;
        }
        snprintf(id, sizeof(id), "%s", PQgetvalue(r, 0, 0));
        PQclear(r);
        r = NULL;
        char raw[65], hash[65];
        new_token(raw);
        digest(raw, hash);
        const char *sv[] = {hash, id};
        if (!command(db, "INSERT INTO app.sessions(token_hash,user_id) VALUES($1,$2::bigint)", 2,
                     sv)) {
            sodium_memzero(raw, sizeof(raw));
            goto done;
        }
        snprintf(cookie, sizeof(cookie),
                 "codaris_session=%s; Path=/api/; HttpOnly; SameSite=Strict; Max-Age=2419200%s", raw,
                 c->production ? "; Secure" : "");
        sodium_memzero(raw, sizeof(raw));
        status = 200;
        message = "{\"message\":\"Signed in\"}";
        goto done;
    }
    if (!strcmp(path, "/api/recover") && post) {
        char email[255];
        if (!email_normalize(field(body, "email"), email)) {
            status = 400;
            message = "{\"message\":\"Enter a valid account email address with a dotted domain, "
                      "no surrounding spaces and at most 254 ASCII characters.\"}";
            goto done;
        }
        /* Never expose account-specific queue failures through recovery responses.
         * Rollback still runs at done; operators receive a non-identifying diagnostic. */
        status = 200;
        message = "{\"message\":\"If an account matches, a password reset link will be "
                  "emailed. You can also sign in with your email instead of your membership ID.\"}";
        char bucket[300];
        snprintf(bucket, sizeof(bucket), "recover:%s", email);
        if (!limited(db, bucket, 3)) {
            const char *v[] = {email};
            if (!command(db, "BEGIN", 0, NULL))
                goto done;
            transaction = 1;
            r = query(db, "SELECT user_id FROM app.accounts WHERE email=$1 FOR UPDATE", 1, v);
            if (!r)
                goto done;
            if (PQntuples(r) == 1 && !queue_mail(db, c, PQgetvalue(r, 0, 0), email, "reset")) {
                fprintf(stderr, "Password recovery queue failed; transaction rolled back.\n");
                goto done;
            }
            if (!command(db, "COMMIT", 0, NULL))
                goto done;
            transaction = 0;
        }
        status = 200;
        message = "{\"message\":\"If an account matches, a password reset link will be "
                  "emailed. You can also sign in with your email instead of your membership ID.\"}";
        goto done;
    }
    if ((!strcmp(path, "/api/verify") || !strcmp(path, "/api/reset")) && post) {
        int reset = !strcmp(path, "/api/reset");
        const char *token = field(body, "token"), *password = field(body, "password");
        if (strlen(token) != 64 || (reset && !password_ok(password)))
            goto invalid;
        char hash[65];
        digest(token, hash);
        const char *v[] = {hash, reset ? "reset" : "verify"};
        if (!command(db, "BEGIN", 0, NULL))
            goto done;
        transaction = 1;
        /* Lock account before consuming token, matching email/password mutation order. */
        r = query(db,
                  "SELECT a.user_id,a.email FROM app.accounts a JOIN app.action_tokens t ON "
                  "t.user_id=a.user_id WHERE t.token_hash=$1 AND t.kind=$2 AND t.email=a.email AND "
                  "t.expires_at>now() FOR UPDATE OF a,t",
                  2, v);
        if (!r)
            goto done;
        if (PQntuples(r) != 1) {
            status = 400;
            message = "{\"message\":\"This link is invalid or expired. Request a new one.\"}";
            goto done;
        }
        snprintf(id, sizeof(id), "%s", PQgetvalue(r, 0, 0));
        char email[255];
        snprintf(email, sizeof(email), "%s", PQgetvalue(r, 0, 1));
        PQclear(r);
        r = NULL;
        const char *av[] = {id};
        if (reset) {
            char pw[crypto_pwhash_STRBYTES];
            if (crypto_pwhash_str(pw, password, strlen(password),
                                  crypto_pwhash_OPSLIMIT_INTERACTIVE,
                                  crypto_pwhash_MEMLIMIT_INTERACTIVE))
                goto done;
            const char *pv[] = {id, pw};
            int ok = command(
                db, "UPDATE app.accounts SET password_hash=$2 WHERE user_id=$1::bigint", 2, pv);
            sodium_memzero(pw, sizeof(pw));
            if (!ok || !command(db, "DELETE FROM app.sessions WHERE user_id=$1::bigint", 1, av) ||
                !queue_mail(db, c, id, email, "password-changed"))
                goto done;
        } else if (!command(db,
                            "UPDATE app.accounts SET email_verified=true WHERE user_id=$1::bigint",
                            1, av) || !command(db,"UPDATE app.membership_credentials SET status='active',issued_at=coalesce(issued_at,now()),updated_at=now() WHERE user_id=$1::bigint AND status='pending'",1,av)) goto done;
        if (!command(db, "DELETE FROM app.action_tokens WHERE token_hash=$1", 1, v) ||
            !command(db, "COMMIT", 0, NULL))
            goto done;
        transaction = 0;
        status = 200;
        message = reset ? "{\"message\":\"Password changed. Sign in with your new password.\"}"
                        : "{\"message\":\"Email verified. Your membership is active.\"}";
        goto done;
    }
    if (!authenticate(db, conn, id, session_hash)) {
        status = 401;
        message = "{\"message\":\"Please sign in.\"}";
        goto done;
    }
    if (!strcmp(path,"/api/credential/public") && post) {
        const char *enabled=field(body,"enabled");
        if(strcmp(enabled,"true")&&strcmp(enabled,"false"))goto invalid;
        const char *v[]={id,enabled};
        if(!command(db,"UPDATE app.membership_credentials SET public_enabled=$2::boolean,consented_at=CASE WHEN $2::boolean THEN coalesce(consented_at,now()) ELSE NULL END,updated_at=now() WHERE user_id=$1::bigint AND status='active'",2,v))goto done;
        status=200;message=!strcmp(enabled,"true")?"{\"message\":\"Public credential verification enabled.\"}":"{\"message\":\"Public credential verification disabled.\"}";goto done;
    }
    if (!strcmp(path,"/api/credential/card") && !strcmp(method,"GET")) {
        const char *v[]={id};
        r=query(db,"SELECT u.display_name,a.role,c.membership_number,c.verification_id::text,c.status,to_char(c.issued_at AT TIME ZONE 'UTC','FMMonth YYYY'),replace(encode(a.avatar_rgba,'base64'),E'\\n','') FROM app.membership_credentials c JOIN app.accounts a ON a.user_id=c.user_id JOIN app.users u ON u.id=c.user_id WHERE c.user_id=$1::bigint AND c.status='active' AND a.email_verified",1,v);
        if(!r||PQntuples(r)!=1)goto done;
        const char *format=MHD_lookup_connection_value(conn,MHD_GET_ARGUMENT_KIND,"format");
        const char *side=MHD_lookup_connection_value(conn,MHD_GET_ARGUMENT_KIND,"side");
        int back=side&&!strcmp(side,"back");
        char url[640];int n=snprintf(url,sizeof(url),"%s/verify/?credential=%s",c->origin,PQgetvalue(r,0,3));
        CodarisCredential card={PQgetvalue(r,0,0),PQgetvalue(r,0,1),PQgetvalue(r,0,2),PQgetvalue(r,0,3),PQgetvalue(r,0,4),PQgetvalue(r,0,5),url,NULL,0};
        unsigned char *avatar=NULL;size_t avatar_size=0;
        if(*PQgetvalue(r,0,6)){avatar=malloc(40000);if(!avatar||sodium_base642bin(avatar,40000,PQgetvalue(r,0,6),strlen(PQgetvalue(r,0,6)),NULL,&avatar_size,NULL,sodium_base64_VARIANT_ORIGINAL)||avatar_size!=40000){free(avatar);avatar=NULL;goto done;}card.avatar_rgba=avatar;card.avatar_size=avatar_size;}
        if(n<0||(size_t)n>=sizeof(url)){free(avatar);goto done;}
        if(format&&!strcmp(format,"pdf")){unsigned char *pdf=NULL;size_t pdf_size=0;if(codaris_credential_pdf(&card,c->credential_font,&pdf,&pdf_size)){struct MHD_Response *response=MHD_create_response_from_buffer(pdf_size,pdf,MHD_RESPMEM_MUST_FREE);if(response){MHD_add_response_header(response,"Content-Type","application/pdf");MHD_add_response_header(response,"Cache-Control","no-store");MHD_add_response_header(response,"Content-Disposition","attachment; filename=codaris-membership.pdf");enum MHD_Result result=MHD_queue_response(conn,200,response);MHD_destroy_response(response);free(avatar);PQclear(r);PQfinish(db);if(body)json_object_put(body);return result;}free(pdf);}}
        else if(!format||!strcmp(format,"svg")){char *svg=NULL;size_t svg_size=0;if(codaris_credential_svg(&card,back,&svg,&svg_size)){struct MHD_Response *response=MHD_create_response_from_buffer(svg_size,svg,MHD_RESPMEM_MUST_FREE);if(response){MHD_add_response_header(response,"Content-Type","image/svg+xml; charset=utf-8");MHD_add_response_header(response,"Cache-Control","no-store");enum MHD_Result result=MHD_queue_response(conn,200,response);MHD_destroy_response(response);free(avatar);PQclear(r);PQfinish(db);if(body)json_object_put(body);return result;}free(svg);}}
        free(avatar);PQclear(r);r=NULL;
        status=400;message="{\"message\":\"Choose SVG or PDF credential output.\"}";goto done;
    }
    if (!strcmp(path, "/api/me") && !strcmp(method, "GET")) {
        const char *v[] = {id};
        r = query(
            db,
            "SELECT "
            "json_build_object('name',u.display_name,'membership_id',u.username,'email',a.email,'"
            "email_verified',a.email_verified,'country',a.country,'role',a.role,'motivation',a."
            "motivation,'linkedin',a.linkedin,'github',a.github,'website',a.website,'avatar',"
            "encode(a.avatar_rgba,'base64'),'credential',json_build_object('membership_number',c.membership_number,'verification_id',c.verification_id::text,'status',c.status,'issued_at',c.issued_at,'public_enabled',c.public_enabled),'progress',"
            "COALESCE((SELECT sum(1::bigint << topic::integer) FROM app.learning_progress WHERE "
            "user_id=a.user_id),0))::text FROM "
            "app.accounts a JOIN app.users u ON u.id=a.user_id JOIN app.membership_credentials c ON c.user_id=a.user_id WHERE a.user_id=$1::bigint",
            1, v);
        if (r && PQntuples(r) == 1) {
            enum MHD_Result result = respond(conn, 200, PQgetvalue(r, 0, 0), NULL);
            PQclear(r);
            PQfinish(db);
            if (body)
                json_object_put(body);
            return result;
        }
        goto done;
    }
    if (!post) {
        status = 404;
        message = "{\"message\":\"Not found\"}";
        goto done;
    }
    if (!strcmp(path, "/api/progress")) {
        const char *topic = field(body, "topic"), *read = field(body, "read");
        char *end = NULL;
        long number = strtol(topic, &end, 10);
        if (!*topic || !end || *end || number < 0 || number > 21 ||
            (strcmp(read, "0") && strcmp(read, "1")))
            goto invalid;
        const char *v[] = {id, topic};
        if (!command(db,
                     !strcmp(read, "1") ? "INSERT INTO app.learning_progress(user_id,topic) "
                                          "VALUES($1::bigint,$2::smallint) ON CONFLICT DO NOTHING"
                                        : "DELETE FROM app.learning_progress WHERE "
                                          "user_id=$1::bigint AND topic=$2::smallint",
                     2, v))
            goto done;
        status = 200;
        message = "{\"message\":\"Progress saved.\"}";
        goto done;
    }
    if (!strcmp(path, "/api/logout")) {
        const char *v[] = {session_hash};
        if (!command(db, "DELETE FROM app.sessions WHERE token_hash=$1", 1, v))
            goto done;
        snprintf(cookie, sizeof(cookie),
                 "codaris_session=; Path=/api/; HttpOnly; SameSite=Strict; Max-Age=0%s",
                 c->production ? "; Secure" : "");
        status = 200;
        message = "{\"message\":\"Signed out\"}";
        goto done;
    }
    if (!command(db, "BEGIN", 0, NULL))
        goto done;
    transaction = 1;
    const char *iv[] = {id};
    r = query(db,
              "SELECT email,password_hash,email_verified FROM app.accounts WHERE "
              "user_id=$1::bigint FOR UPDATE",
              1, iv);
    if (!r || PQntuples(r) != 1)
        goto done;
    char old_email[255];
    snprintf(old_email, sizeof(old_email), "%s", PQgetvalue(r, 0, 0));
    if (!strcmp(path, "/api/profile")) {
        char linkedin[2049], github[2049], website[2049];
        const char *name = field(body, "name"), *country = field(body, "country"),
                   *role = field(body, "role");
        json_object *ignored;
        if (!avatar_ok(field(body, "avatar")) ||
            json_object_object_get_ex(body, "motivation", &ignored) || !length_ok(name, 2, 120) ||
            !length_ok(country, 2, 120) || !role_ok(role) ||
            !url_normalize(field(body, "linkedin"), "linkedin.com", linkedin) ||
            !url_normalize(field(body, "github"), "github.com", github) ||
            !url_normalize(field(body, "website"), "", website))
            goto invalid;
        const char *v[] = {id, name, country, role, linkedin, github, website};
        if (!command(db,
                     "UPDATE app.users SET display_name=$2,updated_at=now() WHERE id=$1::bigint", 2,
                     v))
            goto done;
        const char *p[] = {id, country, role, linkedin, github, website};
        if (!command(db,
                     "UPDATE app.accounts SET country=$2,role=$3,linkedin=$4,github=$5,website=$6 "
                     "WHERE user_id=$1::bigint",
                     6, p))
            goto done;
        const char *image_values[] = {id, field(body, "avatar")};
        if (!command(db,
                     "UPDATE app.accounts SET avatar_rgba=decode(NULLIF($2,''),'base64') WHERE "
                     "user_id=$1::bigint",
                     2, image_values))
            goto done;
        message = "{\"message\":\"Profile saved.\"}";
    } else if (!strcmp(path, "/api/resend")) {
        if (!strcmp(PQgetvalue(r, 0, 2), "t")) {
            message = "{\"message\":\"Your email is already verified.\"}";
        } else {
            if (!queue_mail(db, c, id, old_email, "verify"))
                goto done;
            message = "{\"message\":\"Verification email queued.\"}";
        }
    } else if (!strcmp(path, "/api/email") || !strcmp(path, "/api/password")) {
        const char *current = field(body, "current_password");
        if (strlen(current) > 512 ||
            crypto_pwhash_str_verify(PQgetvalue(r, 0, 1), current, strlen(current))) {
            status = 403;
            message = "{\"message\":\"Current password is incorrect.\"}";
            goto done;
        }
        if (!strcmp(path, "/api/email")) {
            char email[255];
            if (!email_normalize(field(body, "email"), email) || !strcmp(email, old_email))
                goto invalid;
            const char *v[] = {id, email};
            if (!command(db,
                         "UPDATE app.accounts SET email=$2,email_verified=false WHERE "
                         "user_id=$1::bigint",
                         2, v)) {
                status = 409;
                message = "{\"message\":\"That email cannot be used.\"}";
                goto done;
            }
            if (!command(db, "DELETE FROM app.action_tokens WHERE user_id=$1::bigint", 1, iv) ||
                !command(db,
                         "DELETE FROM app.mail_outbox WHERE user_id=$1::bigint AND kind IN "
                         "('verify','reset') AND sent_at IS NULL",
                         1, iv) ||
                !queue_mail(db, c, id, email, "verify") ||
                !queue_mail(db, c, id, old_email, "email-changed"))
                goto done;
            const char *sv[] = {id, session_hash};
            if (!command(db, "DELETE FROM app.sessions WHERE user_id=$1::bigint AND token_hash<>$2",
                         2, sv))
                goto done;
            message = "{\"message\":\"Email changed. Your account is unverified until you confirm "
                      "the link sent to your new address.\"}";
        } else {
            const char *password = field(body, "password");
            if (!password_ok(password) || !strcmp(password, current))
                goto invalid;
            char hash[crypto_pwhash_STRBYTES];
            if (crypto_pwhash_str(hash, password, strlen(password),
                                  crypto_pwhash_OPSLIMIT_INTERACTIVE,
                                  crypto_pwhash_MEMLIMIT_INTERACTIVE))
                goto done;
            const char *v[] = {id, hash};
            int ok = command(
                db, "UPDATE app.accounts SET password_hash=$2 WHERE user_id=$1::bigint", 2, v);
            sodium_memzero(hash, sizeof(hash));
            if (!ok || !command(db, "DELETE FROM app.sessions WHERE user_id=$1::bigint", 1, iv) ||
                !command(db,
                         "DELETE FROM app.action_tokens WHERE user_id=$1::bigint AND kind='reset'",
                         1, iv) ||
                !queue_mail(db, c, id, old_email, "password-changed"))
                goto done;
            message = "{\"message\":\"Password changed. Please sign in again.\"}";
        }
    } else {
        status = 404;
        message = "{\"message\":\"Not found\"}";
        goto done;
    }
    if (!command(db, "COMMIT", 0, NULL))
        goto done;
    transaction = 0;
    status = 200;
    goto done;
invalid:
    status = 400;
    message = "{\"message\":\"Check the fields, password length and HTTPS profile URLs. The "
              "original application reason cannot be edited.\"}";
    goto done;
throttled:
    status = 429;
    message = "{\"message\":\"Too many attempts. Try again later.\"}";
done:
    if (r)
        PQclear(r);
    if (transaction)
        command(db, "ROLLBACK", 0, NULL);
    if (db)
        PQfinish(db);
    if (body && !strcmp(path, "/api/contact")) {
        const char *keys[] = {"name", "email", "topic", "message"};
        for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
            json_object *value = NULL;
            if (json_object_object_get_ex(body, keys[i], &value) &&
                json_object_is_type(value, json_type_string)) {
                const char *text = json_object_get_string(value);
                sodium_memzero((void *)text, (size_t)json_object_get_string_len(value));
            }
        }
    }
    if (body)
        json_object_put(body);
    return respond(conn, status, message, *cookie ? cookie : NULL);
}
static enum MHD_Result handler(void *cls, struct MHD_Connection *conn, const char *url,
                               const char *method, const char *version, const char *data,
                               size_t *size, void **state) {
    (void)version;
    if (!*state) {
        *state = calloc(1, sizeof(Request));
        return *state ? MHD_YES : MHD_NO;
    }
    Request *r = *state;
    if (*size) {
        if (*size > BODY_MAX - r->length)
            r->overflow = 1;
        else if (!r->overflow) {
            memcpy(r->body + r->length, data, *size);
            r->length += *size;
            r->body[r->length] = 0;
        }
        *size = 0;
        return MHD_YES;
    }
    enum MHD_Result result = route(cls, conn, url, method, r);
    sodium_memzero(r->body, sizeof(r->body));
    return result;
}
static void complete(void *cls, struct MHD_Connection *conn, void **state,
                     enum MHD_RequestTerminationCode code) {
    (void)cls;
    (void)conn;
    (void)code;
    if (*state) {
        sodium_memzero(*state, sizeof(Request));
        free(*state);
        *state = NULL;
    }
}
int api_run(const Config *c) {
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(c->port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    struct MHD_Daemon *server = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD, c->port, NULL, NULL, handler, (void *)c,
        MHD_OPTION_SOCK_ADDR, &address, MHD_OPTION_CONNECTION_LIMIT, (unsigned int)64,
        MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int)15, MHD_OPTION_CONNECTION_MEMORY_LIMIT,
        (size_t)32768, MHD_OPTION_NOTIFY_COMPLETED, complete, NULL, MHD_OPTION_END);
    if (!server)
        return 0;
    signal(SIGINT, stop_service);
    signal(SIGTERM, stop_service);
    puts("CODARIS account API listening on loopback.");
    fflush(stdout);
    while (!stopping) {
#ifdef _WIN32
        Sleep(200);
#else
        struct timespec delay = {0, 200000000};
        nanosleep(&delay, NULL);
#endif
    }
    MHD_stop_daemon(server);
    return 1;
}
