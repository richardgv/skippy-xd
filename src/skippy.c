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
		[	CLIENTOP_CLOSE_ICCWM	] = "close-iccwm",
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
static bool
parse_align(session_t *ps, const char *str, enum align *dest) {
	static const char * const STRS_ALIGN[] = {
		[ ALIGN_LEFT  ] = "left",
		[ ALIGN_MID   ] = "mid",
		[ ALIGN_RIGHT ] = "right",
	};
	for (int i = 0; i < sizeof(STRS_ALIGN) / sizeof(STRS_ALIGN[0]); ++i)
		if (!strcmp(STRS_ALIGN[i], str)) {
			*dest = i;
			return true;
		}

	printfef("(\"%s\"): Unrecognized operation.", str);
	return false;
}

static dlist *
update_clients(MainWin *mw, dlist *clients, Bool *touched)
{
	dlist *stack, *iter;
	
	stack = dlist_first(wm_get_stack(mw->ps->dpy));
	iter = clients = dlist_first(clients);
	
	if(touched)
		*touched = False;
	
	/* Terminate clients that are no longer managed */
	while (iter) {
		ClientWin *cw = (ClientWin *)iter->data;
		if (!dlist_find_data(stack, (void *) cw->client.window)) {
			dlist *tmp = iter->next;
			clientwin_destroy((ClientWin *)iter->data, True);
			clients = dlist_remove(iter);
			iter = tmp;
			if(touched)
				*touched = True;
			continue;
		}
		clientwin_update(cw);
		iter = iter->next;
	}
	XFlush(mw->ps->dpy);
	
	/* Add new clients */
	for(iter = dlist_first(stack); iter; iter = iter->next)
	{
		ClientWin *cw = (ClientWin*)dlist_find(clients, clientwin_cmp_func, iter->data);
		if(! cw && (Window)iter->data != mw->window)
		{
			cw = clientwin_create(mw, (Window)iter->data);
			if (!cw) continue;
			clients = dlist_add(clients, cw);
			clientwin_update(cw);
			if(touched)
				*touched = True;
		}
	}

	logd("update clients: %d active\n ", dlist_len(clients));
	
	dlist_free(stack);
	
	return clients;
}

static LAYOUT_MODE
get_layout_mode(const session_t* ps) {
	if (ps->o.layout_desktop) { 
		return LAYOUT_DESKTOP;
	}
	else {
		return LAYOUT_ORIGINAL;
	}
}

static dlist *
do_layout(MainWin *mw, dlist *clients, Window focus, Window leader,float t)
{
	session_t * const ps = mw->ps;

	CARD32 desktop = wm_get_current_desktop(ps->dpy);
	unsigned int width, height;
	int xoff, yoff;
	dlist *iter, *tmp;
	
	/* Update the client table, pick the ones we want and sort them */
	clients = update_clients(mw, clients, 0);
	
	if(mw->cod)
		dlist_free(mw->cod);
	
	tmp = dlist_first(dlist_find_all(clients, (dlist_match_func)clientwin_validate_func, &desktop));
	logd("clients:%d,valid:%d\n",dlist_len(clients),dlist_len(tmp));
	if(leader != None)
	{
		mw->cod = dlist_first(dlist_find_all(tmp, clientwin_check_group_leader_func, (void*)&leader));
		dlist_free(tmp);
	} else
		mw->cod = tmp;
	if(! mw->cod)
		return clients;
	
	dlist_sort(mw->cod, clientwin_sort_func, 0);
	
	/* Move the mini windows around */
	LAYOUT_MODE mode=get_layout_mode(ps);
	Vec2i total_size;
	layout_run(mw, mode, mw->cod, &total_size);
	int extra_border=mw->distance;
//	float factor=layout_factor(mw,width,height,mw->distance);
	for(iter = mw->cod; iter; iter = iter->next)
		clientwin_lerp_client_to_mini((ClientWin*)iter->data,t);


	float factor = (float)(mw->width - extra_border) / total_size.x;
	if(factor * height > mw->height - extra_border)
		factor = (float)(mw->height - extra_border) / total_size.y;
	
	xoff = (mw->width - (float)total_size.x * factor) / 2;
	yoff = (mw->height - (float)total_size.y * factor) / 2;
	mainwin_transform(mw, factor);
	for(iter = mw->cod; iter; iter = iter->next)
		clientwin_create_scaled_image((ClientWin*)iter->data /*, factor, xoff, yoff*/);
	
	/* Get the currently focused window and select which mini-window to focus */
	iter = dlist_find(mw->cod, clientwin_cmp_func, (void *)focus);
	if(! iter)
		iter = mw->cod;
	mw->focus = (ClientWin*)iter->data;
	mw->focus->focused = 1;
	
	/* Map the client windows */
	for(iter = mw->cod; iter; iter = iter->next)
		clientwin_map((ClientWin*)iter->data);
	if (ps->o.movePointerOnStart)
		XWarpPointer(mw->ps->dpy, None, mw->focus->mini.window, 0, 0, 0, 0,
				sw_width(&mw->focus->mini) / 2, sw_height(&mw->focus->mini) / 2);
	
	return clients;
}

void mw_animate(MainWin* mw) {
	dlist* iter;
	for (iter=mw->cod;iter;iter=iter->next){
		ClientWin* cw=(ClientWin*) iter->data;
	}
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

	const char *name = ev_dumpstr_type(ev);

	Window wid = ev_window(ps, ev);
	const char *wextra = "";
	if (ps->root == wid) wextra = "(Root)";
	if (mw && mw->window == wid) wextra = "(Main)";

	print_timestamp(ps);
	printfd("Event %-13.13s wid %#010lx %s", name, wid, wextra);
}

static dlist *
skippy_run(MainWin *mw, dlist *clients, Window focus, Window leader, Bool all_xin)
{
	session_t * const ps = mw->ps;
	XEvent ev;
	int die = 0;
	bool refocus = false;
	
	/* Update the main window's geometry (and Xinerama info if applicable) */
	mainwin_update(mw);
#ifdef CFG_XINERAMA
	if(all_xin)
	{
		mw->xin_active = 0;
	}
#endif /* CFG_XINERAMA */
	
	/* Map the main window and run our event loop */
	if (ps->o.lazyTrans) {
		mainwin_map(mw);
		XFlush(ps->dpy);
	}
	
	float anim_t=1.f;	// disabled. to enable, set to 0.0
	float anim_dt=0.05f;
	clients = do_layout(mw, clients, focus, leader,anim_t);
	if (!mw->cod) {
		printfef("(): No client windows found.");
		return clients;
	}
	
	/* Map the main window and run our event loop */
	if (!ps->o.lazyTrans)
		mainwin_map(mw);
	XFlush(ps->dpy);
	
	int last_rendered = time_in_millis();
	while (!die) {
		//printf("polling\n7");
		int i, now, timeout;
		struct pollfd r_fd;

		XFlush(ps->dpy);

		r_fd.fd = ConnectionNumber(ps->dpy);
		r_fd.events = POLLIN;
		if(mw->poll_time > 0)
			timeout = MAX(0, mw->poll_time + last_rendered - time_in_millis());
		else
			timeout = -1;
		i = poll(&r_fd, 1, timeout);

		now = time_in_millis();
		if(now >= last_rendered + mw->poll_time)
		{

			REDUCE(if( ((ClientWin*)iter->data)->damaged ) clientwin_repair(iter->data), mw->cod);
			last_rendered = now;
		}

		i = XPending(ps->dpy);
		if (!i && anim_t<1.0f) {
			anim_t=MIN(1.0f,(anim_t+anim_dt));
			mw_animate(mw);
			do_layout(mw,clients,focus,leader,anim_t);
		}
		for (int j = 0; j < i && !die; ++j) {
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
				dlist *iter = (wid ? dlist_find(clients, clientwin_cmp_func, (void *) wid): NULL);
				if (iter) {
					ClientWin *cw = (ClientWin *)iter->data;
					clients = dlist_first(dlist_remove(iter));
					iter = dlist_find(mw->cod, clientwin_cmp_func, (void *) wid);
					if(iter)
						mw->cod = dlist_first(dlist_remove(iter));
					clientwin_destroy(cw, true);
					if (!mw->cod) {
						printfef("(): Last client window destroyed/unmapped, exiting.");
						die = 1;
					}
				}
			}
			else if (mw->poll_time >= 0 && ev.type == ps->xinfo.damage_ev_base + XDamageNotify)
			{
				XDamageNotifyEvent *d_ev = (XDamageNotifyEvent *)&ev;
				dlist *iter = dlist_find(mw->cod, clientwin_cmp_func, (void *)d_ev->drawable);
				if(iter)
				{
					if(mw->poll_time == 0)
						clientwin_repair((ClientWin *)iter->data);
					else
						((ClientWin *)iter->data)->damaged = true;
				}

			}
			else if (KeyRelease == ev.type && (mw->key_q == ev.xkey.keycode
						|| mw->key_escape == ev.xkey.keycode)) {
				if (mw->pressed_key) {
					die = 1;
					if (mw->key_escape == ev.xkey.keycode)
						refocus = true;
				}
				else
					report_key_ignored(&ev);
			}
			else if (wid == mw->window)
				die = mainwin_handle(mw, &ev);
			else if(ev.type == PropertyNotify) {
				if(ev.xproperty.atom == ESETROOT_PMAP_ID
						|| ev.xproperty.atom == _XROOTPMAP_ID) {
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
	}
	
	/* Unmap the main window and clean up */
	mainwin_unmap(mw);
	XFlush(ps->dpy);
	
	REDUCE(clientwin_unmap((ClientWin*)iter->data), mw->cod);
	XFlush(ps->dpy);
	dlist_free(mw->cod);
	mw->cod = 0;
	
	if (refocus)
		XSetInputFocus(ps->dpy, focus, RevertToPointerRoot, CurrentTime);
	
	return clients;
}

void send_command_to_daemon_via_fifo(int command, const char* pipePath)
{
	FILE *fp;
	if (access(pipePath, W_OK) != 0)
	{
		fprintf(stderr, "pipe does not exist, exiting...\n");
		exit(1);
	}
	
	fp = fopen(pipePath, "w");
	
	printf("sending command...\n");
	fputc(command, fp);
	
	fclose(fp);
}

void activate_window_picker(const char* pipePath) {
	printf("activating window picker...\n");
	send_command_to_daemon_via_fifo(ACTIVATE_WINDOW_PICKER, pipePath);
}

void exit_daemon(const char* pipePath) {
	printf("exiting daemon...\n");
	send_command_to_daemon_via_fifo(EXIT_RUNNING_DAEMON, pipePath);
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

void show_help()
{
	fputs("skippy-xd (" SKIPPYXD_VERSION ")\n"
			"Usage: skippy-xd [command]\n\n"
			"The available commands are:\n"
			"\t--config                  - Read the specified configuration file.\n"
			"\t--start-daemon            - starts the daemon running.\n"
			"\t--stop-daemon             - stops the daemon running.\n"
			"\t--activate-window-picker  - tells the daemon to show the window picker.\n"
			"\t--help                    - show this message.\n"
			"\t-S                        - Synchronize X operation (debugging).\n"
			"\t-a                        - show windows from all desktops.\n"
			"\t-d                        - 'expo' style Desktop layout .\n"
			, stdout);
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
		OPT_DM_START,
		OPT_DM_STOP,
	};
	static const char * opts_short = "hSagds";
	static const struct option opts_long[] = {
		{ "help",					no_argument,	NULL, 'h'},
		{ "config",					required_argument, NULL, OPT_CONFIG},
		{ "activate-window-picker", no_argument,	NULL, OPT_ACTV_PICKER},
		{ "start-daemon",			no_argument,	NULL, OPT_DM_START},
		{ "stop-daemon",			no_argument,	NULL, OPT_DM_STOP},
		{ "all desktops",			no_argument,	NULL, 'a'},
//		{ "keep layout",			no_argument,	NULL, 'k' },
//		{ "grid layout",			no_argument,	NULL, 'g' },
//		{ "smart layout",			no_argument,	NULL, 's' },
		{ NULL, no_argument, NULL, 0 }
	};

	int o = 0;
	optind = 1;

	// Only parse --config in first pass
	if (first_pass) {
		while ((o = getopt_long(argc, argv, opts_short, opts_long, NULL)) >= 0) {
			switch (o) {
				case OPT_CONFIG:
					ps->o.config_path = mstrdup(optarg);
					break;
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
			case OPT_CONFIG: break;
#define T_CASEBOOL(idx, option) case idx: ps->o.option = true; break
			T_CASEBOOL('S', synchronize);
			T_CASEBOOL('a',	xinerama_showAll);
			T_CASEBOOL('d',	layout_desktop);
			T_CASEBOOL('s',	layout_smart);
			T_CASEBOOL('g',	layout_grid);
			case OPT_ACTV_PICKER:
				ps->o.mode = PROGMODE_ACTV_PICKER;
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

	dlist *clients = NULL;
	Display *dpy = NULL;
	MainWin *mw = NULL;
	Bool invertShift = False;
	Window leader, focused;
	int result;
	int flush_file = 0;
	FILE *fp;
	int piped_input;
	int exitDaemon = 0;
	
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

	// Load configuration file
	{
		dlist *config = NULL;
		{
			if (!ps->o.config_path)
				ps->o.config_path = get_cfg_path();
			if (ps->o.config_path)
				config = config_load(ps->o.config_path);
			else
				printfef("(): WARNING: No configuration file found.");
		}

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
		if (config) {
			config_get_int_wrap(config, "general", "distance", &ps->o.distance, 1, INT_MAX);
			config_get_bool_wrap(config, "general", "useNetWMFullscreen", &ps->o.useNetWMFullscreen);
			config_get_bool_wrap(config, "general", "ignoreSkipTaskbar", &ps->o.ignoreSkipTaskbar);
			config_get_double_wrap(config, "general", "updateFreq", &ps->o.updateFreq, -1000.0, 1000.0);
			config_get_bool_wrap(config, "general", "lazyTrans", &ps->o.lazyTrans);
			config_get_bool_wrap(config, "general", "useNameWindowPixmap", &ps->o.useNameWindowPixmap);
			config_get_bool_wrap(config, "general", "movePointerOnStart", &ps->o.movePointerOnStart);
			config_get_bool_wrap(config, "xinerama", "showAll", &ps->o.xinerama_showAll);
			config_get_int_wrap(config, "normal", "tintOpacity", &ps->o.normal_tintOpacity, 0, 256);
			config_get_int_wrap(config, "normal", "opacity", &ps->o.normal_opacity, 0, 256);
			config_get_int_wrap(config, "highlight", "tintOpacity", &ps->o.highlight_tintOpacity, 0, 256);
			config_get_int_wrap(config, "highlight", "opacity", &ps->o.highlight_opacity, 0, 256);
			config_get_bool_wrap(config, "tooltip", "show", &ps->o.tooltip_show);
			config_get_bool_wrap(config, "tooltip", "followsMouse", &ps->o.tooltip_followsMouse);
			config_get_int_wrap(config, "tooltip", "offsetX", &ps->o.tooltip_offsetX, INT_MIN, INT_MAX);
			config_get_int_wrap(config, "tooltip", "offsetY", &ps->o.tooltip_offsetY, INT_MIN, INT_MAX);
			if (!parse_align(ps, config_get(config, "tooltip", "align", "left"), &ps->o.tooltip_align))
				return RET_BADARG;
			config_get_int_wrap(config, "tooltip", "tintOpacity", &ps->o.highlight_tintOpacity, 0, 256);
			config_get_int_wrap(config, "tooltip", "opacity", &ps->o.tooltip_opacity, 0, 256);

			config_free(config);
		}
	}

	// Second pass
	parse_args(ps, argc, argv, false);
	
	const char* pipePath = ps->o.pipePath;

	switch (ps->o.mode) {
		case PROGMODE_NORMAL:
			break;
		case PROGMODE_ACTV_PICKER:
			activate_window_picker(pipePath);
			goto main_end;
		case PROGMODE_DM_STOP:
			exit_daemon(pipePath);
			goto main_end;
	}

	// Open connection to X
	ps->dpy = dpy = XOpenDisplay(NULL);
	if(!dpy) {
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
	
	if (!wm_check(dpy)) {
		printfef("(): WM not NETWM or GNOME WM Spec compliant. "
				"Troubles ahead.");
		/* ret = 1;
		goto main_end; */
	}
	
	wm_use_netwm_fullscreen(ps->o.useNetWMFullscreen);
	wm_ignore_skip_taskbar(ps->o.ignoreSkipTaskbar);
	
	mw = mainwin_create(ps);
	if (!mw) {
		fprintf(stderr, "FATAL: Couldn't create main window.\n");
		ret = 1;
		goto main_end;
	}
	
	invertShift = !ps->o.xinerama_showAll;
	
	XSelectInput(ps->dpy, ps->root, PropertyChangeMask);

	if (ps->o.runAsDaemon) {
		printf("Running as daemon...\n");

		if (access(pipePath, R_OK) == 0)
		{
			printf("access() returned 0.\n");
			flush_file = 1;
		}
		
		result = mkfifo (pipePath, S_IRUSR| S_IWUSR);
		if (result < 0  && errno != EEXIST)
		{
			fprintf(stderr, "Error creating named pipe.\n");
			perror("mkfifo");
			exit(2);
		}
		
		// Flush the file in non-blocking mode
		if (flush_file) {
			char *buf[BUF_LEN];
			int fd = open(pipePath, O_RDONLY | O_NONBLOCK);

			if (fd < 0) {
				printfef("(): Failed to open() pipe \"%s\".", pipePath);
				ret = RET_UNKNOWN;
				goto main_end;
			}

			while (read(fd, buf, sizeof(buf)) > 0)
				continue;

			close(fd);

			printf("Finished flushing pipe...\n");
		}
		
		if (!(fp = fopen(pipePath, "r"))) {
			printfef("Failed to open pipe \"%s\".\n", pipePath);
			ret = RET_UNKNOWN;
			goto main_end;
		}
		
		while (!exitDaemon)
		{
			piped_input = fgetc(fp);
			switch (piped_input)
			{
				case ACTIVATE_WINDOW_PICKER:
					leader = None, focused = wm_get_focused(ps->dpy);
					clients = skippy_run(mw, clients, focused, leader, !invertShift);
					break;
				
				case EXIT_RUNNING_DAEMON:
					printf("Exit command received, killing daemon cleanly...\n");
					remove(pipePath);
					exitDaemon = 1;
				
				case EOF:
					#ifdef DEBUG_XINERAMA
					printf("EOF reached.\n");
					#endif
					fclose(fp);
					fp = fopen(pipePath, "r");
					break;
				
				default:
					printf("unknown code received: %d\n", piped_input);
					printf("Ignoring...\n");
					break;
			}
		}
	}
	else
	{
		printf("running once then quitting...\n");
		leader = None, focused = wm_get_focused(ps->dpy);
		clients = skippy_run(mw, clients, focused, leader, !invertShift);
	}
	
	dlist_free_with_func(clients, (dlist_free_func)clientwin_destroy);
	
main_end:
	if (mw)
		mainwin_destroy(mw);
	
	// Free session data
	if (ps) {
		// Free configuration strings
		{
			free(ps->o.config_path);
			free(ps->o.pipePath);
			free(ps->o.normal_tint);
			free(ps->o.highlight_tint);
			free(ps->o.tooltip_border);
			free(ps->o.tooltip_background);
			free(ps->o.tooltip_text);
			free(ps->o.tooltip_textShadow);
			free(ps->o.tooltip_font);
		}

		if (ps->dpy)
			XCloseDisplay(dpy);
		free(ps);
	}

	return ret;
}
