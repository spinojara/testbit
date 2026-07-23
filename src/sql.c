#include "sql.h"

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "util.h"

pthread_mutex_t insert_lock;
pthread_mutex_t db_lock;
sqlite3 *db;

struct column {
	const char *name;
	const char *type;
	const char *extra;
};

struct column tests[] = {
	{ "id",          "INTEGER", "PRIMARY KEY AUTOINCREMENT" },
	{ "legacy",      "INTEGER", "DEFAULT FALSE" },
	{ "description", "TEXT" },
	{ "type",        "TEXT", "NOT NULL" },
	{ "status",      "TEXT", "DEFAULT 'queued'" },
	{ "tc",          "TEXT", "NOT NULL" },
	{ "alpha",       "REAL" },
	{ "beta",        "REAL" },
	{ "gamma",       "REAL" },
	{ "A",           "REAL" },
	{ "elo0",        "REAL" },
	{ "elo1",        "REAL" },
	{ "eloe",        "REAL" },
	{ "adjudicate",  "TEXT", "NOT NULL" },
	{ "queuetime",   "INTEGER" },
	{ "starttime",   "INTEGER" },
	{ "donetime",    "INTEGER" },
	{ "elo",         "REAL" },
	{ "pm",          "REAL" },
	{ "llr",         "REAL" },
	{ "t0",          "INTEGER", "DEFAULT 0" },
	{ "t1",          "INTEGER", "DEFAULT 0" },
	{ "t2",          "INTEGER", "DEFAULT 0" },
	{ "p0",          "INTEGER", "DEFAULT 0" },
	{ "p1",          "INTEGER", "DEFAULT 0" },
	{ "p2",          "INTEGER", "DEFAULT 0" },
	{ "p3",          "INTEGER", "DEFAULT 0" },
	{ "p4",          "INTEGER", "DEFAULT 0" },
	{ "commithash",  "TEXT", "NOT NULL" },
	{ "simd",        "TEXT", "NOT NULL" },
	{ "patch",       "TEXT", "NOT NULL" },
	{ "errorlog",    "TEXT" },
	{ "spsa",        "TEXT" },
	{ "priority",    "INTEGER", "DEFAULT 0" },
};

struct column games[] = {
	{ "id",        "INTEGER", "PRIMARY KEY AUTOINCREMENT" },
	{ "testid",    "INTEGER" },
	{ "w",         "INTEGER" },
	{ "d",         "INTEGER" },
	{ "l",         "INTEGER" },
	{ "starttime", "INTEGER", "NOT NULL" },
	{ "donetime",  "INTEGER" },
	{ "spsa",      "TEXT" },
	{ "weight",    "REAL" },
	{ "pgn",       "TEXT" },
};

void json_sql_add(cJSON *json, int type, const char *key, sqlite3_stmt *stmt, int n) {
	if (sqlite3_column_type(stmt, n) == SQLITE_NULL) {
		cJSON_AddNullToObject(json, key);
	}
	else {
		switch (type) {
		case TYPE_STRING:
			cJSON_AddStringOrNullToObject(json, key, (const char *)sqlite3_column_text(stmt, n));
			break;
		case TYPE_INTEGER:
			cJSON_AddNumberToObject(json, key, sqlite3_column_int(stmt, n));
			break;
		case TYPE_REAL:
			cJSON_AddNumberToObject(json, key, sqlite3_column_double(stmt, n));
			break;
		case TYPE_BOOL:
			cJSON_AddBoolToObject(json, key, sqlite3_column_int(stmt, n));
			break;
		default:
			fprintf(stderr, "error: bad type\n");
			exit(1);
		}
	}
}

void create_table(struct column *columns, size_t ncol, const char *table, const char *extra) {
	char *s = strdup("CREATE TABLE IF NOT EXISTS ");
	s = append_string(s, table);
	s = append_string(s, " (\n");

	for (size_t i = 0; i < ncol; i++) {
		struct column *column = &columns[i];
		s = append_string(s, "\t");
		s = append_string(s, column->name);
		s = append_string(s, " ");
		s = append_string(s, column->type);
		if (column->extra) {
			s = append_string(s, " ");
			s = append_string(s, column->extra);
		}
		if (i < ncol - 1 || extra)
			s = append_string(s, ",\n");
		else
			s = append_string(s, "\n");
		/* Just try adding the columns in case the table already exists. */
		char *t = strdup("ALTER TABLE ");
		t = append_string(t, table);
		t = append_string(t, " ADD COLUMN ");
		t = append_string(t, column->name);
		t = append_string(t, " ");
		t = append_string(t, column->type);
		if (column->extra) {
			t = append_string(t, " ");
			t = append_string(t, column->extra);
		}
		t = append_string(t, ";");
		sqlite3_exec(db, t, NULL, NULL, NULL);
		free(t);
	}

	if (extra) {
		s = append_string(s, "\t");
		s = append_string(s, extra);
		s = append_string(s, "\n");
	}
	s = append_string(s, ");");
	sqlite3_exec(db, s, NULL, NULL, NULL);

	free(s);
}

int db_open(const char *path) {
	pthread_mutex_init(&db_lock, NULL);
	pthread_mutex_init(&insert_lock, NULL);
	if (!path || sqlite3_open_v2(path, &db, SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK)
		return 1;

	create_table(tests, SIZE(tests), "tests", "UNIQUE (type, queuetime, patch, commithash)");
	create_table(games, SIZE(games), "games", NULL);

	sqlite3_exec(db,
		"DELETE FROM games WHERE donetime IS NULL;",
		NULL, NULL, NULL);

	return 0;
}

void db_close(void) {
	if (db)
		sqlite3_close(db);
	pthread_mutex_destroy(&db_lock);
	pthread_mutex_destroy(&insert_lock);
}
