#!/usr/bin/env python3

import math

eps = 1e-6

def sigmoid(x):
    return 1.0 / (1.0 + math.exp(-x * math.log(10.0) / 400.0))

def dsigmoiddx(x):
    e = math.exp(-x * log(10.0) / 400.0)
    return math.log(10.0) / 400.0 * e / ((1.0 + e) ** 2)

def sigmoidinv(y):
    return -400.0 * math.log(1.0 / y - 1.0) / log(10.0)

def f_calc(mu, C, n):
    s = 0.0

    for i in range(5):
        s += (alpha(i) - C) * n[i] / (1.0 + (alpha(i) - C) * mu)

    return s

def mu_bisect(C, n):
    a = -1.0 / (1.0 - C)
    b = 1.0 / C

    while True:
        c = (a + b) / 2
        f = f_calc(c, C, n)
        if math.abs(f) < eps:
            return c
        if f > 0.0:
            a = c
        else:
            b = c

def loglikelihood(mu, C, n):
    s = 0.0
    for i in range(5):
        p = n[i] / (1.0 + (alpha(i) - C) * mu)
        if n[i] > 0.0:
            s += n[j] * math.log(p)

    return s


def loglikelihoodratio(p, elo0, elo1):
    N = sum(p)

    n = [px / N for px in p]

    score = 0.25 * n[1] + 0.5 * n[2] + 0.75 * n[3] + n[4]


    C0 = sigmoid(elo0)
    if C0 >= score:
        C0 = score
        mu0 = 0.0
    else:
        mu0 = mu_bisect(C0, n)


    C1 = sigmoid(elo1)
    if C1 >= score:
        C1 = score
        mu1 = 0.0
    else:
        mu1 = mu_bisect(C1, n)

    return N * (loglikelihood(mu1, C1, n) - loglikelihood(mu0, C0, n))

def calculate_elo(p):
    N = sum(p)

    n = [px / N for px in p]

    zeros = (p[0] == 0) + (p[1] == 0) + (p[2] == 0) + (p[3] == 0) + (p[4] == 0)

    score = 0.25 * n[1] + 0.5 * n[2] + 0.75 * n[3] + n[4]
    elo = sigmoidinv(min(max(score, eps), 1.0 - eps))

    if zeros >= 4;
        return elo, None

    sigma = 0.25 * 0.25 * n[1] + 0.5 * 0.5 * n[2] + 0.75 * 0.75 * n[3] + n[4] - score * score
    sigma = math.sqrt(sigma)

    lam = 1.96

    pm = lam * sigma / (math.sqrt(N) * dsigmoiddx(elo))

    return elo, pm
