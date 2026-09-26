#include "codaris/service.h"
#include <sodium.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *env(const char *name, const char *fallback) {
    const char *value = getenv(name);
    return value && *value ? value : fallback;
}
int config_load(Config *c) {
    memset(c, 0, sizeof(*c));
    const char *mode = env("CODARIS_ENV", "development");
    if (strcmp(mode, "development") && strcmp(mode, "production"))
        return 0;
    c->production = !strcmp(mode, "production");
    char *end = NULL;
    long port = strtol(env("CODARIS_PORT", "8080"), &end, 10);
    if (!end || *end || port < 1024 || port > 65535)
        return 0;
    c->port = (unsigned short)port;
    c->database = env("CODARIS_DATABASE_URL", "");
    c->origin =
        env("CODARIS_ORIGIN", c->production ? "https://codaris.org" : "http://127.0.0.1:8081");
    c->smtp_url = env("CODARIS_SMTP_URL", c->production ? "" : "smtp://127.0.0.1:1025");
    c->smtp_user = env("CODARIS_SMTP_USER", "");
    c->smtp_password = env("CODARIS_SMTP_PASSWORD", "");
    c->mail_from = env("CODARIS_MAIL_FROM", "admin@coupyn.com");
    c->credential_font = env("CODARIS_CREDENTIAL_FONT",
                             c->production ? "/opt/codaris/DejaVuSans.ttf" :
#ifdef CODARIS_SOURCE_FONT_PATH
                             CODARIS_SOURCE_FONT_PATH
#else
                             "web/assets/fonts/DejaVuSans.ttf"
#endif
    );
    if (strlen(c->origin) > 512 || strpbrk(c->origin, "<>\"'&\r\n"))
        return 0;
    CURLU *url = curl_url();
    char *host = NULL, *scheme = NULL, *path = NULL, *user = NULL, *query = NULL, *fragment = NULL;
    int ok = url && !curl_url_set(url, CURLUPART_URL, c->origin, 0) &&
             !curl_url_get(url, CURLUPART_HOST, &host, 0) &&
             !curl_url_get(url, CURLUPART_SCHEME, &scheme, 0) &&
             !curl_url_get(url, CURLUPART_PATH, &path, 0) && !strcmp(path, "/") &&
             curl_url_get(url, CURLUPART_USER, &user, 0) == CURLUE_NO_USER &&
             curl_url_get(url, CURLUPART_QUERY, &query, 0) == CURLUE_NO_QUERY &&
             curl_url_get(url, CURLUPART_FRAGMENT, &fragment, 0) == CURLUE_NO_FRAGMENT &&
             (!strcmp(scheme, "https") ||
              (!c->production && !strcmp(scheme, "http") &&
               (!strcmp(host, "127.0.0.1") || !strcmp(host, "localhost")))) &&
             c->origin[strlen(c->origin) - 1] != '/';
    curl_free(host);
    curl_free(scheme);
    curl_free(path);
    curl_free(user);
    curl_free(query);
    curl_free(fragment);
    curl_url_cleanup(url);
    if (!ok || !*c->smtp_url || strpbrk(c->mail_from, "\r\n<> ") || !strchr(c->mail_from, '@'))
        return 0;
    if (strncmp(c->smtp_url, "smtp://", 7) && strncmp(c->smtp_url, "smtps://", 8))
        return 0;
    if (!c->production && strncmp(c->smtp_url, "smtp://127.0.0.1:", 17) &&
        strncmp(c->smtp_url, "smtps://", 8))
        return 0;
    if (c->production && (!*c->smtp_user || !*c->smtp_password))
        return 0;
    const char *key = env("CODARIS_MAIL_KEY", "");
    if (strlen(key) != 64 || sodium_hex2bin(c->mail_key, 32, key, 64, NULL, NULL, NULL))
        return 0;
    return 1;
}
int token_encrypt(const Config *c, const char *token, char *out, size_t cap) {
    unsigned char blob[24 + 16 + 64];
    if (strlen(token) != 64 || cap < sizeof(blob) * 2 + 1)
        return 0;
    randombytes_buf(blob, 24);
    if (crypto_secretbox_easy(blob + 24, (const unsigned char *)token, 64, blob, c->mail_key))
        return 0;
    sodium_bin2hex(out, cap, blob, sizeof(blob));
    sodium_memzero(blob, sizeof(blob));
    return 1;
}
int token_decrypt(const Config *c, const char *cipher, char *out, size_t cap) {
    unsigned char blob[104];
    if (strlen(cipher) != 208 || cap < 65 ||
        sodium_hex2bin(blob, sizeof(blob), cipher, 208, NULL, NULL, NULL))
        return 0;
    int ok = !crypto_secretbox_open_easy((unsigned char *)out, blob + 24, 80, blob, c->mail_key);
    sodium_memzero(blob, sizeof(blob));
    out[64] = 0;
    return ok;
}
