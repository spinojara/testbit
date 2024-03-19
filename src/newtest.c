#include "newtest.h"

void newtest_key(chtype ch, struct windows *wins, struct menustate *ms) {
	switch (ch) {
	case 'j':
	case KEY_DOWN:
		if (ms->topmenu == WINNEWTEST) {
			if (ms->nts.newtest < NEWTESTELOE) {
				highlight_prompt(&ms->nts.prompt[ms->nts.newtest], 0);
				ms->nts.newtest++;
				highlight_prompt(&ms->nts.prompt[ms->nts.newtest], 1);
				wrefresh(wins->win[WINNEWTEST]);
			}
		}
		break;
	case 'k':
	case KEY_UP:
		if (ms->topmenu == WINNEWTEST) {
			if (ms->nts.newtest > NEWTESTTIME) {
				highlight_prompt(&ms->nts.prompt[ms->nts.newtest], 0);
				ms->nts.newtest--;
				highlight_prompt(&ms->nts.prompt[ms->nts.newtest], 1);
				wrefresh(wins->win[WINNEWTEST]);
			}
		}
		break;
	}
}
