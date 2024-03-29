#ifndef ACTIVE_H
#define ACTIVE_H

#include "state.h"

void draw_active(struct oldteststate *os);

char *etastr(char *buf, struct test *test, double A, double B);

#endif
