#include "post.h"

#include <string.h>
#include <stdio.h>
#include <time.h>

#include "tc.h"
#include "git.h"
#include "sql.h"

void test_new(int fd, const struct http *http, int id) {
	(void)id;
	cJSON *patch = cJSON_GetObjectItemCaseSensitive(http->content, "patch");
	if (!patch || !cJSON_IsString(patch)) {
		bad_request(fd, "bad patch");
		return;
	}

	cJSON *type = cJSON_GetObjectItemCaseSensitive(http->content, "type");
	if (!type || !cJSON_IsString(type)) {
		bad_request(fd, "bad type");
		return;
	}

	const char *typestr = type->valuestring;
	if (strcmp(typestr, "sprt") && strcmp(typestr, "elo") && strcmp(typestr, "clop") && strcmp(typestr, "spsa")) {
		bad_request(fd, "bad type");
		return;
	}

	cJSON *tc = cJSON_GetObjectItemCaseSensitive(http->content, "tc");
	const char *tcstr;
	struct tc tcobj;
	if (!tc || !cJSON_IsString(tc) || parsetc((tcstr = tc->valuestring), &tcobj)) {
		bad_request(fd, "bad tc");
		return;
	}

	cJSON *description = cJSON_GetObjectItemCaseSensitive(http->content, "description");
	const char *descriptionstr;
	if (!description || !cJSON_IsString(description) || !(descriptionstr = description->valuestring)[0]) {
		bad_request(fd, "bad description");
		return;
	}

	cJSON *adjudicate = cJSON_GetObjectItemCaseSensitive(http->content, "adjudicate");
	if (!adjudicate || !cJSON_IsString(adjudicate)) {
		bad_request(fd, "bad adjudicate");
		return;
	}
	const char *adjudicatestr = adjudicate->valuestring;
	if (strcmp(adjudicatestr, "none") && strcmp(adjudicatestr, "draw") && strcmp(adjudicatestr, "resign") && strcmp(adjudicatestr, "both")) {
		bad_request(fd, "bad adjudicate");
		return;
	}

	cJSON *simd = cJSON_GetObjectItemCaseSensitive(http->content, "simd");
	if (!simd || !cJSON_IsString(simd) || !strisalnum(simd->valuestring)) {
		bad_request(fd, "bad simd");
		return;
	}

	cJSON *alpha = cJSON_GetObjectItemCaseSensitive(http->content, "alpha");
	cJSON *beta = cJSON_GetObjectItemCaseSensitive(http->content, "beta");
	cJSON *gamma = cJSON_GetObjectItemCaseSensitive(http->content, "gamma");
	cJSON *A = cJSON_GetObjectItemCaseSensitive(http->content, "A");
	cJSON *elo0 = cJSON_GetObjectItemCaseSensitive(http->content, "elo0");
	cJSON *elo1 = cJSON_GetObjectItemCaseSensitive(http->content, "elo1");
	cJSON *eloe = cJSON_GetObjectItemCaseSensitive(http->content, "eloe");
	cJSON *commit = cJSON_GetObjectItemCaseSensitive(http->content, "commit");
	cJSON *spsadata = cJSON_GetObjectItemCaseSensitive(http->content, "spsa");

	cJSON *spsa = NULL;

	if (!strcmp(type->valuestring, "sprt")) {
		if (!alpha || !cJSON_IsNumber(alpha) || alpha->valuedouble <= 0.0) {
			bad_request(fd, "bad alpha");
			return;
		}
		if (!beta || !cJSON_IsNumber(beta) || beta->valuedouble <= 0.0) {
			bad_request(fd, "bad beta");
			return;
		}
		if (!elo0 || !cJSON_IsNumber(elo0)) {
			bad_request(fd, "bad elo0");
			return;
		}
		if (!elo1 || !cJSON_IsNumber(elo1)) {
			bad_request(fd, "bad elo1");
			return;
		}
		if (elo0->valuedouble >= elo1->valuedouble) {
			bad_request(fd, "need elo0 < elo1");
			return;
		}
		if (alpha->valuedouble + beta->valuedouble >= 0.5) {
			bad_request(fd, "need alpha + beta < 0.5");
			return;
		}
		eloe = NULL;
		spsa = NULL;
		A = NULL;
		gamma = NULL;
	}
	else if (!strcmp(type->valuestring, "elo")) {
		if (!eloe || !cJSON_IsNumber(eloe) || eloe->valuedouble <= 0.0) {
			bad_request(fd, "bad eloe");
			return;
		}
		alpha = NULL;
		beta = NULL;
		elo0 = NULL;
		elo1 = NULL;
		spsa = NULL;
		A = NULL;
		gamma = NULL;
	}
	else if (!strcmp(type->valuestring, "spsa")) {
		/* spsadata cannot be empty map. */
		if (!spsadata || !cJSON_IsObject(spsadata) || !spsadata->child || cJSON_GetArraySize(spsadata) > 2048) {
			bad_request(fd, "bad spsa");
			return;
		}
		if (!A || !cJSON_IsNumber(A) || A->valueint < 0) {
			bad_request(fd, "bad A");
			return;
		}
		if (!alpha || !cJSON_IsNumber(alpha) || alpha->valuedouble <= 0.0) {
			bad_request(fd, "bad alpha");
			return;
		}
		if (!gamma || !cJSON_IsNumber(gamma) || gamma->valuedouble <= 0.0) {
			bad_request(fd, "bad gamma");
			return;
		}
		beta = NULL;
		elo0 = NULL;
		elo1 = NULL;
		eloe = NULL;

		spsa = cJSON_CreateObject();
		cJSON *param = NULL;
		cJSON_ArrayForEach(param, spsadata) {
			const char *name = param->string;
			cJSON *theta = cJSON_GetObjectItemCaseSensitive(param, "theta");
			cJSON *min = cJSON_GetObjectItemCaseSensitive(param, "min");
			cJSON *max = cJSON_GetObjectItemCaseSensitive(param, "max");
			cJSON *a = cJSON_GetObjectItemCaseSensitive(param, "a");
			cJSON *c = cJSON_GetObjectItemCaseSensitive(param, "c");

			if (!name || !*name) {
				cJSON_Delete(spsa);
				bad_request(fd, "bad spsa.param.name");
				return;
			}
			if (cJSON_GetObjectItemCaseSensitive(spsa, name)) {
				cJSON_Delete(spsa);
				bad_request(fd, "duplicate spsa.param.name");
				return;
			}
			if (!theta || !cJSON_IsNumber(theta)) {
				cJSON_Delete(spsa);
				bad_request(fd, "bad spsa.param.theta");
				return;
			}
			if (!min || !cJSON_IsNumber(min)) {
				cJSON_Delete(spsa);
				bad_request(fd, "bad spsa.param.min");
				return;
			}
			if (!max || !cJSON_IsNumber(max)) {
				cJSON_Delete(spsa);
				bad_request(fd, "bad spsa.param.max");
				return;
			}
			if (theta->valuedouble < min->valuedouble || theta->valuedouble > max->valuedouble) {
				cJSON_Delete(spsa);
				bad_request(fd, "need spsa.param.min <= spsa.param.theta <= spsa.param.max");
				return;
			}
			if (!a || !cJSON_IsNumber(a) || a->valuedouble <= 0.0) {
				cJSON_Delete(spsa);
				bad_request(fd, "bad spsa.param.a");
				return;
			}
			if (!c || !cJSON_IsNumber(c) || c->valuedouble <= 0.0) {
				cJSON_Delete(spsa);
				bad_request(fd, "bad spsa.param.c");
				return;
			}
			cJSON *m = cJSON_CreateObject();
			cJSON_AddNumberToObject(m, "theta", theta->valuedouble);
			cJSON_AddNumberToObject(m, "min", min->valuedouble);
			cJSON_AddNumberToObject(m, "max", max->valuedouble);
			cJSON_AddNumberToObject(m, "a", a->valuedouble);
			cJSON_AddNumberToObject(m, "c", c->valuedouble);
			cJSON_AddItemToObject(spsa, name, m);
		}
	}
	else if (!strcmp(type->valuestring, "clop")) {
		alpha = NULL;
		beta = NULL;
		elo0 = NULL;
		elo1 = NULL;
		eloe = NULL;
		A = NULL;
		gamma = NULL;

		if (!spsadata || !cJSON_IsObject(spsadata) || !spsadata->child || cJSON_GetArraySize(spsadata) > 2048) {
			bad_request(fd, "bad clop");
			return;
		}

		spsa = cJSON_CreateObject();
		cJSON *param = NULL;
		cJSON_ArrayForEach(param, spsadata) {
			const char *name = param->string;
			cJSON *min = cJSON_GetObjectItemCaseSensitive(param, "min");
			cJSON *max = cJSON_GetObjectItemCaseSensitive(param, "max");

			/* Cannot have names that start with _ when clopping as these
			 * are used for internal variables like _weight.
			 */
			if (!name || !*name || name[0] == '_') {
				cJSON_Delete(spsa);
				bad_request(fd, "bad clop.param.name");
				return;
			}
			if (cJSON_GetObjectItemCaseSensitive(spsa, name)) {
				cJSON_Delete(spsa);
				bad_request(fd, "duplicate clop.param.name");
				return;
			}
			if (!min || !cJSON_IsNumber(min)) {
				cJSON_Delete(spsa);
				bad_request(fd, "bad clop.param.min");
				return;
			}
			if (!max || !cJSON_IsNumber(max)) {
				cJSON_Delete(spsa);
				bad_request(fd, "bad clop.param.max");
				return;
			}
			if (min->valuedouble >= max->valuedouble) {
				cJSON_Delete(spsa);
				bad_request(fd, "need clop.param.min < clop.param.max");
				return;
			}
			cJSON *m = cJSON_CreateObject();
			cJSON_AddNumberToObject(m, "min", min->valuedouble);
			cJSON_AddNumberToObject(m, "max", max->valuedouble);
			cJSON_AddNullToObject(m, "mean");
			cJSON_AddNullToObject(m, "maximum");
			cJSON_AddItemToObject(spsa, name, m);
		}
	}

	int r = 0;
	if (!commit || !cJSON_IsString(commit) || (r = check_ref_format(commit->valuestring))) {
		cJSON_Delete(spsa);
		char message[128];
		sprintf(message, "bad commit (%d)", r);
		bad_request(fd, message);
		return;
	}


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
		"VALUES (\n"
			"?,\n"
			"?,\n"
			"?,\n"
			"?,\n"
			"?,\n"
			"?,\n"
			"?,\n"
			"?,\n"
			"?,\n"
			"?,\n"
			"?,\n"
			"unixepoch(),\n"
			"?,\n"
			"?,\n"
			"?,\n"
			"json(?)\n"
		");",
		-1, &stmt, NULL);

	sqlite3_bind_text(stmt, 1, descriptionstr, -1, NULL);
	sqlite3_bind_text(stmt, 2, typestr, -1, NULL);
	sqlite3_bind_text(stmt, 3, tcstr, -1, NULL);
	if (alpha)
		sqlite3_bind_double(stmt, 4, alpha->valuedouble);
	else
		sqlite3_bind_null(stmt, 4);
	if (beta)
		sqlite3_bind_double(stmt, 5, beta->valuedouble);
	else
		sqlite3_bind_null(stmt, 5);
	if (gamma)
		sqlite3_bind_double(stmt, 6, gamma->valuedouble);
	else
		sqlite3_bind_null(stmt, 6);
	if (A)
		sqlite3_bind_double(stmt, 7, A->valuedouble);
	else
		sqlite3_bind_null(stmt, 7);
	if (elo0)
		sqlite3_bind_double(stmt, 8, elo0->valuedouble);
	else
		sqlite3_bind_null(stmt, 8);
	if (elo1)
		sqlite3_bind_double(stmt, 9, elo1->valuedouble);
	else
		sqlite3_bind_null(stmt, 9);
	if (eloe)
		sqlite3_bind_double(stmt, 10, eloe->valuedouble);
	else
		sqlite3_bind_null(stmt, 10);
	sqlite3_bind_text(stmt, 11, adjudicatestr, -1, NULL);
	sqlite3_bind_text(stmt, 12, commit->valuestring, -1, NULL);
	sqlite3_bind_text(stmt, 13, simd->valuestring, -1, NULL);
	sqlite3_bind_text(stmt, 14, patch->valuestring, -1, NULL);
	sqlite3_bind_text(stmt, 15, cJSON_PrintUnformatted(spsa), -1, cJSON_free);

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	pthread_mutex_unlock(&db_lock);

	cJSON_Delete(spsa);
	send_response(fd, "200 Ok", "{\"message\": \"ok\"}");
}

char backupdir[4096];
time_t last_backup = 0;

void backup_database(int fd, const struct http *http, int id) {
	(void)id;
	if (!backupdir[0]) {
		send_response(fd, "501 Not Implemented", "{\"message\": \"backup directory not set\"}");
		return;
	}
	cJSON *prefix = cJSON_GetObjectItemCaseSensitive(http->content, "prefix");
	if (!prefix || !cJSON_IsString(prefix) || strlen(prefix->valuestring) >= 64 || strchr(prefix->valuestring, '/')) {
		bad_request(fd, "bad prefix");
		return;
	}

	pthread_mutex_lock(&insert_lock);
	pthread_mutex_lock(&db_lock);
	if (time(NULL) < last_backup + 3600) {
		pthread_mutex_unlock(&insert_lock);
		pthread_mutex_unlock(&db_lock);
		send_response(fd, "200 Ok", "{\"message\": \"already backed up\"}");
		return;
	}

	struct tm tm = { 0 };
	time_t now = time(NULL);
	gmtime_r(&now, &tm);
	char backuppath[8192];
	size_t pos = sprintf(backuppath, "%s/%s%s", backupdir, prefix->valuestring, prefix->valuestring[0] ? "-" : "");
	strftime(backuppath + pos, 1024, "%FT%T.sqlite3", &tm);
	printf("backup: '%s'\n", backuppath);

	sqlite3 *backupdb;
	int rc = sqlite3_open(backuppath, &backupdb);
	sqlite3_backup *backup;
	if (rc == SQLITE_OK && (backup = sqlite3_backup_init(backupdb, "main", db, "main"))) {
		sqlite3_backup_step(backup, -1);
		sqlite3_backup_finish(backup);
		rc = sqlite3_errcode(backupdb);
	}

	if (rc == SQLITE_OK && backup) {
		last_backup = time(NULL);
		send_response(fd, "200 Ok", "{\"message\": \"ok\"}");
	}
	else {
		send_response(fd, "500 Internal Server Error", "{\"message\": \"failed to open database\"}");
	}

	sqlite3_close(backupdb);

	pthread_mutex_unlock(&db_lock);
	pthread_mutex_unlock(&insert_lock);
}
