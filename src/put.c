#include "put.h"

#include <stdio.h>
#include <cjson/cJSON.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "sql.h"
#include "elo.h"
#include "clop.h"

void test_data(int fd, const struct http *http, int id) {
	const char *keys[] = {
		"losses",
		"draws",
		"wins",
	};

	int stats[3] = { 0 };

	for (int i = 0; i < 3; i++) {
		const char *key = keys[i];
		cJSON *item = cJSON_GetObjectItemCaseSensitive(http->content, key);
		if (!item || !cJSON_IsNumber(item) || (stats[i] = item->valueint) < 0) {
			bad_request(fd, "bad stats");
			return;
		}
	}

	int losses = stats[0];
	int draws = stats[1];
	int wins = stats[2];

	if (losses + draws + wins != 2) {
		bad_request(fd, "bad stats");
		return;
	}

	cJSON *pgnitem = cJSON_GetObjectItemCaseSensitive(http->content, "pgn");
	if (!pgnitem || !cJSON_IsString(pgnitem)) {
		bad_request(fd, "bad pgn");
		return;
	}

	cJSON *task_iditem = cJSON_GetObjectItemCaseSensitive(http->content, "taskid");
	if (!task_iditem || !cJSON_IsNumber(task_iditem)) {
		bad_request(fd, "bad taskid");
		return;
	}
	int task_id = task_iditem->valueint;

	pthread_mutex_lock(&insert_lock);
	pthread_mutex_lock(&db_lock);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"UPDATE games\n"
		"SET w = ?,\n"
			"d = ?,\n"
			"l = ?,\n"
			"donetime = unixepoch(),\n"
			"pgn = ?\n"
		"WHERE id = ? AND testid = ? AND donetime IS NULL\n"
		"RETURNING spsa;",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, wins);
	sqlite3_bind_int(stmt, 2, draws);
	sqlite3_bind_int(stmt, 3, losses);
	sqlite3_bind_text(stmt, 4, pgnitem->valuestring, -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 5, task_id);
	sqlite3_bind_int(stmt, 6, id);

	if (sqlite3_step(stmt) == SQLITE_DONE) {
		sqlite3_finalize(stmt);
		pthread_mutex_unlock(&db_lock);
		pthread_mutex_unlock(&insert_lock);
		bad_request(fd, "bad id");
		return;
	}

	cJSON *spsaargs = cJSON_Parse((const char *)sqlite3_column_text(stmt, 0));

	sqlite3_finalize(stmt);

	sqlite3_prepare_v2(db,
		"SELECT type,\n"
			"alpha,\n"
			"beta,\n"
			"t0,\n"
			"t1,\n"
			"t2,\n"
			"p0,\n"
			"p1,\n"
			"p2,\n"
			"p3,\n"
			"p4,\n"
			"eloe,\n"
			"elo0,\n"
			"elo1,\n"
			"spsa\n"
		"FROM tests\n"
		"WHERE id = ? AND\n"
			"status = 'running';",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, id);

	if (sqlite3_step(stmt) == SQLITE_DONE) {
		sqlite3_finalize(stmt);
		pthread_mutex_unlock(&db_lock);
		pthread_mutex_unlock(&insert_lock);
		cJSON_Delete(spsaargs);
		/* It could be that the test was already set to done. */
		send_response(fd, "200 Ok", "{\"message\": \"ok\"}");
		return;
	}

	char *type = strdup((const char *)sqlite3_column_text(stmt, 0));
	double alpha = sqlite3_column_double(stmt, 1);
	double beta = sqlite3_column_double(stmt, 2);
	int t0 = sqlite3_column_int(stmt, 3);
	int t1 = sqlite3_column_int(stmt, 4);
	int t2 = sqlite3_column_int(stmt, 5);
	int p0 = sqlite3_column_int(stmt, 6);
	int p1 = sqlite3_column_int(stmt, 7);
	int p2 = sqlite3_column_int(stmt, 8);
	int p3 = sqlite3_column_int(stmt, 9);
	int p4 = sqlite3_column_int(stmt, 10);
	double eloe = sqlite3_column_double(stmt, 11);
	double elo0 = sqlite3_column_double(stmt, 12);
	double elo1 = sqlite3_column_double(stmt, 13);

	cJSON *spsa = cJSON_Parse((const char *)sqlite3_column_text(stmt, 14));

	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	int p[5] = { p0, p1, p2, p3, p4 };
	t0 += losses;
	t1 += draws;
	t2 += wins;
	p[draws + 2 * wins] += 1;

	double elo, pm;
	double n[5] = { 0 };
	for (int i = 0; i < 5; i++)
		n[i] = p[i];
	elo = calculate_elo(n, &pm);
	if (!strcmp(type, "sprt") || !strcmp(type, "elo")) {
		const char *status = "running";
		double llr = 0.0;
		if (!strcmp(type, "sprt")) {
			llr = loglikelihoodratio(p, elo0, elo1);
			double A = log(beta / (1.0 - alpha));
			double B = log((1.0 - beta) / alpha);
			if (llr < A)
				status = "H0 accepted";
			else if (llr > B)
				status = "H1 accepted";
		}
		else if (!strcmp(type, "elo")) {
			if (pm > 0.0 && pm < eloe)
				status = "done";
		}

		pthread_mutex_lock(&db_lock);
		sqlite3_prepare_v2(db,
			"UPDATE tests\n"
			"SET t0 = ?,\n"
				"t1 = ?,\n"
				"t2 = ?,\n"
				"p0 = ?,\n"
				"p1 = ?,\n"
				"p2 = ?,\n"
				"p3 = ?,\n"
				"p4 = ?,\n"
				"status = ?,\n"
				"llr = ?,\n"
				"elo = ?,\n"
				"pm = ?,\n"
				"donetime = CASE\n"
					"WHEN ? IN ('H0 accepted', 'H1 accepted', 'done')\n"
						"THEN unixepoch()\n"
					"ELSE NULL\n"
				"END\n"
			"WHERE id = ? AND\n"
				"status = 'running';",
			-1, &stmt, NULL);
		sqlite3_bind_int(stmt, 1, t0);
		sqlite3_bind_int(stmt, 2, t1);
		sqlite3_bind_int(stmt, 3, t2);
		sqlite3_bind_int(stmt, 4, p[0]);
		sqlite3_bind_int(stmt, 5, p[1]);
		sqlite3_bind_int(stmt, 6, p[2]);
		sqlite3_bind_int(stmt, 7, p[3]);
		sqlite3_bind_int(stmt, 8, p[4]);
		sqlite3_bind_text(stmt, 9, status, -1, NULL);
		if (!strcmp(type, "sprt"))
			sqlite3_bind_double(stmt, 10, llr);
		else
			sqlite3_bind_null(stmt, 10);
		sqlite3_bind_double(stmt, 11, elo);
		if (pm > 0.0)
			sqlite3_bind_double(stmt, 12, pm);
		else
			sqlite3_bind_null(stmt, 12);
		sqlite3_bind_text(stmt, 13, status, -1, NULL);
		sqlite3_bind_int(stmt, 14, id);

		sqlite3_step(stmt);

		sqlite3_finalize(stmt);

		pthread_mutex_unlock(&db_lock);
	}
	else if (!strcmp(type, "spsa")) {
		int dy = wins - losses;

		cJSON *param = NULL;
		cJSON_ArrayForEach(param, spsa) {
			const char *name = param->string;
			cJSON *spsaarg = cJSON_GetObjectItemCaseSensitive(spsaargs, name);
			double Delta = cJSON_GetObjectItemCaseSensitive(spsaarg, "Delta")->valuedouble;
			double ck = cJSON_GetObjectItemCaseSensitive(spsaarg, "ck")->valuedouble;
			double ak = cJSON_GetObjectItemCaseSensitive(spsaarg, "ak")->valuedouble;
			double theta = cJSON_GetObjectItemCaseSensitive(param, "theta")->valuedouble;
			double min = cJSON_GetObjectItemCaseSensitive(param, "min")->valuedouble;
			double max = cJSON_GetObjectItemCaseSensitive(param, "max")->valuedouble;
			double newtheta = fmin(fmax(theta + ak * dy / (2 * ck * Delta), min), max);
			cJSON_DeleteItemFromObject(param, "theta");
			cJSON_AddNumberToObject(param, "theta", newtheta);

			cJSON_DeleteItemFromObject(spsaargs, name);
			cJSON_AddNumberToObject(spsaargs, name, theta);
		}

		pthread_mutex_lock(&db_lock);
		sqlite3_prepare_v2(db,
			"UPDATE tests\n"
			"SET t0 = ?,\n"
				"t1 = ?,\n"
				"t2 = ?,\n"
				"p0 = ?,\n"
				"p1 = ?,\n"
				"p2 = ?,\n"
				"p3 = ?,\n"
				"p4 = ?,\n"
				"spsa = json(?)\n"
			"WHERE id = ? AND\n"
				"status = 'running';",
			-1, &stmt, NULL);
		sqlite3_bind_int(stmt, 1, t0);
		sqlite3_bind_int(stmt, 2, t1);
		sqlite3_bind_int(stmt, 3, t2);
		sqlite3_bind_int(stmt, 4, p[0]);
		sqlite3_bind_int(stmt, 5, p[1]);
		sqlite3_bind_int(stmt, 6, p[2]);
		sqlite3_bind_int(stmt, 7, p[3]);
		sqlite3_bind_int(stmt, 8, p[4]);
		sqlite3_bind_text(stmt, 9, cJSON_PrintUnformatted(spsa), -1, cJSON_free);
		sqlite3_bind_int(stmt, 10, id);

		sqlite3_step(stmt);

		sqlite3_finalize(stmt);
		sqlite3_prepare_v2(db,
			"UPDATE games\n"
			"SET spsa = json(?)\n"
			"WHERE id = ?;",
			-1, &stmt, NULL);
		if (dy)
			sqlite3_bind_text(stmt, 1, cJSON_PrintUnformatted(spsaargs), -1, cJSON_free);
		else
			sqlite3_bind_null(stmt, 1);
		sqlite3_bind_int(stmt, 2, task_iditem->valueint);

		sqlite3_step(stmt);

		sqlite3_finalize(stmt);

		pthread_mutex_unlock(&db_lock);
	}
	else if (!strcmp(type, "clop")) {
		void *cexp = clop_load(id);

		int seed = clop_pop_seed(id, task_id);
		if (cexp && seed >= 0) {
			clop_add_outcome(cexp, seed, wins, draws, losses);

			cJSON *mean = clop_get_mean(cexp);
			cJSON *max = clop_get_max(cexp);

			cJSON *param = NULL;
			cJSON_ArrayForEach(param, spsa) {
				cJSON_DeleteItemFromObject(param, "mean");
				cJSON_DeleteItemFromObject(param, "maximum");
				cJSON_AddNumberToObject(param, "mean", cJSON_GetObjectItemCaseSensitive(mean, param->string)->valuedouble);
				if (max)
					cJSON_AddNumberToObject(param, "maximum", cJSON_GetObjectItemCaseSensitive(max, param->string)->valuedouble);
				else
					cJSON_AddNullToObject(param, "maximum");
			}
			cJSON_Delete(mean);
			cJSON_Delete(max);
		}
		clop_return(cexp);

		pthread_mutex_lock(&db_lock);
		sqlite3_prepare_v2(db,
			"UPDATE tests\n"
			"SET t0 = ?,\n"
				"t1 = ?,\n"
				"t2 = ?,\n"
				"p0 = ?,\n"
				"p1 = ?,\n"
				"p2 = ?,\n"
				"p3 = ?,\n"
				"p4 = ?,\n"
				"spsa = json(?)\n"
			"WHERE id = ? AND status = 'running';",
			-1, &stmt, NULL);

		sqlite3_bind_int(stmt, 1, t0);
		sqlite3_bind_int(stmt, 2, t1);
		sqlite3_bind_int(stmt, 3, t2);
		sqlite3_bind_int(stmt, 4, p[0]);
		sqlite3_bind_int(stmt, 5, p[1]);
		sqlite3_bind_int(stmt, 6, p[2]);
		sqlite3_bind_int(stmt, 7, p[3]);
		sqlite3_bind_int(stmt, 8, p[4]);
		sqlite3_bind_text(stmt, 9, cJSON_PrintUnformatted(spsa), -1, cJSON_free);
		sqlite3_bind_int(stmt, 10, id);

		sqlite3_step(stmt);

		sqlite3_finalize(stmt);

		pthread_mutex_unlock(&db_lock);
	}
	pthread_mutex_unlock(&insert_lock);

	free(type);

	cJSON_Delete(spsa);
	cJSON_Delete(spsaargs);

	send_response(fd, "200 OK", "{\"message\": \"ok\"}");
}

void test_cancel(int fd, const struct http *http, int id) {
	(void)http;
	pthread_mutex_lock(&db_lock);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"UPDATE tests\n"
		"SET status = 'cancelled',\n"
			"starttime = CASE\n"
				"WHEN starttime IS NULL THEN unixepoch()\n"
				"ELSE starttime\n"
			"END,\n"
			"donetime = unixepoch()\n"
		"WHERE status IN ('running', 'queued')\n"
			"AND id = ?;",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, id);

	sqlite3_step(stmt);

	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	clop_unload(id);

	send_response(fd, "200 OK", "{\"message\": \"ok\"}");
}

void test_resume(int fd, const struct http *http, int id) {
	(void)http;
	pthread_mutex_lock(&db_lock);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"UPDATE tests\n"
		"SET status = 'running',\n"
			"donetime = NULL\n"
		"WHERE status = 'cancelled'\n"
			"AND id = ?;",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, id);

	sqlite3_step(stmt);

	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	send_response(fd, "200 OK", "{\"message\": \"ok\"}");
}

void test_requeue(int fd, const struct http *http, int id) {
	(void)http;
	pthread_mutex_lock(&db_lock);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"INSERT INTO tests (\n"
			"description,\n"
			"type,\n"
			"tc,\n"
			"alpha,\n"
			"beta,\n"
			"gamma,\n"
			"A,\n"
			"elo0,\n"
			"elo1,\n"
			"eloe,\n"
			"adjudicate,\n"
			"queuetime,\n"
			"commithash,\n"
			"simd,\n"
			"patch,\n"
			"spsa\n"
		")\n"
		"SELECT\n"
			"concat(description, ' (requeue)'),\n"
			"type,\n"
			"tc,\n"
			"alpha,\n"
			"beta,\n"
			"gamma,\n"
			"A,\n"
			"elo0,\n"
			"elo1,\n"
			"eloe,\n"
			"adjudicate,\n"
			"unixepoch(),\n"
			"commithash,\n"
			"simd,\n"
			"patch,\n"
			"spsa\n"
		"FROM tests\n"
		"WHERE id = ?;\n",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, id);

	sqlite3_step(stmt);

	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	send_response(fd, "200 OK", "{\"message\": \"ok\"}");
}

void test_error(int fd, const struct http *http, int id) {
	cJSON *errorlog = cJSON_GetObjectItemCaseSensitive(http->content, "errorlog");
	if (!errorlog || !cJSON_IsString(errorlog)) {
		bad_request(fd, "bad errorlog");
		return;
	}
	cJSON *task_id = cJSON_GetObjectItemCaseSensitive(http->content, "taskid");
	if (!task_id || !cJSON_IsNumber(task_id)) {
		bad_request(fd, "bad taskid");
		return;
	}

	pthread_mutex_lock(&db_lock);

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"DELETE FROM games\n"
		"WHERE id = ? AND testid = ? AND donetime IS NULL;",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, task_id->valueint);
	sqlite3_bind_int(stmt, 2, id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	sqlite3_prepare_v2(db,
		"UPDATE tests\n"
		"SET status = 'error', errorlog = ?\n"
		"WHERE id = ? AND status = 'running';",
		-1, &stmt, NULL);
	sqlite3_bind_text(stmt, 1, errorlog->valuestring, -1, NULL);
	sqlite3_bind_int(stmt, 2, id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	pthread_mutex_unlock(&db_lock);

	send_response(fd, "200 OK", "{\"message\": \"ok\"}");
}

void task_new(int fd, const struct http *http, int unused) {
	(void)unused;
	(void)http;
	pthread_mutex_lock(&db_lock);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"UPDATE tests\n"
			"SET starttime = CASE\n"
				"WHEN status = 'queued' THEN unixepoch()\n"
				"ELSE starttime\n"
			"END,\n"
			"status = 'running'\n"
		"WHERE id = (\n"
			"SELECT id FROM tests\n"
			"WHERE status IN ('running', 'queued')\n"
			"ORDER BY\n"
				"priority DESC,\n"
				"ifnull(\n"
					"(\n"
						"SELECT max(starttime)\n"
						"FROM games\n"
						"WHERE games.testid = tests.id\n"
					"),\n"
					"0\n"
				") ASC,\n"
				"queuetime ASC\n"
			"LIMIT 1\n"
		")\n"
		"RETURNING id, type, tc, adjudicate, spsa, alpha, gamma, A, t0, t1, t2;",
		-1, &stmt, NULL);

	if (sqlite3_step(stmt) == SQLITE_DONE) {
		sqlite3_finalize(stmt);
		pthread_mutex_unlock(&db_lock);
		send_response(fd, "200 Ok", "{\"id\": null}");
		return;
	}

	cJSON *response = cJSON_CreateObject();
	cJSON *argsplus = cJSON_CreateArray();
	cJSON *argsminus = cJSON_CreateArray();
	cJSON_AddItemToObject(response, "argsplus", argsplus);
	cJSON_AddItemToObject(response, "argsminus", argsminus);


	int id = sqlite3_column_int(stmt, 0);
	printf("got id %d\n", id);
	char *type = strdup((const char *)sqlite3_column_text(stmt, 1));
	char *tc = strdup((const char *)sqlite3_column_text(stmt, 2));
	char *adjudicate = strdup((const char *)sqlite3_column_text(stmt, 3));
	const char *jsoncolumn = (const char *)sqlite3_column_text(stmt, 4);
	char *jsonstr = NULL;
	if (jsoncolumn)
		jsonstr = strdup(jsoncolumn);

	double alpha = sqlite3_column_double(stmt, 5);
	double gamma = sqlite3_column_double(stmt, 6);
	double A = sqlite3_column_double(stmt, 7);
	double t0 = sqlite3_column_int(stmt, 8);
	double t1 = sqlite3_column_int(stmt, 9);
	double t2 = sqlite3_column_int(stmt, 10);
	int k = (t0 + t1 + t2) / 2;
	double weight = 0.0;
	cJSON *spsaargs = NULL;

	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	int seed;
	if (!strcmp(type, "spsa")) {
		cJSON *spsa = cJSON_Parse(jsonstr);
		spsaargs = cJSON_CreateObject();

		cJSON *param;
		cJSON_ArrayForEach(param, spsa) {
			const char *name = param->string;
			double a = cJSON_GetObjectItemCaseSensitive(param, "a")->valuedouble;
			double c = cJSON_GetObjectItemCaseSensitive(param, "c")->valuedouble;
			double theta = cJSON_GetObjectItemCaseSensitive(param, "theta")->valuedouble;
			double min = cJSON_GetObjectItemCaseSensitive(param, "min")->valuedouble;
			double max = cJSON_GetObjectItemCaseSensitive(param, "max")->valuedouble;

			int Delta = (rand() & 1) ? 1 : -1;
			double ak = a / pow(A + k + 1, alpha);
			double ck = c / pow(A + k + 1, gamma);

			double thetaplus = fmin(fmax(theta + ck * Delta, min), max);
			double thetaminus = fmin(fmax(theta - ck * Delta, min), max);

			char *splus = malloc(strlen(name) + 64);
			char *sminus = malloc(strlen(name) + 64);
			sprintf(splus, "option.%s=%lf", name, thetaplus);
			sprintf(sminus, "option.%s=%lf", name, thetaminus);
			cJSON *strplus = cJSON_CreateString(splus);
			cJSON *strminus = cJSON_CreateString(sminus);
			cJSON_AddItemToArray(argsplus, strplus);
			cJSON_AddItemToArray(argsminus, strminus);

			cJSON *constants = cJSON_CreateObject();
			cJSON_AddItemToObject(spsaargs, name, constants);
			cJSON_AddNumberToObject(constants, "ak", ak);
			cJSON_AddNumberToObject(constants, "ck", ck);
			cJSON_AddNumberToObject(constants, "Delta", Delta);

			free(splus);
			free(sminus);
		}

		cJSON_Delete(spsa);
	}
	else if (!strcmp(type, "clop")) {
		printf("loading clop\n");
		void *cexp = clop_load(id);

		printf("loaded %p\n", cexp);
		spsaargs = clop_next_sample(cexp, &seed, &weight);
		cJSON *param;
		cJSON_ArrayForEach(param, spsaargs) {
			const char *name = param->string;
			char *s = malloc(strlen(name) + 64);
			sprintf(s, "option.%s=%lf", name, param->valuedouble);
			cJSON *str = cJSON_CreateString(s);
			free(s);
			cJSON_AddItemToArray(argsplus, str);
		}


		clop_return(cexp);
		printf("returned clop\n");
	}

	pthread_mutex_lock(&db_lock);
	sqlite3_prepare_v2(db,
		"INSERT INTO games (\n"
			"testid,\n"
			"starttime,\n"
			"spsa,\n"
			"weight\n"
		")\n"
		"VALUES (\n"
			"?,\n"
			"unixepoch(),\n"
			"json(?),\n"
			"?\n"
		")\n"
		"RETURNING id;",
		-1, &stmt, NULL);

	sqlite3_bind_int(stmt, 1, id);
	if (spsaargs) {
		sqlite3_bind_text(stmt, 2, cJSON_PrintUnformatted(spsaargs), -1, cJSON_free);
		cJSON_Delete(spsaargs);
	}
	else {
		sqlite3_bind_null(stmt, 2);
	}
	sqlite3_bind_double(stmt, 3, weight);

	sqlite3_step(stmt);

	int task_id = sqlite3_column_int(stmt, 0);

	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	if (!strcmp(type, "clop"))
		clop_store_seed(id, task_id, seed);

	cJSON_AddNumberToObject(response, "id", id);
	cJSON_AddStringToObject(response, "tc", tc);
	cJSON_AddStringToObject(response, "adjudicate", adjudicate);
	cJSON_AddNumberToObject(response, "taskid", task_id);

	send_json_response(fd, "200 OK", response);
	cJSON_Delete(response);

	free(type);
	free(tc);
	free(adjudicate);
	free(jsonstr);
}
