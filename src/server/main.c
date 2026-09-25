#include "codaris/service.h"
#include <curl/curl.h>
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv) {
    if (argc > 2 || (argc == 2 && strcmp(argv[1], "--mail-once"))) {
        fputs("Usage: codaris_server [--mail-once]\n", stderr);
        return EXIT_FAILURE;
    }
    if (sodium_init() < 0 || curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
        return EXIT_FAILURE;
    Config config;
    if (!config_load(&config)) {
        fputs("Invalid service environment. Check config examples; values are not logged.\n",
              stderr);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    int ok = argc == 2 && !strcmp(argv[1], "--mail-once") ? mail_run(&config) : api_run(&config);
    sodium_memzero(&config, sizeof(config));
    curl_global_cleanup();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
