#include <ncurses.h>
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

struct memory {
	char *response;
	size_t size;
};

#define TPPERSEC ((int64_t)1000000000ll)
#define TPPERMS  ((int64_t)1000000ll)
typedef int64_t timepoint_t;

timepoint_t time_now(void) {
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (timepoint_t)tp.tv_sec * TPPERSEC + (timepoint_t)tp.tv_nsec;
}

size_t cb(char *data, size_t size, size_t nmemb, void *clientp) {
	(void)size;
	size_t realsize = nmemb;
	struct memory *mem = (struct memory *)clientp;

	char *ptr = realloc(mem->response, mem->size + realsize + 1);
	if (!ptr)
		return 0;

	mem->response = ptr;
	memcpy(&mem->response[mem->size], data, realsize);
	mem->size += realsize;
	mem->response[mem->size] = '\0';

	return realsize;
}

char *do_curl(CURL *curl, const char *url) {
	struct memory chunk = { 0 };
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
	CURLcode result = curl_easy_perform(curl);
	if (result != CURLE_OK) {
		fprintf(stderr, "error: '%s'\n", curl_easy_strerror(result));
		endwin();
		exit(1);
	}
	return chunk.response;
}

const double scale = 500.0;
double K(double x, double y, double z, double w) {
	return exp(-scale * ((x - z) * (x - z) + (y - w) * (y - w)));
}

void init_colors(void);
void set_color(double score, double weight);
void set_purple(void);

double signed_pow(double x, double y) {
	return copysign(pow(fabs(x), y), x);
}

pthread_mutex_t mutex;
pthread_cond_t cond;

int has_new_data = 0;
size_t length = 0;
double *weight = NULL;
double *score = NULL;
double *pointx = NULL;
double *pointy = NULL;

pthread_mutex_t bounds;
double min[2] = { 0 };
double max[2] = { 0 };

void *worker_curl(void *arg) {
	const char **argv = arg;
	CURL *curl = curl_easy_init();

	const char *param[2] = { argv[2], argv[3] };

	const char *baseurl = "http://192.168.1.214:2718/clop/";
	char *url = malloc(strlen(baseurl) + strlen(argv[1]) + 1);
	sprintf(url, "%s%s", baseurl, argv[1]);

	while (true) {
		char *response = do_curl(curl, url);
		cJSON *json = cJSON_Parse(response);
		free(response);
		char *message = cJSON_GetObjectItem(json, "message")->valuestring;
		cJSON *test = cJSON_GetObjectItem(json, "test");
		cJSON *spsa = cJSON_GetObjectItem(test, "spsa");
		cJSON *spsahistory = cJSON_GetObjectItem(test, "spsahistory");
		if (strcmp(message, "ok")) {
			fprintf(stderr, "%s\n", message);
			endwin();
			exit(1);
		}
		double min_new[2];
		double max_new[2];
		for (int i = 0; i < 2; i++) {
			cJSON *paraminfo = cJSON_GetObjectItem(spsa, param[i]);
			if (!paraminfo) {
				fprintf(stderr, "parameter '%s' does not exist\n", param[i]);
				endwin();
				exit(1);
			}
			min_new[i] = cJSON_GetObjectItem(paraminfo, "min")->valuedouble;
			max_new[i] = cJSON_GetObjectItem(paraminfo, "max")->valuedouble;
		}

		size_t length_new = cJSON_GetArraySize(spsahistory);
		if (length_new <= 0) {
			sleep(5);
			cJSON_Delete(json);
			continue;
		}
		double *weight_new = malloc(length_new * sizeof(*weight_new));
		double *score_new = malloc(length_new * sizeof(*score_new));
		double *pointx_new = malloc(length_new * sizeof(*pointx_new));
		double *pointy_new = malloc(length_new * sizeof(*pointy_new));
		for (size_t i = 0; i < length_new; i++) {
			cJSON *point = cJSON_GetArrayItem(spsahistory, i);
			pointx_new[i] = (cJSON_GetObjectItem(point, param[0])->valuedouble - min_new[0]) / (max_new[0] - min_new[0]);
			pointy_new[i] = (max_new[1] - cJSON_GetObjectItem(point, param[1])->valuedouble) / (max_new[1] - min_new[1]);
			weight_new[i] = cJSON_GetObjectItem(point, "_weight")->valuedouble;
			score_new[i] = cJSON_GetObjectItem(point, "_score")->valuedouble;
		}
		cJSON_Delete(json);

		pthread_mutex_lock(&mutex);
		free(weight);
		free(score);
		free(pointx);
		free(pointy);
		weight = weight_new;
		score = score_new;
		pointx = pointx_new;
		pointy = pointy_new;
		for (int i = 0; i < 2; i++) {
			if (min[i] != min_new[i]) {
				pthread_mutex_lock(&bounds);
				min[i] = min_new[i];
				pthread_mutex_unlock(&bounds);
			}
			if (max[i] != max_new[i]) {
				pthread_mutex_lock(&bounds);
				max[i] = max_new[i];
				pthread_mutex_unlock(&bounds);
			}
		}
		has_new_data = 1;
		length = length_new;
		pthread_cond_broadcast(&cond);
		//printf("Done with request\n");
		pthread_mutex_unlock(&mutex);
		sleep(10);
	}
}

pthread_mutex_t resize;
int lines, cols;
atomic_int *image = NULL;

int get_purple(void);
int get_color(double score, double weight);
int get_color_max_weight(int color);

void handle_resize(void) {
	pthread_mutex_lock(&resize);
	free(image);
	lines = LINES;
	cols = COLS;
	image = calloc(lines * cols, sizeof(*image));
	pthread_mutex_unlock(&resize);
}

void *worker_create_image(void *arg) {
	(void)arg;

	while (true) {
		pthread_mutex_lock(&mutex);
		while (!has_new_data)
			pthread_cond_wait(&cond, &mutex);
		has_new_data = 0;
		size_t length_now = length;
		double *weight_now = malloc(length_now * sizeof(*weight_now));
		double *score_now = malloc(length_now * sizeof(*score_now));
		double *pointx_now = malloc(length_now * sizeof(*pointx_now));
		double *pointy_now = malloc(length_now * sizeof(*pointy_now));
		memcpy(weight_now, weight, length_now * sizeof(*weight_now));
		memcpy(score_now, score, length_now * sizeof(*score_now));
		memcpy(pointx_now, pointx, length_now * sizeof(*pointx_now));
		memcpy(pointy_now, pointy, length_now * sizeof(*pointy_now));
		pthread_mutex_unlock(&mutex);
		pthread_mutex_lock(&resize);

		for (int line = 0; line < lines; line++) {
			for (int col = 0; col < cols; col++) {
				int index = col + cols * line;
				double x = (double)col / (cols - 1);
				double y = (double)line / (lines - 1);
				double total_kernel = 0.0;
				double total_weight = 0.0;
				double total_score = 0.0;
				for (size_t i = 0; i < length_now; i++) {
					double kernel = K(pointx_now[i], pointy_now[i], x, y);
					total_kernel += kernel;
					total_weight += kernel * weight_now[i];
					total_score += kernel * score_now[i];
				}
				double score_here = total_score / total_kernel;
				double weight_here = total_weight / total_kernel;
				score_here = signed_pow(2.0 * (score_here - 0.5), scale / 900.0) / 2.0 + 0.5;

				int color = get_color(score_here, weight_here);
				if (color && 0) {
					endwin();
					printf("color: %d\n", color);
					exit(1);
				}
				atomic_store_explicit(&image[index], color, memory_order_relaxed);
			}
		}

		free(weight_now);
		free(score_now);
		free(pointx_now);
		free(pointy_now);

		//printf("Created image\n");
		//attrset(0);
		//mvprintw(0, 0, "Created image");
		pthread_mutex_unlock(&resize);
	}
}

int main(int argc, char **argv) {
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&resize, NULL);
	pthread_mutex_init(&bounds, NULL);
	pthread_cond_init(&cond, NULL);
	int use_weight = 1;

	if (argc < 4) {
	printf("usage: %s id x-parameter y-parameter\n", argv[0]);
		return 1;
	}

	pthread_t thread_curl, thread_image;
	pthread_create(&thread_curl, NULL, &worker_curl, argv);
	pthread_create(&thread_image, NULL, &worker_create_image, argv);

	initscr();
	mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
	keypad(stdscr, TRUE);
	curs_set(0);
	cbreak();
	noecho();
	timeout(0);
	init_colors();
	handle_resize();

	double mousex = -1.0;
	double mousey = -1.0;
	int mousecol = -1;
	int mouseline = -1;
	int running = 1;
	char message[2][4096] = { 0 };
	while (running) {
#if 0
		char bufx[4096] = { 0 };
		char bufy[4096] = { 0 };
#endif
		int ch = getch();
		MEVENT event;
		switch (ch) {
		case 'w':
			use_weight = !use_weight;
			break;
		case 'q':
			running = 0;
			break;
		case KEY_MOUSE:
			if (getmouse(&event) == OK) {
				mousex = (double)event.x / (cols - 1);
				mousey = (double)event.y / (lines - 1);
				mouseline = event.y;
				mousecol = event.x;
				attrset(COLOR_PAIR(get_purple()));
				mvaddch(event.y, event.x, ' ');
			}
			break;
		case KEY_RESIZE:
			handle_resize();
			mousex = mousey = -1.0;
			mousecol = mouseline = 0;
			break;
		default:
			break;
		}

		pthread_mutex_lock(&bounds);
		if (mousex >= 0 && mousey >= 0) {
			sprintf(message[0], "%s: %.3f", argv[2], min[0] + (max[0] - min[0]) * mousex);
			sprintf(message[1], "%s: %.3f", argv[3], max[1] + (min[1] - max[1]) * mousey);
		}
		else {
			message[0][0] = 0;
			message[1][0] = 0;
		}
		pthread_mutex_unlock(&bounds);
		for (int line = 0; line < lines; line++) {
			for (int col = 0; col < cols; col++) {
				int index = col + cols * line;
				int color = atomic_load_explicit(&image[index], memory_order_relaxed);

				if (col == mousecol && line == mouseline)
					color = get_purple();
				else if (!use_weight)
					color = get_color_max_weight(color);


				attrset(COLOR_PAIR(color));
				char c = ' ';
				if (line < 2 && col < (int)strlen(message[line]))
					c = message[line][col];
				mvaddch(line, col, c);
			}
		}

		refresh();
	}

	endwin();
}
