#include <stdio.h>
#include <ini.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <pthread.h>
#include <unistd.h>
#include <cJSON.h>
#include <signal.h>
#include <stdatomic.h>

#include "test.h"
#include "util.h"
#include "build.h"
#include "tc.h"
#include "cgroup.h"

struct threadinfo {
	struct config *cfg;
	struct cpu *cpu;
};

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
	size_t realsize = size * nmemb;
	struct memory *mem = userdata;
	char *p = realloc(mem->data, mem->size + realsize + 1);
	if (!p)
		return 0;
	mem->data = p;
	memcpy(mem->data + mem->size, ptr, realsize);
	mem->size += realsize;
	mem->data[mem->size] = 0;
	return realsize;
}

struct config {
	char *hostname;
	char *port;
	long long workers;
	char *syzygy;

	double tcfactor;
};

void free_config(struct config *cfg) {
	free(cfg->hostname);
	free(cfg->port);
	free(cfg->syzygy);
}

static int config_handler(void *user, const char *section, const char *name, const char *value) {
	char *endptr = NULL;
	struct config *cfg = user;
	if (!strcmp(section, "testbitn")) {
		if (!strcmp(name, "workers")) {
			errno = 0;
			cfg->workers = strtoll(value, &endptr, 10);
			if (errno || *endptr || cfg->workers <= 0 || cfg->workers > 256) {
				fprintf(stderr, "error: bad workers '%s'\n", value);
				return 0;
			}
		}
		else if (!strcmp(name, "host")) {
			cfg->hostname = strdup(value);
		}
		else if (!strcmp(name, "port")) {
			cfg->port = strdup(value);
		}
		else if (!strcmp(name, "syzygy")) {
			cfg->syzygy = strdup(value);
		}
	}
	else if (!strcmp(section, "timecontrol")) {
		if (!strcmp(name, "tcfactor")) {
			errno = 0;
			cfg->tcfactor = strtod(value, &endptr);
			if (errno || *endptr || cfg->tcfactor <= 0.0) {
				fprintf(stderr, "error: bad tcfactor '%s'\n", value);
				return 0;
			}
		}
	}
	return 1;
}

static void *worker(void *arg) {
	struct threadinfo *ti = arg;
	struct config *cfg = ti->cfg;
	struct cpu *cpu = ti->cpu;
	struct memory chunk;
	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	cJSON *json;
	CURLcode res;

	while (1) {
		chunk.data = NULL;
		chunk.size = 0;
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
#warning set hostname and port
		curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3333/test/task");
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
		json = NULL;
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			fprintf(stderr, "error: curl: %s\n", curl_easy_strerror(res));
			free(chunk.data);
			fprintf(stderr, "sleeping for 300 seconds\n");
			release_cpu(cpu);
			sleep(300);
			goto end;
		}
		long code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
		if (code != 200) {
			fprintf(stderr, "error: got response code %ld\n", code);
			release_cpu(cpu);
			sleep(300);
			goto end;
		}

		if (!chunk.data) {
			fprintf(stderr, "error: got no response\n");
			release_cpu(cpu);
			sleep(300);
			goto end;
		}

		printf("got: '%s'\n", chunk.data);
		json = cJSON_Parse(chunk.data);
		if (!json) {
			fprintf(stderr, "error: bad json '%s'\n", chunk.data);
			release_cpu(cpu);
			sleep(300);
			goto end;
		}

		cJSON *idobject = cJSON_GetObjectItemCaseSensitive(json, "id");
		if (!idobject) {
			fprintf(stderr, "error: no id\n");
			release_cpu(cpu);
			sleep(300);
			goto end;
		}

		if (cJSON_IsNull(idobject)) {
			release_cpu(cpu);
			sleep(10);
			goto end;
		}

		if (!cJSON_IsNumber(idobject)) {
			fprintf(stderr, "error: id is not number\n");
			release_cpu(cpu);
			sleep(300);
			goto end;
		}

		cJSON *tcobject = cJSON_GetObjectItemCaseSensitive(json, "tc");

		struct tc tc;
		if (!tcobject || !cJSON_IsString(tcobject) || parsetc(tcobject->valuestring, &tc)) {
			fprintf(stderr, "error: bad tc\n");
			release_cpu(cpu);
			sleep(300);
			goto end;
		}

		cJSON *adjudicate = cJSON_GetObjectItemCaseSensitive(json, "adjudicate");
		if (!adjudicate || !cJSON_IsString(adjudicate) || (strcmp(adjudicate->valuestring, "none") && strcmp(adjudicate->valuestring, "both") && strcmp(adjudicate->valuestring, "resign") && strcmp(adjudicate->valuestring, "draw"))) {
			fprintf(stderr, "error: bad adjudicate\n");
			release_cpu(cpu);
			sleep(300);
			goto end;
		}

		int id = idobject->valueint;
		cJSON *argplusobject = cJSON_GetObjectItemCaseSensitive(json, "argsplus");
		cJSON *argminusobject = cJSON_GetObjectItemCaseSensitive(json, "argsminus");

		if (cJSON_IsNull(argplusobject)) {
			argplusobject = NULL;
		}
		else if (!cJSON_IsArray(argplusobject)) {
			fprintf(stderr, "error: argplus is not array\n");
			release_cpu(cpu);
			sleep(300);
			goto end;
		}

		if (cJSON_IsNull(argminusobject)) {
			argminusobject = NULL;
		}
		else if (!cJSON_IsArray(argminusobject)) {
			fprintf(stderr, "error: argminus is not array\n");
			release_cpu(cpu);
			sleep(300);
			goto end;
		}

		const char *dir;
		if (load_test(id, "http://localhost:3333", curl, &dir) || !dir) {
			fprintf(stderr, "error: failed to load test\n");
			release_cpu(cpu);
			goto end;
		}

		printf("got: %s\n", dir);

		adjusttc(&tc, cfg->tcfactor);
		char tcstr[128];
		tctostr(tcstr, &tc);
		/* Ok if cpu is already claimed. */
		claim_cpu(cpu);
		fastchess(curl, cpu, dir, adjudicate->valuestring, cfg->syzygy, tcstr);

		return_test(id);
end:
		cJSON_Delete(json);
		free(chunk.data);
		printf("break\n");
		break;
	}
	curl_easy_cleanup(curl);
	return NULL;
}

#warning make setting stop actually stop all threads
atomic_int stop;
static void sigint_handler(int signum) {
	atomic_store_explicit(&stop, 1, memory_order_relaxed);
}

struct cpus cpus = { 0 };

static void cleanup_cpus(void) {
	printf("cleaning up\n");
	for (int i = 0; i < cpus.n; i++)
		release_cpu(&cpus.cpus[i]);
}

int main(int argc, char **argv) {
	atexit(cleanup_cpus);
	signal(SIGINT, sigint_handler);
	curl_global_init(CURL_GLOBAL_ALL);
	test_init();
	struct config cfg = { .workers = 1, .tcfactor = 1.0 };
	if (ini_parse("/etc/bitbit.ini", config_handler, &cfg)) {
		fprintf(stderr, "error: failed to load '/etc/bitbit.ini'\n");
		goto error;
	}

	if (cpu_strategy(&cpus, cfg.workers)) {
		fprintf(stderr, "error: failed to make cpu strategy\n");
		goto error;
	}

	pthread_t *thread = malloc(cfg.workers * sizeof(*thread));
	struct threadinfo *ti = malloc(cfg.workers * sizeof(*ti));
	for (int i = 0; i < cfg.workers; i++) {
		ti[i].cfg = &cfg;
		ti[i].cpu = &cpus.cpus[i];
		pthread_create(&thread[i], NULL, worker, &ti[i]);
	}

	for (int i = 0; i < cfg.workers; i++)
		pthread_join(thread[i], NULL);

	printf("joined all threads\n");

	free(ti);
	free(thread);
error:
	free_cpus(&cpus);
	curl_global_cleanup();
	free_config(&cfg);
	test_term();
}
