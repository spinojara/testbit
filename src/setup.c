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
#include "cgroup.h"

void setup(SSL *ssl, int type, int ncpus, char *syzygy, const char *tc,
		double alpha, double beta, double elo0,
		double elo1, double eloe, int adjudicate,
		const char *branch, const char *commit, const char *simd) {
	
	char dtemp[] = "testbitn-bitbit-XXXXXX";
	int r, fd, nnuefd, error = 0;
	if ((r = git_clone(dtemp, branch, commit))) {
		sendf(ssl, "cc", REQUESTNODEDONE, r == 1 ? TESTERRBRANCH : TESTERRCOMMIT);
		error = 1;
	}
	/* We are now inside dtemp even if an error occured. */
	if ((fd = open("patch", O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		fprintf(stderr, "open patch\n");
		exit(16);
	}

	if ((nnuefd = open("nnue.nnue", O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		fprintf(stderr, "open nnue\n");
		exit(16);
	}

	if (recvf(ssl, "ff", fd, -1, nnuefd, -1))
		exit(20);

	if (close(fd)) {
		fprintf(stderr, "error: close patch\n");
		exit(17);
	}

	int hasnnue = lseek(nnuefd, 0, SEEK_CUR);

	if (close(nnuefd)) {
		fprintf(stderr, "error: close nnue\n");
		exit(17);
	}


	if (error)
		goto cleanup;

	if (make(simd, "etc/current.nnue")) {
		/* This should probably never fail. But if a bad commit
		 * is pushed to github it can fail.
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

	if (make(simd, hasnnue ? "nnue.nnue" : "etc/current.nnue")) {
		sendf(ssl, "cc", REQUESTNODEDONE, TESTERRMAKE);
		goto cleanup;
	}

	int *cpus = malloc(ncpus * sizeof(*cpus));

	if (claim_cpus(ncpus, cpus))
		exit(56);
	sprt(ssl, type, ncpus, cpus, syzygy, tc, alpha, beta, elo0, elo1, eloe, adjudicate);
	if (release_cpus())
		exit(57);
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
