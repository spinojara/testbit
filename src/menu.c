#include "menu.h"

#include "draw.h"
#include "oldtest.h"
#include "newtest.h"
#include "single.h"
#include "tui.h"

void handle_menu(struct state *st, chtype ch) {
	int changed = 0;
	switch (ch) {
	case KEY_RESIZE:
		draw_resize(st);
		break;
	case 'q':
		st->selected = MENUQUIT;
		break;
	case 'h':
	case KEY_LEFT:
		if (st->selected > 0) {
			st->selected--;
			changed = 1;
		}
		break;
	case 'l':
	case KEY_RIGHT:
		if (st->selected < MENUMAX - 1) {
			st->selected++;
			changed = 1;
		}
		break;
	default:
		switch (st->selected) {
		case MENUNEWTEST:
			handle_newtest(&st->ns, ch);
			break;
		case MENUACTIVE:
			handle_oldtest(&st->as, ch);
			break;
		case MENUDONE:
			handle_oldtest(&st->ds, ch);
			break;
		case MENUCANCELLED:
			handle_oldtest(&st->cs, ch);
			break;
		case MENUFAILED:
			handle_oldtest(&st->fs, ch);
			break;
		}
		break;
	}

	if (changed) {
		switch (st->selected) {
		case MENUNEWTEST:
			draw_newtest(&st->ns);
			break;
		case MENUACTIVE:
			if (st->as.single)
				draw_single(&st->as, 0, 1, 0);
			else
				draw_oldtest(&st->as, 0, 1);
			break;
		case MENUDONE:
			if (st->ds.single)
				draw_single(&st->ds, 0, 1, 0);
			else
				draw_oldtest(&st->ds, 0, 1);
			break;
		case MENUCANCELLED:
			if (st->cs.single)
				draw_single(&st->cs, 0, 1, 0);
			else
				draw_oldtest(&st->cs, 0, 1);
			break;
		case MENUFAILED:
			if (st->fs.single)
				draw_single(&st->fs, 0, 1, 0);
			else
				draw_oldtest(&st->fs, 0, 1);
			break;
		}
		draw_menu(st);
	}
}

void draw_menu(struct state *st) {
	draw_fill(st->win, &cs.bordershadow, 0, 0, getmaxy(st->win), getmaxx(st->win));

	int gap = 2;

	const char *options[] = { "New Test", "Active", "Done", "Cancelled", "Failed" };
	for (int i = 0, x = gap; i < MENUMAX; i++) {
		wattrset(st->win, i == st->selected ? cs.texthl.attr : cs.text.attr);
		mvwprintw(st->win, 1, x, "< %s >", options[i]);
		x += strlen(options[i]) + 4 + gap;
	}

	wrefresh(st->win);
}
