/* Skippy - Seduces Kids Into Perversion
 *
 * Copyright (C) 2004 Hyriand <hyriand@thegraveyard.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "skippy.h"

typedef float (*dist_func)(SkippyWindow *, SkippyWindow *);
typedef int (*match_func)(dlist *, SkippyWindow *);

/**
 * @brief Focus the mini window of a client window.
 */
static void
focus_miniw_dir(ClientWin *cw, match_func match, dist_func func) {
	float diff = 0.0;
	ClientWin *candidate = NULL;
	session_t * const ps = cw->mainwin->ps;

	dlist *candidates = dlist_first(dlist_find_all(cw->mainwin->cod, (dlist_match_func) match, &cw->mini));
	if (!candidates) return;

	foreach_dlist (candidates) {
		ClientWin *win = (ClientWin *) iter->data;
		float distance = func(&cw->mini, &win->mini);
		if (!candidate || distance < diff) {
			candidate = win;
			diff = distance;
		}
	}

	focus_miniw(ps, candidate);
	dlist_free(candidates);
}

#define HALF_H(w) (w->x + (int)w->width / 2)
#define HALF_V(w) (w->y + (int)w->height / 2)
#define SQR(x) pow(x, 2)

#define DISTFUNC(name, d_x, d_y) \
static float name (SkippyWindow *a, SkippyWindow *b) \
{ return sqrt(SQR(d_x) + SQR(d_y)); }

#define QUALFUNC(name, expr) \
static int name(dlist *l, SkippyWindow *b) \
{ SkippyWindow *a = &((ClientWin*)l->data)->mini; return expr; }

#define FOCUSFUNC(name, qual, dist) \
void name(ClientWin *cw) { focus_miniw_dir(cw, qual, dist); }

DISTFUNC(dist_top_bottom, HALF_H(a) - HALF_H(b), a->y - b->y - (int)b->height)
DISTFUNC(dist_bottom_top, HALF_H(a) - HALF_H(b), b->y - a->y - (int)a->height)
DISTFUNC(dist_left_right, HALF_V(a) - HALF_V(b), a->x - b->x - (int)b->width)
DISTFUNC(dist_right_left, HALF_V(a) - HALF_V(b), b->x - a->x - (int)a->width)

QUALFUNC(win_above, a->y + a->height < b->y)
QUALFUNC(win_below, b->y + b->height < a->y)
QUALFUNC(win_left, a->x + a->width < b->x)
QUALFUNC(win_right, b->x + b->width < a->x)

FOCUSFUNC(focus_up, win_above, dist_top_bottom)
FOCUSFUNC(focus_down, win_below, dist_bottom_top)
FOCUSFUNC(focus_left, win_left, dist_left_right)
FOCUSFUNC(focus_right, win_right, dist_right_left)
