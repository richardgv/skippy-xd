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

	// ICCWM atoms
	WM_PROTOCOLS,
	WM_DELETE_WINDOW,

	// Window type atoms
	_NET_WM_WINDOW_TYPE_DESKTOP,
	_NET_WM_WINDOW_TYPE_DOCK,
	_NET_WM_WINDOW_TYPE_NORMAL,
	_NET_WM_WINDOW_TYPE_TOOLTIP,

	// EWMH atoms
	_NET_CLOSE_WINDOW,
	_NET_WM_STATE,
	_NET_WM_STATE_SHADED;

void wm_get_atoms(session_t *ps);
char wm_check(Display *dpy);
void wm_use_netwm_fullscreen(Bool b);
dlist *wm_get_stack(Display *dpy);
Pixmap wm_get_root_pmap(Display *dpy);
CARD32 wm_get_current_desktop(Display *dpy);
FcChar8 *wm_get_window_title(session_t *ps, Window wid, int *length_return);
Window wm_get_group_leader(Display *dpy, Window window);
void wm_set_fullscreen(Display *dpy, Window window, int x, int y, unsigned int width, unsigned int height);
int wm_validate_window(Display *dpy, Window win);
CARD32 wm_get_window_desktop(Display *dpy, Window win);
Window wm_get_focused(Display *dpy);
void wm_ignore_skip_taskbar(Bool b);

char *wm_wid_get_prop_rstr(session_t *ps, Window wid, Atom prop);
char *wm_wid_get_prop_utf8(session_t *ps, Window wid, Atom prop);
void wm_wid_set_info(session_t *ps, Window wid, const char *name,
		Atom window_type);
void wm_send_clientmsg(session_t *ps, Window twid, Window wid, Atom msg_type,
		int fmt, long event_mask, int len, const unsigned char *data);

static inline void
wm_send_clientmsg_iccwm(session_t *ps, Window wid, Atom msg_type,
		int len, const long *data) {
	wm_send_clientmsg(ps, wid, wid, msg_type, 32, NoEventMask, len,
			(const unsigned char *) data);
}

static inline void
wm_close_window_iccwm(session_t *ps, Window wid) {
	long data[] = { WM_DELETE_WINDOW, CurrentTime };
	wm_send_clientmsg_iccwm(ps, wid, WM_PROTOCOLS,
			sizeof(data) / sizeof(data[0]), data);
}

static inline void
wm_send_clientmsg_ewmh_root(session_t *ps, Window wid, Atom msg_type,
		int len, const long *data) {
	wm_send_clientmsg(ps, ps->root, wid, msg_type, 32,
			SubstructureNotifyMask | SubstructureRedirectMask, len,
			(const unsigned char *) data);
}

static inline void
wm_close_window_ewmh(session_t *ps, Window wid) {
	long data[] = { CurrentTime, 2 };
	wm_send_clientmsg_ewmh_root(ps, wid, _NET_CLOSE_WINDOW,
			sizeof(data) / sizeof(data[0]), data);
}

static inline void
wm_shade_window_ewmh(session_t *ps, Window wid) {
	long data[] = { 2, _NET_WM_STATE_SHADED };
	wm_send_clientmsg_ewmh_root(ps, wid, _NET_WM_STATE,
			sizeof(data) / sizeof(data[0]), data);
}

#endif /* SKIPPY_WM_H */
