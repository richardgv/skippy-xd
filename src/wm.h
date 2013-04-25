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

extern Atom
	/* Root pixmap / wallpaper atoms */
	_XROOTPMAP_ID,
	ESETROOT_PMAP_ID,

	// Window type atoms
	_NET_WM_WINDOW_TYPE_DESKTOP,
	_NET_WM_WINDOW_TYPE_DOCK,
	_NET_WM_WINDOW_TYPE_NORMAL,
	_NET_WM_WINDOW_TYPE_TOOLTIP;

void wm_get_atoms(session_t *ps);
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

void wm_wid_set_info(session_t *ps, Window wid, const char *name,
		Atom window_type);

#endif /* SKIPPY_WM_H */
