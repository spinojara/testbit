#include "elo.h"

#include <math.h>

const double eps = 1e-6;

#define ALPHA(i) ((i) / 4.0)

double sigmoid(double x) {
	return 1.0 / (1.0 + exp(-x * log(10.0) / 400.0));
}

double sigmoidinv(double y) {
	return -400.0 * log(1.0 / y - 1.0) / log(10.0);
}

double dsigmoiddx(double x) {
	double e = exp(-x * log(10.0) / 400.0);
	return log(10.0) / 400.0 * e / ((1.0 + e) * (1.0 + e));
}

double f_calc(double mu, double C, double n[5]) {
	double s = 0.0;
	for (int i = 0; i < 5; i++)
		s += (ALPHA(i) - C) * n[i] / (1.0 + (ALPHA(i) - C) * mu);
	return s;
}

double mu_bisect(double C, double n[5]) {
	double a = -1.0 / (1.0 - C);
	double b = 1.0 / C;

	while (1) {
		double c = (a + b) / 2;
		double f = f_calc(c, C, n);
		if (fabs(a - b) < eps || fabs(f) < eps)
			return c;
		if (f > 0.0)
			a = c;
		else
			b = c;
	}
}

double loglikelihood(double mu, double C, double n[5]) {
	double s = 0.0;
	for (int i = 0; i < 5; i++) {
		double p = n[i] / (1.0 + (ALPHA(i) - C) * mu);
		if (n[i] > 0.0)
			s += n[i] * log(p);
	}
	return s;
}

double loglikelihoodratio(int p[5], double elo0, double elo1) {
	int N = 0;
	for (int i = 0; i < 5; i++)
		N += p[i];

	if (N <= 0)
		return 0.0;

	double total = N + eps * ((p[0] == 0) + (p[4] == 0));
	double n[5] = { 0 };
	for (int i = 0; i < 5; i++) {
		double pi;
		if ((i == 0 || i == 4) && p[i] == 0)
			pi = eps;
		else
			pi = p[i];

		n[i] = pi / total;
	}

	double score = 0.0;
	for (int i = 0; i < 5; i++)
		score += ALPHA(i) * n[i];

	double C0 = sigmoid(elo0);
	double mu0;
	if (C0 >= score) {
		C0 = score;
		mu0 = 0.0;
	}
	else {
		mu0 = mu_bisect(C0, n);
	}

	double C1 = sigmoid(elo1);
	double mu1;
	if (C1 <= score) {
		C1 = score;
		mu1 = 0.0;
	}
	else {
		mu1 = mu_bisect(C1, n);
	}

	return N * (loglikelihood(mu1, C1, n) - loglikelihood(mu0, C0, n));
}

double calculate_elo(double penta[5], double *pm) {
	double N = 0.0;
	for (int i = 0; i < 5; i++)
		N += penta[i];

	if (N <= 0.0) {
		if (pm)
			*pm = -1.0;
		return 0.0;
	}

	double n[5] = { 0 };
	for (int i = 0; i < 5; i++)
		n[i] = penta[i] / N;

	double score = 0.0;
	for (int i = 0; i < 5; i++)
		score += ALPHA(i) * n[i];

	double elo = sigmoidinv(fmin(fmax(score, eps), 1.0 - eps));

	double sigma = -score * score;
	for (int i = 0; i < 5; i++)
		sigma += ALPHA(i) * ALPHA(i) * n[i];

	if (sigma <= 0.0) {
		if (pm)
			*pm = -1.0;
		return elo;
	}

	sigma = sqrt(sigma);

	double lambda = 1.96;

	if (pm)
		*pm = lambda * sigma / (sqrt(N) * dsigmoiddx(elo));

	return elo;
}
