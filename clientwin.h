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

struct _SkippyWindow {
	Window window;
	int x, y;
	unsigned int width, height;
	XRenderPictFormat *format;
};

typedef struct _SkippyWindow SkippyWindow;

struct _MainWin;
struct _ClientWin{
	struct _MainWin *mainwin;
	
	SkippyWindow client;
	SkippyWindow mini;
	
	Pixmap pixmap;
	Picture origin, destination;
	Damage damage;
	float factor;
	
	int focused;
	
	Bool damaged;
	/* XserverRegion repair; */
	
	/* These are virtual positions set by the layout routine */
	int x, y;
};
typedef struct _ClientWin ClientWin;

int clientwin_validate_func(dlist *, void *);
int clientwin_sort_func(dlist *, dlist *, void *);
ClientWin *clientwin_create(struct _MainWin *, Window);
void clientwin_destroy(ClientWin *, Bool parentDestroyed);
void clientwin_move(ClientWin *, float, int, int);
void clientwin_map(ClientWin *);
void clientwin_unmap(ClientWin *);
int clientwin_handle(ClientWin *, XEvent *);
int clientwin_cmp_func(dlist *, void*);
void clientwin_update(ClientWin *cw);
int clientwin_check_group_leader_func(dlist *l, void *data);
void clientwin_render(ClientWin *);
void clientwin_schedule_repair(ClientWin *cw, XRectangle *area);
void clientwin_repair(ClientWin *cw);

#endif /* SKIPPY_CLIENT_H */
