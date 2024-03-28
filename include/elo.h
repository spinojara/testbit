#ifndef ELO_H
#define ELO_H

#include <stdint.h>

double loglikelihoodratio(const int32_t penta[5], double elo0, double elo1);

double elo_calc(const int32_t penta[5], double *pm);

#endif
