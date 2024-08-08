#ifndef SOURCE_H
#define SOURCE_H

void kill_parent(void);

int git_clone(char *dtemp, const char *branch, const char *commit);

int git_patch(void);

int make(void);

#endif
