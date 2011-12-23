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

#ifndef SKIPPY_WM_H
#define SKIPPY_WM_H

Atom
	/* Generic atoms */
	XA_WM_STATE,
	WM_CLIENT_LEADER,
	XA_UTF8_STRING,
	
	/* Root pixmap / wallpaper atoms */
	_XROOTPMAP_ID,
	ESETROOT_PMAP_ID,
	
	/* NetWM atoms */
	_NET_SUPPORTING_WM_CHECK,
	_NET_SUPPORTED,
	_NET_NUMBER_OF_DESKTOPS,
	_NET_CLIENT_LIST,
	_NET_CLIENT_LIST_STACKING,
	_NET_CURRENT_DESKTOP,
	_NET_WM_DESKTOP,
	_NET_WM_STATE,
	_NET_WM_STATE_HIDDEN,
	_NET_WM_STATE_SKIP_TASKBAR,
	_NET_WM_STATE_SKIP_PAGER,
	_NET_WM_STATE_FULLSCREEN,
	_NET_WM_STATE_SHADED,
	_NET_WM_STATE_ABOVE,
	_NET_WM_STATE_STICKY,
	_NET_WM_WINDOW_TYPE,
	_NET_WM_WINDOW_TYPE_DESKTOP,
	_NET_WM_WINDOW_TYPE_DOCK,
	_NET_WM_VISIBLE_NAME,
	_NET_WM_NAME,
	
	/* Old gnome atoms */
	_WIN_SUPPORTING_WM_CHECK,
	_WIN_WORKSPACE,
	_WIN_WORKSPACE_COUNT,
	_WIN_PROTOCOLS,
	_WIN_CLIENT_LIST,
	_WIN_STATE,
	_WIN_HINTS;

void wm_get_atoms(Display *dpy);
char wm_check(Display *dpy);
void wm_use_netwm_fullscreen(Bool b);
dlist *wm_get_stack(Display *dpy);
Pixmap wm_get_root_pmap(Display *dpy);
CARD32 wm_get_current_desktop(Display *dpy);
FcChar8 *wm_get_window_title(Display *dpy, Window window, int *length_return);
Window wm_get_group_leader(Display *dpy, Window window);
void wm_set_fullscreen(Display *dpy, Window window, int x, int y, unsigned int width, unsigned int height);
int wm_validate_window(Display *dpy, Window win);
CARD32 wm_get_window_desktop(Display *dpy, Window win);
Window wm_get_focused(Display *dpy);
void wm_ignore_skip_taskbar(Bool b);

#endif /* SKIPPY_WM_H */
