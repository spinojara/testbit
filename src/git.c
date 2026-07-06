#include "git.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "test.h"

char *remove_binary(const char *patch) {
	char *smallpatch = strdup(patch);
	char *p = smallpatch;

	while ((p = strstr(p, "\nGIT binary patch\n"))) {
		char *end = strstr(p, "\ndiff --git ");
		if (end) {
			memmove(p + strlen("\nGIT binary patch\n"), end, strchr(p, '\0') - end + 1);
			p++;
		}
		else {
			p[strlen("\nGIT binary patch\n")] = 0;
			break;
		}
	}

	return smallpatch;
}

int check_ref_format(const char *branch) {
	if (!branch || !*branch)
		return -1;
	size_t len = strlen(branch);
	char lastchar = branch[len - 1];

	if (*branch == '.' || strstr(branch, "/.") || (len >= 5 && !strcmp(branch + len - 5, ".lock")) || strstr(branch, ".lock/"))
		return 1;

	if (strstr(branch, ".."))
		return 3;

	const char *c = branch;
	for (size_t i = 0; i < len; i++)
		if (c[i] < 040 || c[i] >= 0177 || c[i] == ' ' || c[i] == '~' || c[i] == '^' || c[i] == ':')
			return 4;

	for (size_t i = 0; i < len; i++)
		if (c[i] == '?' || c[i] == '*' || c[i] == '[')
			return 5;

	if (*branch == '/' || lastchar == '/' || strstr(branch, "//"))
		return 6;

	if (lastchar == '.')
		return 7;

	if (strstr(branch, "@{"))
		return 8;

	if (!strcmp(branch, "@"))
		return 9;

	if (strchr(branch, '\\'))
		return 10;

	return 0;
}
