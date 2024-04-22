#include "node.h"

#include <stdio.h>
#include <stdlib.h>

#include "con.h"
#include "req.h"
#include "setup.h"
#include "cgroup.h"

void nodeloop(SSL *ssl, int cpus) {
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
		char adjudicate;
		double maintime, increment;
		double alpha, beta;
		double elo0, elo1;
		double eloe;
		char branch[128];
		char commit[128];

		if (recvf(ssl, "cDDDDDDDcss",
					&type, &maintime, &increment,
					&alpha, &beta, &elo0, &elo1,
					&eloe, &adjudicate,
					branch, sizeof(branch),
					commit, sizeof(commit)))
			exit(6);

		if (claim_cpus(cpus))
			exit(56);
		setup(ssl, type, cpus, maintime, increment, alpha, beta, elo0, elo1, eloe, adjudicate, branch, commit);
		if (release_cpus())
			exit(57);
	}
}
