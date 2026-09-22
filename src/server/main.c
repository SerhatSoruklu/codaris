#include "codaris/db.h"
#include "codaris/version.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char *connection_string = getenv("CODARIS_DATABASE_URL");

    /*
     * An empty connection string tells libpq to use PGHOST, PGPORT, PGDATABASE,
     * PGUSER, PGPASSWORD and the standard libpq password-file mechanisms.
     */
    if (connection_string == NULL) {
        connection_string = "";
    }

    printf("CODARIS backend skeleton %s\n", CODARIS_VERSION_STRING);

    CodarisDb *db = codaris_db_open(connection_string);
    if (db == NULL) {
        fprintf(stderr, "Failed to allocate/open the PostgreSQL connection.\n");
        return EXIT_FAILURE;
    }

    if (!codaris_db_is_ready(db)) {
        fprintf(stderr, "PostgreSQL connection failed: %s", codaris_db_error(db));
        codaris_db_close(db);
        return EXIT_FAILURE;
    }

    char database_name[128] = {0};
    char user_name[128] = {0};

    if (!codaris_db_healthcheck(
            db,
            database_name,
            sizeof(database_name),
            user_name,
            sizeof(user_name))) {
        fprintf(stderr, "PostgreSQL health check failed: %s", codaris_db_error(db));
        codaris_db_close(db);
        return EXIT_FAILURE;
    }

    printf("Database connection OK: database=%s user=%s\n", database_name, user_name);
    printf("HTTP/API layer is intentionally not implemented in this baseline.\n");

    codaris_db_close(db);
    return EXIT_SUCCESS;
}
