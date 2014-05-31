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

#ifndef SKIPPY_CLIENT_H
#define SKIPPY_CLIENT_H

typedef struct {
	Window window;
	int x, y;
	int width, height;
	XRenderPictFormat *format;
} SkippyWindow;

#define SKIPPYWINT_INIT { .window = None }

struct _clientwin_t {
	MainWin *mainwin;

	client_disp_mode_t mode;
	Window wid_client;
	SkippyWindow src;
	bool redirected;
	Pixmap cpixmap;
	pictw_t *pict_filled;
	pictw_t *icon_pict;
	pictw_t *icon_pict_filled;

	SkippyWindow mini;

	Pixmap pixmap;
	Picture origin, destination;
	Damage damage;
	float factor;

	bool focused;

	bool damaged;
	/* XserverRegion repair; */
	
	/* These are virtual positions set by the layout routine */
	int x, y;
};

#define CLIENTWT_INIT { \
	.src = SKIPPYWINT_INIT, \
	.mini = SKIPPYWINT_INIT, \
	.mainwin = NULL \
}

static inline client_disp_mode_t
clientwin_get_disp_mode(session_t *ps, ClientWin *cw) {
	XWindowAttributes wattr = { };
	XGetWindowAttributes(ps->dpy, cw->src.window, &wattr);

	if (!ps->o.showUnmapped && IsViewable != wattr.map_state)
		return CLIDISP_NONE;

	for (client_disp_mode_t *p = ps->o.clientDisplayModes; *p; p++) {
		switch (*p) {
			case CLIDISP_THUMBNAIL_ICON:
				if (IsViewable == wattr.map_state && cw->origin && cw->icon_pict)
					return *p;
				break;
			case CLIDISP_THUMBNAIL:
				if (IsViewable == wattr.map_state && cw->origin) return *p;
				break;
			case CLIDISP_ICON:
				if (cw->icon_pict) return *p;
				break;
			case CLIDISP_FILLED:
			case CLIDISP_NONE:
				return *p;
		}
	}

	return CLIDISP_NONE;
}

static inline void
clientwin_free_res2(session_t *ps, ClientWin *cw) {
	free_pictw(ps, &cw->icon_pict_filled);
	free_pictw(ps, &cw->pict_filled);
}

static inline void
clientwin_free_res(session_t *ps, ClientWin *cw) {
	clientwin_free_res2(ps, cw);
	free_pixmap(ps, &cw->cpixmap);
	free_picture(ps, &cw->origin);
	free_pictw(ps, &cw->icon_pict);
}

int clientwin_validate_func(dlist *, void *);
int clientwin_sort_func(dlist *, dlist *, void *);
ClientWin *clientwin_create(MainWin *, Window);
void clientwin_destroy(ClientWin *, bool destroyed);
void clientwin_move(ClientWin *, float, int, int);
void clientwin_map(ClientWin *);
void clientwin_unmap(ClientWin *);
int clientwin_handle(ClientWin *, XEvent *);
int clientwin_cmp_func(dlist *, void*);
bool clientwin_update(ClientWin *cw);
bool clientwin_update2(ClientWin *cw);
int clientwin_check_group_leader_func(dlist *l, void *data);
void clientwin_render(ClientWin *);
void clientwin_schedule_repair(ClientWin *cw, XRectangle *area);
void clientwin_repair(ClientWin *cw);
void childwin_focus(ClientWin *cw);

#endif /* SKIPPY_CLIENT_H */
