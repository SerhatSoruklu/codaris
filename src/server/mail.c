#include "codaris/service.h"
#include <curl/curl.h>
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
    PQfinish(db);
    return ok;
}
