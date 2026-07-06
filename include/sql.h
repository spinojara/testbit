#ifndef SQL_H
#define SQL_H

#include <sqlite3.h>
#include <pthread.h>
#include <cjson/cJSON.h>

enum {
	TYPE_STRING,
	TYPE_INTEGER,
	TYPE_REAL,
	TYPE_BOOL,
};

extern pthread_mutex_t db_lock;
extern pthread_mutex_t insert_lock;
extern sqlite3 *db;

int db_open(const char *path);

void db_close(void);

void json_sql_add(cJSON *json, int type, const char *key, sqlite3_stmt *stmt, int n);

#endif
