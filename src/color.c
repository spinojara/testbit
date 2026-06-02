#include <ncurses.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const int WEIGHT_GRANULARITY = 7;
const int SCORE_GRANULARITY = 34;

void init_colors(void) {
	start_color();
	if (can_change_color() == FALSE) {
		endwin();
		fprintf(stderr, "Can't change color?\n");
		exit(1);
	}
	for (int weight = 0; weight < WEIGHT_GRANULARITY; weight++) {
		for (int score = 0; score < SCORE_GRANULARITY; score++) {
			int index = 16 + weight + score * WEIGHT_GRANULARITY;
			double weightfactor = (double)weight / (WEIGHT_GRANULARITY - 1);
			if (score < SCORE_GRANULARITY / 2) {
				double scorefactor = (double)score / (SCORE_GRANULARITY / 2 - 1);
				if (init_color(index, 1000.0 * weightfactor, 1000.0 * scorefactor * weightfactor, 0.0) == ERR) {
					endwin();
					printf("%f %f\n", scorefactor, weightfactor);
					fprintf(stderr, "color error (%d)\n", index);
					exit(1);
				}
			}
			else {
				double scorefactor = (double)(score - SCORE_GRANULARITY / 2) / (SCORE_GRANULARITY / 2 - 1);
				if (init_color(index, 1000.0 * (1.0 - scorefactor) * weightfactor, 1000.0 * weightfactor, 0.0) == ERR) {
					endwin();
					printf("%f %f\n", scorefactor, weightfactor);
					fprintf(stderr, "color error (%d)\n", index);
					exit(1);
				}
			}
		}
	}

	init_color(255, 1000.0, 0.0, 1000.0);
	for (int index = 16; index < 16 + WEIGHT_GRANULARITY * SCORE_GRANULARITY; index++)
		init_pair(index, 0, index);
	init_pair(255, 0, 255);
}

int get_purple(void) {
	return 255;
}

int get_color(double score, double weight) {
	return 16 + round(weight * (WEIGHT_GRANULARITY - 1)) + WEIGHT_GRANULARITY * round(score * (SCORE_GRANULARITY - 1));
}

int get_color_max_weight(int color) {
	return 16 + WEIGHT_GRANULARITY - 1 + ((color - 16) / WEIGHT_GRANULARITY) * WEIGHT_GRANULARITY;
}
