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

#define INTERSECTS(x1, y1, w1, h1, x2, y2, w2, h2) \
	(((x1 >= x2 && x1 < (x2 + w2)) || (x2 >= x1 && x2 < (x1 + w1))) && \
	 ((y1 >= y2 && y1 < (y2 + h2)) || (y2 >= y1 && y2 < (y1 + h1))))

static int
clientwin_action(ClientWin *cw, enum cliop action);

int
clientwin_cmp_func(dlist *l, void *data) {
	ClientWin *cw = (ClientWin *) l->data;
	return cw->src.window == (Window) data
		|| cw->wid_client == (Window) data;
}

int
clientwin_validate_func(dlist *l, void *data) {
	ClientWin *cw = l->data;
	MainWin *mw = cw->mainwin;
	session_t *ps = mw->ps;

#ifdef CFG_XINERAMA
	if (mw->xin_active && !INTERSECTS(cw->src.x, cw->src.y, cw->src.width,
				cw->src.height, mw->xin_active->x_org, mw->xin_active->y_org,
				mw->xin_active->width, mw->xin_active->height))
		return false;
#endif

	if (!ps->o.showAllDesktops) {
		CARD32 desktop = (*(CARD32 *)data),
			w_desktop = wm_get_window_desktop(ps, cw->wid_client);

		if (!(w_desktop == (CARD32) -1 || desktop == w_desktop))
			return false;
	}

	return wm_validate_window(mw->ps, cw->wid_client);
}

int
clientwin_check_group_leader_func(dlist *l, void *data)
{
	ClientWin *cw = (ClientWin *)l->data;
	return wm_get_group_leader(cw->mainwin->ps->dpy, cw->wid_client) == *((Window*)data);
}

int
clientwin_sort_func(dlist* a, dlist* b, void* data)
{
	unsigned int pa = ((ClientWin *) a->data)->src.x * ((ClientWin *) a->data)->src.y,
	             pb = ((ClientWin *) b->data)->src.x * ((ClientWin *) b->data)->src.y;
	return (pa < pb) ? -1 : (pa == pb) ? 0 : 1;
}

ClientWin *
clientwin_create(MainWin *mw, Window client) {
	session_t *ps = mw->ps;
	ClientWin *cw = NULL;

	XWindowAttributes attr = { };
	XGetWindowAttributes(ps->dpy, client, &attr);

	// Check if window is mapped
	// TODO: Move to validate function?
	if (!ps->o.showUnmapped && IsViewable != attr.map_state)
		goto clientwin_create_err;

	cw = smalloc(1, ClientWin);
	{
		static const ClientWin CLIENTWT_DEF = CLIENTWT_INIT;
		memcpy(cw, &CLIENTWT_DEF, sizeof(ClientWin));
	}

	cw->mainwin = mw;
	cw->wid_client = client;
	if (ps->o.includeFrame)
		cw->src.window = wm_find_frame(ps, client);
	if (!cw->src.window)
		cw->src.window = client;
	cw->mini.format = mw->format;
	{
		XSetWindowAttributes sattr = {
			.border_pixel = 0,
			.background_pixel = 0,
			.colormap = mw->colormap,
			.event_mask = ButtonPressMask | ButtonReleaseMask | KeyPressMask
				| KeyReleaseMask | EnterWindowMask | LeaveWindowMask
				| PointerMotionMask | ExposureMask | FocusChangeMask,
			.override_redirect = ps->o.lazyTrans,
		};
		cw->mini.window = XCreateWindow(ps->dpy,
				(ps->o.lazyTrans ? ps->root : mw->window), 0, 0, 1, 1, 0,
				mw->depth, InputOutput, mw->visual,
				CWColormap | CWBackPixel | CWBorderPixel | CWEventMask | CWOverrideRedirect, &sattr);
	}
	if (!cw->mini.window)
		goto clientwin_create_err;

	{
		static const char *PREFIX = "mini window of ";
		const int len = strlen(PREFIX) + 20;
		char *str = allocchk(malloc(len));
		snprintf(str, len, "%s%#010lx", PREFIX, cw->src.window);
		wm_wid_set_info(cw->mainwin->ps, cw->mini.window, str, None);
		free(str);
	}

	// Listen to events on the window. We don't want to miss any changes so
	// this is to be done as early as possible
	XSelectInput(cw->mainwin->ps->dpy, cw->src.window, SubstructureNotifyMask | StructureNotifyMask);

	return cw;

clientwin_create_err:
	if (cw)
		clientwin_destroy(cw, False);

	return NULL;
}

/**
 * @brief Update window data to prepare for rendering.
 */
bool
clientwin_update(ClientWin *cw) {
	MainWin *mw = cw->mainwin;
	session_t *ps = mw->ps;

	clientwin_free_res2(ps, cw);

	// Get window attributes
	XWindowAttributes wattr = { };
	XGetWindowAttributes(ps->dpy, cw->src.window, &wattr);

	{
		{
			Window tmpwin = None;
			XTranslateCoordinates(ps->dpy, cw->src.window, wattr.root,
					-wattr.border_width, -wattr.border_width,
					&cw->src.x, &cw->src.y, &tmpwin);
		}
		cw->src.width = wattr.width;
		cw->src.height = wattr.height;
		cw->src.format = XRenderFindVisualFormat(ps->dpy, wattr.visual);
	}

	if (IsViewable == wattr.map_state
			&& !(ps->o.includeAllScreens && ps->o.avoidThumbnailsFromOtherScreens && ps->root != wattr.root)) {
		// Get window pixmap
		if (ps->o.useNameWindowPixmap) {
			XCompositeRedirectWindow(ps->dpy, cw->src.window,
					CompositeRedirectAutomatic);
			cw->redirected = true;
			cw->cpixmap = XCompositeNameWindowPixmap(ps->dpy, cw->src.window);
		}

		// Create window picture
		if (!(ps->o.useNameWindowPixmap && ps->o.forceNameWindowPixmap
					&& !cw->cpixmap)) {
			Drawable draw = cw->cpixmap;
			if (!draw) draw = cw->src.window;
			static XRenderPictureAttributes pa = { .subwindow_mode = IncludeInferiors };
			cw->origin = XRenderCreatePicture(ps->dpy,
					draw, cw->src.format, CPSubwindowMode, &pa);
		}
		if (cw->origin) {
			XRenderSetPictureFilter(ps->dpy, cw->origin, FilterBest, 0, 0);
		}
	}

	// Get window icon
	cw->icon_pict = simg_load_icon(ps, cw->wid_client, ps->o.preferredIconSize);
	if (!cw->icon_pict && ps->o.iconDefault)
		cw->icon_pict = clone_pictw(ps, ps->o.iconDefault);

	// Reset mini window parameters
	cw->mini.x = cw->mini.y = 0;
	cw->mini.width = cw->mini.height = 1;

	cw->mode = clientwin_get_disp_mode(ps, cw);
	// printfdf("(%#010lx): %d", cw->wid_client, cw->mode);

	return true;
}

static inline bool
clientwin_update2_filled(session_t *ps, MainWin *mw, ClientWin *cw) {
	cw->pict_filled = simg_postprocess(ps,
			clone_pictw(ps, ps->o.fillSpec.img),
			ps->o.fillSpec.mode,
			cw->mini.width, cw->mini.height,
			ps->o.fillSpec.alg, ps->o.fillSpec.valg,
			&ps->o.fillSpec.c);
	return cw->pict_filled;
}

static inline bool
clientwin_update2_icon(session_t *ps, MainWin *mw, ClientWin *cw) {
	cw->icon_pict_filled = simg_postprocess(ps,
			clone_pictw(ps, cw->icon_pict),
			ps->o.iconFillSpec.mode,
			cw->mini.width, cw->mini.height,
			ps->o.iconFillSpec.alg, ps->o.iconFillSpec.valg,
			&ps->o.iconFillSpec.c);
	return cw->icon_pict_filled;
}

bool
clientwin_update2(ClientWin *cw) {
	MainWin *mw = cw->mainwin;
	session_t *ps = mw->ps;

	clientwin_free_res2(ps, cw);

	switch (cw->mode) {
		case CLIDISP_NONE:
			break;
		case CLIDISP_FILLED:
			clientwin_update2_filled(ps, mw, cw);
			break;
		case CLIDISP_ICON:
			clientwin_update2_icon(ps, mw, cw);
			break;
		case CLIDISP_THUMBNAIL:
		case CLIDISP_THUMBNAIL_ICON:
			break;
	}

	return true;
}

void
clientwin_destroy(ClientWin *cw, bool destroyed) {
	MainWin *mw = cw->mainwin;
	session_t * const ps = mw->ps;

	free_picture(ps, &cw->origin);
	free_picture(ps, &cw->destination);
	free_pixmap(ps, &cw->pixmap);
	free_pixmap(ps, &cw->cpixmap);
	free_pictw(ps, &cw->icon_pict);
	free_pictw(ps, &cw->icon_pict_filled);
	free_pictw(ps, &cw->pict_filled);

	if (cw->src.window && !destroyed) {
		free_damage(ps, &cw->damage);

		// Stop listening to events, this should be safe because we don't
		// monitor window re-map anyway
		XSelectInput(ps->dpy, cw->src.window, 0);

		if (cw->redirected)
			XCompositeUnredirectWindow(ps->dpy, cw->src.window, CompositeRedirectAutomatic);
	}

	if (cw->mini.window) {
		// Somehow it doesn't work without unmapping firstly
		XUnmapWindow(ps->dpy, cw->mini.window);
		XDestroyWindow(ps->dpy, cw->mini.window);
	}

	free(cw);
}

static void
clientwin_repaint(ClientWin *cw, const XRectangle *pbound) {
	session_t *ps = cw->mainwin->ps;
	Picture source = None;
	int s_x = 0, s_y = 0, s_w = cw->mini.width, s_h = cw->mini.height;
	if (pbound) {
		s_x = pbound->x;
		s_y = pbound->y;
		s_w = pbound->width;
		s_h = pbound->height;
	}

	// printfdf("(%#010lx): %d, %d, %d, %d", cw->wid_client, s_x, s_y, s_w, s_h);

	switch (cw->mode) {
		case CLIDISP_NONE:
			break;
		case CLIDISP_FILLED:
			source = cw->pict_filled->pict;
			break;
		case CLIDISP_ICON:
			source = cw->icon_pict_filled->pict;
			break;
		case CLIDISP_THUMBNAIL:
		// We will draw the icon later
		case CLIDISP_THUMBNAIL_ICON:
			source = cw->origin;
			break;
	}


	if (!source) return;

	// Drawing main picture
	{
		const Picture mask = (cw->focused ? cw->mainwin->highlightPicture :
				cw->mainwin->normalPicture);
		if (ps->o.lazyTrans) {
			XRenderComposite(ps->dpy, PictOpSrc, source, mask,
					cw->destination, s_x, s_y, 0, 0, s_x, s_y, s_w, s_h);
		}
		else {
			XRenderComposite(ps->dpy, PictOpSrc, cw->mainwin->background, None,
					cw->destination, cw->mini.x + s_x, cw->mini.y + s_y, 0, 0,
					s_x, s_y, s_w, s_h);
			XRenderComposite(ps->dpy, PictOpOver, source, mask,
					cw->destination, s_x, s_y, 0, 0, s_x, s_y, s_w, s_h);
		}
		if (CLIDISP_THUMBNAIL_ICON == cw->mode) {
			assert(cw->icon_pict && cw->icon_pict->pict);
			img_composite_params_t params = IMG_COMPOSITE_PARAMS_INIT;
			simg_get_composite_params(cw->icon_pict,
					cw->mini.width, cw->mini.height,
					ps->o.iconFillSpec.mode, ps->o.iconFillSpec.alg, ps->o.iconFillSpec.valg,
					&params);
			simg_composite(ps, cw->icon_pict, cw->destination,
					cw->mini.width, cw->mini.height, &params, NULL, mask, pbound);
		}
	}

	// Tinting
	{
		XRenderColor *tint = (cw->focused ? &cw->mainwin->highlightTint
				: &cw->mainwin->normalTint);
		if (tint->alpha)
			XRenderFillRectangle(cw->mainwin->ps->dpy, PictOpOver, cw->destination, tint, s_x, s_y, s_w, s_h);
	}

	XClearArea(cw->mainwin->ps->dpy, cw->mini.window, s_x, s_y, s_w, s_h, False);
}

void
clientwin_render(ClientWin *cw) {
	clientwin_repaint(cw, NULL);
}

void
clientwin_repair(ClientWin *cw) {
	session_t *ps = cw->mainwin->ps;

	int nrects = 0;
	XRectangle *rects = NULL;

	{
		XserverRegion rgn = XFixesCreateRegion(ps->dpy, 0, 0);
		XDamageSubtract(ps->dpy, cw->damage, None, rgn);
		if (CLIDISP_THUMBNAIL == cw->mode || CLIDISP_THUMBNAIL_ICON == cw->mode)
			rects = XFixesFetchRegion(ps->dpy, rgn, &nrects);
		XFixesDestroyRegion(ps->dpy, rgn);
	}
	for (int i = 0; i < nrects; i++) {
		XRectangle r = {
			.x = rects[i].x * cw->factor,
			.y = rects[i].y * cw->factor,
			.width = rects[i].width * cw->factor,
			.height = rects[i].height * cw->factor,
		};
		clientwin_repaint(cw, &r);
	}

	if (rects)
		XFree(rects);
	
	cw->damaged = false;
}

void
clientwin_schedule_repair(ClientWin *cw, XRectangle *area)
{
	cw->damaged = true;
}

void
clientwin_move(ClientWin *cw, float f, int x, int y)
{
	/* int border = MAX(1, (double)DISTANCE(cw->mainwin) * f * 0.25); */
	int border = 0;
	XSetWindowBorderWidth(cw->mainwin->ps->dpy, cw->mini.window, border);
	
	cw->factor = f;
	cw->mini.x = x + (int)cw->x * f;
	cw->mini.y = y + (int)cw->y * f;
	if(cw->mainwin->ps->o.lazyTrans)
	{
		cw->mini.x += cw->mainwin->x;
		cw->mini.y += cw->mainwin->y;
	}
	cw->mini.width = MAX(1, cw->src.width * f);
	cw->mini.height = MAX(1, cw->src.height * f);
	XMoveResizeWindow(cw->mainwin->ps->dpy, cw->mini.window, cw->mini.x - border, cw->mini.y - border, cw->mini.width, cw->mini.height);
	
	if(cw->pixmap)
		XFreePixmap(cw->mainwin->ps->dpy, cw->pixmap);
	
	if(cw->destination)
		XRenderFreePicture(cw->mainwin->ps->dpy, cw->destination);
	
	cw->pixmap = XCreatePixmap(cw->mainwin->ps->dpy, cw->mini.window, cw->mini.width, cw->mini.height, cw->mainwin->depth);
	XSetWindowBackgroundPixmap(cw->mainwin->ps->dpy, cw->mini.window, cw->pixmap);
	
	cw->destination = XRenderCreatePicture(cw->mainwin->ps->dpy, cw->pixmap, cw->mini.format, 0, 0);
}

void
clientwin_map(ClientWin *cw) {
	session_t *ps = cw->mainwin->ps;
	free_damage(ps, &cw->damage);
	
	if (!cw->mode)
		return;

	if (cw->origin) {
		cw->damage = XDamageCreate(ps->dpy, cw->src.window, XDamageReportDeltaRectangles);
		XRenderSetPictureTransform(ps->dpy, cw->origin, &cw->mainwin->transform);
	}

	clientwin_render(cw);

	XMapWindow(ps->dpy, cw->mini.window);
	XRaiseWindow(ps->dpy, cw->mini.window);
}

void
clientwin_unmap(ClientWin *cw) {
	session_t *ps = cw->mainwin->ps;

	free_damage(ps, &cw->damage);
	free_picture(ps, &cw->destination);
	free_pixmap(ps, &cw->pixmap);

	XUnmapWindow(ps->dpy, cw->mini.window);
	XSetWindowBackgroundPixmap(ps->dpy, cw->mini.window, None);

	cw->focused = false;
}

void
childwin_focus(ClientWin *cw) {
	session_t * const ps = cw->mainwin->ps;

	if (ps->o.movePointerOnRaise)
		XWarpPointer(ps->dpy, None, cw->wid_client,
				0, 0, 0, 0, cw->src.width / 2, cw->src.height / 2);
	XRaiseWindow(ps->dpy, cw->wid_client);
	wm_activate_window(ps, cw->wid_client);
	XFlush(ps->dpy);
}

int
clientwin_handle(ClientWin *cw, XEvent *ev) {
	MainWin *mw = cw->mainwin;
	session_t *ps = mw->ps;

	if (ev->type == ButtonRelease) {
		const unsigned button = ev->xbutton.button;
		if (cw->mainwin->pressed_mouse) {
			if (button < MAX_MOUSE_BUTTONS) {
				int ret = clientwin_action(cw,
						ps->o.bindings_miwMouse[button]);
				if (ret) {
					printfef("(): Quitting.");
					return ret;
				}
			}
		}
		else
			printfef("(): ButtonRelease %u ignored.", button);
	} else if (ev->type == KeyRelease) {
		XKeyEvent * const evk = &ev->xkey;
		if (cw->mainwin->pressed_key) {
			const keydef_t KEY_NEXT = {
				.key = XKeysymToKeycode(ps->dpy, XK_Tab),
				.mod = KEYMOD_CTRL,
			};
			const keydef_t KEY_PREV = {
				.key = XKeysymToKeycode(ps->dpy, XK_Tab),
				.mod = KEYMOD_CTRL | KEYMOD_SHIFT,
			};

			if (evk->keycode == cw->mainwin->key_up ||
					evk->keycode == cw->mainwin->key_k)
				focus_up(cw);
			else if (evk->keycode == cw->mainwin->key_down ||
					evk->keycode == cw->mainwin->key_j)
				focus_down(cw);
			else if (evk->keycode == cw->mainwin->key_left ||
					evk->keycode == cw->mainwin->key_h)
				focus_left(cw);
			else if (evk->keycode == cw->mainwin->key_right ||
					evk->keycode == cw->mainwin->key_l)
				focus_right(cw);
			else if (evk->keycode == cw->mainwin->key_enter
					|| evk->keycode == cw->mainwin->key_space) {
				mw->client_to_focus = cw;
				return 1;
			}
			else if (ev_iskey(evk, &KEY_NEXT)) {
				focus_miniw_next(ps, cw);
			}
			else if (ev_iskey(evk, &KEY_PREV)) {
				focus_miniw_prev(ps, cw);
			}
			else
				report_key_unbinded(ev);
		}
		else
			report_key_ignored(ev);
	}
	else if (ev->type == ButtonPress) {
		cw->mainwin->pressed_mouse = true;
		/* if (ev->xbutton.button == 1)
			cw->mainwin->pressed = cw; */
	}
	else if (KeyPress == ev->type) {
		cw->mainwin->pressed_key = true;
	}
	else if (ev->type == FocusIn) {
		cw->focused = true;
		clientwin_render(cw);
		XFlush(ps->dpy);
	} else if (ev->type == FocusOut) {
		cw->focused = false;
		clientwin_render(cw);
		XFlush(ps->dpy);
	} else if(ev->type == EnterNotify) {
		XSetInputFocus(ps->dpy, cw->mini.window, RevertToParent, CurrentTime);
		if (cw->mainwin->tooltip) {
			int win_title_len = 0;
			FcChar8 *win_title = wm_get_window_title(ps, cw->wid_client, &win_title_len);
			if (win_title) {
				tooltip_map(cw->mainwin->tooltip,
						ev->xcrossing.x_root, ev->xcrossing.y_root,
						win_title, win_title_len);
				free(win_title);
			}
		}
	} else if(ev->type == LeaveNotify) {
		// XSetInputFocus(ps->dpy, mw->window, RevertToParent, CurrentTime);
		if(cw->mainwin->tooltip)
			tooltip_unmap(cw->mainwin->tooltip);
	}
	return 0;
}

static int
clientwin_action(ClientWin *cw, enum cliop action) {
	MainWin *mw = cw->mainwin;
	session_t * const ps = mw->ps;
	const Window wid = cw->wid_client;

	switch (action) {
		case CLIENTOP_NO:
			break;
		case CLIENTOP_FOCUS:
			mw->client_to_focus = cw;
			return 1;
		case CLIENTOP_ICONIFY:
			XIconifyWindow(ps->dpy, wid, ps->screen);
			break;
		case CLIENTOP_SHADE_EWMH:
			wm_shade_window_ewmh(ps, wid);
			break;
		case CLIENTOP_CLOSE_ICCCM:
			wm_close_window_icccm(ps, wid);
			break;
		case CLIENTOP_CLOSE_EWMH:
			wm_close_window_ewmh(ps, wid);
			break;
		case CLIENTOP_DESTROY:
			XDestroyWindow(cw->mainwin->ps->dpy, wid);
			break;
	}

	return 0;
}
