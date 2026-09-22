#ifndef CODARIS_DB_H
#define CODARIS_DB_H

#include <stddef.h>

typedef struct CodarisDb CodarisDb;

/*
 * Opens a PostgreSQL connection using libpq.
 *
 * connection_string may be an empty string. In that case libpq can use its
 * normal PG* environment variables and password-file mechanisms.
 */
CodarisDb *codaris_db_open(const char *connection_string);

/* Returns 1 when the connection is healthy, otherwise 0. */
int codaris_db_is_ready(const CodarisDb *db);

/*
 * Executes a minimal health query and writes the current database/user names.
 * Returns 1 on success, 0 on failure.
 */
int codaris_db_healthcheck(
    CodarisDb *db,
    char *database_name,
    size_t database_name_size,
    char *user_name,
    size_t user_name_size
);

/* Returns a libpq error string owned by the connection. Never free it. */
const char *codaris_db_error(const CodarisDb *db);

void codaris_db_close(CodarisDb *db);

#endif
