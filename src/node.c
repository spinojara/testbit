#include "node.h"

#include <stdio.h>
#include <stdlib.h>

#include "con.h"
#include "req.h"
#include "setup.h"
#include "cgroup.h"
#include "tc.h"

extern int daemon_mode;

void nodeloop(SSL *ssl, int cpus, char *syzygy) {
	char password[4096], response = RESPONSEPERMISSIONDENIED;
	printf("Enter Passphrase: ");
	if (read_secret(password, sizeof(password), !daemon_mode))
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
		char tc[128], adjusted[128];
		double alpha, beta;
		double elo0, elo1;
		double eloe;
		char branch[128];
		char commit[128];
		char simd[128];

		fprintf(stderr, "waiting...\n");
		if (recvf(ssl, "csDDDDDcsss",
					&type, tc, sizeof(tc),
					&alpha, &beta, &elo0, &elo1,
					&eloe, &adjudicate,
					branch, sizeof(branch),
					commit, sizeof(commit),
					simd, sizeof(simd)))
			exit(6);
		fprintf(stderr, "got test!\n");

		tcadjust(tc, adjusted, 128);

		if (claim_cpus(cpus))
			exit(56);
		setup(ssl, type, cpus, syzygy, adjusted, alpha, beta, elo0, elo1, eloe, adjudicate, branch, commit, simd);
		if (release_cpus())
			exit(57);

		fprintf(stderr, "finished test!\n");
	}
}
