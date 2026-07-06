#ifndef ELO_H
#define ELO_H

double calculate_elo(double penta[5], double *pm);

double loglikelihoodratio(int p[5], double elo0, double elo1);

#endif
