#include "node.h"

#include <stdio.h>
#include <stdlib.h>

#include "con.h"
#include "req.h"
#include "setup.h"

void nodeloop(SSL *ssl, int nthreads) {
	char password[128], response = RESPONSEPERMISSIONDENIED;
	printf("Enter Passphrase: ");
	if (read_secret(password, sizeof(password)))
		exit(4);

	sendf(ssl, "cs", REQUESTPRIVILEGE, password);
	recvf(ssl, "c", &response);
	if (response != RESPONSEOK) {
		fprintf(stderr, "Permission denied.\n");
		exit(5);
	}

	while (1) {
		char type;
		double maintime, increment;
		double alpha, beta;
		double elo0, elo1;
		double eloe;
		uint32_t games;
		char *branch = NULL;
		char *commit = NULL;

		if (recvf(ssl, "cDDDDDDDLss",
					&type, &maintime, &increment,
					&alpha, &beta, &elo0, &elo1,
					&eloe, &games, &branch, &commit))
			exit(6);

		setup(ssl, type, games, nthreads, maintime, increment, alpha, beta, elo0, elo1, eloe, branch, commit);

		free(branch);
		free(commit);
	}
}
