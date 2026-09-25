#ifndef CODARIS_SERVICE_H
#define CODARIS_SERVICE_H
#include <libpq-fe.h>
#include <stddef.h>
typedef struct {
    int production;
    unsigned short port;
    const char *database, *origin, *smtp_url, *smtp_user, *smtp_password, *mail_from;
    unsigned char mail_key[32];
} Config;
int config_load(Config *config);
int api_run(const Config *config);
int mail_run(const Config *config);
int token_encrypt(const Config *config, const char *token, char *out, size_t capacity);
int token_decrypt(const Config *config, const char *cipher, char *out, size_t capacity);
#endif
