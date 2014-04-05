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
	_NET_WM_STATE_SHADED,
	_NET_ACTIVE_WINDOW,
	_NET_WM_ICON,
	_NET_CURRENT_DESKTOP,

	// Other atoms
	KWM_WIN_ICON;

/// Structure representing Window property value.
typedef struct {
  // All pointers have the same length, right?
  union {
    unsigned char *data8;
    short *data16;
    long *data32;
  };
  unsigned long nitems;
  Atom type;
  int format;
} winprop_t;

void wm_get_atoms(session_t *ps);

bool wm_check_netwm(session_t *ps);
bool wm_check_gnome(session_t *ps);
static inline bool
wm_check(session_t *ps) {
	if (wm_check_netwm(ps)) {
		ps->wmpsn = WMPSN_EWMH;
		printfdf("(): Your WM looks EWMH compliant.");
		return true;
	}
	if (wm_check_gnome(ps)) {
		ps->wmpsn = WMPSN_GNOME;
		printfdf("(): Your WM looks GNOME compliant.");
		return true;
	}
	printfef("(): Your WM is neither EWMH nor GNOME WM compliant. "
			"Troubles ahead.");
	return false;
}

dlist *wm_get_stack(session_t *ps);
Pixmap wm_get_root_pmap(Display *dpy);
long wm_get_current_desktop(session_t *ps);
FcChar8 *wm_get_window_title(session_t *ps, Window wid, int *length_return);
Window wm_get_group_leader(Display *dpy, Window window);
void wm_set_fullscreen(session_t *ps, Window window,
		int x, int y, unsigned width, unsigned height);
bool wm_validate_window(session_t *ps, Window wid);
long wm_get_window_desktop(session_t *ps, Window wid);
Window wm_get_focused(session_t *ps);

char *wm_wid_get_prop_rstr(session_t *ps, Window wid, Atom prop);
char *wm_wid_get_prop_utf8(session_t *ps, Window wid, Atom prop);
bool wm_wid_set_prop_utf8(session_t *ps, Window wid, Atom prop, char *text);
void wm_wid_set_info(session_t *ps, Window wid, const char *name,
		Atom window_type);
void wm_send_clientmsg(session_t *ps, Window twid, Window wid, Atom msg_type,
		int fmt, long event_mask, int len, const unsigned char *data);

static inline void
wm_send_clientmsg_icccm(session_t *ps, Window wid, Atom msg_type,
		int len, const long *data) {
	wm_send_clientmsg(ps, wid, wid, msg_type, 32, NoEventMask, len,
			(const unsigned char *) data);
}

static inline void
wm_close_window_icccm(session_t *ps, Window wid) {
	long data[] = { WM_DELETE_WINDOW, CurrentTime };
	wm_send_clientmsg_icccm(ps, wid, WM_PROTOCOLS,
			CARR_LEN(data), data);
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
			CARR_LEN(data), data);
}

static inline void
wm_activate_window_ewmh(session_t *ps, Window wid) {
	long data[] = { 2, CurrentTime, 0 };
	wm_send_clientmsg_ewmh_root(ps, wid, _NET_ACTIVE_WINDOW,
			CARR_LEN(data), data);
}

static inline void
wm_set_desktop_ewmh(session_t *ps, long desktop) {
	long data[] = { desktop, CurrentTime };
	wm_send_clientmsg_ewmh_root(ps, ps->root, _NET_CURRENT_DESKTOP,
			CARR_LEN(data), data);
}

/**
 * Activate a window using whatever the approach we love.
 */
static inline void
wm_activate_window(session_t *ps, Window wid) {
	if (!wid) return;

	if (ps->o.switchDesktopOnActivate) {
		long tgt = wm_get_window_desktop(ps, wid);
		if (tgt >= 0)
			wm_set_desktop_ewmh(ps, tgt);
	}
	// Order is important, to avoid "intelligent" WMs fixing our focus stealing
	wm_activate_window_ewmh(ps, wid);
	XSetInputFocus(ps->dpy, wid, RevertToParent, CurrentTime);
}

Window wm_find_frame(session_t *ps, Window wid);

/**
 * Determine if a window has a specific property.
 *
 * @param ps current session
 * @param w window to check
 * @param atom atom of property to check
 * @return 1 if it has the attribute, 0 otherwise
 */
static inline bool
wid_has_prop(const session_t *ps, Window w, Atom atom) {
  Atom type = None;
  int format;
  unsigned long nitems, after;
  unsigned char *data;

  if (Success == XGetWindowProperty(ps->dpy, w, atom, 0, 0, False,
        AnyPropertyType, &type, &format, &nitems, &after, &data)) {
    sxfree(data);
    if (type) return true;
  }

  return false;
}

winprop_t
wid_get_prop_adv(const session_t *ps, Window w, Atom atom, long offset,
    long length, Atom rtype, int rformat);

/**
 * Wrapper of wid_get_prop_adv().
 */
static inline winprop_t
wid_get_prop(const session_t *ps, Window wid, Atom atom, long length,
    Atom rtype, int rformat) {
  return wid_get_prop_adv(ps, wid, atom, 0L, length, rtype, rformat);
}

/**
 * Get the numeric property value from a win_prop_t.
 */
static inline long
winprop_get_int(const winprop_t *pprop) {
  long tgt = 0;

  if (!pprop->nitems)
    return 0;

  switch (pprop->format) {
    case 8:   tgt = *(pprop->data8);    break;
    case 16:  tgt = *(pprop->data16);   break;
    case 32:  tgt = *(pprop->data32);   break;
    default:  assert(0);
              break;
  }

  return tgt;
}

bool
wid_get_text_prop(session_t *ps, Window wid, Atom prop,
    char ***pstrlst, int *pnstr);

/**
 * Free a <code>winprop_t</code>.
 *
 * @param pprop pointer to the <code>winprop_t</code> to free.
 */
static inline void
free_winprop(winprop_t *pprop) {
  // Empty the whole structure to avoid possible issues
  spxfree(&pprop->data8);
  pprop->nitems = 0;
}

#endif /* SKIPPY_WM_H */
