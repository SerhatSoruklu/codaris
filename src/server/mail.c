#include "codaris/service.h"
#include "../shared/contact_options.h"
#include "../shared/contact_policy.h"
#include <curl/curl.h>
#include <sodium.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
/* Fixed copy and a validated origin mean user data never enters HTML or headers.
 * Token-containing buffers are wiped; provider errors are not logged. */
static int send_mail(const Config *c, const char *id, const char *recipient, const char *kind,
                     const char *token) {
    if (strpbrk(recipient, "\r\n<> ") || !strchr(recipient, '@'))
        return 0;
    const char *subject, *copy, *button;
    if (!strcmp(kind, "verify")) {
        subject = "Verify your CODARIS email";
        copy = "Confirm your email address to activate your membership. This link expires in 24 "
               "hours and works once. If you did not create this account, ignore this message.";
        button = "Verify email";
    } else if (!strcmp(kind, "reset")) {
        subject = "Reset your CODARIS password";
        copy = "Choose a new password. This link expires in 30 minutes and works once. If you did "
               "not request this, ignore this message.";
        button = "Reset password";
    } else if (!strcmp(kind, "email-changed")) {
        subject = "Your CODARIS email changed";
        copy = "Your account email was changed. The new address must be verified before member "
               "access resumes. If this was not you, contact contact@codaris.org immediately.";
        button = "Open CODARIS";
    } else {
        subject = "Your CODARIS password changed";
        copy = "Your password was changed and all sessions were signed out. If this was not you, "
               "contact contact@codaris.org immediately.";
        button = "Open CODARIS";
    }
    char link[1024], text[2048], html[4096], from[320], to[320], title[128], message_id[128];
    int n =
        snprintf(link, sizeof(link), "%s/%s/%s%s", c->origin,
                 *token ? (!strcmp(kind, "reset") ? "reset-password" : "verify-email") : "login",
                 *token ? "#" : "", token);
    if (n < 0 || (size_t)n >= sizeof(link))
        return 0;
    /* Config origin is parsed as an origin; escape its only possible HTML metacharacter. */
    if (strpbrk(link, "<>\"'&"))
        return 0;
    n = snprintf(text, sizeof(text),
                 "CODARIS — BUILD. VERIFY. ADVANCE.\n\n%s\n\n%s\n\n%s: %s\n\nNever share your "
                 "password.\nContact: contact@codaris.org\n",
                 subject, copy, button, link);
    if (n < 0 || (size_t)n >= sizeof(text))
        return 0;
    n = snprintf(
        html, sizeof(html),
        "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" "
        "content=\"width=device-width,initial-scale=1\"><title>%s</title></head><body "
        "style=\"margin:0;background:#ffffff;color:#172b36;font-family:Arial,sans-serif\"><table "
        "role=\"presentation\" width=\"100%%\"><tr><td align=\"center\" style=\"padding:28px "
        "16px\"><table role=\"presentation\" width=\"100%%\" "
        "style=\"max-width:620px;background:#ffffff;border:1px solid "
        "#dce5e9;border-radius:14px\"><tr><td style=\"padding:32px\"><p "
        "style=\"color:#087c86;letter-spacing:2px\">CODARIS</p><h1 "
        "style=\"font-size:26px\">%s</h1><p style=\"line-height:1.7;color:#526571\">%s</p><p "
        "style=\"padding:16px 0\"><a href=\"%s\" style=\"display:inline-block;padding:14px "
        "20px;background:#4be3ed;color:#080f16;border-radius:8px;text-decoration:none;font-weight:"
        "bold\">%s</a></p><p style=\"font-size:12px;color:#526571\">Never share your password. If "
        "the button does not work, use the link in the plain-text version of this "
        "email.</p></td></tr></table><p style=\"font-size:12px;color:#526571\">BUILD. VERIFY. "
        "ADVANCE.<br>contact@codaris.org</p></td></tr></table></body></html>",
        subject, subject, copy, link, button);
    if (n < 0 || (size_t)n >= sizeof(html))
        return 0;
    snprintf(from, sizeof(from), "From: CODARIS <%s>", c->mail_from);
    snprintf(to, sizeof(to), "To: <%s>", recipient);
    snprintf(title, sizeof(title), "Subject: %s", subject);
    snprintf(message_id, sizeof(message_id), "Message-ID: <codaris-%s@codaris.org>", id);
    CURL *curl = curl_easy_init();
    curl_mime *mime = NULL;
    struct curl_slist *headers = NULL, *recipients = NULL;
    int ok = 0;
    if (!curl)
        goto done;
    char date[80];
    time_t now = time(NULL);
    struct tm *utc = gmtime(&now);
    if (now == (time_t)-1 || !utc ||
        !strftime(date, sizeof(date), "Date: %a, %d %b %Y %H:%M:%S +0000", utc))
        goto done;
    const char *lines[] = {from,
                           to,
                           title,
                           message_id,
                           date,
                           "Reply-To: contact@codaris.org",
                           "MIME-Version: 1.0",
                           "Content-Type: multipart/alternative"};
    for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        struct curl_slist *next = curl_slist_append(headers, lines[i]);
        if (!next)
            goto done;
        headers = next;
    }
    recipients = curl_slist_append(NULL, recipient);
    if (!recipients)
        goto done;
    mime = curl_mime_init(curl);
    if (!mime)
        goto done;
    curl_mimepart *part = curl_mime_addpart(mime);
    if (!part || curl_mime_data(part, text, CURL_ZERO_TERMINATED) ||
        curl_mime_type(part, "text/plain; charset=utf-8") ||
        curl_mime_encoder(part, "quoted-printable"))
        goto done;
    part = curl_mime_addpart(mime);
    if (!part || curl_mime_data(part, html, CURL_ZERO_TERMINATED) ||
        curl_mime_type(part, "text/html; charset=utf-8") ||
        curl_mime_encoder(part, "quoted-printable"))
        goto done;
#define OPT(k, v)                                                                                  \
    do {                                                                                           \
        if (curl_easy_setopt(curl, k, v) != CURLE_OK)                                              \
            goto done;                                                                             \
    } while (0)
    OPT(CURLOPT_URL, c->smtp_url);
    OPT(CURLOPT_MAIL_FROM, c->mail_from);
    OPT(CURLOPT_MAIL_RCPT, recipients);
    OPT(CURLOPT_USERNAME, c->smtp_user);
    OPT(CURLOPT_PASSWORD, c->smtp_password);
    OPT(CURLOPT_USE_SSL, c->production ? CURLUSESSL_ALL : CURLUSESSL_NONE);
    OPT(CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    OPT(CURLOPT_SSL_VERIFYPEER, 1L);
    OPT(CURLOPT_SSL_VERIFYHOST, 2L);
    OPT(CURLOPT_CONNECTTIMEOUT, 10L);
    OPT(CURLOPT_TIMEOUT, 30L);
    OPT(CURLOPT_NOSIGNAL, 1L);
    OPT(CURLOPT_HTTPHEADER, headers);
    OPT(CURLOPT_MIMEPOST, mime);
    ok = curl_easy_perform(curl) == CURLE_OK;
#undef OPT
done:
    curl_slist_free_all(headers);
    curl_slist_free_all(recipients);
    curl_mime_free(mime);
    curl_easy_cleanup(curl);
    sodium_memzero(link, sizeof(link));
    sodium_memzero(text, sizeof(text));
    sodium_memzero(html, sizeof(html));
    return ok;
}
static int contact_email_valid(const char *email) {
    size_t n = strlen(email);
    const char *at = strchr(email, '@');
    if (n < 3 || n > 254 || !at || at == email || strchr(at + 1, '@') || !strchr(at + 1, '.') ||
        at[1] == '.' || email[n - 1] == '.' || email[0] == '.' || at[-1] == '.' ||
        strstr(email, "..") || strpbrk(email, "\r\n<> ")) return 0;
    for (const unsigned char *p = (const unsigned char *)email; *p; ++p)
        if (!(isalnum(*p) && *p < 128) && !strchr(".!#$%&'*+-/=?^_`{|}~@", *p)) return 0;
    return 1;
}
static int mail_header_add(struct curl_slist **headers, const char *value) {
    struct curl_slist *next = curl_slist_append(*headers, value);
    if (!next) return 0;
    *headers = next;
    return 1;
}
static char *contact_payload_decrypt(const Config *c, const char *encoded) {
    size_t hex_length = strlen(encoded);
    if (hex_length < 80 || hex_length > 36100 || (hex_length & 1u)) return NULL;
    size_t blob_capacity = hex_length / 2;
    unsigned char *blob = malloc(blob_capacity);
    if (!blob) return NULL;
    size_t blob_length = 0;
    int decoded = !sodium_hex2bin(blob, blob_capacity, encoded, hex_length, NULL, &blob_length, NULL);
    if (!decoded || blob_length < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
        sodium_memzero(blob, blob_capacity); free(blob); return NULL;
    }
    size_t plain_length = blob_length - crypto_secretbox_NONCEBYTES - crypto_secretbox_MACBYTES;
    char *plain = malloc(plain_length + 1);
    int opened = plain && !crypto_secretbox_open_easy((unsigned char *)plain,
        blob + crypto_secretbox_NONCEBYTES,
        (unsigned long long)(blob_length - crypto_secretbox_NONCEBYTES), blob, c->mail_key);
    sodium_memzero(blob, blob_capacity); free(blob);
    if (!opened) { free(plain); return NULL; }
    plain[plain_length] = 0;
    return plain;
}
static int send_contact_mail(const Config *c, const char *id, const char *kind,
                             const char *recipient, const char *reply_to, const char *name,
                             const char *email, const char *topic, const char *message) {
    int is_admin = !strcmp(kind, "admin");
    if ((!is_admin && strcmp(kind, "receipt")) || !contact_email_valid(recipient) ||
        (is_admin && !contact_email_valid(reply_to)) || !codaris_contact_topic_allowed(topic)) return 0;
    size_t cap = strlen(message) + 2048;
    if (cap > 20000) return 0;
    char *text = malloc(cap);
    if (!text) return 0;
    int n = is_admin
        ? snprintf(text, cap, "New CODARIS contact message\n\nName: %s\nEmail: %s\nTopic: %s\n\nMessage:\n%s\n", name, email, topic, message)
        : snprintf(text, cap, "Hello,\n\nCODARIS has received your message about: %s.\n\nWe aim to reply within " CODARIS_CONTACT_REPLY_TARGET ". This is an aim, not a guaranteed deadline.\n\nPlease do not reply with passwords, verification links or sensitive personal information.\n\nCODARIS\n", topic);
    if (n < 0 || (size_t)n >= cap) { sodium_memzero(text, cap); free(text); return 0; }
    size_t body_len = (size_t)n;
    char from[320], to[320], subject[128], message_id[160], reply[320], date[80];
    n = snprintf(from, sizeof(from), "From: CODARIS <%s>", c->mail_from);
    int valid = n > 0 && (size_t)n < sizeof(from);
    if (valid) {
        n = snprintf(to, sizeof(to), "To: <%s>", recipient);
        valid = n > 0 && (size_t)n < sizeof(to);
    }
    const char *subject_text = is_admin ? "CODARIS contact message" : "We received your message to CODARIS";
    if (valid) {
        n = snprintf(subject, sizeof(subject), "Subject: %s", subject_text);
        valid = n > 0 && (size_t)n < sizeof(subject);
    }
    if (valid) {
        n = snprintf(message_id, sizeof(message_id), "Message-ID: <codaris-contact-%s-%s@codaris.org>", id, kind);
        valid = n > 0 && (size_t)n < sizeof(message_id);
    }
    if (valid && is_admin) {
        n = snprintf(reply, sizeof(reply), "Reply-To: <%s>", reply_to);
        valid = n > 0 && (size_t)n < sizeof(reply);
    }
    time_t now = time(NULL);
    struct tm utc_buffer;
#ifdef _WIN32
    int time_ok = now != (time_t)-1 && gmtime_s(&utc_buffer, &now) == 0;
#else
    int time_ok = now != (time_t)-1 && gmtime_r(&now, &utc_buffer) != NULL;
#endif
    if (valid && time_ok)
        valid = strftime(date, sizeof(date), "Date: %a, %d %b %Y %H:%M:%S +0000", &utc_buffer) != 0;
    else
        valid = 0;
    struct curl_slist *headers = NULL, *recipients = NULL;
    CURL *curl = NULL;
    curl_mime *mime = NULL;
    int ok = 0;
    if (valid) valid = mail_header_add(&headers, from) && mail_header_add(&headers, to) &&
        mail_header_add(&headers, subject) && mail_header_add(&headers, message_id) &&
        mail_header_add(&headers, date) && mail_header_add(&headers, "MIME-Version: 1.0") &&
        (!is_admin || mail_header_add(&headers, reply));
    if (valid) recipients = curl_slist_append(NULL, recipient);
    if (valid && recipients) curl = curl_easy_init();
    if (curl) mime = curl_mime_init(curl);
    curl_mimepart *part = mime ? curl_mime_addpart(mime) : NULL;
    if (valid && recipients && curl && mime && part && !curl_mime_data(part, text, body_len) &&
        !curl_mime_type(part, "text/plain; charset=utf-8") &&
        !curl_mime_encoder(part, "quoted-printable")) {
#define CONTACT_SETOPT(k, v) do { if (curl_easy_setopt(curl, k, v) != CURLE_OK) valid = 0; } while (0)
        CONTACT_SETOPT(CURLOPT_URL, c->smtp_url);
        CONTACT_SETOPT(CURLOPT_MAIL_FROM, c->mail_from);
        CONTACT_SETOPT(CURLOPT_MAIL_RCPT, recipients);
        CONTACT_SETOPT(CURLOPT_USERNAME, c->smtp_user);
        CONTACT_SETOPT(CURLOPT_PASSWORD, c->smtp_password);
        CONTACT_SETOPT(CURLOPT_USE_SSL, c->production ? CURLUSESSL_ALL : CURLUSESSL_NONE);
        CONTACT_SETOPT(CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        CONTACT_SETOPT(CURLOPT_SSL_VERIFYPEER, 1L);
        CONTACT_SETOPT(CURLOPT_SSL_VERIFYHOST, 2L);
        CONTACT_SETOPT(CURLOPT_CONNECTTIMEOUT, 10L);
        CONTACT_SETOPT(CURLOPT_TIMEOUT, 30L);
        CONTACT_SETOPT(CURLOPT_NOSIGNAL, 1L);
        CONTACT_SETOPT(CURLOPT_HTTPHEADER, headers);
        CONTACT_SETOPT(CURLOPT_MIMEPOST, mime);
#undef CONTACT_SETOPT
        if (valid) ok = curl_easy_perform(curl) == CURLE_OK;
    }
    curl_slist_free_all(headers);
    curl_slist_free_all(recipients);
    if (mime) curl_mime_free(mime);
    if (curl) curl_easy_cleanup(curl);
    sodium_memzero(text, cap);
    free(text);
    return ok;
}
static int deliver_contact(const Config *c, const char *id, const char *cipher, const char *kind) {
    char *plain = contact_payload_decrypt(c, cipher);
    if (!plain) return 0;
    json_tokener *tokener = json_tokener_new_ex(8);
    json_object *payload = tokener ? json_tokener_parse_ex(tokener, plain, (int)strlen(plain)) : NULL;
    int valid = payload && tokener && json_object_is_type(payload, json_type_object) &&
                json_tokener_get_error(tokener) == json_tokener_success;
    if (tokener) json_tokener_free(tokener);
    const char *name = "", *email = "", *topic = "", *message = "";
    json_object *value = NULL;
#define GET_CONTACT_FIELD(k, dst) do { \
    if (!payload || !json_object_object_get_ex(payload, k, &value) || \
        !json_object_is_type(value, json_type_string)) valid = 0; \
    else dst = json_object_get_string(value); \
} while (0)
    if (valid) {
        GET_CONTACT_FIELD("name", name);
        GET_CONTACT_FIELD("email", email);
        GET_CONTACT_FIELD("topic", topic);
        GET_CONTACT_FIELD("message", message);
    }
#undef GET_CONTACT_FIELD
    int ok = 0;
    if (valid && strlen(name) <= 512 && contact_email_valid(email) &&
        codaris_contact_topic_allowed(topic) && strlen(message) <= 16000) {
        const char *recipient = !strcmp(kind, "admin") ? CODARIS_CONTACT_INBOX : email;
        ok = send_contact_mail(c, id, kind, recipient, email, name, email, topic, message);
    }
    if (payload) {
        const char *keys[] = {"name", "email", "topic", "message"};
        for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
            json_object *field = NULL;
            if (json_object_object_get_ex(payload, keys[i], &field) &&
                json_object_is_type(field, json_type_string)) {
                const char *text = json_object_get_string(field);
                sodium_memzero((void *)text, (size_t)json_object_get_string_len(field));
            }
        }
        json_object_put(payload);
    }
    sodium_memzero(plain, strlen(plain));
    free(plain);
    return ok;
}
/* Process one contact queue item. Return 1 after delivery, 0 when empty, -1 on failure. */
static int contact_outbox_once(PGconn *db, const Config *c) {
    PGresult *r = PQexec(db, "BEGIN");
    if (!r || PQresultStatus(r) != PGRES_COMMAND_OK) {
        if (r) PQclear(r);
        return -1;
    }
    PQclear(r);
    r = PQexec(db,
        "SELECT id,encrypted_payload,(admin_sent_at IS NULL AND admin_attempts<8) "
        "FROM app.contact_outbox WHERE created_at>now()-interval '30 days' "
        "AND available_at<=now() AND ((admin_sent_at IS NULL AND admin_attempts<8) OR "
        "(receipt_sent_at IS NULL AND receipt_attempts<8)) ORDER BY id "
        "FOR UPDATE SKIP LOCKED LIMIT 1");
    if (!r || PQresultStatus(r) != PGRES_TUPLES_OK) {
        if (r) PQclear(r);
        r = PQexec(db, "ROLLBACK");
        if (r) PQclear(r);
        return -1;
    }
    if (!PQntuples(r)) {
        PQclear(r);
        r = PQexec(db, "COMMIT");
        int committed = r && PQresultStatus(r) == PGRES_COMMAND_OK;
        if (r) PQclear(r);
        return committed ? 0 : -1;
    }
    char row_id[32];
    int id_length = snprintf(row_id, sizeof(row_id), "%s", PQgetvalue(r, 0, 0));
    if (id_length <= 0 || (size_t)id_length >= sizeof(row_id)) {
        PQclear(r);
        r = PQexec(db, "ROLLBACK");
        if (r) PQclear(r);
        return -1;
    }
    const char *cipher = PQgetvalue(r, 0, 1);
    int is_admin = !strcmp(PQgetvalue(r, 0, 2), "t");
    int sent = deliver_contact(c, row_id, cipher, is_admin ? "admin" : "receipt");
    const char *values[] = {row_id};
    const char *sql;
    if (is_admin)
        sql = sent
            ? "UPDATE app.contact_outbox SET admin_sent_at=now(),admin_attempts=admin_attempts+1,available_at=now() WHERE id=$1::bigint"
            : "UPDATE app.contact_outbox SET admin_attempts=admin_attempts+1,available_at=now()+interval '5 minutes'*(admin_attempts+1) WHERE id=$1::bigint";
    else
        sql = sent
            ? "UPDATE app.contact_outbox SET receipt_sent_at=now(),receipt_attempts=receipt_attempts+1,available_at=now() WHERE id=$1::bigint"
            : "UPDATE app.contact_outbox SET receipt_attempts=receipt_attempts+1,available_at=now()+interval '5 minutes'*(receipt_attempts+1) WHERE id=$1::bigint";
    PGresult *updated = PQexecParams(db, sql, 1, NULL, values, NULL, NULL, 0);
    PQclear(r);
    if (!updated || PQresultStatus(updated) != PGRES_COMMAND_OK) {
        if (updated) PQclear(updated);
        r = PQexec(db, "ROLLBACK");
        if (r) PQclear(r);
        return -1;
    }
    PQclear(updated);
    if (sent) {
        PGresult *removed = PQexecParams(db,
            "DELETE FROM app.contact_outbox WHERE id=$1::bigint AND admin_sent_at IS NOT NULL AND receipt_sent_at IS NOT NULL",
            1, NULL, values, NULL, NULL, 0);
        int deleted = removed && PQresultStatus(removed) == PGRES_COMMAND_OK;
        if (removed) PQclear(removed);
        if (!deleted) {
            r = PQexec(db, "ROLLBACK");
            if (r) PQclear(r);
            return -1;
        }
    }
    r = PQexec(db, "COMMIT");
    int committed = r && PQresultStatus(r) == PGRES_COMMAND_OK;
    if (r) PQclear(r);
    if (!committed) return -1;
    if (!sent) {
        fputs("Contact mail delivery failed; retry scheduled.\n", stderr);
        return -1;
    }
    return 1;
}

int mail_run(const Config *c) {
    PGconn *db = PQconnectdb(c->database);
    if (!db || PQstatus(db) != CONNECTION_OK) {
        if (db)
            PQfinish(db);
        return 0;
    }
    PGresult *cleanup = PQexec(
        db, "DELETE FROM app.sessions WHERE expires_at<now();"
            "DELETE FROM app.action_tokens WHERE expires_at<now();"
            "DELETE FROM app.rate_limits WHERE window_start<now()-interval '1 day';"
            "DELETE FROM app.mail_outbox WHERE created_at<now()-interval '30 days';"
            "DELETE FROM app.contact_outbox WHERE created_at<now()-interval '30 days' OR "
            "(admin_sent_at IS NOT NULL AND receipt_sent_at IS NOT NULL);"
            "DELETE FROM app.mail_outbox o WHERE sent_at IS NULL AND kind IN ('verify','reset') "
            "AND NOT EXISTS (SELECT 1 FROM app.action_tokens t WHERE t.user_id=o.user_id AND "
            "t.kind=o.kind AND t.email=o.recipient AND t.expires_at>now());");
    if (!cleanup || PQresultStatus(cleanup) != PGRES_COMMAND_OK) {
        if (cleanup)
            PQclear(cleanup);
        PQfinish(db);
        return 0;
    }
    PQclear(cleanup);
    int ok = 1;
    /* A bounded drain is suitable for a systemd timer. Row locks prevent concurrent
     * workers from sending the same event at the same time. */
    for (int i = 0; i < 20; i++) {
        PGresult *r = PQexec(db, "BEGIN");
        if (!r || PQresultStatus(r) != PGRES_COMMAND_OK) {
            if (r)
                PQclear(r);
            ok = 0;
            break;
        }
        PQclear(r);
        r = PQexec(
            db,
            "SELECT id,recipient,kind,encrypted_token FROM app.mail_outbox WHERE sent_at IS NULL "
            "AND attempts<8 AND available_at<=now() ORDER BY id FOR UPDATE SKIP LOCKED LIMIT 1");
        if (!r || PQresultStatus(r) != PGRES_TUPLES_OK) {
            if (r)
                PQclear(r);
            ok = 0;
            break;
        }
        if (!PQntuples(r)) {
            PQclear(r);
            r = PQexec(db, "COMMIT");
            if (!r || PQresultStatus(r) != PGRES_COMMAND_OK)
                ok = 0;
            if (r)
                PQclear(r);
            break;
        }
        char token[65] = "";
        const char *cipher = PQgetvalue(r, 0, 3);
        int sent =
            (!*cipher || token_decrypt(c, cipher, token, sizeof(token))) &&
            send_mail(c, PQgetvalue(r, 0, 0), PQgetvalue(r, 0, 1), PQgetvalue(r, 0, 2), token);
        sodium_memzero(token, sizeof(token));
        const char *v[] = {PQgetvalue(r, 0, 0)};
        PGresult *updated = PQexecParams(
            db,
            sent ? "UPDATE app.mail_outbox SET "
                   "sent_at=now(),encrypted_token='',attempts=attempts+1 WHERE id=$1::bigint"
                 : "UPDATE app.mail_outbox SET attempts=attempts+1,available_at=now()+interval '5 "
                   "minutes' * (attempts+1) WHERE id=$1::bigint",
            1, NULL, v, NULL, NULL, 0);
        PQclear(r);
        if (!updated || PQresultStatus(updated) != PGRES_COMMAND_OK) {
            if (updated)
                PQclear(updated);
            ok = 0;
            break;
        }
        PQclear(updated);
        r = PQexec(db, "COMMIT");
        if (!r || PQresultStatus(r) != PGRES_COMMAND_OK) {
            if (r)
                PQclear(r);
            ok = 0;
            break;
        }
        PQclear(r);
        if (!sent) {
            fputs("Mail delivery failed; retry scheduled.\n", stderr);
            ok = 0;
            break;
        }
    }
    /* Contact bodies remain encrypted in PostgreSQL until both mail deliveries succeed. */
    for (int i = 0; i < 20 && ok; i++) {
        int processed = contact_outbox_once(db, c);
        if (processed < 0) ok = 0;
        if (processed <= 0) break;
    }
    PQfinish(db);
    return ok;
}
