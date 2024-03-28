#include "elo.h"

#include <math.h>

#include "util.h"

#define ALPHA(i) ((double)i / 4)

double sigmoid(double x) {
	return 1.0 / (1.0 + exp(-x * log(10.0) / 400.0));
}

double dsigmoiddx(double x) {
	double e = exp(-x * log(10.0) / 400.0);
	return log(10) / 400.0 * e / ((1 + e) * (1 + e));
}

double sigmoidinv(double y) {
	return -400.0 * log(1.0 / y - 1.0) / log(10.0);
}

int32_t normalize(const int32_t penta[5], double n[5]) {
	int32_t N = 0;
	for (int j = 0; j < 5; j++)
		N += penta[j];

	double sum = 0.0;
	for (int j = 0; j < 5; j++) {
		n[j] = (double)penta[j] / N;
		if (j == 0 || j == 4)
			n[j] = fmax(n[j], eps);

		sum += n[j];
	}

	for (int j = 0; j < 5; j++)
		n[j] /= sum;

	return N;
}

double f_calc(const double mu, const double n[5], double C) {
	double sum = 0.0;
	for (int i = 0; i < 5; i++) {
		double num = (ALPHA(i) - C) * n[i];
		if (num)
			sum += num / (1.0 + (ALPHA(i) - C) * mu);
	}
	return sum;
}

double mu_bisect(const double n[5], double C) {
	double a = -1.0 / (1.0 - C);
	double b = 1.0 / C;
	while (1) {
		double c = (a + b) / 2;
		double f = f_calc(c, n, C);
		if (fabs(f) < eps) {
			return c;
		}
		if (f > 0)
			a = c;
		else
			b = c;
	}
}

double loglikelihood(double mu, double C, const double n[5]) {
	double p, sum = 0.0;
	for (int j = 0; j < 5; j++) {
		p = n[j] / (1.0 + (ALPHA(j) - C) * mu);
		/* Avoid 0 * log(0) which has limit 0 but IEEE says nan. */
		if (n[j])
			sum += n[j] * log(p);
	}

	return sum;
}

double loglikelihoodratio(const int32_t penta[5], double elo0, double elo1) {
	double C0, C1;
	double n[5];
	int32_t N;

	double mu0, mu1;

	if ((N = normalize(penta, n)) <= 0)
		return 0.0 / 0.0;

	double score = 0.0;
	for (int j = 0; j < 5; j++)
		score += ALPHA(j) * n[j];


	C0 = sigmoid(elo0);
	if (C0 >= score) {
		C0 = score;
		mu0 = 0.0;
	}
	else {
		mu0 = mu_bisect(n, C0);
	}
	C1 = sigmoid(elo1);
	if (C1 <= score) {
		C1 = score;
		mu1 = 0.0;
	}
	else {
		mu1 = mu_bisect(n, C1);
	}

	return N * (loglikelihood(mu1, C1, n) - loglikelihood(mu0, C0, n));
}

double elo_calc(const int32_t penta[5], double *pm) {
	double n[5];
	int32_t N;

	if ((N = normalize(penta, n)) <= 0)
		return 0.0 / 0.0;

	double score = 0.0;
	for (int j = 0; j < 5; j++)
		score += ALPHA(j) * n[j];

	score = fclamp(score, eps, 1.0 - eps);
	double elo = sigmoidinv(score);

	if (pm) {
		double sigma = -score * score;
		for (int j = 0; j < 5; j++)
			sigma += ALPHA(j) * ALPHA(j) * n[j];
		sigma = sqrt(sigma);

		double lambda = 1.96;

		double dSdx = dsigmoiddx(elo);

		*pm = lambda * sigma / (sqrt(N) * dSdx);
	}

	return elo;
}
