#include "codaris/db.h"

#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>

struct CodarisDb {
    PGconn *connection;
};

CodarisDb *codaris_db_open(const char *connection_string) {
    CodarisDb *db = calloc(1, sizeof(*db));
    if (db == NULL) {
        return NULL;
    }

    db->connection = PQconnectdb(connection_string != NULL ? connection_string : "");
    if (db->connection == NULL) {
        free(db);
        return NULL;
    }

    return db;
}

int codaris_db_is_ready(const CodarisDb *db) {
    return db != NULL && db->connection != NULL && PQstatus(db->connection) == CONNECTION_OK;
}

int codaris_db_healthcheck(
    CodarisDb *db,
    char *database_name,
    size_t database_name_size,
    char *user_name,
    size_t user_name_size
) {
    if (!codaris_db_is_ready(db) || database_name == NULL || database_name_size == 0 ||
        user_name == NULL || user_name_size == 0) {
        return 0;
    }

    PGresult *result = PQexec(db->connection, "SELECT current_database(), current_user;");
    if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK || PQntuples(result) != 1 ||
        PQnfields(result) != 2) {
        if (result != NULL) {
            PQclear(result);
        }
        return 0;
    }

    snprintf(database_name, database_name_size, "%s", PQgetvalue(result, 0, 0));
    snprintf(user_name, user_name_size, "%s", PQgetvalue(result, 0, 1));

    PQclear(result);
    return 1;
}

const char *codaris_db_error(const CodarisDb *db) {
    if (db == NULL || db->connection == NULL) {
        return "PostgreSQL connection object is not available.";
    }

    return PQerrorMessage(db->connection);
}

void codaris_db_close(CodarisDb *db) {
    if (db == NULL) {
        return;
    }

    if (db->connection != NULL) {
        PQfinish(db->connection);
    }

    free(db);
}
