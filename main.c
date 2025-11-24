//compile command:
// gcc main.c sqlite3.c -o main.exe -I. -Wall

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"

typedef struct {
    sqlite3* conn;
} db;

// Open database and create table if it doesn't exist
int db_init(db* d, const char* filename) {
    if (sqlite3_open(filename, &d->conn) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(d->conn));
        return 0;
    }

    const char* sql_create =
        "CREATE TABLE IF NOT EXISTS kv ("
        "key TEXT PRIMARY KEY, "
        "value TEXT);";

    char* errmsg = NULL;
    if (sqlite3_exec(d->conn, sql_create, 0, 0, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        return 0;
    }

    return 1;
}

// Close database
void db_close(db* d) {
    sqlite3_close(d->conn);
}

// Set a key-value pair (insert or update)
int db_set(db* d, const char* key, const char* value) {
    const char* sql_insert =
        "INSERT INTO kv (key, value) VALUES (?, ?) "
        "ON CONFLICT(key) DO UPDATE SET value=excluded.value;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(d->conn, sql_insert, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(d->conn));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(d->conn));
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

// Get the value of a key
char* db_get(db* d, const char* key) {
    const char* sql_select = "SELECT value FROM kv WHERE key=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(d->conn, sql_select, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(d->conn));
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);

    char* result = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* val = sqlite3_column_text(stmt, 0);
        result = strdup((const char*)val); // Copy to dynamically allocated memory
    }

    sqlite3_finalize(stmt);
    return result; // Caller must free
}

// Print all key-value pairs
void db_print(db* d) {
    const char* sql_select = "SELECT key, value FROM kv;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(d->conn, sql_select, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(d->conn));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* key = sqlite3_column_text(stmt, 0);
        const unsigned char* value = sqlite3_column_text(stmt, 1);
        printf("%s : %s\n", key, value);
    }

    sqlite3_finalize(stmt);
}

int main() {
    db data;

    if (!db_init(&data, "mydb.sqlite")) {
        return 1;
    }

    db_set(&data, "temperature", "23.4");
    db_set(&data, "status", "OK");
    db_set(&data, "temperature", "24.1"); // update existing

    char* temp = db_get(&data, "temperature");
    if (temp) {
        printf("Temperature: %s\n", temp);
        free(temp);
    } else {
        printf("Temperature key not found\n");
    }

    printf("All key-value pairs:\n");
    db_print(&data);

    db_close(&data);
    return 0;
}
