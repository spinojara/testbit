#define _DEFAULT_SOURCE
#include "setup.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>

#include "con.h"
#include "util.h"
#include "sprt.h"
#include "req.h"
#include "source.h"

void setup(SSL *ssl, int type, int cpus, char *syzygy, const char *tc,
		double alpha, double beta, double elo0,
		double elo1, double eloe, int adjudicate,
		const char *branch, const char *commit) {
	
	char dtemp[] = "testbitn-bitbit-XXXXXX";
	int r, fd, error = 0;
	if ((r = git_clone(dtemp, branch, commit))) {
		sendf(ssl, "cc", REQUESTNODEDONE, r == 1 ? TESTERRBRANCH : TESTERRCOMMIT);
		error = 1;
	}
	/* We are now inside dtemp even if an error occured. */
	if ((fd = open("patch", O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		fprintf(stderr, "open patch\n");
		exit(16);
	}

	if (recvf(ssl, "f", fd, -1))
		exit(20);

	if (close(fd)) {
		fprintf(stderr, "error: close patch\n");
		exit(17);
	}

	if (error)
		goto cleanup;

	if (make()) {
		/* This should probably never fail. But if a bad commit
		 * is pushed to the github it can fail.
		 */
		sendf(ssl, "cc", REQUESTNODEDONE, TESTERRMAKE);
		goto cleanup;
	}

	if (rename("bitbit", "bitbitold")) {
		fprintf(stderr, "error: rename\n");
		exit(27);
	}

	if (git_patch()) {
		sendf(ssl, "cc", REQUESTNODEDONE, TESTERRPATCH);
		goto cleanup;
	}

	if (make()) {
		sendf(ssl, "cc", REQUESTNODEDONE, TESTERRMAKE);
		goto cleanup;
	}

	sprt(ssl, type, cpus, syzygy, tc, alpha, beta, elo0, elo1, eloe, adjudicate);

cleanup:
	if (chdir("/tmp")) {
		fprintf(stderr, "error: chdir /tmp\n");
		exit(18);
	}

	if (rmdir_r(dtemp)) {
		fprintf(stderr, "error: rmdir %s\n", dtemp);
		exit(19);
	}
}
