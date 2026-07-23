#include "get.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <cjson/cJSON.h>
#include <sqlite3.h>

#include "sql.h"
#include "git.h"
#include "elo.h"

void test_fetch_single(int fd, const struct http *http, int id) {
	char *endptr = NULL;
	errno = 0;
	long long delta = strtol(map_get(&http->query, "delta", "-1"), &endptr, 10);
	if (errno || *endptr) {
		bad_request(fd, "bad delta");
		return;
	}
	int fullnnue = !strcasecmp(map_get(&http->query, "fullnnue", "false"), "true");

	pthread_mutex_lock(&db_lock);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"SELECT\n"
			"tests.legacy,\n"
			"tests.description,\n"
			"tests.type,\n"
			"tests.status,\n"
			"tests.tc,\n"
			"tests.alpha,\n"
			"tests.beta,\n"
			"tests.elo0,\n"
			"tests.elo1,\n"
			"tests.eloe,\n"
			"tests.adjudicate,\n"
			"tests.queuetime,\n"
			"tests.starttime,\n"
			"tests.donetime,\n"
			"tests.elo,\n"
			"tests.pm,\n"
			"tests.llr,\n"
			"tests.t0,\n"
			"tests.t1,\n"
			"tests.t2,\n"
			"tests.p0,\n"
			"tests.p1,\n"
			"tests.p2,\n"
			"tests.p3,\n"
			"tests.p4,\n"
			"tests.commithash,\n"
			"tests.simd,\n"
			"tests.patch,\n"
			"tests.errorlog,\n"
			"(unixepoch() - min(games.donetime + games.starttime) / 2.0) / count(games.id)\n"
		"FROM tests\n"
		"LEFT OUTER JOIN games\n"
			"ON tests.id = games.testid\n"
			"AND games.donetime IS NOT NULL\n"
			"AND (? < 0 OR unixepoch() - ? <= games.donetime)\n"
		/* sqlite3 query planner is very stupid, let's trick it to get better performance */
		"WHERE NOT (tests.id != ?) AND NOT (tests.type NOT IN ('elo', 'sprt'))\n"
		"GROUP BY tests.id;",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, delta);
	sqlite3_bind_int(stmt, 2, delta);
	sqlite3_bind_int(stmt, 3, id);

	if (sqlite3_step(stmt) == SQLITE_DONE) {
		sqlite3_finalize(stmt);
		pthread_mutex_unlock(&db_lock);
		bad_request(fd, "bad id");
		return;
	}

	const char *patch = (const char *)sqlite3_column_text(stmt, 27);
	char *smallpatch = NULL;
	if (patch && !fullnnue) {
		smallpatch = remove_binary(patch);
	}

	cJSON *root = cJSON_CreateObject();
	cJSON *test = cJSON_CreateObject();

	json_sql_add(test, TYPE_BOOL, "legacy", stmt, 0);
	json_sql_add(test, TYPE_STRING, "description", stmt, 1);
	json_sql_add(test, TYPE_STRING, "type", stmt, 2);
	const char *status = (const char *)sqlite3_column_text(stmt, 3);
	cJSON_AddStringOrNullToObject(test, "status", status);
	json_sql_add(test, TYPE_STRING, "tc", stmt, 4);
	json_sql_add(test, TYPE_REAL, "alpha", stmt, 5);
	json_sql_add(test, TYPE_REAL, "beta", stmt, 6);
	json_sql_add(test, TYPE_REAL, "elo0", stmt, 7);
	json_sql_add(test, TYPE_REAL, "elo1", stmt, 8);
	json_sql_add(test, TYPE_REAL, "eloe", stmt, 9);
	json_sql_add(test, TYPE_STRING, "adjudicate", stmt, 10);
	json_sql_add(test, TYPE_INTEGER, "queuetime", stmt, 11);
	json_sql_add(test, TYPE_INTEGER, "starttime", stmt, 12);
	json_sql_add(test, TYPE_INTEGER, "donetime", stmt, 13);
	json_sql_add(test, TYPE_REAL, "elo", stmt, 14);
	json_sql_add(test, TYPE_REAL, "pm", stmt, 15);
	json_sql_add(test, TYPE_REAL, "llr", stmt, 16);
	json_sql_add(test, TYPE_REAL, "t0", stmt, 17);
	json_sql_add(test, TYPE_REAL, "t1", stmt, 18);
	json_sql_add(test, TYPE_REAL, "t2", stmt, 19);
	json_sql_add(test, TYPE_REAL, "p0", stmt, 20);
	json_sql_add(test, TYPE_REAL, "p1", stmt, 21);
	json_sql_add(test, TYPE_REAL, "p2", stmt, 22);
	json_sql_add(test, TYPE_REAL, "p3", stmt, 23);
	json_sql_add(test, TYPE_REAL, "p4", stmt, 24);
	json_sql_add(test, TYPE_STRING, "commit", stmt, 25);
	json_sql_add(test, TYPE_STRING, "simd", stmt, 26);
	cJSON_AddStringOrNullToObject(test, "patch", smallpatch ? smallpatch : patch);
	json_sql_add(test, TYPE_STRING, "errorlog", stmt, 28);

	/* status will never bu NULL */
	if (!strcmp(status, "running"))
		json_sql_add(test, TYPE_REAL, "gametimeavg", stmt, 29);
	else
		cJSON_AddNullToObject(test, "gametimeavg");


	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	cJSON_AddStringToObject(root, "message", "ok");
	cJSON_AddItemToObject(root, "test", test);

	free(smallpatch);
	send_json_response(fd, "200 OK", root);
	cJSON_Delete(root);
}

void test_fetch_all(int fd, const struct http *http, int id) {
	(void)id;
	char *endptr = NULL;
	errno = 0;
	long long delta = strtol(map_get(&http->query, "delta", "-1"), &endptr, 10);
	if (errno || *endptr) {
		bad_request(fd, "bad delta");
		return;
	}

	pthread_mutex_lock(&db_lock);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"SELECT\n"
			"tests.id,\n"
			"tests.legacy,\n"
			"tests.description,\n"
			"tests.type,\n"
			"tests.status,\n"
			"tests.tc,\n"
			"tests.alpha,\n"
			"tests.beta,\n"
			"tests.elo0,\n"
			"tests.elo1,\n"
			"tests.eloe,\n"
			"tests.adjudicate,\n"
			"tests.queuetime,\n"
			"tests.starttime,\n"
			"tests.donetime,\n"
			"tests.elo,\n"
			"tests.pm,\n"
			"tests.llr,\n"
			"tests.t0,\n"
			"tests.t1,\n"
			"tests.t2,\n"
			"tests.p0,\n"
			"tests.p1,\n"
			"tests.p2,\n"
			"tests.p3,\n"
			"tests.p4,\n"
			"tests.commithash,\n"
			"tests.simd,\n"
			"(unixepoch() - min(games.donetime + games.starttime) / 2.0) / count(games.id)\n"
		"FROM tests\n"
		"LEFT OUTER JOIN games\n"
			"ON tests.id = games.testid\n"
			"AND games.donetime IS NOT NULL\n"
			"AND (? < 0 OR unixepoch() - ? <= games.donetime)\n"
		/* sqlite3 query planner is very stupid, let's trick it to get better performance */
		"WHERE NOT (tests.type NOT IN ('elo', 'sprt'))\n"
		"GROUP BY tests.id;",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, delta);
	sqlite3_bind_int(stmt, 2, delta);

	cJSON *root = cJSON_CreateObject();
	cJSON *tests = cJSON_CreateArray();
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		cJSON *test = cJSON_CreateObject();

		json_sql_add(test, TYPE_INTEGER, "id", stmt, 0);
		json_sql_add(test, TYPE_BOOL, "legacy", stmt, 1);
		json_sql_add(test, TYPE_STRING, "description", stmt, 2);
		json_sql_add(test, TYPE_STRING, "type", stmt, 3);
		const char *status = (const char *)sqlite3_column_text(stmt, 4);
		cJSON_AddStringOrNullToObject(test, "status", status);
		json_sql_add(test, TYPE_STRING, "tc", stmt, 5);
		json_sql_add(test, TYPE_REAL, "alpha", stmt, 6);
		json_sql_add(test, TYPE_REAL, "beta", stmt, 7);
		json_sql_add(test, TYPE_REAL, "elo0", stmt, 8);
		json_sql_add(test, TYPE_REAL, "elo1", stmt, 9);
		json_sql_add(test, TYPE_REAL, "eloe", stmt, 10);
		json_sql_add(test, TYPE_STRING, "adjudicate", stmt, 11);
		json_sql_add(test, TYPE_INTEGER, "queuetime", stmt, 12);
		json_sql_add(test, TYPE_INTEGER, "starttime", stmt, 13);
		json_sql_add(test, TYPE_INTEGER, "donetime", stmt, 14);
		json_sql_add(test, TYPE_REAL, "elo", stmt, 15);
		json_sql_add(test, TYPE_REAL, "pm", stmt, 16);
		json_sql_add(test, TYPE_REAL, "llr", stmt, 17);
		json_sql_add(test, TYPE_REAL, "t0", stmt, 18);
		json_sql_add(test, TYPE_REAL, "t1", stmt, 19);
		json_sql_add(test, TYPE_REAL, "t2", stmt, 20);
		json_sql_add(test, TYPE_REAL, "p0", stmt, 21);
		json_sql_add(test, TYPE_REAL, "p1", stmt, 22);
		json_sql_add(test, TYPE_REAL, "p2", stmt, 23);
		json_sql_add(test, TYPE_REAL, "p3", stmt, 24);
		json_sql_add(test, TYPE_REAL, "p4", stmt, 25);
		json_sql_add(test, TYPE_STRING, "commit", stmt, 26);
		json_sql_add(test, TYPE_STRING, "simd", stmt, 27);

		if (!strcmp(status, "running"))
			json_sql_add(test, TYPE_REAL, "gametimeavg", stmt, 28);
		else
			cJSON_AddNullToObject(test, "gametimeavg");

		cJSON_AddItemToArray(tests, test);
	}
	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	cJSON_AddStringToObject(root, "message", "ok");
	cJSON_AddItemToObject(root, "tests", tests);

	send_json_response(fd, "200 OK", root);
	cJSON_Delete(root);
}

void clop_fetch_single(int fd, const struct http *http, int id) {
	(void)http;
	int fullnnue = !strcasecmp(map_get(&http->query, "fullnnue", "false"), "true");

	clock_t c = clock();
	pthread_mutex_lock(&db_lock);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"SELECT\n"
			"tests.legacy,\n"
			"tests.description,\n"
			"tests.status,\n"
			"tests.tc,\n"
			"tests.adjudicate,\n"
			"tests.queuetime,\n"
			"tests.starttime,\n"
			"tests.donetime,\n"
			"tests.commithash,\n"
			"tests.simd,\n"
			"tests.patch,\n"
			"tests.spsa,\n"
			"(tests.t0 + tests.t1 + tests.t2) / 2,\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 0 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 1 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 2 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 3 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 4 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 0 THEN games.weight ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 1 THEN games.weight ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 2 THEN games.weight ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 3 THEN games.weight ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 4 THEN games.weight ELSE 0 END),\n"
			"SUM(CASE WHEN games.weight = 1.0 AND 2 * games.w + games.d = 0 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN games.weight = 1.0 AND 2 * games.w + games.d = 1 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN games.weight = 1.0 AND 2 * games.w + games.d = 2 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN games.weight = 1.0 AND 2 * games.w + games.d = 3 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN games.weight = 1.0 AND 2 * games.w + games.d = 4 THEN 1 ELSE 0 END),\n"
			"json_group_array(\n"
				"json_set(json(games.spsa), '$._weight', games.weight, '$._score', 0.5 * games.w + 0.25 * games.d)\n"
				"ORDER BY games.starttime ASC\n"
			") FILTER (WHERE games.spsa IS NOT NULL),\n"
			"tests.errorlog\n"
		"FROM tests\n"
		"LEFT OUTER JOIN games\n"
			"ON tests.id = games.testid\n"
			"AND games.donetime IS NOT NULL\n"
			"AND games.w + games.d + games.l = 2\n"
			"AND games.spsa IS NOT NULL\n"
		/* sqlite3 query planner is very stupid, let's trick it to get better performance */
		"WHERE NOT (tests.id != ?) AND NOT (tests.type != 'clop')\n"
		"GROUP BY tests.id;",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, id);

	if (sqlite3_step(stmt) == SQLITE_DONE) {
		sqlite3_finalize(stmt);
		pthread_mutex_unlock(&db_lock);
		bad_request(fd, "bad id");
		return;
	}

	const char *patch = (const char *)sqlite3_column_text(stmt, 10);
	char *smallpatch = NULL;
	if (patch && !fullnnue) {
		smallpatch = remove_binary(patch);
	}

	cJSON *root = cJSON_CreateObject();
	cJSON *test = cJSON_CreateObject();

	double elo[3], pm[3];
	for (int i = 0; i < 3; i++) {
		double penta[5] = {
			sqlite3_column_double(stmt, 13 + 5 * i),
			sqlite3_column_double(stmt, 14 + 5 * i),
			sqlite3_column_double(stmt, 15 + 5 * i),
			sqlite3_column_double(stmt, 16 + 5 * i),
			sqlite3_column_double(stmt, 17 + 5 * i),
		};
		elo[i] = calculate_elo(penta, &pm[i]);
	}

	json_sql_add(test, TYPE_BOOL, "legacy", stmt, 0);
	json_sql_add(test, TYPE_STRING, "description", stmt, 1);
	json_sql_add(test, TYPE_STRING, "status", stmt, 2);
	json_sql_add(test, TYPE_STRING, "tc", stmt, 3);
	json_sql_add(test, TYPE_STRING, "adjudicate", stmt, 4);
	json_sql_add(test, TYPE_INTEGER, "queuetime", stmt, 5);
	json_sql_add(test, TYPE_INTEGER, "starttime", stmt, 6);
	json_sql_add(test, TYPE_INTEGER, "donetime", stmt, 7);
	json_sql_add(test, TYPE_STRING, "commit", stmt, 8);
	json_sql_add(test, TYPE_STRING, "simd", stmt, 9);
	cJSON_AddStringToObject(test, "patch", smallpatch ? smallpatch : patch);
	json_sql_add(test, TYPE_INTEGER, "N", stmt, 12);
	cJSON *spsa = cJSON_Parse((const char *)sqlite3_column_text(stmt, 11));
	cJSON_AddItemToObject(test, "spsa", spsa);

	cJSON_AddNumberToObject(test, "eloall", elo[0]);
	if (pm[0] >= 0.0)
		cJSON_AddNumberToObject(test, "pmall", pm[0]);
	else
		cJSON_AddNullToObject(test, "pmall");
	cJSON_AddNumberToObject(test, "eloweighted", elo[1]);
	if (pm[1] >= 0.0)
		cJSON_AddNumberToObject(test, "pmweighted", pm[1]);
	else
		cJSON_AddNullToObject(test, "pmweighted");
	cJSON_AddNumberToObject(test, "elocentral", elo[2]);
	if (pm[2] >= 0.0)
		cJSON_AddNumberToObject(test, "pmcentral", pm[2]);
	else
		cJSON_AddNullToObject(test, "pmcentral");

	cJSON *spsahistory = cJSON_Parse((const char *)sqlite3_column_text(stmt, 28));
	cJSON_AddItemToObject(test, "spsahistory", spsahistory);
	json_sql_add(test, TYPE_STRING, "errorlog", stmt, 29);

	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	cJSON_AddStringToObject(root, "message", "ok");
	cJSON_AddItemToObject(root, "test", test);

	send_json_response(fd, "200 OK", root);
	printf("That took %lf\n", (double)(clock() - c) / CLOCKS_PER_SEC);
	cJSON_Delete(root);
	free(smallpatch);
}

void clop_fetch_all(int fd, const struct http *http, int id) {
	(void)http;
	(void)id;
	pthread_mutex_lock(&db_lock);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"SELECT\n"
			"tests.id,\n"
			"tests.legacy,\n"
			"tests.description,\n"
			"tests.status,\n"
			"tests.tc,\n"
			"tests.adjudicate,\n"
			"tests.queuetime,\n"
			"tests.starttime,\n"
			"tests.donetime,\n"
			"tests.commithash,\n"
			"tests.simd,\n"
			"(tests.t0 + tests.t1 + tests.t2) / 2,\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 0 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 1 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 2 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 3 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 4 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 0 THEN games.weight ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 1 THEN games.weight ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 2 THEN games.weight ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 3 THEN games.weight ELSE 0 END),\n"
			"SUM(CASE WHEN 2 * games.w + games.d = 4 THEN games.weight ELSE 0 END),\n"
			"SUM(CASE WHEN games.weight = 1.0 AND 2 * games.w + games.d = 0 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN games.weight = 1.0 AND 2 * games.w + games.d = 1 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN games.weight = 1.0 AND 2 * games.w + games.d = 2 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN games.weight = 1.0 AND 2 * games.w + games.d = 3 THEN 1 ELSE 0 END),\n"
			"SUM(CASE WHEN games.weight = 1.0 AND 2 * games.w + games.d = 4 THEN 1 ELSE 0 END)\n"
		"FROM tests\n"
		"LEFT OUTER JOIN games\n"
			"ON tests.id = games.testid\n"
			"AND games.donetime IS NOT NULL\n"
			"AND games.w + games.d + games.l = 2\n"
			"AND games.spsa IS NOT NULL\n"
		/* sqlite3 query planner is very stupid, let's trick it to get better performance */
		"WHERE NOT (tests.type != 'clop')\n"
		"GROUP BY tests.id;",
		-1, &stmt, NULL);

	cJSON *root = cJSON_CreateObject();
	cJSON *tests = cJSON_CreateArray();
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		cJSON *test = cJSON_CreateObject();

		double elo[3], pm[3];
		for (int i = 0; i < 3; i++) {
			double penta[5] = {
				sqlite3_column_double(stmt, 12 + 5 * i),
				sqlite3_column_double(stmt, 13 + 5 * i),
				sqlite3_column_double(stmt, 14 + 5 * i),
				sqlite3_column_double(stmt, 15 + 5 * i),
				sqlite3_column_double(stmt, 16 + 5 * i),
			};
			elo[i] = calculate_elo(penta, &pm[i]);
		}

		json_sql_add(test, TYPE_INTEGER, "id", stmt, 0);
		json_sql_add(test, TYPE_BOOL, "legacy", stmt, 1);
		json_sql_add(test, TYPE_STRING, "description", stmt, 2);
		json_sql_add(test, TYPE_STRING, "status", stmt, 3);
		json_sql_add(test, TYPE_STRING, "tc", stmt, 4);
		json_sql_add(test, TYPE_STRING, "adjudicate", stmt, 5);
		json_sql_add(test, TYPE_INTEGER, "queuetime", stmt, 6);
		json_sql_add(test, TYPE_INTEGER, "starttime", stmt, 7);
		json_sql_add(test, TYPE_INTEGER, "donetime", stmt, 8);
		json_sql_add(test, TYPE_STRING, "commit", stmt, 9);
		json_sql_add(test, TYPE_STRING, "simd", stmt, 10);
		json_sql_add(test, TYPE_INTEGER, "N", stmt, 11);

		cJSON_AddNumberToObject(test, "eloall", elo[0]);
		if (pm[0] >= 0.0)
			cJSON_AddNumberToObject(test, "pmall", pm[0]);
		else
			cJSON_AddNullToObject(test, "pmall");
		cJSON_AddNumberToObject(test, "eloweighted", elo[1]);
		if (pm[1] >= 0.0)
			cJSON_AddNumberToObject(test, "pmweighted", pm[1]);
		else
			cJSON_AddNullToObject(test, "pmweighted");
		cJSON_AddNumberToObject(test, "elocentral", elo[2]);
		if (pm[2] >= 0.0)
			cJSON_AddNumberToObject(test, "pmcentral", pm[2]);
		else
			cJSON_AddNullToObject(test, "pmcentral");

		cJSON_AddItemToArray(tests, test);
	}
	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	cJSON_AddStringToObject(root, "message", "ok");
	cJSON_AddItemToObject(root, "tests", tests);

	send_json_response(fd, "200 OK", root);
	cJSON_Delete(root);
}

void spsa_fetch_single(int fd, const struct http *http, int id) {
	(void)http;
	int fullnnue = !strcasecmp(map_get(&http->query, "fullnnue", "false"), "true");

	clock_t c = clock();
	pthread_mutex_lock(&db_lock);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"SELECT\n"
			"tests.legacy,\n"
			"tests.description,\n"
			"tests.status,\n"
			"tests.tc,\n"
			"tests.alpha,\n"
			"tests.gamma,\n"
			"tests.A,\n"
			"tests.adjudicate,\n"
			"tests.queuetime,\n"
			"tests.starttime,\n"
			"tests.donetime,\n"
			"tests.commithash,\n"
			"tests.simd,\n"
			"tests.patch,\n"
			"tests.errorlog,\n"
			"tests.spsa,\n"
			"(tests.t0 + tests.t1 + tests.t2) / 2,\n"
			"json_group_array(\n"
				"json(games.spsa)\n"
				"ORDER BY games.starttime ASC\n"
			") FILTER (WHERE games.spsa IS NOT NULL)\n"
		"FROM tests\n"
		"LEFT OUTER JOIN games\n"
			"ON tests.id = games.testid\n"
			"AND games.donetime IS NOT NULL\n"
			"AND games.w + games.d + games.l = 2\n"
			"AND games.spsa IS NOT NULL\n"
		/* sqlite3 query planner is very stupid, let's trick it to get better performance */
		"WHERE NOT (tests.id != ?) AND NOT (tests.type != 'spsa')\n"
		"GROUP BY tests.id;",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, id);

	if (sqlite3_step(stmt) == SQLITE_DONE) {
		sqlite3_finalize(stmt);
		pthread_mutex_unlock(&db_lock);
		bad_request(fd, "bad id");
		return;
	}

	const char *patch = (const char *)sqlite3_column_text(stmt, 13);
	char *smallpatch = NULL;
	if (patch && !fullnnue) {
		smallpatch = remove_binary(patch);
	}

	cJSON *root = cJSON_CreateObject();
	cJSON *test = cJSON_CreateObject();

	json_sql_add(test, TYPE_BOOL, "legacy", stmt, 0);
	json_sql_add(test, TYPE_STRING, "description", stmt, 1);
	json_sql_add(test, TYPE_STRING, "status", stmt, 2);
	json_sql_add(test, TYPE_STRING, "tc", stmt, 3);
	json_sql_add(test, TYPE_REAL, "alpha", stmt, 4);
	json_sql_add(test, TYPE_REAL, "gamma", stmt, 5);
	json_sql_add(test, TYPE_REAL, "A", stmt, 6);
	json_sql_add(test, TYPE_STRING, "adjudicate", stmt, 7);
	json_sql_add(test, TYPE_INTEGER, "queuetime", stmt, 8);
	json_sql_add(test, TYPE_INTEGER, "starttime", stmt, 9);
	json_sql_add(test, TYPE_INTEGER, "donetime", stmt, 10);
	json_sql_add(test, TYPE_STRING, "commit", stmt, 11);
	json_sql_add(test, TYPE_STRING, "simd", stmt, 12);
	cJSON_AddStringOrNullToObject(test, "patch", smallpatch ? smallpatch : patch);
	json_sql_add(test, TYPE_STRING, "errorlog", stmt, 14);
	json_sql_add(test, TYPE_INTEGER, "N", stmt, 16);
	cJSON *spsa = cJSON_Parse((const char *)sqlite3_column_text(stmt, 15));
	cJSON *spsahistory = cJSON_Parse((const char *)sqlite3_column_text(stmt, 17));
	cJSON_AddItemToObject(test, "spsa", spsa);
	cJSON_AddItemToObject(test, "spsahistory", spsahistory);

	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	cJSON_AddStringToObject(root, "message", "ok");
	cJSON_AddItemToObject(root, "test", test);

	send_json_response(fd, "200 OK", root);
	printf("That took %lf\n", (double)(clock() - c) / CLOCKS_PER_SEC);
	cJSON_Delete(root);
	free(smallpatch);
}

void spsa_fetch_all(int fd, const struct http *http, int id) {
	(void)http;
	(void)id;
	pthread_mutex_lock(&db_lock);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"SELECT\n"
			"tests.id,\n"
			"tests.legacy,\n"
			"tests.description,\n"
			"tests.status,\n"
			"tests.tc,\n"
			"tests.alpha,\n"
			"tests.gamma,\n"
			"tests.A,\n"
			"tests.adjudicate,\n"
			"tests.queuetime,\n"
			"tests.starttime,\n"
			"tests.donetime,\n"
			"tests.commithash,\n"
			"tests.simd,\n"
			"(tests.t0 + tests.t1 + tests.t2) / 2\n"
		"FROM tests\n"
		"WHERE tests.type = 'spsa'\n",
		-1, &stmt, NULL);

	cJSON *root = cJSON_CreateObject();
	cJSON *tests = cJSON_CreateArray();
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		cJSON *test = cJSON_CreateObject();

		json_sql_add(test, TYPE_INTEGER, "id", stmt, 0);
		json_sql_add(test, TYPE_BOOL, "legacy", stmt, 1);
		json_sql_add(test, TYPE_STRING, "description", stmt, 2);
		json_sql_add(test, TYPE_STRING, "status", stmt, 3);
		json_sql_add(test, TYPE_STRING, "tc", stmt, 4);
		json_sql_add(test, TYPE_REAL, "alpha", stmt, 5);
		json_sql_add(test, TYPE_REAL, "gamma", stmt, 6);
		json_sql_add(test, TYPE_REAL, "A", stmt, 7);
		json_sql_add(test, TYPE_STRING, "adjudicate", stmt, 8);
		json_sql_add(test, TYPE_INTEGER, "queuetime", stmt, 9);
		json_sql_add(test, TYPE_INTEGER, "starttime", stmt, 10);
		json_sql_add(test, TYPE_INTEGER, "donetime", stmt, 11);
		json_sql_add(test, TYPE_STRING, "commit", stmt, 12);
		json_sql_add(test, TYPE_STRING, "simd", stmt, 13);
		json_sql_add(test, TYPE_INTEGER, "N", stmt, 14);

		cJSON_AddItemToArray(tests, test);
	}
	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	cJSON_AddStringToObject(root, "message", "ok");
	cJSON_AddItemToObject(root, "tests", tests);

	send_json_response(fd, "200 OK", root);
	cJSON_Delete(root);
}

void build_info(int fd, const struct http *http, int id) {
	(void)http;
	pthread_mutex_lock(&db_lock);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"SELECT\n"
			"patch,\n"
			"commithash,\n"
			"simd\n"
		"FROM tests\n"
		"WHERE id = ?",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, id);

	if (sqlite3_step(stmt) == SQLITE_DONE) {
		sqlite3_finalize(stmt);
		pthread_mutex_unlock(&db_lock);
		bad_request(fd, "bad id");
		return;
	}

	cJSON *root = cJSON_CreateObject();
	json_sql_add(root, TYPE_STRING, "patch", stmt, 0);
	json_sql_add(root, TYPE_STRING, "commit", stmt, 1);
	json_sql_add(root, TYPE_STRING, "simd", stmt, 2);
	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	send_json_response(fd, "200 OK", root);
	cJSON_Delete(root);
}
