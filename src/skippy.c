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

enum pipe_cmd_t {
	// Not ordered properly for backward compatibility
	PIPECMD_ACTIVATE_WINDOW_PICKER = 1,
	PIPECMD_EXIT_RUNNING_DAEMON,
	PIPECMD_DEACTIVATE_WINDOW_PICKER,
	PIPECMD_TOGGLE_WINDOW_PICKER,
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

	printfef("(\"%s\"): Unrecognized operation.", str);
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

	printfef("(\"%s\"): Unrecognized operation.", str);
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

	printfef("(\"%s\"): Unrecognized operation.", str);
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
			printfef("(\"%s\"): Failed to read color segment.", s);
			return 0;
		}
		if (!(next = parse_color_sub((s += next), &pc->alpha)))
			pc->alpha = 0xffff;
		s += next;
		return s - sorig;
	}

	printfef("(\"%s\"): Unrecognized color format.", s);
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
				printfef("(\"%s\"): Invalid height.", s);
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
		printfef("(\"%s\"): Trailing characters.", s);
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

static client_disp_mode_t *
parse_client_disp_mode(session_t *ps, const char *s) {
	static const struct {
		client_disp_mode_t mode;
		const char *name;
	} ENTRIES[] = {
		{ CLIDISP_NONE, "none" },
		{ CLIDISP_FILLED, "filled" },
		{ CLIDISP_ICON, "icon" },
		{ CLIDISP_THUMBNAIL, "thumbnail" },
		{ CLIDISP_THUMBNAIL_ICON, "thumbnail-icon" },
	};
	static const int ALLOC_STEP = 3;
	int capacity = 0;
	client_disp_mode_t *ret = NULL;

	int i = 0;
	for (; s; ++i) {
		char *word = NULL;
		s = str_get_word(s, &word);
		if (!word)
			break;
		if (capacity <= i + 1) {
			capacity += ALLOC_STEP;
			ret = srealloc(ret, capacity, client_disp_mode_t);
		}
		{
			bool found = false;
			for (int j = 0; j < CARR_LEN(ENTRIES); ++j)
				if (!strcmp(word, ENTRIES[j].name)) {
					found = true;
					ret[i] = ENTRIES[j].mode;
				}
			if (!found) {
				printfef("(\"%s\"): Invalid mode \"%s\" ignored.", s, word);
				--i;
			}
		}
		free(word);
	}

	if (!i) {
		free(ret);
	}
	else {
		ret[i] = CLIDISP_NONE;
	}

	return ret;
}

static dlist *
update_clients(MainWin *mw, dlist *clients, Bool *touched) {
	dlist *stack = dlist_first(wm_get_stack(mw->ps));
	clients = dlist_first(clients);

	if (touched)
		*touched = False;
	
	// Terminate clients that are no longer managed
	for (dlist *iter = clients; iter; ) {
		ClientWin *cw = (ClientWin *) iter->data;
		if (dlist_find_data(stack, (void *) cw->src.window)
				&& clientwin_update(cw)) {
			iter = iter->next;
		}
		else {
			dlist *tmp = iter->next;
			clientwin_destroy((ClientWin *) iter->data, True);
			clients = dlist_remove(iter);
			iter = tmp;
			if (touched)
				*touched = True;
		}
	}
	XFlush(mw->ps->dpy);

	// Add new clients
	foreach_dlist (stack) {
		ClientWin *cw = (ClientWin *)
			dlist_find(clients, clientwin_cmp_func, iter->data);
		if (!cw && ((Window) iter->data) != mw->window) {
			cw = clientwin_create(mw, (Window)iter->data);
			if (!cw) continue;
			clients = dlist_add(clients, cw);
			clientwin_update(cw);
			if (touched)
				*touched = True;
		}
	}

	dlist_free(stack);

	return clients;
}

static dlist *
do_layout(MainWin *mw, dlist *clients, Window focus, Window leader) {
	session_t * const ps = mw->ps;

	long desktop = wm_get_current_desktop(ps);
	float factor;
	
	/* Update the client table, pick the ones we want and sort them */
	clients = update_clients(mw, clients, 0);
	if (!clients) {
		printfef("(): No client windows found.");
		return clients;
	}

	dlist_free(mw->cod);
	mw->cod = NULL;

	{
		dlist *tmp = dlist_first(dlist_find_all(clients,
					(dlist_match_func) clientwin_validate_func, &desktop));
		if (!tmp) {
			printfef("(): No client window on current desktop found.");
			return clients;
		}

		if (leader) {
			mw->cod = dlist_first(dlist_find_all(tmp, clientwin_check_group_leader_func, (void*)&leader));
			dlist_free(tmp);
		}
		else {
			mw->cod = tmp;
		}
	}
	
	if (!mw->cod)
		return clients;
	
	dlist_sort(mw->cod, clientwin_sort_func, 0);
	
	/* Move the mini windows around */
	{
		unsigned int width = 0, height = 0;
		layout_run(mw, mw->cod, &width, &height);
		factor = (float) (mw->width - 100) / width;
		if (factor * height > mw->height - 100)
			factor = (float) (mw->height - 100) / height;
		if (!ps->o.allowUpscale)
			factor = MIN(factor, 1.0f);

		int xoff = (mw->width - (float) width * factor) / 2;
		int yoff = (mw->height - (float) height * factor) / 2;
		mainwin_transform(mw, factor);
		foreach_dlist (mw->cod) {
			clientwin_move((ClientWin *) iter->data, factor, xoff, yoff);
		}
	}

	foreach_dlist(mw->cod) {
		clientwin_update2((ClientWin *) iter->data);
	}

	// Get the currently focused window and select which mini-window to focus
	{
		dlist *iter = dlist_find(mw->cod, clientwin_cmp_func, (void *) focus);
		if (!iter)
			iter = mw->cod;
		mw->focus = (ClientWin *) iter->data;
		mw->focus->focused = 1;
	}

	// Map the client windows
	foreach_dlist (mw->cod) {
		clientwin_map((ClientWin*)iter->data);
	}

	// Unfortunately it does not work...
	focus_miniw_adv(ps, mw->focus, ps->o.movePointerOnStart);

	return clients;
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

	printf("(): Failed to find window for event type %d. Troubles ahead.",
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
	printfd("Event %-13.13s wid %#010lx %s", name, wid, wextra);
}

static bool
skippy_run_init(MainWin *mw, Window leader) {
	session_t *ps = mw->ps;

	// Do this window before main window gets mapped
	mw->revert_focus_win = wm_get_focused(ps);

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

	mw->clients = do_layout(mw, mw->clients, mw->revert_focus_win, leader);
	if (!mw->cod) {
		printfef("(): Failed to build layout.");
		return false;
	}

	/* Map the main window and run our event loop */
	if (!ps->o.lazyTrans)
		mainwin_map(mw);
	XFlush(ps->dpy);

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
		printfef("(): Failed to open pipe \"%s\": %d", ps->o.pipePath, errno);
		perror("open");
	}

	return false;
}

static void
mainloop(session_t *ps, bool activate_on_start) {
	MainWin *mw = NULL;
	bool die = false;
	bool activate = activate_on_start;
	bool refocus = false;
	bool pending_damage = false;
	long last_rendered = 0L;

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
			if (skippy_run_init(ps->mainwin, None)) {
				last_rendered = time_in_millis();
				mw = ps->mainwin;
				refocus = false;
				pending_damage = false;
			}
		}
		if (mw)
			activate = false;

		// Main window destruction, before poll()
		if (mw && die) {
			// Unmap the main window and all clients, to make sure focus doesn't fall out
			// when we start setting focus on client window
			mainwin_unmap(mw);
			foreach_dlist(mw->cod) { clientwin_unmap((ClientWin *) iter->data); }
			XSync(ps->dpy, False);

			// Focus the client window only after the main window get unmapped and
			// keyboard gets ungrabbed.
			if (mw->client_to_focus) {
				childwin_focus(mw->client_to_focus);
				mw->client_to_focus = NULL;
				refocus = false;
				pending_damage = false;
			}

			// Cleanup
			dlist_free(mw->cod);
			mw->cod = 0;

			if (refocus && mw->revert_focus_win) {
				// No idea why. Plain XSetInputFocus() no longer works after ungrabbing.
				wm_activate_window(ps, mw->revert_focus_win);
				refocus = false;
			}

			// Catch all errors, but remove all events
			XSync(ps->dpy, False);
			XSync(ps->dpy, True);

			mw = NULL;
		}
		if (!mw)
			die = false;
		if (activate_on_start && !mw)
			return;

		// Poll for events
		int timeout = (pending_damage && mw && mw->poll_time > 0 ?
				MAX(0, mw->poll_time + last_rendered - time_in_millis()): -1);
		poll(r_fd, (r_fd[1].fd >= 0 ? 2: 1), timeout);

		if (mw) {
			// Process X events
			XEvent ev = { };
			while (XEventsQueued(ps->dpy, QueuedAfterReading)) {
				XNextEvent(ps->dpy, &ev);
#ifdef DEBUG_EVENTS
				ev_dump(ps, mw, &ev);
#endif
				const Window wid = ev_window(ps, &ev);

				if (MotionNotify == ev.type) {
					if (mw->tooltip && ps->o.tooltip_followsMouse)
						tooltip_move(mw->tooltip,
								ev.xmotion.x_root, ev.xmotion.y_root);
				}
				else if (ev.type == DestroyNotify || ev.type == UnmapNotify) {
					dlist *iter = (wid ? dlist_find(mw->clients, clientwin_cmp_func, (void *) wid): NULL);
					if (iter) {
						ClientWin *cw = (ClientWin *) iter->data;
						if (DestroyNotify != ev.type)
							cw->mode = clientwin_get_disp_mode(ps, cw);
						if (DestroyNotify == ev.type || !cw->mode) {
							mw->clients = dlist_first(dlist_remove(iter));
							iter = dlist_find(mw->cod, clientwin_cmp_func, (void *) wid);
							if (iter)
								mw->cod = dlist_first(dlist_remove(iter));
							clientwin_destroy(cw, true);
							if (!mw->cod) {
								printfef("(): Last client window destroyed/unmapped, "
										"exiting.");
								die = true;
							}
						}
						else {
							free_pixmap(ps, &cw->cpixmap);
							free_picture(ps, &cw->origin);
							free_damage(ps, &cw->damage);
							clientwin_update2(cw);
							clientwin_render(cw);
						}
					}
				}
				else if (ps->xinfo.damage_ev_base + XDamageNotify == ev.type) {
					// XDamageNotifyEvent *d_ev = (XDamageNotifyEvent *) &ev;
					dlist *iter = dlist_find(mw->cod, clientwin_cmp_func,
							(void *) wid);
					pending_damage = true;
					if (iter) {
						if (!mw->poll_time)
							clientwin_repair((ClientWin *)iter->data);
						else
							((ClientWin *)iter->data)->damaged = true;
					}

				}
				else if (KeyRelease == ev.type && (mw->key_q == ev.xkey.keycode
							|| mw->key_escape == ev.xkey.keycode)) {
					if (mw->pressed_key) {
						die = true;
						if (mw->key_escape == ev.xkey.keycode)
							refocus = true;
					}
					else
						report_key_ignored(&ev);
				}
				else if (wid == mw->window)
					die = mainwin_handle(mw, &ev);
				else if (PropertyNotify == ev.type) {
					if (!ps->o.background &&
							(ESETROOT_PMAP_ID == ev.xproperty.atom
							 || _XROOTPMAP_ID == ev.xproperty.atom)) {
						mainwin_update_background(mw);
						REDUCE(clientwin_render((ClientWin *)iter->data), mw->cod);
					}
				}
				else if (mw->tooltip && wid == mw->tooltip->window)
					tooltip_handle(mw->tooltip, &ev);
				else if (wid) {
					for (dlist *iter = mw->cod; iter; iter = iter->next) {
						ClientWin *cw = (ClientWin *) iter->data;
						if (cw->mini.window == wid) {
							die = clientwin_handle(cw, &ev);
							break;
						}
					}
				}
			}

			// Do delayed painting if it's active
			if (mw->poll_time && pending_damage && !die) {
				long now = time_in_millis();
				if (now >= last_rendered + mw->poll_time) {
					pending_damage = false;
					foreach_dlist(mw->cod) {
						if (((ClientWin *) iter->data)->damaged)
							clientwin_repair(iter->data);
					}
					last_rendered = now;
				}
			}

			XFlush(ps->dpy);
		}
		else {
			// Discards all events so that poll() won't constantly hit data to read
			XSync(ps->dpy, True);
			assert(!XEventsQueued(ps->dpy, QueuedAfterReading));
		}

		// Handle daemon commands
		if (POLLIN & r_fd[1].revents) {
			unsigned char piped_input = 0;
			int read_ret = read(ps->fd_pipe, &piped_input, 1);
			if (0 == read_ret) {
				printfdf("(): EOF reached on pipe \"%s\".", ps->o.pipePath);
				open_pipe(ps, r_fd);
			}
			else if (-1 == read_ret) {
				if (EAGAIN != errno)
					printfef("(): Reading pipe \"%s\" failed: %d", ps->o.pipePath, errno);
			}
			else {
				assert(1 == read_ret);
				printfdf("(): Received pipe command: %d", piped_input);
				switch (piped_input) {
					case PIPECMD_ACTIVATE_WINDOW_PICKER:
						activate = true;
						break;
					case PIPECMD_DEACTIVATE_WINDOW_PICKER:
						if (mw)
							die = true;
						break;
					case PIPECMD_TOGGLE_WINDOW_PICKER:
						if (mw)
							die = true;
						else
							activate = true;
						break;
					case PIPECMD_EXIT_RUNNING_DAEMON:
						printfdf("(): Exit command received, killing daemon...");
						unlink(ps->o.pipePath);
						return;
					default:
						printfdf("(): Unknown daemon command \"%d\" received.", piped_input);
						break;
				}
			}
		}

		if (POLLHUP & r_fd[1].revents) {
			printfdf("(): PIPEHUP on pipe \"%s\".", ps->o.pipePath);
			open_pipe(ps, r_fd);
		}
	}
}

static bool
send_command_to_daemon_via_fifo(int command, const char *pipePath) {
	{
		int access_ret = 0;
		if ((access_ret = access(pipePath, W_OK))) {
			printfef("(): Failed to access() pipe \"%s\": %d", pipePath, access_ret);
			perror("access");
			exit(1);
		}
	}

	FILE *fp = fopen(pipePath, "w");

	printfdf("(): Sending command...");
	fputc(command, fp);

	fclose(fp);

	return true;
}

static inline bool
activate_window_picker(const char *pipePath) {
	printfdf("(): Activating window picker...");
	return send_command_to_daemon_via_fifo(PIPECMD_ACTIVATE_WINDOW_PICKER, pipePath);
}

static inline bool
exit_daemon(const char *pipePath) {
	printfdf("(): Killing daemon...");
	return send_command_to_daemon_via_fifo(PIPECMD_EXIT_RUNNING_DAEMON, pipePath);
}

static inline bool
deactivate_window_picker(const char *pipePath) {
	printfdf("(): Deactivating window picker...");
	return send_command_to_daemon_via_fifo(PIPECMD_DEACTIVATE_WINDOW_PICKER, pipePath);
}

static inline bool
toggle_window_picker(const char *pipePath) {
	printfdf("(): Toggling window picker...");
	return send_command_to_daemon_via_fifo(PIPECMD_TOGGLE_WINDOW_PICKER, pipePath);
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
		printf("error %d (%s) request %d minor %d serial %lu (\"%s\")\n",
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
	fputs("skippy-xd (" SKIPPYXD_VERSION ")\n"
			"Usage: skippy-xd [command]\n\n"
			"The available commands are:\n"
			"  --config                    - Read the specified configuration file.\n"
			"  --start-daemon              - starts the daemon running.\n"
			"  --stop-daemon               - stops the daemon running.\n"
			"  --activate-window-picker    - tells the daemon to show the window picker.\n"
			"  --deactivate-window-picker  - tells the daemon to hide the window picker.\n"
			"  --toggle-window-picker      - tells the daemon to toggle the window picker.\n"
			"\n"
			"  --help                      - show this message.\n"
			"  -S                          - Synchronize X operation (debugging).\n"
			, stdout);
#ifdef CFG_LIBPNG
	spng_about(stdout);
#endif
}

static inline bool
init_xexts(session_t *ps) {
	Display * const dpy = ps->dpy;
#ifdef CFG_XINERAMA
	ps->xinfo.xinerama_exist = XineramaQueryExtension(dpy,
			&ps->xinfo.xinerama_ev_base, &ps->xinfo.xinerama_err_base);
# ifdef DEBUG_XINERAMA
	printfef("(): Xinerama extension: %s",
			(ps->xinfo.xinerama_exist ? "yes": "no"));
# endif /* DEBUG_XINERAMA */
#endif /* CFG_XINERAMA */

	if(!XDamageQueryExtension(dpy,
				&ps->xinfo.damage_ev_base, &ps->xinfo.damage_err_base)) {
		printfef("(): FATAL: XDamage extension not found.");
		return false;
	}

	if(!XCompositeQueryExtension(dpy, &ps->xinfo.composite_ev_base,
				&ps->xinfo.composite_err_base)) {
		printfef("(): FATAL: XComposite extension not found.");
		return false;
	}

	if(!XRenderQueryExtension(dpy,
				&ps->xinfo.render_ev_base, &ps->xinfo.render_err_base)) {
		printfef("(): FATAL: XRender extension not found.");
		return false;
	}

	if(!XFixesQueryExtension(dpy,
				&ps->xinfo.fixes_ev_base, &ps->xinfo.fixes_err_base)) {
		printfef("(): FATAL: XFixes extension not found.");
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
	// Check $XDG_CONFIG_DIRS
	if (!((dir = getenv("XDG_CONFIG_DIRS")) && strlen(dir)))
		dir = PATH_CONFIG_SYS;
	{
		char *dir_free = mstrdup(dir);
		char *part = strtok(dir_free, ":");
		while (part) {
			path = mstrjoin(part, PATH_CONFIG_SYS_SUFFIX);
			if (fexists(path)) {
				free(dir_free);
				goto get_cfg_path_found;
			}
			free(path);
			path = NULL;
			part = strtok(NULL, ":");
		}
		free(dir_free);
	}

	return NULL;

get_cfg_path_found:
	return path;
}

static void
parse_args(session_t *ps, int argc, char **argv, bool first_pass) {
	enum options {
		OPT_CONFIG = 256,
		OPT_ACTV_PICKER,
		OPT_DEACTV_PICKER,
		OPT_TOGGLE_PICKER,
		OPT_DM_START,
		OPT_DM_STOP,
	};
	static const char * opts_short = "hS";
	static const struct option opts_long[] = {
		{ "help",                     no_argument,       NULL, 'h' },
		{ "config",                   required_argument, NULL, OPT_CONFIG },
		{ "activate-window-picker",   no_argument,       NULL, OPT_ACTV_PICKER },
		{ "deactivate-window-picker", no_argument,       NULL, OPT_DEACTV_PICKER },
		{ "toggle-window-picker",     no_argument,       NULL, OPT_TOGGLE_PICKER },
		{ "start-daemon",             no_argument,       NULL, OPT_DM_START },
		{ "stop-daemon",              no_argument,       NULL, OPT_DM_STOP },
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
				T_CASEBOOL('S', synchronize);
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
			case OPT_ACTV_PICKER:
				ps->o.mode = PROGMODE_ACTV_PICKER;
				break;
			case OPT_DEACTV_PICKER:
				ps->o.mode = PROGMODE_DEACTV_PICKER;
				break;
			case OPT_TOGGLE_PICKER:
				ps->o.mode = PROGMODE_TOGGLE_PICKER;
				break;
			T_CASEBOOL(OPT_DM_START, runAsDaemon);
			case OPT_DM_STOP:
				ps->o.mode = PROGMODE_DM_STOP;
				break;
#undef T_CASEBOOL
			default:
				printfef("(0): Unimplemented option %d.", o);
				exit(RET_UNKNOWN);
		}
	}
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
		printfef("(): FATAL: Couldn't connect to display.");
		ret = RET_XFAIL;
		goto main_end;
	}
	if (!init_xexts(ps)) {
		ret = RET_XFAIL;
		goto main_end;
	}
	if (ps->o.synchronize)
		XSynchronize(ps->dpy, True);
	XSetErrorHandler(xerror);

	ps->screen = DefaultScreen(dpy);
	ps->root = RootWindow(dpy, ps->screen);

	wm_get_atoms(ps);

	// Load configuration file
	{
		dlist *config = NULL;
		{
			bool user_specified_config = ps->o.config_path;
			if (!ps->o.config_path)
				ps->o.config_path = get_cfg_path();
			if (ps->o.config_path)
				config = config_load(ps->o.config_path);
			else
				printfef("(): WARNING: No configuration file found.");
			if (!config && user_specified_config)
				return 1;
		}

		char *lc_numeric_old = mstrdup(setlocale(LC_NUMERIC, NULL));
		setlocale(LC_NUMERIC, "C");

		// Read configuration into ps->o, because searching all the time is much
		// less efficient, may introduce inconsistent default value, and
		// occupies a lot more memory for non-string types.
		ps->o.pipePath = mstrdup(config_get(config, "general", "pipePath", "/tmp/skippy-xd-fifo"));
		ps->o.normal_tint = mstrdup(config_get(config, "normal", "tint", "black"));
		ps->o.highlight_tint = mstrdup(config_get(config, "highlight", "tint", "#101020"));
		ps->o.tooltip_border = mstrdup(config_get(config, "tooltip", "border", "#e0e0e0"));
		ps->o.tooltip_background = mstrdup(config_get(config, "tooltip", "background", "#404040"));
		ps->o.tooltip_text = mstrdup(config_get(config, "tooltip", "text", "#e0e0e0"));
		ps->o.tooltip_textShadow = mstrdup(config_get(config, "tooltip", "textShadow", "black"));
		ps->o.tooltip_font = mstrdup(config_get(config, "tooltip", "font", "fixed-11:weight=bold"));
		if (!parse_cliop(ps, config_get(config, "bindings", "miwMouse1", "focus"), &ps->o.bindings_miwMouse[1])
				|| !parse_cliop(ps, config_get(config, "bindings", "miwMouse2", "close-ewmh"), &ps->o.bindings_miwMouse[2])
				|| !parse_cliop(ps, config_get(config, "bindings", "miwMouse3", "iconify"), &ps->o.bindings_miwMouse[3]))
			return RET_BADARG;
		config_get_int_wrap(config, "general", "distance", &ps->o.distance, 1, INT_MAX);
		config_get_bool_wrap(config, "general", "useNetWMFullscreen", &ps->o.useNetWMFullscreen);
		config_get_bool_wrap(config, "general", "ignoreSkipTaskbar", &ps->o.ignoreSkipTaskbar);
		config_get_bool_wrap(config, "general", "acceptOvRedir", &ps->o.acceptOvRedir);
		config_get_bool_wrap(config, "general", "acceptWMWin", &ps->o.acceptWMWin);
		config_get_double_wrap(config, "general", "updateFreq", &ps->o.updateFreq, -1000.0, 1000.0);
		config_get_bool_wrap(config, "general", "lazyTrans", &ps->o.lazyTrans);
		config_get_bool_wrap(config, "general", "useNameWindowPixmap", &ps->o.useNameWindowPixmap);
		config_get_bool_wrap(config, "general", "forceNameWindowPixmap", &ps->o.forceNameWindowPixmap);
		config_get_bool_wrap(config, "general", "includeFrame", &ps->o.includeFrame);
		config_get_bool_wrap(config, "general", "allowUpscale", &ps->o.allowUpscale);
		config_get_int_wrap(config, "general", "preferredIconSize", &ps->o.preferredIconSize, 1, INT_MAX);
		config_get_bool_wrap(config, "general", "includeAllScreens", &ps->o.includeAllScreens);
		config_get_bool_wrap(config, "general", "avoidThumbnailsFromOtherScreens", &ps->o.avoidThumbnailsFromOtherScreens);
		config_get_bool_wrap(config, "general", "showAllDesktops", &ps->o.showAllDesktops);
		config_get_bool_wrap(config, "general", "showUnmapped", &ps->o.showUnmapped);
		config_get_bool_wrap(config, "general", "movePointerOnStart", &ps->o.movePointerOnStart);
		config_get_bool_wrap(config, "general", "movePointerOnSelect", &ps->o.movePointerOnSelect);
		config_get_bool_wrap(config, "general", "movePointerOnRaise", &ps->o.movePointerOnRaise);
		config_get_bool_wrap(config, "general", "switchDesktopOnActivate", &ps->o.switchDesktopOnActivate);
		config_get_bool_wrap(config, "xinerama", "showAll", &ps->o.xinerama_showAll);
		config_get_int_wrap(config, "normal", "tintOpacity", &ps->o.normal_tintOpacity, 0, 256);
		config_get_int_wrap(config, "normal", "opacity", &ps->o.normal_opacity, 0, 256);
		config_get_int_wrap(config, "highlight", "tintOpacity", &ps->o.highlight_tintOpacity, 0, 256);
		config_get_int_wrap(config, "highlight", "opacity", &ps->o.highlight_opacity, 0, 256);
		config_get_bool_wrap(config, "tooltip", "show", &ps->o.tooltip_show);
		config_get_bool_wrap(config, "tooltip", "followsMouse", &ps->o.tooltip_followsMouse);
		config_get_int_wrap(config, "tooltip", "offsetX", &ps->o.tooltip_offsetX, INT_MIN, INT_MAX);
		config_get_int_wrap(config, "tooltip", "offsetY", &ps->o.tooltip_offsetY, INT_MIN, INT_MAX);
		if (!parse_align_full(ps, config_get(config, "tooltip", "align", "left"), &ps->o.tooltip_align))
			return RET_BADARG;
		config_get_int_wrap(config, "tooltip", "tintOpacity", &ps->o.highlight_tintOpacity, 0, 256);
		config_get_int_wrap(config, "tooltip", "opacity", &ps->o.tooltip_opacity, 0, 256);
		{
			const char *s = config_get(config, "general", "clientDisplayModes", NULL);
			if (s && !(ps->o.clientDisplayModes = parse_client_disp_mode(ps, s)))
				return RET_BADARG;
			if (!ps->o.clientDisplayModes) {
				static const client_disp_mode_t DEF_CLIDISPM[] = {
					CLIDISP_THUMBNAIL, CLIDISP_ICON, CLIDISP_FILLED, CLIDISP_NONE
				};
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
	}

	// Second pass
	parse_args(ps, argc, argv, false);

	const char* pipePath = ps->o.pipePath;

	// Handle special modes
	switch (ps->o.mode) {
		case PROGMODE_NORMAL:
			break;
		case PROGMODE_ACTV_PICKER:
			activate_window_picker(pipePath);
			goto main_end;
		case PROGMODE_DEACTV_PICKER:
			deactivate_window_picker(pipePath);
			goto main_end;
		case PROGMODE_TOGGLE_PICKER:
			toggle_window_picker(pipePath);
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
		printfef("(): FATAL: Couldn't create main window.");
		ret = 1;
		goto main_end;
	}
	ps->mainwin = mw;

	XSelectInput(ps->dpy, ps->root, PropertyChangeMask);

	// Daemon mode
	if (ps->o.runAsDaemon) {
		bool flush_file = false;

		printfdf("(): Running as daemon...");

		// Flush file if we could access() it (or, usually, if it exists)
		if (!access(pipePath, R_OK))
			flush_file = true;

		{
			int result = mkfifo(pipePath, S_IRUSR | S_IWUSR);
			if (result < 0  && EEXIST != errno) {
				printfef("(): Failed to create named pipe \"%s\": %d", pipePath, result);
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
		if (flush_file) {
			char *buf[BUF_LEN];
			while (read(ps->fd_pipe, buf, sizeof(buf)) > 0)
				continue;
			printfdf("(): Finished flushing pipe \"%s\".", pipePath);
		}

		mainloop(ps, false);
	}
	else {
		printfdf("(): running once then quitting...");
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
			free(ps->o.tooltip_border);
			free(ps->o.tooltip_background);
			free(ps->o.tooltip_text);
			free(ps->o.tooltip_textShadow);
			free(ps->o.tooltip_font);
			free_pictw(ps, &ps->o.background);
			free_pictw(ps, &ps->o.iconDefault);
			free_pictspec(ps, &ps->o.iconFillSpec);
			free_pictspec(ps, &ps->o.fillSpec);
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
