#ifndef TC_H
#define TC_H

struct tc {
	long long moves;
	double maintime;
	double increment;
};

int parsetc(const char *tc, struct tc *dst);

void adjusttc(struct tc *tc, double factor);

void tctostr(char *str, struct tc *tc);

#endif
