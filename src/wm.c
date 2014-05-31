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

Atom
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

static Atom
	/* Generic atoms */
	XA_WM_STATE,
	WM_CLIENT_LEADER,
	XA_UTF8_STRING,

	/* EWMH atoms */
	_NET_SUPPORTING_WM_CHECK,
	_NET_SUPPORTED,
	_NET_NUMBER_OF_DESKTOPS,
	_NET_CLIENT_LIST,
	_NET_CLIENT_LIST_STACKING,
	_NET_WM_DESKTOP,
	_NET_WM_STATE_HIDDEN,
	_NET_WM_STATE_SKIP_TASKBAR,
	_NET_WM_STATE_SKIP_PAGER,
	_NET_WM_STATE_FULLSCREEN,
	_NET_WM_STATE_ABOVE,
	_NET_WM_STATE_STICKY,
	_NET_WM_WINDOW_TYPE,
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

/* From WindowMaker's gnome.c */
#define WIN_HINTS_SKIP_FOCUS      (1<<0) /*"alt-tab" skips this win*/
#define WIN_HINTS_SKIP_WINLIST    (1<<1) /*do not show in window list*/
#define WIN_HINTS_SKIP_TASKBAR    (1<<2) /*do not show on taskbar*/
#define WIN_HINTS_GROUP_TRANSIENT (1<<3) /*Reserved - definition is unclear*/
#define WIN_HINTS_FOCUS_ON_CLICK  (1<<4) /*app only accepts focus if clicked*/
#define WIN_HINTS_DO_NOT_COVER    (1<<5) /* attempt to not cover this window */


#define WIN_STATE_STICKY          (1<<0) /*everyone knows sticky*/
#define WIN_STATE_MINIMIZED       (1<<1) /*Reserved - definition is unclear*/
#define WIN_STATE_MAXIMIZED_VERT  (1<<2) /*window in maximized V state*/
#define WIN_STATE_MAXIMIZED_HORIZ (1<<3) /*window in maximized H state*/
#define WIN_STATE_HIDDEN          (1<<4) /*not on taskbar but window visible*/
#define WIN_STATE_SHADED          (1<<5) /*shaded (MacOS / Afterstep style)*/
/* these are bogus states defined in "the spec" */
#define WIN_STATE_HID_WORKSPACE   (1<<6) /*not on current desktop*/
#define WIN_STATE_HID_TRANSIENT   (1<<7) /*owner of transient is hidden*/
#define WIN_STATE_FIXED_POSITION  (1<<8) /*window is fixed in position even*/
#define WIN_STATE_ARRANGE_IGNORE  (1<<9) /*ignore for auto arranging*/

/**
 * @brief Wrapper of XInternAtom().
 */
static inline Atom
get_atom(session_t *ps, const char *name) {
	return XInternAtom(ps->dpy, name, False);
}

/**
 * @brief Initialize X atoms.
 */
void
wm_get_atoms(session_t *ps) {
#define T_GETATOM(name) name = get_atom(ps, # name)
	XA_WM_STATE = get_atom(ps, "WM_STATE");
	T_GETATOM(WM_CLIENT_LEADER);
	XA_UTF8_STRING = get_atom(ps, "UTF8_STRING");
	
	T_GETATOM(_XROOTPMAP_ID);
	T_GETATOM(ESETROOT_PMAP_ID);
	
	T_GETATOM(WM_PROTOCOLS),
	T_GETATOM(WM_DELETE_WINDOW),

	T_GETATOM(_NET_SUPPORTING_WM_CHECK);
	T_GETATOM(_NET_SUPPORTED);
	T_GETATOM(_NET_NUMBER_OF_DESKTOPS);
	T_GETATOM(_NET_CLIENT_LIST);
	T_GETATOM(_NET_CLIENT_LIST_STACKING);
	T_GETATOM(_NET_CURRENT_DESKTOP);
	T_GETATOM(_NET_WM_DESKTOP);
	T_GETATOM(_NET_WM_STATE);
	T_GETATOM(_NET_WM_STATE_HIDDEN);
	T_GETATOM(_NET_WM_STATE_SKIP_TASKBAR);
	T_GETATOM(_NET_WM_STATE_SKIP_PAGER);
	T_GETATOM(_NET_WM_STATE_FULLSCREEN);
	T_GETATOM(_NET_WM_STATE_ABOVE);
	T_GETATOM(_NET_WM_STATE_STICKY);
	T_GETATOM(_NET_WM_WINDOW_TYPE);
	T_GETATOM(_NET_WM_WINDOW_TYPE_DESKTOP);
	T_GETATOM(_NET_WM_WINDOW_TYPE_DOCK);
	T_GETATOM(_NET_WM_WINDOW_TYPE_NORMAL);
	T_GETATOM(_NET_WM_WINDOW_TYPE_TOOLTIP);
	T_GETATOM(_NET_WM_VISIBLE_NAME);
	T_GETATOM(_NET_WM_NAME);
	T_GETATOM(_NET_ACTIVE_WINDOW);
	T_GETATOM(_NET_CLOSE_WINDOW);
	T_GETATOM(_NET_WM_STATE_SHADED);
	T_GETATOM(_NET_WM_ICON);

	T_GETATOM(KWM_WIN_ICON);

	T_GETATOM(_WIN_SUPPORTING_WM_CHECK);
	T_GETATOM(_WIN_WORKSPACE);
	T_GETATOM(_WIN_WORKSPACE_COUNT);
	T_GETATOM(_WIN_PROTOCOLS);
	T_GETATOM(_WIN_CLIENT_LIST);
	T_GETATOM(_WIN_STATE);
	T_GETATOM(_WIN_HINTS);
#undef T_GETATOM
}

bool
wm_check_netwm(session_t *ps) {
	Display *dpy = ps->dpy;

	Window wm_check = None;
	unsigned char *data = NULL;

	int real_format = 0;
	Atom real_type = None;
	unsigned long items_read = 0, items_left = 0;

	bool success = (Success == XGetWindowProperty(dpy, ps->root,
				_NET_SUPPORTING_WM_CHECK,
				0L, 1L, False, XA_WINDOW, &real_type, &real_format,
				&items_read, &items_left, &data)
			&& items_read && data && 32 == real_format);
	if (success)
		wm_check = *((long *)data);
	spxfree(&data);
	if (!success) return false;
	
	success = (Success == XGetWindowProperty(dpy, wm_check,
				_NET_SUPPORTING_WM_CHECK,
				0L, 1L, False, XA_WINDOW, &real_type, &real_format,
				&items_read, &items_left, &data)
			&& items_read && data && 32 == real_format
			&& wm_check == *((long *)data));
	spxfree(&data);
	if (!success) return false;
	
	success = (Success == XGetWindowProperty(dpy, ps->root, _NET_SUPPORTED,
	                  0L, 8192L, False, XA_ATOM, &real_type, &real_format,
	                  &items_read, &items_left, &data)
			&& items_read && data && 32 == real_format);
	if (!success)
		items_read = 0;

	long *ldata = (long *) data;
	int req = 0;
	for (int i = 0; i < items_read; i++) {
		if (_NET_NUMBER_OF_DESKTOPS == ldata[i])
			req |= 1;
		else if (_NET_CURRENT_DESKTOP == ldata[i])
			req |= 2;
		else if (_NET_WM_STATE == ldata[i])
			req |= 4;
		else if (_NET_CLIENT_LIST == ldata[i])
			req |= 8;
		else if (_NET_CLIENT_LIST_STACKING == ldata[i]) {
			req |= 8;
			_NET_CLIENT_LIST = _NET_CLIENT_LIST_STACKING;
		}
		else if (_NET_WM_STATE_FULLSCREEN == ldata[i])
			ps->has_ewmh_fullscreen = true;
	}

	spxfree(&data);

	return ((req & 15) == 15);
}

bool
wm_check_gnome(session_t *ps) {
	Display *dpy = ps->dpy;
	unsigned char *data = NULL;
	
	Window wm_check = None;
	int real_format = 0;
	Atom real_type = None;
	unsigned long items_read = 0, items_left = 0;
	
	// Make sure _WIN_SUPPORTING_WM_CHECK is present on root window
	bool success = (Success ==
			XGetWindowProperty(dpy, ps->root, _WIN_SUPPORTING_WM_CHECK,
				0L, 1L, False, XA_CARDINAL, &real_type, &real_format,
				&items_read, &items_left, &data)
			&& items_read && data && 32 == real_format);
	
	if (success)
		wm_check = ((long *)data)[0];
	spxfree(&data);
	if (!success)
		return success;

	/*
	// Make sure _WIN_SUPPORTING_WM_CHECK is present on the WM check window
	// as well
	success = (Success == XGetWindowProperty(dpy, wm_check, _WIN_SUPPORTING_WM_CHECK,
				0L, 1L, False, XA_CARDINAL, &real_type, &real_format,
				&items_read, &items_left, &data) && items_real
			&& wm_check == *((long *) data));
	spxfree(&data);
	if (!success)
		return success;
	*/

	// Check supported protocols
	success = (Success == XGetWindowProperty(dpy, ps->root, _WIN_PROTOCOLS,
				0L, 8192L, False, XA_ATOM, &real_type, &real_format,
				&items_read, &items_left, &data)
			&& items_read && data && 32 == real_format);
	if (!success)
		items_read = 0;

	long *ldata = (long *) data;
	int req = 0;
	for (int i = 0; i < items_read; i++) {
		if (_WIN_WORKSPACE == ldata[i])
			req |= 1;
		else if (_WIN_WORKSPACE_COUNT == ldata[i])
			req |= 2;
		else if (_WIN_STATE == ldata[i])
			req |= 4;
		else if (_WIN_CLIENT_LIST == ldata[i])
			req |= 8;
	}
	spxfree(&data);

	return ((req & 15) == 15);
}

/**
 * @brief Find the client window under a specific frame window.
 *
 * Using a depth-first search.
 */
static Window
wm_find_client(session_t *ps, Window wid) {
	dlist *stack = dlist_add(NULL, (void *) wid);
	Window result = None;
	while (stack) {
		dlist *stack2 = NULL;
		foreach_dlist (stack) {
			Window cur = (Window) iter->data;
			if (wid_has_prop(ps, cur, XA_WM_STATE)) {
				result = cur;
				break;
			}
			Window *children = NULL;
			unsigned nchildren = 0;
			Window rroot = None, rparent = None;
			if (XQueryTree(ps->dpy, cur, &rroot, &rparent,
						&children, &nchildren) && nchildren && children)
				for (int i = 0; i < nchildren; ++i)
					stack2 = dlist_add(stack2, (void *) children[i]);
			sxfree(children);
		}
		dlist_free(stack);
		if (result) {
			free(stack2);
			break;
		}
		else {
			stack = stack2;
		}
	}

	return result;
}

static inline dlist *
wm_get_stack_fromprop(session_t *ps, Window root, Atom a) {
	dlist *l = NULL;
	unsigned char *data = NULL;
	int real_format = 0;
	Atom real_type = None;
	unsigned long items_read = 0, items_left = 0;
	int status = XGetWindowProperty(ps->dpy, root, a,
			0L, 8192L, False, XA_WINDOW, &real_type, &real_format,
			&items_read, &items_left, &data);
	if (Success == status && 32 == real_format && data)
		for (int i = 0; i < items_read; i++) {
			l = dlist_add(l, (void *) ((long *) data)[i]);
		}

	sxfree(data);
	return l;
}

static inline dlist *
wm_get_stack_sub(session_t *ps, Window root) {
	dlist *l = NULL;

	if (!(ps->o.acceptOvRedir || ps->o.acceptWMWin)) {
		// EWMH
		l = wm_get_stack_fromprop(ps, root, _NET_CLIENT_LIST);
		if (l) {
			printfdf("(): Retrieved window stack from _NET_CLIENT_LIST.");
			return l;
		}

		// GNOME WM
		l = wm_get_stack_fromprop(ps, root, _WIN_CLIENT_LIST);
		if (l) {
			printfdf("(): Retrieved window stack from _WIN_CLIENT_LIST.");
			return l;
		}
	}

	// Stupid method
	{
		Window *children = NULL;
		unsigned nchildren = 0;
		Window rroot = None, rparent = None;
		if (XQueryTree(ps->dpy, root, &rroot, &rparent,
					&children, &nchildren) && nchildren && children) {
			// Fluxbox sets override-redirect on its frame windows,
			// so we can't skip override-redirect windows.
			for (int i = 0; i < nchildren; ++i) {
				Window wid = children[i];
				Window client = wm_find_client(ps, wid);
				if (!client && (ps->o.acceptOvRedir || ps->o.acceptWMWin)) {
					XWindowAttributes attr = { };
					if (XGetWindowAttributes(ps->dpy, wid, &attr)
						&& ((attr.override_redirect && ps->o.acceptOvRedir)
								|| (!attr.override_redirect && ps->o.acceptWMWin))) {
						client = wid;
					}
				}
				if (client)
					l = dlist_add(l, (void *) client);
			}
		}
		sxfree(children);
		printfdf("(): Retrieved window stack by querying all children.");
	}

	return l;
}

dlist *
wm_get_stack(session_t *ps) {
	if (ps->o.includeAllScreens) {
		dlist *l = NULL;
		for (int i = 0; i < ScreenCount(ps->dpy); ++i)
			l = dlist_join(l, wm_get_stack_sub(ps, RootWindow(ps->dpy, i)));
		return l;
	}
	else return wm_get_stack_sub(ps, ps->root);
}

Pixmap
wm_get_root_pmap(Display *dpy)
{
	Pixmap rootpmap = None;
	unsigned char *data;
	int status, real_format;
	Atom real_type;
	unsigned long items_read, items_left;
	
	status = XGetWindowProperty(dpy, DefaultRootWindow(dpy), _XROOTPMAP_ID,
	                  0L, 1L, False, XA_PIXMAP, &real_type, &real_format,
	                  &items_read, &items_left, &data);
	if(status != Success) {
		status = XGetWindowProperty(dpy, DefaultRootWindow(dpy), ESETROOT_PMAP_ID,
		                  0L, 1L, False, XA_PIXMAP, &real_type, &real_format,
		                  &items_read, &items_left, &data);
		if(status != Success)
			return None;
	}
	
	if(items_read)
		rootpmap = ((Pixmap*)data)[0];
	
	XFree(data);
	
	return rootpmap;
}

long
wm_get_current_desktop(session_t *ps) {
	winprop_t prop = { };
	long desktop = 0;

	prop = wid_get_prop(ps, ps->root, _NET_CURRENT_DESKTOP,
			1, XA_CARDINAL, 0);
	desktop = winprop_get_int(&prop);
	free_winprop(&prop);
	if (!desktop) {
		prop = wid_get_prop(ps, ps->root, _WIN_WORKSPACE, 1, XA_CARDINAL, 0);
		desktop = winprop_get_int(&prop);
		free_winprop(&prop);
	}

	return desktop;
}

/**
 * @brief Retrieve the title of a window.
 *
 * Must be a UTF-8 string.
 */
FcChar8 *
wm_get_window_title(session_t *ps, Window wid, int *length_return) {
	char *ret = NULL;

	// wm_wid_get_prop_utf8() is certainly more appropriate, yet
	// I found Xlib failing to interpret CJK characters in WM_NAME with
	// type STRING, so we have to keep using the old way here.
	ret = wm_wid_get_prop_rstr(ps, wid, _NET_WM_VISIBLE_NAME);
	if (!ret)
		ret = wm_wid_get_prop_rstr(ps, wid, _NET_WM_NAME);
	if (!ret)
		ret = wm_wid_get_prop_rstr(ps, wid, XA_WM_NAME);
	if (ret && length_return)
		*length_return = strlen(ret);

	return (FcChar8 *) ret;
}

Window
wm_get_group_leader(Display *dpy, Window window)
{
	unsigned char *data;
	int status, real_format;
	Atom real_type;
	unsigned long items_read, items_left;
	Window leader = None;
	
	status = XGetWindowProperty(dpy, window, WM_CLIENT_LEADER,
	                  0, 1, False, XA_WINDOW, &real_type, &real_format,
	                  &items_read, &items_left, &data);
	
	if(status != Success)
	{
		XWMHints *hints = XGetWMHints(dpy, window);
		if(! hints)
			return None;
		
		if(hints->flags & WindowGroupHint)
			leader = hints->window_group;
		
		return leader;
	}
	
	if(items_read)
		leader = ((Window*)data)[0];
	
	XFree(data);
	
	return leader;
}

void
wm_set_fullscreen(session_t *ps, Window window,
		int x, int y, unsigned width, unsigned height) {
	Display *dpy = ps->dpy;
	if (ps->o.useNetWMFullscreen && ps->has_ewmh_fullscreen) {
		Atom props[] = {
			_NET_WM_STATE_FULLSCREEN,
			_NET_WM_STATE_SKIP_TASKBAR,
			_NET_WM_STATE_SKIP_PAGER,
			_NET_WM_STATE_ABOVE,
			_NET_WM_STATE_STICKY,
			0,
		};
		long desktop = -1L;
		
		XChangeProperty(dpy, window, _NET_WM_STATE, XA_ATOM, 32,
				PropModeReplace, (unsigned char *) props,
				sizeof(props) / sizeof(props[0]) - 1);
		XChangeProperty(dpy, window, _NET_WM_DESKTOP, XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *) &desktop, 1);
	}
	else {
		XSetWindowAttributes wattr;
		wattr.override_redirect = True;
		XChangeWindowAttributes(dpy, window, CWOverrideRedirect, &wattr);
		XMoveResizeWindow(dpy, window, x, y, width, height);
	}
}

bool
wm_validate_window(session_t *ps, Window wid) {
	winprop_t prop = { };
	bool result = true;

	// Check _NET_WM_WINDOW_TYPE
	prop = wid_get_prop(ps, wid, _NET_WM_WINDOW_TYPE, 1, XA_ATOM, 32);
	{
		long v = winprop_get_int(&prop);
		if ((_NET_WM_WINDOW_TYPE_DESKTOP == v
					|| _NET_WM_WINDOW_TYPE_DOCK == v))
			result = false;
	}
	free_winprop(&prop);

	if (!result) return result;

	if (WMPSN_EWMH == ps->wmpsn) {
		// Check _NET_WM_STATE
		prop = wid_get_prop(ps, wid, _NET_WM_STATE, 8192, XA_ATOM, 32);
		for (int i = 0; result && i < prop.nitems; i++) {
			long v = prop.data32[i];
			if (!ps->o.showUnmapped && _NET_WM_STATE_HIDDEN == v)
				result = false;
			else if (ps->o.ignoreSkipTaskbar
					&& _NET_WM_STATE_SKIP_TASKBAR == v)
				result = false;
			else if (_NET_WM_STATE_SHADED == v)
				result = false;
		}
		free_winprop(&prop);

	}
	else if (WMPSN_GNOME == ps->wmpsn) {
		// Check _WIN_STATE
		prop = wid_get_prop(ps, wid, _WIN_STATE, 1, XA_CARDINAL, 0);
		if (!ps->o.showUnmapped && winprop_get_int(&prop)
				& (WIN_STATE_MINIMIZED | WIN_STATE_SHADED | WIN_STATE_HIDDEN))
			result = false;
		free_winprop(&prop);

		if (result && ps->o.ignoreSkipTaskbar) {
			prop = wid_get_prop(ps, wid, _WIN_HINTS, 1, XA_CARDINAL, 0);
			if (winprop_get_int(&prop) & WIN_HINTS_SKIP_TASKBAR)
				result = false;
			free_winprop(&prop);
		}
	}

	return result;
}

long
wm_get_window_desktop(session_t *ps, Window wid) {
	long desktop = LONG_MIN;
	winprop_t prop = { };

	// Check for sticky window
	if (WMPSN_GNOME == ps->wmpsn) {
		prop = wid_get_prop(ps, wid, _WIN_STATE, 1, XA_CARDINAL, 0);
		if (WIN_STATE_STICKY & winprop_get_int(&prop))
			desktop = -1;
		free_winprop(&prop);
		if (LONG_MIN != desktop)
			return desktop;
	}

	prop = wid_get_prop(ps, wid, _NET_WM_DESKTOP, 1, XA_CARDINAL, 0);
	if (prop.nitems)
		desktop = winprop_get_int(&prop);
	if ((long) 0xFFFFFFFFL == desktop)
		desktop = -1;
	free_winprop(&prop);
	if (LONG_MIN != desktop) return desktop;

	prop = wid_get_prop(ps, wid, _WIN_WORKSPACE, 1, XA_CARDINAL, 0);
	if (prop.nitems)
		desktop = winprop_get_int(&prop);
	free_winprop(&prop);
	if (LONG_MIN != desktop) return desktop;

	return wm_get_current_desktop(ps);
}

/* Get focused window and traverse towards the root window until a window with WM_STATE is found */
Window
wm_get_focused(session_t *ps) {
	Window focused = None;

	{
		int revert_to = 0;
		if (!XGetInputFocus(ps->dpy, &focused, &revert_to)) {
			printfef("(): Failed to get current focused window.");
			return None;
		}
		// printfdf("(): Focused window is %#010lx.", focused);
	}

	while (focused) {
		// Discard insane values
		if (ps->root == focused || PointerRoot == focused)
			return None;

		// Check for WM_STATE
		if (wid_has_prop(ps, focused, XA_WM_STATE))
			return focused;

		// Query window parent
		{
			Window rroot = None, parent = None;
			Window *children = NULL;
			unsigned int nchildren = 0;

			Status status =
				XQueryTree(ps->dpy, focused, &rroot, &parent, &children, &nchildren);
			sxfree(children);
			if (!status) {
				printfef("(): Failed to get parent window of %#010lx.", focused);
				return None;
			}
			// printfdf("(): Parent window of %#010lx is %#010lx.", focused, parent);
			focused = parent;
			assert(ps->root == rroot);
		}
	}

	return focused;
}

/**
 * @brief Get the raw string from a string property.
 */
char *
wm_wid_get_prop_rstr(session_t *ps, Window wid, Atom prop) {
	Atom type_ret = None;
	int fmt_ret = 0;
	unsigned long nitems = 0;
	unsigned long bytes_after_ret = 0;
	unsigned char *data = NULL;
	char *ret = NULL;
	if (Success == XGetWindowProperty(ps->dpy, wid, prop, 0, BUF_LEN,
				False, AnyPropertyType, &type_ret, &fmt_ret, &nitems,
				&bytes_after_ret, &data) && nitems && 8 == fmt_ret)
		ret = mstrdup((char *) data);
	sxfree(data);
	return ret;
}

/**
 * @brief Get the first string in a UTF-8 string property on a window.
 */
char *
wm_wid_get_prop_utf8(session_t *ps, Window wid, Atom prop) {
	XTextProperty text_prop = { };
	char *ret = NULL;
	if (XGetTextProperty(ps->dpy, wid, &text_prop, prop)) {
		char **strlst = NULL;
		int cstr = 0;
		Xutf8TextPropertyToTextList(ps->dpy, &text_prop, &strlst, &cstr);
		if (cstr) ret = mstrdup(strlst[0]);
		if (strlst) XFreeStringList(strlst);
	}
	sxfree(text_prop.value);
	return ret;
}

/**
 * @brief Set a UTF-8 string property on a window.
 */
bool
wm_wid_set_prop_utf8(session_t *ps, Window wid, Atom prop, char *text) {
	XTextProperty text_prop = { };
	bool success = (Success == XmbTextListToTextProperty(ps->dpy, &text, 1,
				XUTF8StringStyle, &text_prop));
	if (success)
		XSetTextProperty(ps->dpy, wid, &text_prop, prop);
	sxfree(text_prop.value);
	return success;
}

/**
 * @brief Set basic properties on a window.
 */
void
wm_wid_set_info(session_t *ps, Window wid, const char *name,
		Atom window_type) {
	// Set window name
	{
		char *textcpy = mstrjoin("skippy-xd ", name);
		{
			XTextProperty text_prop = { };
			if (Success == XmbTextListToTextProperty(ps->dpy, &textcpy, 1,
						XStdICCTextStyle, &text_prop))
				XSetWMName(ps->dpy, wid, &text_prop);
			sxfree(text_prop.value);
		}
		wm_wid_set_prop_utf8(ps, wid, _NET_WM_NAME, textcpy);
		free(textcpy);
	}

	// Set window class
	{
		XClassHint *classh = allocchk(XAllocClassHint());
		classh->res_name = "skippy-xd";
		classh->res_class = "skippy-xd";
		XSetClassHint(ps->dpy, wid, classh);
		XFree(classh);
	}

	// Set window type
	{
		if (!window_type)
			window_type = _NET_WM_WINDOW_TYPE_NORMAL;
		long val = window_type;
		XChangeProperty(ps->dpy, wid, _NET_WM_WINDOW_TYPE, XA_ATOM, 32,
				PropModeReplace, (unsigned char *) &val, 1);
	}
}

/**
 * @brief Send a X client messsage.
 */
void
wm_send_clientmsg(session_t *ps, Window twid, Window wid, Atom msg_type,
		int fmt, long event_mask, int len, const unsigned char *data) {
	assert(twid);
	assert(8 == fmt || 16 == fmt || 32 == fmt);
	assert(len * fmt <= 20 * 8);
	XClientMessageEvent ev = {
		.type = ClientMessage,
		.window = wid,
		.message_type = msg_type,
		.format = fmt,
	};
	int seglen = 0;
	switch (fmt) {
		case 32: seglen = sizeof(long); break;
		case 16: seglen = sizeof(short); break;
		case 8: seglen = sizeof(char); break;
	}
	memcpy(ev.data.l, data, seglen * len);
	XSendEvent(ps->dpy, twid, False, event_mask, (XEvent *) &ev);
}

/**
 * @brief Find out the WM frame of a client window by querying X.
 *
 * @param ps current session
 * @param wid window ID
 * @return window ID of the frame window
 */
Window
wm_find_frame(session_t *ps, Window wid) {
  // We traverse through its ancestors to find out the frame
  for (Window cwid = wid; cwid && cwid != ps->root; ) {
    Window rroot = None;
    Window *children = NULL;
    unsigned nchildren = 0;
	wid = cwid;
    if (!XQueryTree(ps->dpy, cwid, &rroot, &cwid, &children,
          &nchildren))
			cwid = 0;
    sxfree(children);
  }

  return wid;
}

/**
 * Get a specific attribute of a window.
 *
 * Returns a blank structure if the returned type and format does not
 * match the requested type and format.
 *
 * @param ps current session
 * @param w window
 * @param atom atom of attribute to fetch
 * @param length length to read
 * @param rtype atom of the requested type
 * @param rformat requested format
 * @return a <code>winprop_t</code> structure containing the attribute
 *    and number of items. A blank one on failure.
 */
winprop_t
wid_get_prop_adv(const session_t *ps, Window w, Atom atom, long offset,
    long length, Atom rtype, int rformat) {
  Atom type = None;
  int format = 0;
  unsigned long nitems = 0, after = 0;
  unsigned char *data = NULL;

	if (Success == XGetWindowProperty(ps->dpy, w, atom, offset, length,
				False, rtype, &type, &format, &nitems, &after, &data)
			&& nitems && (AnyPropertyType == type || type == rtype)
			&& (!rformat || format == rformat)
			&& (8 == format || 16 == format || 32 == format)) {
		return (winprop_t) {
			.data8 = data,
			.nitems = nitems,
			.type = type,
			.format = format,
		};
	}

  sxfree(data);

  return (winprop_t) {
    .data8 = NULL,
    .nitems = 0,
    .type = AnyPropertyType,
    .format = 0
  };
}

