#ifndef CLOP_H
#define CLOP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cjson/cJSON.h>

void *clop_load(int id);

void clop_return(void *clop);

void clop_unload(int id);

void clop_clear(void);

void clop_init(void);

void clop_term(void);

cJSON *clop_next_sample(void *e, int *seed, double *weight);

void clop_store_seed(int id, int task_id, int seed);

int clop_pop_seed(int id, int task_id);

void clop_add_outcome(void *e, int seed, int w, int d, int l);

cJSON *clop_get_mean(void *e);

cJSON *clop_get_max(void *e);

#ifdef __cplusplus
}
#endif
#endif
