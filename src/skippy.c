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
#include <errno.h>
#include <locale.h>
#include <getopt.h>
#include <strings.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

bool debuglog = false;

enum pipe_cmd_t {
	// Not ordered properly for backward compatibility
	PIPECMD_RELOAD_CONFIG = 0,
	PIPECMD_ACTIVATE_SWITCHER = 1,
	PIPECMD_TOGGLE_SWITCHER,
	PIPECMD_ACTIVATE_EXPOSE,
	PIPECMD_TOGGLE_EXPOSE,
	PIPECMD_ACTIVATE_PAGING,
	PIPECMD_TOGGLE_PAGING,
	PIPECMD_DEACTIVATE,
	PIPECMD_EXIT_DAEMON,
	PIPECMD_QUEUE_FI_PREV,
	PIPECMD_QUEUE_FI_NEXT,
};

session_t *ps_g = NULL;

/**
 * @brief Parse a string representation of enum cliop.
 */
static bool
parse_cliop(session_t *ps, const char *str, enum cliop *dest) {
	static const char * const STRS_CLIENTOP[] = {
		[	CLIENTOP_NO						] = "no",
		[	CLIENTOP_FOCUS				] = "focus",
		[	CLIENTOP_ICONIFY			] = "iconify",
		[	CLIENTOP_SHADE_EWMH		] = "shade-ewmh",
		[	CLIENTOP_CLOSE_ICCCM	] = "close-icccm",
		[	CLIENTOP_CLOSE_EWMH		] = "close-ewmh",
		[	CLIENTOP_DESTROY			] = "destroy",
	};
	for (int i = 0; i < sizeof(STRS_CLIENTOP) / sizeof(STRS_CLIENTOP[0]); ++i)
		if (!strcmp(STRS_CLIENTOP[i], str)) {
			*dest = i;
			return true;
		}

	printfef(true, "() (\"%s\"): Unrecognized operation.", str);
	return false;
}

/**
 * @brief Parse a string representation of enum align.
 */
static int
parse_align(session_t *ps, const char *str, enum align *dest) {
	static const char * const STRS_ALIGN[] = {
		[ ALIGN_LEFT  ] = "left",
		[ ALIGN_MID   ] = "mid",
		[ ALIGN_RIGHT ] = "right",
	};
	for (int i = 0; i < CARR_LEN(STRS_ALIGN); ++i)
		if (str_startswithword(str, STRS_ALIGN[i])) {
			*dest = i;
			return strlen(STRS_ALIGN[i]);
		}

	printfef(true, "() (\"%s\"): Unrecognized operation.", str);
	return 0;
}

static inline bool
parse_align_full(session_t *ps, const char *str, enum align *dest) {
	int r = parse_align(ps, str, dest);
	if (r && str[r]) r = 0;
	return r;
}

/**
 * @brief Parse a string representation of picture positioning mode.
 */
static int
parse_pict_posp_mode(session_t *ps, const char *str, enum pict_posp_mode *dest) {
	static const char * const STRS_PICTPOSP[] = {
		[ PICTPOSP_ORIG			] = "orig",
		[ PICTPOSP_SCALE		] = "scale",
		[ PICTPOSP_SCALEK		] = "scalek",
		[ PICTPOSP_SCALEE		] = "scalee",
		[ PICTPOSP_SCALEEK	] = "scaleek",
		[ PICTPOSP_TILE	 		] = "tile",
	};
	for (int i = 0; i < CARR_LEN(STRS_PICTPOSP); ++i)
		if (str_startswithword(str, STRS_PICTPOSP[i])) {
			*dest = i;
			return strlen(STRS_PICTPOSP[i]);
		}

	printfef(true, "() (\"%s\"): Unrecognized operation.", str);
	return 0;
}
static inline int
parse_color_sub(const char *s, unsigned short *dest) {
	static const int SEG = 2;

	char *endptr = NULL;
	long v = 0L;
	char *s2 = mstrncpy(s, SEG);
	v = strtol(s2, &endptr, 16);
	int ret = 0;
	if (endptr && s2 + strlen(s2) == endptr)
		ret = endptr - s2;
	free(s2);
	if (!ret) return ret;
	*dest = (double) v / 0xff * 0xffff;
	return ret;
}

/**
 * @brief Parse an option string into XRenderColor.
 */
static int
parse_color(session_t *ps, const char *s, XRenderColor *pc) {
	const char * const sorig = s;
	static const struct {
		const char *name;
		XRenderColor c;
	} PREDEF_COLORS[] = {
		{ "black", { 0x0000, 0x0000, 0x0000, 0xFFFF } },
		{ "red", { 0xffff, 0x0000, 0x0000, 0xFFFF } },
	};

	// Predefined color names
	for (int i = 0; i < CARR_LEN(PREDEF_COLORS); ++i)
		if (str_startswithwordi(s, PREDEF_COLORS[i].name)) {
			*pc = PREDEF_COLORS[i].c;
			return strlen(PREDEF_COLORS[i].name);
		}

	// RRGGBBAA color
	if ('#' == s[0]) {
		++s;
		int next = 0;
		if (!((next = parse_color_sub(s, &pc->red))
					&& (next = parse_color_sub((s += next), &pc->green))
					&& (next = parse_color_sub((s += next), &pc->blue)))) {
			printfef(true, "() (\"%s\"): Failed to read color segment.", s);
			return 0;
		}
		if (!(next = parse_color_sub((s += next), &pc->alpha)))
			pc->alpha = 0xffff;
		s += next;
		return s - sorig;
	}

	printfef(true, "(\"%s\"): Unrecognized color format.", s);
	return 0;
}

/**
 * @brief Parse a size string.
 */
static int
parse_size(const char *s, int *px, int *py) {
	const char * const sorig = s;
	long val = 0L;
	char *endptr = NULL;
	bool hasdata = false;

#define T_NEXTFIELD() do { \
	hasdata = true; \
	if (isspace0(*s)) goto parse_size_end; \
} while(0)

	// Parse width
	// Must be base 10, because "0x0..." may appear
	val = strtol(s, &endptr, 10);
	if (endptr && s != endptr) {
		*px = val;
		assert(*px >= 0);
		s = endptr;
		T_NEXTFIELD();
	}

	// Parse height
	if ('x' == *s) {
		++s;
		val = strtol(s, &endptr, 10);
		if (endptr && s != endptr) {
			*py = val;
			if (*py < 0) {
				printfef(true, "() (\"%s\"): Invalid height.", s);
				return 0;
			}
			s = endptr;
		}
		T_NEXTFIELD();
	}

#undef T_NEXTFIELD

	if (!hasdata)
		return 0;

	if (!isspace0(*s)) {
		printfef(true, "() (\"%s\"): Trailing characters.", s);
		return 0;
	}

parse_size_end:
	return s - sorig;
}

/**
 * @brief Parse an image specification.
 */
static bool
parse_pictspec(session_t *ps, const char *s, pictspec_t *dest) {
#define T_NEXTFIELD() do { \
	s += next; \
	while (isspace(*s)) ++s; \
	if (!*s) goto parse_pictspec_end; \
} while (0)

	int next = 0;
	T_NEXTFIELD();
	if (!(next = parse_size(s, &dest->twidth, &dest->theight)))
		dest->twidth = dest->theight = 0;
	T_NEXTFIELD();
	if (!(next = parse_pict_posp_mode(ps, s, &dest->mode)))
		dest->mode = PICTPOSP_ORIG;
	T_NEXTFIELD();
	if (!(next = parse_align(ps, s, &dest->alg)))
		dest->alg = ALIGN_MID;
	T_NEXTFIELD();
	if (!(next && (next = parse_align(ps, s, &dest->valg))))
		dest->valg = ALIGN_MID;
	T_NEXTFIELD();
	next = parse_color(ps, s, &dest->c);
	T_NEXTFIELD();
	if (*s)
		dest->path = mstrdup(s);
#undef T_NEXTFIELD

parse_pictspec_end:
	return true;
}

static void
anime(
	MainWin *mw,
	dlist *clients,
	float timeslice
)
{
	clients = dlist_first(clients);
	float multiplier = 1.0 + timeslice * (mw->multiplier - 1.0);
	mainwin_transform(mw, multiplier);
	foreach_dlist (mw->clientondesktop) {
		ClientWin *cw = (ClientWin *) iter->data;
		clientwin_move(cw, multiplier, mw->xoff, mw->yoff, timeslice);
		clientwin_update2(cw);
		clientwin_map(cw);
	}
}

static void
update_clients(MainWin *mw, Bool *touched)
{
	// Update the client table, pick the ones we want and sort them
	dlist *stack = dlist_first(wm_get_stack(mw->ps));
	mw->clients = dlist_first(mw->clients);

	if (touched)
		*touched = False;
	
	// Terminate mw->clients that are no longer managed
	for (dlist *iter = mw->clients; iter; ) {
		ClientWin *cw = (ClientWin *) iter->data;
		if (dlist_find_data(stack, (void *) cw->wid_client)) {
			iter = iter->next;
		}
		else {
			dlist *tmp = iter->next;
			clientwin_destroy((ClientWin *) iter->data, True);
			mw->clients = dlist_remove(iter);
			iter = tmp;
			if (touched)
				*touched = True;
		}
	}
	XFlush(mw->ps->dpy);

	// Add new mw->clients
	foreach_dlist (stack) {
		ClientWin *cw = (ClientWin *)
			dlist_find(mw->clients, clientwin_cmp_func, iter->data);
		if (!cw && ((Window) iter->data) != mw->window) {
			cw = clientwin_create(mw, (Window)iter->data);
			if (!cw) continue;
			mw->clients = dlist_add(mw->clients, cw);
			if (touched)
				*touched = True;
		}
	}

	dlist_free(stack);
}

static void
daemon_count_clients(MainWin *mw, Bool *touched)
{
	// given the client table, update the clientondesktop
	// the difference between mw->clients and mw->clientondesktop
	// is that mw->clients is all the client windows 
	// while mw->clientondesktop is only those in current virtual desktop
	// if that option is user supplied

	update_clients(mw, 0);
	if (!mw->clients) {
		printfdf(false, "(): No client windows found.");
		return;
	}

	dlist_free(mw->clientondesktop);
	mw->clientondesktop = NULL;

	{
		session_t * const ps = mw->ps;
		long desktop = wm_get_current_desktop(ps);

		dlist *tmp = dlist_first(dlist_find_all(mw->clients,
					(dlist_match_func) clientwin_validate_func, &desktop));
		if (!tmp) {
			printfdf(false, "(): No client window on current desktop found.");
			return;
		}

		mw->clientondesktop = tmp;
	}

	return;
}

static void
init_focus(MainWin *mw, enum layoutmode layout, Window leader) {
	session_t *ps = mw->ps;

	// ordering of client windows list
	// is important for prev/next window selection
	mw->focuslist = dlist_dup(mw->clientondesktop);

	/*if (layout == LAYOUTMODE_SWITCHER)
		dlist_sort(mw->focuslist, sort_cw_by_row, 0);
	else*/ if (layout == LAYOUTMODE_EXPOSE)
		dlist_sort(mw->focuslist, sort_cw_by_column, 0);

	// Get the currently focused window and select which mini-window to focus
	dlist *iter = dlist_find(mw->focuslist, clientwin_cmp_func, (void *) leader);

	// check if the user specified --prev or --next on the cmdline
	if(ps->o.focus_initial)
	{

		// ps->mainwin->ignore_next_refocus = 1;
		// ps->mainwin->ignore_next_refocus = 2;
		// ps->mainwin->ignore_next_refocus = 4;


		if(ps->o.focus_initial == FI_PREV)
		{
			// here, mw->focuslist is the first (dlist*) item in the list
			if (iter == mw->focuslist)
				iter = dlist_last(mw->focuslist);
			else
			{
				dlist *i = mw->focuslist;
				for (; i != NULL; i = i->next)
					if (i->next && i->next == iter)
						break;
				iter = i;
			}
		}
		else if(ps->o.focus_initial == FI_NEXT)
			iter = iter->next;

	}

	// then clear this flag, so daemon not remember on its next activation
	ps->o.focus_initial = 0;

	if (!iter) {
		dlist * first = dlist_first(mw->focuslist);
		mw->client_to_focus = first->data;
		mw->client_to_focus_on_cancel = NULL;
	}
	else {
		mw->client_to_focus = (ClientWin *) iter->data;
		mw->client_to_focus_on_cancel = (ClientWin *) iter->data;
		mw->client_to_focus->focused = 1;
	}
}

static bool
init_layout(MainWin *mw, enum layoutmode layout, Window leader)
{
	if (!mw->clientondesktop)
		return true;
	
	/* set up the windows layout */
	{
		unsigned int newwidth = 0, newheight = 0;
		layout_run(mw, mw->clientondesktop, &newwidth, &newheight, layout);

		float multiplier = (float) (mw->width - 2 * mw->distance) / newwidth;
		if (multiplier * newheight > mw->height - 2 * mw->distance)
			multiplier = (float) (mw->height - 2 * mw->distance) / newheight;
		if (!mw->ps->o.allowUpscale)
			multiplier = MIN(multiplier, 1.0f);

		int xoff = (mw->width - (float) newwidth * multiplier) / 2;
		int yoff = (mw->height - (float) newheight * multiplier) / 2;

		mw->multiplier = multiplier;
		mw->xoff = xoff;
		mw->yoff = yoff;
	}

	init_focus(mw, layout, leader);

	return true;
}

static bool
init_paging_layout(MainWin *mw, enum layoutmode layout, Window leader)
{
	if (!mw->clients)
		return true;

	int screencount = wm_get_desktops(mw->ps);
	if (screencount == -1)
		screencount = 1;
	int desktop_dim = ceil(sqrt(screencount));

    {
		int totalwidth = desktop_dim * (mw->width + mw->distance) - mw->distance;
		int totalheight = desktop_dim * (mw->height + mw->distance) - mw->distance;

		float multiplier = (float) (mw->width - 1 * mw->distance) / (float) totalwidth;
		if (multiplier * totalheight > mw->height - 1 * mw->distance)
			multiplier = (float) (mw->height - 1 * mw->distance) / (float) totalheight;

		int xoff = (mw->width - (float) totalwidth * multiplier) / 2;
		int yoff = (mw->height - (float) totalheight * multiplier) / 2;

		mw->multiplier = multiplier;
		mw->xoff = xoff;
		mw->yoff = yoff;
	}

	foreach_dlist (mw->clients) {
		ClientWin *cw = (ClientWin *) iter->data;
		int win_desktop = wm_get_window_desktop(mw->ps, cw->wid_client);
		int current_desktop = wm_get_current_desktop(mw->ps);

		int win_desktop_x = win_desktop % desktop_dim;
		int win_desktop_y = win_desktop / desktop_dim;

		int current_desktop_x = current_desktop % desktop_dim;
		int current_desktop_y = current_desktop / desktop_dim;

		cw->x = cw->src.x + win_desktop_x * (mw->width + mw->distance);
		cw->y = cw->src.y + win_desktop_y * (mw->height + mw->distance);

		cw->src.x += (win_desktop_x - current_desktop_x) * (mw->width + mw->distance);
		cw->src.y += (win_desktop_y - current_desktop_y) * (mw->height + mw->distance);
	}

	// create windows which represent each virtual desktop
	int current_desktop = wm_get_current_desktop(mw->ps);
	for (int j=0; j<desktop_dim; j++) {
		for (int i=0; i<desktop_dim; i++) {
			int desktop_idx = desktop_dim * j + i;
			XSetWindowAttributes sattr = {
				.border_pixel = 0,
				.background_pixel = 0,
				.colormap = mw->colormap,
				/*.event_mask = ButtonPressMask | ButtonReleaseMask | KeyPressMask
					| KeyReleaseMask | EnterWindowMask | LeaveWindowMask
					| PointerMotionMask | ExposureMask | FocusChangeMask,*/
				.override_redirect = false,
                // exclude window frame
			};
			Window desktopwin = XCreateWindow(mw->ps->dpy,
					mw->window,
					0, 0, 0, 0,
					0, mw->depth, InputOnly, mw->visual,
					CWColormap | CWBackPixel | CWBorderPixel | CWEventMask | CWOverrideRedirect, &sattr);
			if (!desktopwin) return false;

			if (!mw->desktopwins)
				mw->desktopwins = dlist_add(NULL, &desktopwin);
			else
				mw->desktopwins = dlist_add(mw->desktopwins, &desktopwin);

			ClientWin *cw = clientwin_create(mw, desktopwin);
			if (!cw) return false;

			cw->slots = desktop_idx;

			{
				static const char *PREFIX = "virtual desktop ";
				const int len = strlen(PREFIX) + 20;
				char *str = allocchk(malloc(len));
				snprintf(str, len, "%s%d", PREFIX, cw->slots);
				wm_wid_set_info(cw->mainwin->ps, cw->mini.window, str, None);
				free(str);
			}

			cw->zombie = false;
			cw->mode = CLIDISP_DESKTOP;

			cw->x = cw->src.x = (i * (mw->width + mw->distance)) * mw->multiplier;
			cw->y = cw->src.y = (j * (mw->height + mw->distance)) * mw->multiplier;
			cw->src.width = mw->width;
			cw->src.height = mw->height;

			clientwin_move(cw, mw->multiplier, mw->xoff, mw->yoff, 1);

			if (!mw->dminis)
				mw->dminis = dlist_add(NULL, cw);
			else
				dlist_add(mw->dminis, cw);

			XRaiseWindow(mw->ps->dpy, cw->mini.window);

			if (cw->slots == current_desktop) {
				mw->client_to_focus = cw;
				mw->client_to_focus_on_cancel = cw;
				mw->client_to_focus->focused = 1;
			}
		}
	}

	mw->focuslist = dlist_dup(mw->dminis);

	return true;
}

static void
desktopwin_map(ClientWin *cw)
{
	session_t *ps = cw->mainwin->ps;

	free_damage(ps, &cw->damage);
	free_pixmap(ps, &cw->pixmap);

	XUnmapWindow(ps->dpy, cw->mini.window);
	XSetWindowBackgroundPixmap(ps->dpy, cw->mini.window, None);

	XRenderPictureAttributes pa = { };

	if (cw->origin)
		free_picture(ps, &cw->origin);
	cw->origin = XRenderCreatePicture(ps->dpy,
			None, cw->mainwin->format, CPSubwindowMode, &pa);
	XRenderSetPictureFilter(ps->dpy, cw->origin, FilterBest, 0, 0);

	XCompositeRedirectWindow(ps->dpy, cw->src.window,
			CompositeRedirectAutomatic);
	cw->redirected = true;
	
	clientwin_render(cw);

	XMapWindow(ps->dpy, cw->mini.window);
	XRaiseWindow(ps->dpy, cw->mini.window);
}

static inline const char *
ev_dumpstr_type(const XEvent *ev) {
	switch (ev->type) {
		CASESTRRET(KeyPress);
		CASESTRRET(KeyRelease);
		CASESTRRET(ButtonPress);
		CASESTRRET(ButtonRelease);
		CASESTRRET(MotionNotify);
		CASESTRRET(EnterNotify);
		CASESTRRET(LeaveNotify);
		CASESTRRET(FocusIn);
		CASESTRRET(FocusOut);
		CASESTRRET(KeymapNotify);
		CASESTRRET(Expose);
		CASESTRRET(GraphicsExpose);
		CASESTRRET(NoExpose);
		CASESTRRET(CirculateRequest);
		CASESTRRET(ConfigureRequest);
		CASESTRRET(MapRequest);
		CASESTRRET(ResizeRequest);
		CASESTRRET(CirculateNotify);
		CASESTRRET(ConfigureNotify);
		CASESTRRET(CreateNotify);
		CASESTRRET(DestroyNotify);
		CASESTRRET(GravityNotify);
		CASESTRRET(MapNotify);
		CASESTRRET(MappingNotify);
		CASESTRRET(ReparentNotify);
		CASESTRRET(UnmapNotify);
		CASESTRRET(VisibilityNotify);
		CASESTRRET(ColormapNotify);
		CASESTRRET(ClientMessage);
		CASESTRRET(PropertyNotify);
		CASESTRRET(SelectionClear);
		CASESTRRET(SelectionNotify);
		CASESTRRET(SelectionRequest);
	}

	return "Unknown";
}

static inline Window
ev_window(session_t *ps, const XEvent *ev) {
#define T_SETWID(type, ele) case type: return ev->ele.window
	switch (ev->type) {
		case KeyPress:
		T_SETWID(KeyRelease, xkey);
		case ButtonPress:
		T_SETWID(ButtonRelease, xbutton);
		T_SETWID(MotionNotify, xmotion);
		case EnterNotify:
		T_SETWID(LeaveNotify, xcrossing);
		case FocusIn:
		T_SETWID(FocusOut, xfocus);
		T_SETWID(KeymapNotify, xkeymap);
		T_SETWID(Expose, xexpose);
		case GraphicsExpose: return ev->xgraphicsexpose.drawable;
		case NoExpose: return ev->xnoexpose.drawable;
		T_SETWID(CirculateNotify, xcirculate);
		T_SETWID(ConfigureNotify, xconfigure);
		T_SETWID(CreateNotify, xcreatewindow);
		T_SETWID(DestroyNotify, xdestroywindow);
		T_SETWID(GravityNotify, xgravity);
		T_SETWID(MapNotify, xmap);
		T_SETWID(MappingNotify, xmapping);
		T_SETWID(ReparentNotify, xreparent);
		T_SETWID(UnmapNotify, xunmap);
		T_SETWID(VisibilityNotify, xvisibility);
		T_SETWID(ColormapNotify, xcolormap);
		T_SETWID(ClientMessage, xclient);
		T_SETWID(PropertyNotify, xproperty);
		T_SETWID(SelectionClear, xselectionclear);
		case SelectionNotify: return ev->xselection.requestor;
	}
#undef T_SETWID
	if (ps->xinfo.damage_ev_base + XDamageNotify == ev->type)
	  return ((XDamageNotifyEvent *) ev)->drawable;

	printfef(false, "(): Failed to find window for event type %d. Troubles ahead.",
			ev->type);

	return ev->xany.window;
}

static inline void
ev_dump(session_t *ps, const MainWin *mw, const XEvent *ev) {
	if (!ev || (ps->xinfo.damage_ev_base + XDamageNotify) == ev->type) return;
	// if (MotionNotify == ev->type) return;

	const char *name = ev_dumpstr_type(ev);

	Window wid = ev_window(ps, ev);
	const char *wextra = "";
	if (ps->root == wid) wextra = "(Root)";
	if (mw && mw->window == wid) wextra = "(Main)";

	print_timestamp(ps);
	printfdf(false, "(): Event %-13.13s wid %#010lx %s", name, wid, wextra);
}

static bool
skippy_activate(MainWin *mw, enum layoutmode layout)
{
	session_t *ps = mw->ps;

	// Do this window before main window gets mapped
	Window focus_win = wm_get_focused(ps);

	// Update the main window's geometry (and Xinerama info if applicable)
	mainwin_update(mw);
#ifdef CFG_XINERAMA
	if (ps->o.xinerama_showAll)
		mw->xin_active = 0;
#endif /* CFG_XINERAMA */

	// Map the main window and run our event loop
	if (ps->o.lazyTrans) {
		mainwin_map(mw);
		XFlush(ps->dpy);
	}

	mw->client_to_focus = NULL;

	daemon_count_clients(mw, 0);
	if (!mw->clients || !mw->clientondesktop) {
		return false;
	}

	foreach_dlist(mw->clients) {
		clientwin_update((ClientWin *) iter->data);
		clientwin_update2((ClientWin *) iter->data);
	}

	if (layout == LAYOUTMODE_PAGING) {
		if (!init_paging_layout(mw, layout, focus_win)) {
			printfef(false, "(): failed.");
			return false;
		}
	}
	else {
		if (!init_layout(mw, layout, focus_win)) {
			printfef(false, "(): failed.");
			return false;
		}
	}

	foreach_dlist(mw->clients) {
		ClientWin *cw = iter->data;
		if (mw->ps->o.lazyTrans)
		{
			cw->x += cw->mainwin->x;
			cw->y += cw->mainwin->y;
		}
		cw->x *= mw->multiplier;
		cw->y *= mw->multiplier;
	}

	return true;
}

static inline bool
open_pipe(session_t *ps, struct pollfd *r_fd) {
	if (ps->fd_pipe >= 0) {
		close(ps->fd_pipe);
		ps->fd_pipe = -1;
		if (r_fd)
			r_fd[1].fd = ps->fd_pipe;
	}
	ps->fd_pipe = open(ps->o.pipePath, O_RDONLY | O_NONBLOCK);
	if (ps->fd_pipe >= 0) {
		if (r_fd)
			r_fd[1].fd = ps->fd_pipe;
		return true;
	}
	else {
		printfef(true, "(): Failed to open pipe \"%s\": %d", ps->o.pipePath, errno);
		perror("open");
	}

	return false;
}

static void
mainloop(session_t *ps, bool activate_on_start) {
	MainWin *mw = NULL;
	bool die = false;
	bool activate = activate_on_start;
	bool pending_damage = false;
	long last_rendered = 0L;
	enum layoutmode layout = LAYOUTMODE_SWITCHER;
	bool animating = activate;
	long first_animated = 0L;

	switch (ps->o.mode) {
		case PROGMODE_ACTV_EXPOSE:
		case PROGMODE_TGG_EXPOSE:
			layout = LAYOUTMODE_EXPOSE;
			break;
		case PROGMODE_ACTV_PAGING:
		case PROGMODE_TGG_PAGING:
			layout = LAYOUTMODE_PAGING;
			break;
		default:
			layout = LAYOUTMODE_SWITCHER;
			break;
	}

	struct pollfd r_fd[2] = {
		{
			.fd = ConnectionNumber(ps->dpy),
			.events = POLLIN,
		},
		{
			.fd = ps->fd_pipe,
			.events = POLLIN,
		},
	};

	while (true) {
		// Clear revents in pollfd
		for (int i = 0; i < CARR_LEN(r_fd); ++i)
			r_fd[i].revents = 0;

		// Activation goes first, so that it won't be delayed by poll()
		if (!mw && activate) {
			assert(ps->mainwin);
			activate = false;

			if (skippy_activate(ps->mainwin, layout)) {
				last_rendered = time_in_millis();
				mw = ps->mainwin;
				pending_damage = false;
				first_animated = time_in_millis();
			}
		}
		if (mw)
			activate = false;

		// Main window destruction, before poll()
		if (mw && die) {
			// Unmap the main window and all clients, to make sure focus doesn't fall out
			// when we start setting focus on client window
			mainwin_unmap(mw);
			foreach_dlist(mw->clientondesktop) { clientwin_unmap((ClientWin *) iter->data); }
			XSync(ps->dpy, False);

			// Focus the client window only after the main window get unmapped and
			// keyboard gets ungrabbed.
			long new_desktop = -1;
			if (mw->client_to_focus) {
				if (layout == LAYOUTMODE_PAGING) {
					if (!mw->refocus)
						new_desktop = mw->client_to_focus->slots;
					else
						new_desktop = mw->client_to_focus_on_cancel->slots;
				}
				else {
					if (!mw->refocus)
						childwin_focus(mw->client_to_focus);
					else if(mw->client_to_focus_on_cancel)
						childwin_focus(mw->client_to_focus_on_cancel);
				}
				mw->refocus = false;
				mw->client_to_focus = NULL;
				pending_damage = false;
			}

			// Cleanup
			dlist_free(mw->clientondesktop);
			mw->clientondesktop = 0;
			dlist_free(mw->focuslist);

			// free all mini desktop representations
			dlist_free_with_func(mw->dminis, (dlist_free_func) clientwin_destroy);
			mw->dminis = NULL;

			foreach_dlist (mw->desktopwins) {
				XDestroyWindow(ps->dpy, (Window) (iter->data));
				//XSelectInput(ps->dpy, (Window) (iter->data), 0);
			}
			dlist_free(mw->desktopwins);
			mw->desktopwins = NULL;

			// Catch all errors, but remove all events
			XSync(ps->dpy, False);
			XSync(ps->dpy, True);

			mw = NULL;

			if (new_desktop != -1)
				wm_set_desktop_ewmh(ps, new_desktop);
		}
		if (!mw)
			die = false;
		if (activate_on_start && !mw)
			return;

		{
			// animation!
			if (mw && animating) {
				int timeslice = time_in_millis() - first_animated;
				if (layout != LAYOUTMODE_SWITCHER
						&& timeslice < ps->o.animationDuration
						&& timeslice + first_animated >=
						last_rendered + (1000.0 / 60.0)) {
					anime(ps->mainwin, ps->mainwin->clients,
						((float)timeslice)/(float)ps->o.animationDuration);
					last_rendered = time_in_millis();

					/* Map the main window and run our event loop */
					if (!ps->o.lazyTrans && !mw->mapped)
						mainwin_map(mw);
					XFlush(ps->dpy);
				}
				else if (timeslice >= ps->o.animationDuration) {
					anime(ps->mainwin, ps->mainwin->clients, 1);
					animating = false;
					last_rendered = time_in_millis();
					focus_miniw_adv(ps, mw->client_to_focus,
							ps->o.movePointerOnStart);

					if (layout == LAYOUTMODE_PAGING) {
						foreach_dlist (mw->dminis) {
							desktopwin_map(((ClientWin *) iter->data));
						}
					}

					/* Map the main window and run our event loop */
					if (!ps->o.lazyTrans && !mw->mapped)
						mainwin_map(mw);
					XFlush(ps->dpy);
				}

				continue; // while animating, do not allow user actions
			}

			// Process X events
			int num_events = 0;
			XEvent ev = { };
			while ((num_events = XEventsQueued(ps->dpy, QueuedAfterReading)))
			{
				XNextEvent(ps->dpy, &ev);

#ifdef DEBUG_EVENTS
				ev_dump(ps, mw, &ev);
#endif
				Window wid = ev_window(ps, &ev);

				if (mw && MotionNotify == ev.type)
				{
					// Speed up responsiveness when the user is moving the mouse around
					// The queue gets filled up with consquetive MotionNotify events
					// discard all except the last MotionNotify event in a contiguous block of MotionNotify events

					XEvent ev_next = { };
					while(num_events > 0)
					{
						XPeekEvent(ps->dpy, &ev_next);

						if(ev_next.type != MotionNotify)
							break;

						XNextEvent(ps->dpy, &ev);
						wid = ev_window(ps, &ev);

						num_events--;
					}

					// the mouse has moved
					// refocus enable
					// mw->ignore_next_refocus = 0;

					// we also need to refocus here
					if(mw->cw_tooltip && (mw->cw_tooltip != mw->client_to_focus))
					{
						focus_miniw(ps, mw->cw_tooltip);
						clientwin_render(mw->client_to_focus);
					}


					if (mw->tooltip && ps->o.tooltip_followsMouse)
						tooltip_move(mw->tooltip,
								ev.xmotion.x_root, ev.xmotion.y_root);
				}
				else if (mw && ev.type == DestroyNotify) {
					printfdf(false, "(): else if (ev.type == DestroyNotify) {");
					daemon_count_clients(ps->mainwin, 0);
					if (!mw->clientondesktop) {
						printfdf(false, "(): Last client window destroyed/unmapped, "
								"exiting.");
						die = true;
						mw->client_to_focus = NULL;
					}
				}
				else if (ev.type == MapNotify || ev.type == UnmapNotify) {
					printfdf(false, "(): else if (ev.type == MapNotify || ev.type == UnmapNotify) {");
					daemon_count_clients(ps->mainwin, 0);
					dlist *iter = (wid ? dlist_find(ps->mainwin->clients, clientwin_cmp_func, (void *) wid): NULL);
					if (iter) {
						ClientWin *cw = (ClientWin *) iter->data;
						clientwin_update(cw);
						clientwin_update2(cw);
					}
				}
				else if (mw && (ps->xinfo.damage_ev_base + XDamageNotify == ev.type)) {
					//printfdf(false, "(): else if (ev.type == XDamageNotify) {");
					pending_damage = true;
					dlist *iter = dlist_find(ps->mainwin->clients,
							clientwin_cmp_func, (void *) wid);
					if (iter) {
						((ClientWin *)iter->data)->damaged = true;
					}
					iter = dlist_find(ps->mainwin->dminis,
							clientwin_cmp_func, (void *) wid);
					if (iter) {
						((ClientWin *)iter->data)->damaged = true;
					}
				}
				else if (mw && wid == mw->window)
					die = mainwin_handle(mw, &ev);
				else if (mw && PropertyNotify == ev.type) {
					printfdf(false, "(): else if (ev.type == PropertyNotify) {");

					if (!ps->o.background &&
							(ESETROOT_PMAP_ID == ev.xproperty.atom
							 || _XROOTPMAP_ID == ev.xproperty.atom)) {

						mainwin_update_background(mw);
						REDUCE(clientwin_render((ClientWin *)iter->data), mw->clientondesktop);
					}
				}
				else if (mw && mw->tooltip && wid == mw->tooltip->window)
					tooltip_handle(mw->tooltip, &ev);
				else if (mw && wid) {
					dlist *iter = mw->clientondesktop;
					if (layout == LAYOUTMODE_PAGING)
						iter = mw->dminis;
					for (; iter; iter = iter->next) {
						ClientWin *cw = (ClientWin *) iter->data;
						if (cw->mini.window == wid) {
                            if (!(POLLIN & r_fd[1].revents))
                            {
							    die = clientwin_handle(cw, &ev);
                            }
							break;
						}
					}
				}
			}

			// Do delayed painting if it's active
			if (mw && pending_damage && !die) {
				//printfdf(false, "(): delayed painting");
				pending_damage = false;
				foreach_dlist(mw->clientondesktop) {
					if (((ClientWin *) iter->data)->damaged)
						clientwin_repair(iter->data);
				}

				if (layout == LAYOUTMODE_PAGING) {
					foreach_dlist (mw->dminis) {
						desktopwin_map(((ClientWin *) iter->data));
					}
				}
				last_rendered = time_in_millis();
			}

			// Discards all events so that poll() won't constantly hit data to read
			//XSync(ps->dpy, True);
			//assert(!XEventsQueued(ps->dpy, QueuedAfterReading));

			last_rendered = time_in_millis();
			XFlush(ps->dpy);
		}

		// Poll for events
		int timeout = ps->mainwin->poll_time;
		int time_offset = last_rendered - time_in_millis();
		timeout -= time_offset;
		if (timeout < 0)
			timeout = 0;
		if (pending_damage)
			timeout = 0;
		poll(r_fd, (r_fd[1].fd >= 0 ? 2: 1), timeout);

		// Handle daemon commands
		if (POLLIN & r_fd[1].revents) {
			unsigned char piped_input = 0;
			int read_ret = read(ps->fd_pipe, &piped_input, 1);
			if (0 == read_ret) {
				printfdf(false, "(): EOF reached on pipe \"%s\".", ps->o.pipePath);
				open_pipe(ps, r_fd);
			}
			else if (-1 == read_ret) {
				if (EAGAIN != errno)
					printfdf(false, "(): Reading pipe \"%s\" failed: %d", ps->o.pipePath, errno);
			}
			else {
				assert(1 == read_ret);
				printfdf(false, "(): Received pipe command: %d", piped_input);

				switch (piped_input) {
					case PIPECMD_RELOAD_CONFIG:
						load_config_file(ps);
						mainwin_reload(ps, ps->mainwin);
						break;
					case PIPECMD_ACTIVATE_SWITCHER:
					case PIPECMD_ACTIVATE_EXPOSE:
					case PIPECMD_ACTIVATE_PAGING:
						if (piped_input == PIPECMD_ACTIVATE_SWITCHER) {
							ps->o.mode = PROGMODE_ACTV_SWITCHER;
							layout = LAYOUTMODE_SWITCHER;
						}
						else if (piped_input == PIPECMD_ACTIVATE_EXPOSE) {
							ps->o.mode = PROGMODE_ACTV_EXPOSE;
							layout = LAYOUTMODE_EXPOSE;
						}
						else /* if (piped_input == PIPECMD_ACTIVATE_PAGING) */ {
							ps->o.mode = PROGMODE_ACTV_PAGING;
							layout = LAYOUTMODE_PAGING;
						}

						printfdf(false, "(): case PIPECMD_ACTIVATE, mode=%d", layout);
						if (ps->mainwin->mapped)
						{
							printfdf(false, "(): if (ps->mainwin->mapped)");
							fflush(stdout);fflush(stderr);

							// There is a glitch whereby calling focus_miniw_prev() or focus_miniw_next()
							// does not trigger an Xev to focus-out and un-highlight the focus of the
							// 1st highlighted win, so we manually unfocus it here first, before moving on
							// to focus and highlight the next window... it's probably because we miss the Xev
							// since we are not in the right place in the main loop, cant unwind the call stack
							mw->client_to_focus->focused = 0;
							clientwin_render(mw->client_to_focus);

							if (ps->o.focus_initial == FI_PREV)
							{
								printfdf(false, "(): focus_miniw_prev(ps, mw->client_to_focus);");
								focus_miniw_prev(ps, mw->client_to_focus);
							}

							else if (ps->o.focus_initial == FI_NEXT)
							{
								printfdf(false, "(): focus_miniw_next(ps, mw->client_to_focus);");
								focus_miniw_next(ps, mw->client_to_focus);
							}
							clientwin_render(mw->client_to_focus);
						}
						else
						{
							printfdf(false, "(): activate = true;");
							animating = activate = true;
						}
						break;
					case PIPECMD_DEACTIVATE:
						if (mw)
							die = true;
						break;
					case PIPECMD_TOGGLE_SWITCHER:
					case PIPECMD_TOGGLE_EXPOSE:
					case PIPECMD_TOGGLE_PAGING:
						if (mw) {
							mw->refocus = die = true;
						}
						else {
							animating = activate = true;
							if (piped_input == PIPECMD_TOGGLE_SWITCHER) {
								ps->o.mode = PROGMODE_TGG_SWITCHER;
								layout = LAYOUTMODE_SWITCHER;
							}
							else if (piped_input == PIPECMD_TOGGLE_EXPOSE) {
								ps->o.mode = PROGMODE_TGG_EXPOSE;
								layout = LAYOUTMODE_EXPOSE;
							}
							else /* if (piped_input == PIPECMD_TOGGLE_PAGING) */ {
								ps->o.mode = PROGMODE_TGG_PAGING;
								layout = LAYOUTMODE_PAGING;
							}
						}
						printfdf(false, "(): case PIPECMD_TOGGLE, mode=%d", layout);
						break;
					case PIPECMD_EXIT_DAEMON:
						printfdf(false, "(): Exit command received, killing daemon...");
						unlink(ps->o.pipePath);
						return;
					case PIPECMD_QUEUE_FI_PREV:
						ps->o.focus_initial = FI_PREV;
						break;
					case PIPECMD_QUEUE_FI_NEXT:
						ps->o.focus_initial = FI_NEXT;
						break;
					default:
						printfdf(false, "(): Unknown daemon command \"%d\" received.", piped_input);
						break;
				}
			}
		}

		if (POLLHUP & r_fd[1].revents) {
			printfdf(false, "(): PIPEHUP on pipe \"%s\".", ps->o.pipePath);
			open_pipe(ps, r_fd);
		}
	}
}

static bool
send_command_to_daemon_via_fifo(int command, const char *pipePath) {
	{
		int access_ret = 0;
		if ((access_ret = access(pipePath, W_OK))) {
			printfef(true, "(): Failed to access() pipe \"%s\": %d", pipePath, access_ret);
			perror("access");
			exit(1);
		}
	}

	FILE *fp = fopen(pipePath, "w");

	printfdf(false, "(): Sending command...");
	fputc(command, fp);

	fclose(fp);

	return true;
}

static inline bool
queue_reload_config(const char *pipePath) {
	printfdf(false, "(): Reload config file...");
	return send_command_to_daemon_via_fifo(PIPECMD_RELOAD_CONFIG, pipePath);
}

static inline bool
queue_initial_focus_prev(const char *pipePath) {
	printfdf(false, "(): Set initial focus to previous selection...");
	return send_command_to_daemon_via_fifo(PIPECMD_QUEUE_FI_PREV, pipePath);
}

static inline bool
queue_initial_focus_next(const char *pipePath) {
	printfdf(false, "(): Set initial focus to next selection...");
	return send_command_to_daemon_via_fifo(PIPECMD_QUEUE_FI_NEXT, pipePath);
}

static inline bool
activate_switcher(const char *pipePath) {
	printfdf(false, "(): Activating switcher...");
	return send_command_to_daemon_via_fifo(PIPECMD_ACTIVATE_SWITCHER, pipePath);
}

static inline bool
activate_expose(const char *pipePath) {
	printfdf(false, "(): Activating expose...");
	return send_command_to_daemon_via_fifo(PIPECMD_ACTIVATE_EXPOSE, pipePath);
}

static inline bool
activate_paging(const char *pipePath) {
	printfdf(false, "(): Activating paging...");
	return send_command_to_daemon_via_fifo(PIPECMD_ACTIVATE_PAGING, pipePath);
}

static inline bool
exit_daemon(const char *pipePath) {
	printfdf(false, "(): Killing daemon...");
	return send_command_to_daemon_via_fifo(PIPECMD_EXIT_DAEMON, pipePath);
}

static inline bool
deactivate(const char *pipePath) {
	printfdf(false, "(): Deactivating...");
	return send_command_to_daemon_via_fifo(PIPECMD_DEACTIVATE, pipePath);
}

static inline bool
toggle_switcher(const char *pipePath) {
	printfdf(false, "(): Toggling switcher...");
	return send_command_to_daemon_via_fifo(PIPECMD_TOGGLE_SWITCHER, pipePath);
}

static inline bool
toggle_expose(const char *pipePath) {
	printfdf(false, "(): Toggling expose...");
	return send_command_to_daemon_via_fifo(PIPECMD_TOGGLE_EXPOSE, pipePath);
}

static inline bool
toggle_paging(const char *pipePath) {
	printfdf(false, "(): Toggling paging...");
	return send_command_to_daemon_via_fifo(PIPECMD_TOGGLE_PAGING, pipePath);
}

/**
 * @brief Xlib error handler function.
 */
static int
xerror(Display *dpy, XErrorEvent *ev) {
	session_t * const ps = ps_g;

	int o;
	const char *name = "Unknown";

#define CASESTRRET2(s)	 case s: name = #s; break

	o = ev->error_code - ps->xinfo.fixes_err_base;
	switch (o) {
		CASESTRRET2(BadRegion);
	}

	o = ev->error_code - ps->xinfo.damage_err_base;
	switch (o) {
		CASESTRRET2(BadDamage);
	}

	o = ev->error_code - ps->xinfo.render_err_base;
	switch (o) {
		CASESTRRET2(BadPictFormat);
		CASESTRRET2(BadPicture);
		CASESTRRET2(BadPictOp);
		CASESTRRET2(BadGlyphSet);
		CASESTRRET2(BadGlyph);
	}

	switch (ev->error_code) {
		CASESTRRET2(BadAccess);
		CASESTRRET2(BadAlloc);
		CASESTRRET2(BadAtom);
		CASESTRRET2(BadColor);
		CASESTRRET2(BadCursor);
		CASESTRRET2(BadDrawable);
		CASESTRRET2(BadFont);
		CASESTRRET2(BadGC);
		CASESTRRET2(BadIDChoice);
		CASESTRRET2(BadImplementation);
		CASESTRRET2(BadLength);
		CASESTRRET2(BadMatch);
		CASESTRRET2(BadName);
		CASESTRRET2(BadPixmap);
		CASESTRRET2(BadRequest);
		CASESTRRET2(BadValue);
		CASESTRRET2(BadWindow);
	}

#undef CASESTRRET2

	print_timestamp(ps);
	{
		char buf[BUF_LEN] = "";
		XGetErrorText(ps->dpy, ev->error_code, buf, BUF_LEN);
		printfef(false, "(): error %d (%s) request %d minor %d serial %lu (\"%s\")",
				ev->error_code, name, ev->request_code,
				ev->minor_code, ev->serial, buf);
	}

	return 0;
}

#ifndef SKIPPYXD_VERSION
#define SKIPPYXD_VERSION "unknown"
#endif

static void
show_help() {
	fputs("skippy-xd " SKIPPYXD_VERSION "\n"
			"Usage: skippy-xd [command]\n\n"
			"The available commands are:\n"
			"  [no command]        - activate expose once without daemon.\n"
			"  --config-reload     - reload configuration file; currently the file path must remain unchanged.\n"
			"  --config            - read the specified configuration file.\n"
			"  --start-daemon      - starts the daemon running.\n"
			"  --stop-daemon       - stops the daemon running.\n"
			"  --activate-switcher - connects to daemon and activate switcher.\n"
			"  --toggle-switcher   - connects to daemon and toggle switcher.\n"
			"  --activate-expose   - connects to daemon and activate expose.\n"
			"  --toggle-expose     - connects to daemon and toggle expose.\n"
			"  --activate-paging   - connects to daemon and activate paging.\n"
			"  --toggle-paging     - connects to daemon and toggle paging.\n"
			"  --deactivate        - connects to daemon and deactivate switcher, expose or paging.\n"
			"  --prev              - focus window to previous.\n"
			"  --next              - focus window to next.\n"
			// "  --test                      - Temporary development testing. To be removed.\n"
			"\n"
			"  --help              - show this message.\n"
			"  -S                  - enable debugging logs.\n"
			, stdout);
#ifdef CFG_LIBPNG
	spng_about(stdout);
#endif
}

// static void
// developer_tests() {
// 	fputs("skippy-xd (" SKIPPYXD_VERSION ")\n", stdout);
// 	fputs("Running: developer tests\n", stdout);
// 	fputs("\n", stdout);

// 	char *str = "one two three four,five six!!!!";
//     fprintf(stdout, "testing str_count_words(), str=\"%s\"\n", str);

// 	int num_words = str_count_words(str);
//     fprintf(stdout, "num_words=%i\n", num_words);

// 	fputs("done.\n", stdout);
// 	fputs("\n", stdout);

// 	fputs("sleep(0.3);\n", stdout);
// 	usleep(0.3 *1000000);
// 	fputs("done.\n", stdout);


// 	fputs("\n", stdout);
// 	fputs("Finished. Exiting.\n"
// 			, stdout);
// }

static inline bool
init_xexts(session_t *ps) {
	Display * const dpy = ps->dpy;
#ifdef CFG_XINERAMA
	ps->xinfo.xinerama_exist = XineramaQueryExtension(dpy,
			&ps->xinfo.xinerama_ev_base, &ps->xinfo.xinerama_err_base);
# ifdef DEBUG_XINERAMA
	printfef(true, "(): Xinerama extension: %s",
			(ps->xinfo.xinerama_exist ? "yes": "no"));
# endif /* DEBUG_XINERAMA */
#endif /* CFG_XINERAMA */

	if(!XDamageQueryExtension(dpy,
				&ps->xinfo.damage_ev_base, &ps->xinfo.damage_err_base)) {
		printfef(true, "(): FATAL: XDamage extension not found.");
		return false;
	}

	if(!XCompositeQueryExtension(dpy, &ps->xinfo.composite_ev_base,
				&ps->xinfo.composite_err_base)) {
		printfef(true, "(): FATAL: XComposite extension not found.");
		return false;
	}

	if(!XRenderQueryExtension(dpy,
				&ps->xinfo.render_ev_base, &ps->xinfo.render_err_base)) {
		printfef(true, "(): FATAL: XRender extension not found.");
		return false;
	}

	if(!XFixesQueryExtension(dpy,
				&ps->xinfo.fixes_ev_base, &ps->xinfo.fixes_err_base)) {
		printfef(true, "(): FATAL: XFixes extension not found.");
		return false;
	}

	return true;
}

/**
 * @brief Check if a file exists.
 *
 * access() may not actually be reliable as according to glibc manual it uses
 * real user ID instead of effective user ID, but stat() is just too costly
 * for this purpose.
 */
static inline bool
fexists(const char *path) {
	return !access(path, F_OK);
}

/**
 * @brief Find path to configuration file.
 */
static inline char *
get_cfg_path(void) {
	static const char *PATH_CONFIG_HOME_SUFFIX = "/skippy-xd/skippy-xd.rc";
	static const char *PATH_CONFIG_HOME = "/.config";
	static const char *PATH_CONFIG_SYS_SUFFIX = "/skippy-xd.rc";
	static const char *PATH_CONFIG_SYS = "/etc/xdg";

	char *path = NULL;
	const char *dir = NULL;

	// Check $XDG_CONFIG_HOME
	if ((dir = getenv("XDG_CONFIG_HOME")) && strlen(dir)) {
		path = mstrjoin(dir, PATH_CONFIG_HOME_SUFFIX);
		if (fexists(path))
			goto get_cfg_path_found;
		free(path);
		path = NULL;
	}
	// Check ~/.config
	if ((dir = getenv("HOME")) && strlen(dir)) {
		path = mstrjoin3(dir, PATH_CONFIG_HOME, PATH_CONFIG_HOME_SUFFIX);
		if (fexists(path))
			goto get_cfg_path_found;
		free(path);
		path = NULL;
	}

	// Look in env $XDG_CONFIG_DIRS
	if ((dir = getenv("XDG_CONFIG_DIRS")))
	{
		char *dir_free = mstrdup(dir);
		char *part = strtok(dir_free, ":");
		while (part) {
			path = mstrjoin(part, PATH_CONFIG_SYS_SUFFIX);
			if (fexists(path))
			{
				free(dir_free);
				goto get_cfg_path_found;
			}
			free(path);
			path = NULL;
			part = strtok(NULL, ":");
		}
		free(dir_free);
	}

	// Use the default location if env var not set
	{
		dir = PATH_CONFIG_SYS;
		path = mstrjoin(dir, PATH_CONFIG_SYS_SUFFIX);
		if (fexists(path))
			goto get_cfg_path_found;
		free(path);
		path = NULL;
	}

	return NULL;

get_cfg_path_found:
	return path;
}

static void
parse_args(session_t *ps, int argc, char **argv, bool first_pass) {
	enum options {
		OPT_CONFIG = 256,
		OPT_CONFIG_RELOAD,
		OPT_ACTV_SWITCHER,
		OPT_TGG_SWITCHER,
		OPT_ACTV_EXPOSE,
		OPT_TGG_EXPOSE,
		OPT_ACTV_PAGING,
		OPT_TGG_PAGING,
		OPT_DEACTV,
		OPT_DM_START,
		OPT_DM_STOP,
		OPT_PREV,
		OPT_NEXT,
	};
	static const char * opts_short = "hS";
	static const struct option opts_long[] = {
		{ "help",                     no_argument,       NULL, 'h' },
		{ "config",                   required_argument, NULL, OPT_CONFIG },
		{ "config-reload",            no_argument,       NULL, OPT_CONFIG_RELOAD },
		{ "activate-switcher",        no_argument,       NULL, OPT_ACTV_SWITCHER },
		{ "toggle-switcher",          no_argument,       NULL, OPT_TGG_SWITCHER },
		{ "activate-expose",          no_argument,       NULL, OPT_ACTV_EXPOSE },
		{ "toggle-expose",            no_argument,       NULL, OPT_TGG_EXPOSE },
		{ "activate-paging",          no_argument,       NULL, OPT_ACTV_PAGING },
		{ "toggle-paging",            no_argument,       NULL, OPT_TGG_PAGING },
		{ "deactivate",               no_argument,       NULL, OPT_DEACTV },
		{ "start-daemon",             no_argument,       NULL, OPT_DM_START },
		{ "stop-daemon",              no_argument,       NULL, OPT_DM_STOP },
		{ "prev",                     no_argument,       NULL, OPT_PREV },
		{ "next",                     no_argument,       NULL, OPT_NEXT },
		// { "test",                     no_argument,       NULL, 't' },
		{ NULL, no_argument, NULL, 0 }
	};

	int o = 0;
	optind = 1;

	// Only parse --config in first pass
	if (first_pass) {
		while ((o = getopt_long(argc, argv, opts_short, opts_long, NULL)) >= 0) {
			switch (o) {
#define T_CASEBOOL(idx, option) case idx: ps->o.option = true; break
				case OPT_CONFIG:
					ps->o.config_path = mstrdup(optarg);
					break;
				case OPT_PREV:
					ps->o.focus_initial = FI_PREV;
					printfdf(false, "(): ps->o.focus_initial=%i\n", ps->o.focus_initial);
					break;
				case OPT_NEXT:
					ps->o.focus_initial = FI_NEXT;
					printfdf(false, "(): ps->o.focus_initial=%i\n", ps->o.focus_initial);
					break;
				case 'S':
					debuglog = true;
					break;
				// case 't':
				// 	developer_tests();
				// 	exit('t' == o ? RET_SUCCESS: RET_BADARG);
				case '?':
				case 'h':
					show_help();
					// Return a non-zero value on unrecognized option
					exit('h' == o ? RET_SUCCESS: RET_BADARG);
				default:
					break;
			}
		}
		return;
	}

	while ((o = getopt_long(argc, argv, opts_short, opts_long, NULL)) >= 0) {
		switch (o) {
			case 'S': break;
			case OPT_CONFIG: break;
			case OPT_CONFIG_RELOAD:
				ps->o.mode = PROGMODE_RELOAD_CONFIG;
				break;
			case OPT_ACTV_SWITCHER:
				ps->o.mode = PROGMODE_ACTV_SWITCHER;
				break;
			case OPT_TGG_SWITCHER:
				ps->o.mode = PROGMODE_TGG_SWITCHER;
				break;
			case OPT_ACTV_EXPOSE:
				ps->o.mode = PROGMODE_ACTV_EXPOSE;
				break;
			case OPT_TGG_EXPOSE:
				ps->o.mode = PROGMODE_TGG_EXPOSE;
				break;
			case OPT_ACTV_PAGING:
				ps->o.mode = PROGMODE_ACTV_PAGING;
				break;
			case OPT_TGG_PAGING:
				ps->o.mode = PROGMODE_TGG_PAGING;
				break;
			case OPT_DEACTV:
				ps->o.mode = PROGMODE_DEACTV;
				break;
			case OPT_DM_STOP:
				ps->o.mode = PROGMODE_DM_STOP;
				break;
			T_CASEBOOL(OPT_DM_START, runAsDaemon);
			case OPT_PREV: break;
			case OPT_NEXT: break;
#undef T_CASEBOOL
			default:
				printfef(false, "(0): Unimplemented option %d.", o);
				exit(RET_UNKNOWN);
		}
	}
}

int
load_config_file(session_t *ps)
{
    dlist *config = NULL;
    {
        bool user_specified_config = ps->o.config_path;
        if (!ps->o.config_path)
            ps->o.config_path = get_cfg_path();
        if (ps->o.config_path)
            config = config_load(ps->o.config_path);
        else
            printfef(true, "(): WARNING: No configuration file found.");
        if (!config && user_specified_config)
            return 1;
    }

    char *lc_numeric_old = mstrdup(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");

    // Read configuration into ps->o, because searching all the time is much
    // less efficient, may introduce inconsistent default value, and
    // occupies a lot more memory for non-string types.
	{
	// Appending UID to the file name
		// Dash-separated initial single-digit string
		int xid = XConnectionNumber(ps->dpy), pipeStrLen = 3;
		{
			int num;
			for (num = xid; num >= 10; num /= 10) pipeStrLen++;
		}

		const char * path = config_get(config, "general", "pipePath", "/tmp/skippy-xd-fifo");
		pipeStrLen += strlen(path);

		char * pipePath = malloc (pipeStrLen * sizeof(unsigned char));
		sprintf(pipePath, "%s-%i", path, xid);

		ps->o.pipePath = pipePath;
	}
    ps->o.normal_tint = mstrdup(config_get(config, "normal", "tint", "black"));
    ps->o.highlight_tint = mstrdup(config_get(config, "highlight", "tint", "#101020"));
    ps->o.shadow_tint = mstrdup(config_get(config, "shadow", "tint", "#010101"));
    ps->o.tooltip_border = mstrdup(config_get(config, "tooltip", "border", "#e0e0e0"));
    ps->o.tooltip_background = mstrdup(config_get(config, "tooltip", "background", "#404040"));
    ps->o.tooltip_text = mstrdup(config_get(config, "tooltip", "text", "#e0e0e0"));
    ps->o.tooltip_textShadow = mstrdup(config_get(config, "tooltip", "textShadow", "black"));
    ps->o.tooltip_font = mstrdup(config_get(config, "tooltip", "font", "fixed-11:weight=bold"));

    // load keybindings settings
    ps->o.bindings_keysUp = mstrdup(config_get(config, "bindings", "keysUp", "Up w"));
    ps->o.bindings_keysDown = mstrdup(config_get(config, "bindings", "keysDown", "Down s"));
    ps->o.bindings_keysLeft = mstrdup(config_get(config, "bindings", "keysLeft", "Left a"));
    ps->o.bindings_keysRight = mstrdup(config_get(config, "bindings", "keysRight", "Right Tab d"));
    ps->o.bindings_keysPrev = mstrdup(config_get(config, "bindings", "keysPrev", "p b"));
    ps->o.bindings_keysNext = mstrdup(config_get(config, "bindings", "keysNext", "n f"));
    ps->o.bindings_keysExitCancelOnPress = mstrdup(config_get(config, "bindings", "keysExitCancelOnPress", "Escape BackSpace x q"));
    ps->o.bindings_keysExitCancelOnRelease = mstrdup(config_get(config, "bindings", "keysExitCancelOnRelease", ""));
    ps->o.bindings_keysExitSelectOnPress = mstrdup(config_get(config, "bindings", "keysExitSelectOnPress", "Return space"));
    ps->o.bindings_keysExitSelectOnRelease = mstrdup(config_get(config, "bindings", "keysExitSelectOnRelease", "Super_L Super_R Alt_L Alt_R ISO_Level3_Shift"));
    ps->o.bindings_keysReverseDirection = mstrdup(config_get(config, "bindings", "keysReverseDirection", "Tab"));
    ps->o.bindings_modifierKeyMasksReverseDirection = mstrdup(config_get(config, "bindings", "modifierKeyMasksReverseDirection", "ShiftMask ControlMask"));

    // print an error message for any key bindings that aren't recognized
    check_keysyms(ps->o.config_path, ": [bindings] keysUp =", ps->o.bindings_keysUp);
    check_keysyms(ps->o.config_path, ": [bindings] keysDown =", ps->o.bindings_keysDown);
    check_keysyms(ps->o.config_path, ": [bindings] keysLeft =", ps->o.bindings_keysLeft);
    check_keysyms(ps->o.config_path, ": [bindings] keysRight =", ps->o.bindings_keysRight);
    check_keysyms(ps->o.config_path, ": [bindings] keysPrev =", ps->o.bindings_keysPrev);
    check_keysyms(ps->o.config_path, ": [bindings] keysNext =", ps->o.bindings_keysNext);
    check_keysyms(ps->o.config_path, ": [bindings] keysExitCancelOnPress =", ps->o.bindings_keysExitCancelOnPress);
    check_keysyms(ps->o.config_path, ": [bindings] keysExitCancelOnRelease =", ps->o.bindings_keysExitCancelOnRelease);
    check_keysyms(ps->o.config_path, ": [bindings] keysExitSelectOnPress =", ps->o.bindings_keysExitSelectOnPress);
    check_keysyms(ps->o.config_path, ": [bindings] keysExitSelectOnRelease =", ps->o.bindings_keysExitSelectOnRelease);
    check_keysyms(ps->o.config_path, ": [bindings] keysReverseDirection =", ps->o.bindings_keysReverseDirection);
    check_modmasks(ps->o.config_path, ": [bindings] modifierKeyMasksReverseDirection =", ps->o.bindings_modifierKeyMasksReverseDirection);

	if (!parse_cliop(ps, config_get(config, "bindings", "miwMouse1", "focus"), &ps->o.bindings_miwMouse[1])
			|| !parse_cliop(ps, config_get(config, "bindings", "miwMouse2", "close-ewmh"), &ps->o.bindings_miwMouse[2])
			|| !parse_cliop(ps, config_get(config, "bindings", "miwMouse3", "iconify"), &ps->o.bindings_miwMouse[3])) {
		return RET_BADARG;
	}

	{
		const char *s = config_get(config, "general", "exposeLayout", NULL);
		if (s) {
			if (strcmp(s,"boxy") == 0) {
				ps->o.exposeLayout = LAYOUT_BOXY;
			}
			else if (strcmp(s,"xd") == 0) {
				ps->o.exposeLayout = LAYOUT_XD;
			}
			else {
				ps->o.exposeLayout = LAYOUT_BOXY;
			}
		}
		else
			ps->o.exposeLayout = LAYOUT_BOXY;
    }

    config_get_int_wrap(config, "general", "distance", &ps->o.distance, 1, INT_MAX);
    config_get_bool_wrap(config, "general", "useNetWMFullscreen", &ps->o.useNetWMFullscreen);
    config_get_bool_wrap(config, "general", "ignoreSkipTaskbar", &ps->o.ignoreSkipTaskbar);
    config_get_bool_wrap(config, "general", "acceptOvRedir", &ps->o.acceptOvRedir);
    config_get_bool_wrap(config, "general", "acceptWMWin", &ps->o.acceptWMWin);
    config_get_double_wrap(config, "general", "updateFreq", &ps->o.updateFreq, -1000.0, 1000.0);
    config_get_int_wrap(config, "general", "animationDuration", &ps->o.animationDuration, 0, 2000);
    config_get_bool_wrap(config, "general", "lazyTrans", &ps->o.lazyTrans);
    config_get_bool_wrap(config, "general", "includeFrame", &ps->o.includeFrame);
    config_get_bool_wrap(config, "general", "allowUpscale", &ps->o.allowUpscale);
	config_get_int_wrap(config, "general", "cornerRadius", &ps->o.cornerRadius, 0, INT_MAX);
    config_get_int_wrap(config, "general", "preferredIconSize", &ps->o.preferredIconSize, 1, INT_MAX);
    config_get_bool_wrap(config, "general", "showAllDesktops", &ps->o.showAllDesktops);
    config_get_bool_wrap(config, "general", "showShadow", &ps->o.showShadow);
    config_get_bool_wrap(config, "general", "movePointerOnStart", &ps->o.movePointerOnStart);
    config_get_bool_wrap(config, "general", "movePointerOnSelect", &ps->o.movePointerOnSelect);
    config_get_bool_wrap(config, "general", "movePointerOnRaise", &ps->o.movePointerOnRaise);
    config_get_bool_wrap(config, "general", "switchDesktopOnActivate", &ps->o.switchDesktopOnActivate);
    config_get_bool_wrap(config, "xinerama", "showAll", &ps->o.xinerama_showAll);
    config_get_int_wrap(config, "normal", "tintOpacity", &ps->o.normal_tintOpacity, 0, 256);
    config_get_int_wrap(config, "normal", "opacity", &ps->o.normal_opacity, 0, 256);
    config_get_int_wrap(config, "highlight", "tintOpacity", &ps->o.highlight_tintOpacity, 0, 256);
    config_get_int_wrap(config, "highlight", "opacity", &ps->o.highlight_opacity, 0, 256);
    config_get_int_wrap(config, "shadow", "tintOpacity", &ps->o.shadow_tintOpacity, 0, 256);
    config_get_int_wrap(config, "shadow", "opacity", &ps->o.shadow_opacity, 0, 256);
    config_get_bool_wrap(config, "tooltip", "show", &ps->o.tooltip_show);
    config_get_bool_wrap(config, "tooltip", "followsMouse", &ps->o.tooltip_followsMouse);
    config_get_int_wrap(config, "tooltip", "offsetX", &ps->o.tooltip_offsetX, INT_MIN, INT_MAX);
    config_get_int_wrap(config, "tooltip", "offsetY", &ps->o.tooltip_offsetY, INT_MIN, INT_MAX);
    if (!parse_align_full(ps, config_get(config, "tooltip", "align", "left"), &ps->o.tooltip_align))
        return RET_BADARG;
    config_get_int_wrap(config, "tooltip", "tintOpacity", &ps->o.highlight_tintOpacity, 0, 256);
    config_get_int_wrap(config, "tooltip", "opacity", &ps->o.tooltip_opacity, 0, 256);
    {
        static client_disp_mode_t DEF_CLIDISPM[] = {
            CLIDISP_THUMBNAIL, CLIDISP_ZOMBIE, CLIDISP_ICON, CLIDISP_FILLED, CLIDISP_NONE
        };

        static client_disp_mode_t DEF_CLIDISPM_ICON[] = {
            CLIDISP_THUMBNAIL_ICON, CLIDISP_THUMBNAIL, CLIDISP_ZOMBIE_ICON,
            CLIDISP_ZOMBIE, CLIDISP_ICON, CLIDISP_FILLED, CLIDISP_NONE
        };

        bool thumbnail_icons = false;
        config_get_bool_wrap(config, "general", "showIconsOnThumbnails", &thumbnail_icons);
        if (thumbnail_icons) {
            ps->o.clientDisplayModes = allocchk(malloc(sizeof(DEF_CLIDISPM_ICON)));
            memcpy(ps->o.clientDisplayModes, &DEF_CLIDISPM_ICON, sizeof(DEF_CLIDISPM_ICON));
        }
        else {
            ps->o.clientDisplayModes = allocchk(malloc(sizeof(DEF_CLIDISPM)));
            memcpy(ps->o.clientDisplayModes, &DEF_CLIDISPM, sizeof(DEF_CLIDISPM));
        }
    }
    {
        const char *sspec = config_get(config, "general", "background", NULL);
        if (sspec && strlen(sspec)) {
            pictspec_t spec = PICTSPECT_INIT;
            if (!parse_pictspec(ps, sspec, &spec))
                return RET_BADARG;
            int root_width = DisplayWidth(ps->dpy, ps->screen),
                    root_height = DisplayHeight(ps->dpy, ps->screen);
            if (!(spec.twidth || spec.theight)) {
                spec.twidth = root_width;
                spec.theight = root_height;
            }
            pictw_t *p = simg_load_s(ps, &spec);
            if (!p)
                exit(1);
            if (p->width != root_width || p->height != root_height)
                ps->o.background = simg_postprocess(ps, p, PICTPOSP_ORIG,
                        root_width, root_height, spec.alg, spec.valg, &spec.c);
            else
                ps->o.background = p;
            free_pictspec(ps, &spec);
        }
		else {
			ps->o.background = None;
		}
    }
    if (!parse_pictspec(ps, config_get(config, "general", "iconFillSpec", "orig mid mid #FFFFFF"), &ps->o.iconFillSpec)
            || !parse_pictspec(ps, config_get(config, "general", "fillSpec", "orig mid mid #FFFFFF"), &ps->o.fillSpec))
        return RET_BADARG;
    if (!simg_cachespec(ps, &ps->o.fillSpec))
        return RET_BADARG;
    if (ps->o.iconFillSpec.path
            && !(ps->o.iconDefault = simg_load(ps, ps->o.iconFillSpec.path,
                    PICTPOSP_SCALEK, ps->o.preferredIconSize, ps->o.preferredIconSize,
                    ALIGN_MID, ALIGN_MID, NULL)))
        return RET_BADARG;

    setlocale(LC_NUMERIC, lc_numeric_old);
    free(lc_numeric_old);
    config_free(config);

	return RET_SUCCESS;
}

int main(int argc, char *argv[]) {
	session_t *ps = NULL;
	int ret = RET_SUCCESS;
	Display *dpy = NULL;

	/* Set program locale */
	setlocale (LC_ALL, "");

	// Initialize session structure
	{
		static const session_t SESSIONT_DEF = SESSIONT_INIT;
		ps_g = ps = allocchk(malloc(sizeof(session_t)));
		memcpy(ps, &SESSIONT_DEF, sizeof(session_t));
		gettimeofday(&ps->time_start, NULL);
	}

	// First pass
	parse_args(ps, argc, argv, true);

	// Open connection to X
	if (!(ps->dpy = dpy = XOpenDisplay(NULL))) {
		printfef(true, "(): FATAL: Couldn't connect to display.");
		ret = RET_XFAIL;
		goto main_end;
	}
	if (!init_xexts(ps)) {
		ret = RET_XFAIL;
		goto main_end;
	}
	if (debuglog)
		XSynchronize(ps->dpy, True);
	XSetErrorHandler(xerror);

	ps->screen = DefaultScreen(dpy);
	ps->root = RootWindow(dpy, ps->screen);

	wm_get_atoms(ps);

	int config_load_ret = load_config_file(ps);
	if (config_load_ret != 0)
		return config_load_ret;

	// Second pass
	parse_args(ps, argc, argv, false);

	printfdf(false, "(): after 2nd pass:  ps->o.focus_initial =  %i", ps->o.focus_initial);

	const char* pipePath = ps->o.pipePath;

	// Handle special modes
	switch (ps->o.mode) {
		case PROGMODE_NORMAL:
			break;
		case PROGMODE_ACTV_SWITCHER:
		case PROGMODE_ACTV_EXPOSE:
		case PROGMODE_ACTV_PAGING:
			if(ps->o.focus_initial)
			{
				if(ps->o.focus_initial == FI_PREV)
					queue_initial_focus_prev(pipePath);

				else if(ps->o.focus_initial == FI_NEXT)
					queue_initial_focus_next(pipePath);

				// we must pause slightly, otherwise will miss next read() call in this loop()
				usleep(10000);
			}
			if (ps->o.mode == PROGMODE_ACTV_SWITCHER)
				activate_switcher(pipePath);
			else if (ps->o.mode == PROGMODE_ACTV_EXPOSE)
				activate_expose(pipePath);
			else if (ps->o.mode == PROGMODE_ACTV_PAGING)
				activate_paging(pipePath);
			goto main_end;
		case PROGMODE_TGG_SWITCHER:
		case PROGMODE_TGG_EXPOSE:
		case PROGMODE_TGG_PAGING:
			if(ps->o.focus_initial)
			{
				if(ps->o.focus_initial == FI_PREV)
					queue_initial_focus_prev(pipePath);
				else if(ps->o.focus_initial == FI_NEXT)
					queue_initial_focus_next(pipePath);

				// we must pause slightly, otherwise will miss next read() call in this loop()
				usleep(10000);
			}
			if (ps->o.mode == PROGMODE_TGG_SWITCHER)
				toggle_switcher(pipePath);
			else if (ps->o.mode == PROGMODE_TGG_EXPOSE)
				toggle_expose(pipePath);
			else if (ps->o.mode == PROGMODE_TGG_PAGING)
				toggle_paging(pipePath);
			goto main_end;
		case PROGMODE_DEACTV:
			deactivate(pipePath);
			goto main_end;
		case PROGMODE_RELOAD_CONFIG:
			queue_reload_config(pipePath);
			goto main_end;
		case PROGMODE_DM_STOP:
			exit_daemon(pipePath);
			goto main_end;
	}

	if (!wm_check(ps)) {
		/* ret = 1;
		goto main_end; */
	}

	// Main branch
	MainWin *mw = mainwin_create(ps);
	if (!mw) {
		printfef(true, "(): FATAL: Couldn't create main window.");
		ret = 1;
		goto main_end;
	}
	ps->mainwin = mw;

	XSelectInput(ps->dpy, ps->root, PropertyChangeMask);

	// Daemon mode
	if (ps->o.runAsDaemon) {

		printfdf(false, "(): Running as daemon...");

		{
			int result = mkfifo(pipePath, S_IRUSR | S_IWUSR);
			if (result < 0  && EEXIST != errno) {
				printfef(true, "(): Failed to create named pipe \"%s\": %d", pipePath, result);
				perror("mkfifo");
				ret = 2;
				goto main_end;
			}
		}

		// Opening pipe
		if (!open_pipe(ps, NULL)) {
			ret = 2;
			goto main_end;
		}
		assert(ps->fd_pipe >= 0);

		{
			char *buf[BUF_LEN];
			while (read(ps->fd_pipe, buf, sizeof(buf)))
				continue;
			printfdf(false, "(): Finished flushing pipe \"%s\".", pipePath);
		}

		daemon_count_clients(mw, 0);

		foreach_dlist(mw->clients) {
			clientwin_update((ClientWin *) iter->data);
			clientwin_update2((ClientWin *) iter->data);
		}

		mainloop(ps, false);
	}
	else {
		printfdf(false, "(): running once then quitting...");
		mainloop(ps, true);
	}

main_end:
	// Free session data
	if (ps) {
		// Free configuration strings
		{
			free(ps->o.config_path);
			free(ps->o.pipePath);
			free(ps->o.clientDisplayModes);
			free(ps->o.normal_tint);
			free(ps->o.highlight_tint);
			free(ps->o.shadow_tint);
			free(ps->o.tooltip_border);
			free(ps->o.tooltip_background);
			free(ps->o.tooltip_text);
			free(ps->o.tooltip_textShadow);
			free(ps->o.tooltip_font);
			free_pictw(ps, &ps->o.background);
			free_pictw(ps, &ps->o.iconDefault);
			free_pictspec(ps, &ps->o.iconFillSpec);
			free_pictspec(ps, &ps->o.fillSpec);
			free(ps->o.bindings_keysUp);
			free(ps->o.bindings_keysDown);
			free(ps->o.bindings_keysLeft);
			free(ps->o.bindings_keysRight);
			free(ps->o.bindings_keysPrev);
			free(ps->o.bindings_keysNext);
			free(ps->o.bindings_keysExitCancelOnPress);
			free(ps->o.bindings_keysExitCancelOnRelease);
			free(ps->o.bindings_keysExitSelectOnPress);
			free(ps->o.bindings_keysExitSelectOnRelease);
			free(ps->o.bindings_keysReverseDirection);
			free(ps->o.bindings_modifierKeyMasksReverseDirection);
		}

		if (ps->fd_pipe >= 0)
			close(ps->fd_pipe);

		if (ps->mainwin)
			mainwin_destroy(ps->mainwin);

		if (ps->dpy)
			XCloseDisplay(dpy);

		free(ps);
	}

	return ret;
}
