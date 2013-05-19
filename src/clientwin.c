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
clientwin_cmp_func(dlist *l, void *data)
{
	return ((ClientWin*)l->data)->client.window == (Window)data;
}

int
clientwin_validate_func(dlist *l, void *data) {
	ClientWin *cw = (ClientWin *)l->data;
	MainWin *mw = cw->mainwin;

	CARD32 desktop = (*(CARD32*)data),
		w_desktop = wm_get_window_desktop(mw->ps->dpy, cw->client.window);
	
#ifdef CFG_XINERAMA
	if(mw->xin_active && ! INTERSECTS(cw->client.x, cw->client.y, cw->client.width, cw->client.height,
	                                           mw->xin_active->x_org, mw->xin_active->y_org,
	                                           mw->xin_active->width, mw->xin_active->height))
		return 0;
#endif
	
	return (w_desktop == (CARD32)-1 || desktop == w_desktop) &&
	       wm_validate_window(mw->ps->dpy, cw->client.window);
}

int
clientwin_check_group_leader_func(dlist *l, void *data)
{
	ClientWin *cw = (ClientWin *)l->data;
	return wm_get_group_leader(cw->mainwin->ps->dpy, cw->client.window) == *((Window*)data);
}

int
clientwin_sort_func(dlist* a, dlist* b, void* data)
{
	unsigned int pa = ((ClientWin*)a->data)->client.x * ((ClientWin*)a->data)->client.y,
	             pb = ((ClientWin*)b->data)->client.x * ((ClientWin*)b->data)->client.y;
	return (pa < pb) ? -1 : (pa == pb) ? 0 : 1;
}

ClientWin *
clientwin_create(MainWin *mw, Window client) {
	session_t *ps = mw->ps;
	ClientWin *cw = (ClientWin *)malloc(sizeof(ClientWin));
	if (!cw)
		return NULL;
	{
		static const ClientWin CLIENTWT_DEF = CLIENTWT_INIT;
		memcpy(cw, &CLIENTWT_DEF, sizeof(ClientWin));
	}

	XWindowAttributes attr;
	
	cw->mainwin = mw;
	cw->pixmap = None;
	cw->focused = 0;
	cw->origin = cw->destination = None;
	cw->damage = None;
	cw->damaged = false;
	/* cw->repair = None; */
	
	cw->client.window = client;
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
	
	wm_wid_set_info(cw->mainwin->ps, cw->mini.window, "mini window", None);
	// Listen to events on the window. We don't want to miss any changes so
	// this is to be done as early as possible
	XSelectInput(cw->mainwin->ps->dpy, cw->client.window, SubstructureNotifyMask | StructureNotifyMask);

	XGetWindowAttributes(ps->dpy, client, &attr);
	cw->client.format = XRenderFindVisualFormat(ps->dpy, attr.visual);
	
	// Get window pixmap
	// Seemingly we could only redirect IsViewable windows
	if (ps->o.useNameWindowPixmap && IsViewable == attr.map_state) {
		XCompositeRedirectWindow(ps->dpy, cw->client.window, CompositeRedirectAutomatic);
		cw->redirected = true;
		cw->cpixmap = XCompositeNameWindowPixmap(ps->dpy, cw->client.window);
	}
	// Create window picture
	{
		Drawable draw = cw->cpixmap;
		if (!draw) draw = cw->client.window;
		XRenderPictureAttributes pa = { .subwindow_mode = IncludeInferiors };
		cw->origin = XRenderCreatePicture (cw->mainwin->ps->dpy,
				draw, cw->client.format, CPSubwindowMode, &pa);
	}
	if (!cw->origin)
		goto clientwin_create_err;
	
	XRenderSetPictureFilter(cw->mainwin->ps->dpy, cw->origin, FilterBest, 0, 0);
	
	
	return cw;

clientwin_create_err:
	if (cw)
		clientwin_destroy(cw, False);

	return NULL;
}

void
clientwin_update(ClientWin *cw)
{
	Window tmpwin;
	XWindowAttributes wattr;
	
	XGetWindowAttributes(cw->mainwin->ps->dpy, cw->client.window, &wattr);
	
	cw->client.format = XRenderFindVisualFormat(cw->mainwin->ps->dpy, wattr.visual);
	XTranslateCoordinates(cw->mainwin->ps->dpy, cw->client.window, wattr.root,
		                      -wattr.border_width,
		                      -wattr.border_width,
		                      &cw->client.x, &cw->client.y, &tmpwin);
	
	cw->client.width = wattr.width;
	cw->client.height = wattr.height;
	
	cw->mini.x = cw->mini.y = 0;
	cw->mini.width = cw->mini.height = 1;
}

void
clientwin_destroy(ClientWin *cw, bool destroyed) {
	MainWin *mw = cw->mainwin;
	session_t * const ps = mw->ps;

	free_picture(ps, &cw->origin);
	free_picture(ps, &cw->destination);
	free_pixmap(ps, &cw->pixmap);
	free_pixmap(ps, &cw->cpixmap);

	if (cw->client.window && !destroyed) {
		free_damage(ps, &cw->damage);

		// Stop listening to events, this should be safe because we don't
		// monitor window re-map anyway
		XSelectInput(ps->dpy, cw->client.window, 0);

		if (cw->redirected)
			XCompositeUnredirectWindow(ps->dpy, cw->client.window, CompositeRedirectAutomatic);
	}

	if (cw->mini.window) {
		// Somehow it doesn't work without unmapping firstly
		XUnmapWindow(ps->dpy, cw->mini.window);
		XDestroyWindow(ps->dpy, cw->mini.window);
	}

	free(cw);
}

static void
clientwin_repaint(ClientWin *cw, XRectangle *rect)
{
	XRenderColor *tint = cw->focused ? &cw->mainwin->highlightTint : &cw->mainwin->normalTint;
	int s_x = (double)rect->x * cw->factor,
	    s_y = (double)rect->y * cw->factor,
	    s_w = (double)rect->width * cw->factor,
	    s_h = (double)rect->height * cw->factor;
	
	if(cw->mainwin->ps->o.lazyTrans)
	{
		XRenderComposite(cw->mainwin->ps->dpy, PictOpSrc, cw->origin,
		                 cw->focused ? cw->mainwin->highlightPicture : cw->mainwin->normalPicture,
		                 cw->destination, s_x, s_y, 0, 0, s_x, s_y, s_w, s_h);
	}
	else
	{
		XRenderComposite(cw->mainwin->ps->dpy, PictOpSrc, cw->mainwin->background, None, cw->destination, cw->mini.x + s_x, cw->mini.y + s_y, 0, 0, s_x, s_y, s_w, s_h);
		XRenderComposite(cw->mainwin->ps->dpy, PictOpOver, cw->origin,
		                 cw->focused ? cw->mainwin->highlightPicture : cw->mainwin->normalPicture,
		                 cw->destination, s_x, s_y, 0, 0, s_x, s_y, s_w, s_h);
	}
	
	if(tint->alpha)
		XRenderFillRectangle(cw->mainwin->ps->dpy, PictOpOver, cw->destination, tint, s_x, s_y, s_w, s_h);
	
	XClearArea(cw->mainwin->ps->dpy, cw->mini.window, s_x, s_y, s_w, s_h, False);
}

void
clientwin_render(ClientWin *cw)
{
	XRectangle rect;
	rect.x = rect.y = 0;
	rect.width = cw->client.width;
	rect.height = cw->client.height;
	clientwin_repaint(cw, &rect);
}

void
clientwin_repair(ClientWin *cw)
{
	int nrects, i;
	XRectangle *rects;
	XserverRegion rgn = XFixesCreateRegion(cw->mainwin->ps->dpy, 0, 0);
	
	XDamageSubtract(cw->mainwin->ps->dpy, cw->damage, None, rgn);
	
	rects = XFixesFetchRegion(cw->mainwin->ps->dpy, rgn, &nrects);
	XFixesDestroyRegion(cw->mainwin->ps->dpy, rgn);
	
	for(i = 0; i < nrects; i++)
		clientwin_repaint(cw, &rects[i]);
	
	if(rects)
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
	cw->mini.width = MAX(1, (int)cw->client.width * f);
	cw->mini.height = MAX(1, (int)cw->client.height * f);
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
	
	cw->damage = XDamageCreate(ps->dpy, cw->client.window, XDamageReportDeltaRectangles);
	XRenderSetPictureTransform(ps->dpy, cw->origin, &cw->mainwin->transform);
	
	clientwin_render(cw);
	
	XMapWindow(ps->dpy, cw->mini.window);
	XRaiseWindow(ps->dpy, cw->mini.window);
}

void
clientwin_unmap(ClientWin *cw)
{
	if(cw->damage)
	{
		XDamageDestroy(cw->mainwin->ps->dpy, cw->damage);
		cw->damage = None;
	}
	
	if(cw->destination)
	{
		XRenderFreePicture(cw->mainwin->ps->dpy, cw->destination);
		cw->destination = None;
	}
	
	if(cw->pixmap)
	{
		XFreePixmap(cw->mainwin->ps->dpy, cw->pixmap);
		cw->pixmap = None;
	}
	
	XUnmapWindow(cw->mainwin->ps->dpy, cw->mini.window);
	XSetWindowBackgroundPixmap(cw->mainwin->ps->dpy, cw->mini.window, None);
	
	cw->focused = 0;
}

static void
childwin_focus(ClientWin *cw)
{
	XWarpPointer(cw->mainwin->ps->dpy, None, cw->client.window, 0, 0, 0, 0, cw->client.width / 2, cw->client.height / 2);
	XRaiseWindow(cw->mainwin->ps->dpy, cw->client.window);
	XSetInputFocus(cw->mainwin->ps->dpy, cw->client.window, RevertToParent, CurrentTime);
}

int
clientwin_handle(ClientWin *cw, XEvent *ev)
{
	if (ev->type == ButtonRelease && cw->mainwin->pressed_mouse) {
		const unsigned button = ev->xbutton.button;
		if (button < MAX_MOUSE_BUTTONS) {
			int ret = clientwin_action(cw, cw->mainwin->ps->o.bindings_miwMouse[button]);
			if (ret) {
				printfef("(): Quitting.");
				return ret;
			}
		}
	} else if(ev->type == KeyRelease && cw->mainwin->pressed_key) {
		if(ev->xkey.keycode == cw->mainwin->key_up)
			focus_up(cw);
		else if(ev->xkey.keycode == cw->mainwin->key_down)
			focus_down(cw);
		else if(ev->xkey.keycode == cw->mainwin->key_left)
			focus_left(cw);
		else if(ev->xkey.keycode == cw->mainwin->key_right)
			focus_right(cw);
		else if(ev->xkey.keycode == cw->mainwin->key_enter || ev->xkey.keycode == cw->mainwin->key_space) {
			childwin_focus(cw);
			return 1;
		}
	}
	else if (ev->type == ButtonPress) {
		cw->mainwin->pressed_mouse = true;
		/* if (ev->xbutton.button == 1)
			cw->mainwin->pressed = cw; */
	}
	else if (KeyPress == ev->type) {
		cw->mainwin->pressed_key = true;
	}
	else if(ev->type == FocusIn) {
		cw->focused = 1;
		clientwin_render(cw);
		XFlush(cw->mainwin->ps->dpy);
	} else if(ev->type == FocusOut) {
		cw->focused = 0;
		clientwin_render(cw);
		XFlush(cw->mainwin->ps->dpy);
	} else if(ev->type == EnterNotify) {
		if(cw->mainwin->tooltip)
		{
			int win_title_len = 0;
			FcChar8 *win_title = wm_get_window_title(cw->mainwin->ps, cw->client.window, &win_title_len);
			if(win_title)
			{
				tooltip_map(cw->mainwin->tooltip,
				            ev->xcrossing.x_root + 20, ev->xcrossing.y_root + 20,
				            win_title, win_title_len);
				free(win_title);
			}
		}
	} else if(ev->type == LeaveNotify) {
		if(cw->mainwin->tooltip)
			tooltip_unmap(cw->mainwin->tooltip);
	}
	return 0;
}

static int
clientwin_action(ClientWin *cw, enum cliop action) {
	session_t * const ps = cw->mainwin->ps;
	const Window wid = cw->client.window;

	switch (action) {
		case CLIENTOP_NO:
			break;
		case CLIENTOP_FOCUS:
			childwin_focus(cw);
			return 1;
		case CLIENTOP_ICONIFY:
			XIconifyWindow(ps->dpy, wid, ps->screen);
			break;
		case CLIENTOP_SHADE_EWMH:
			wm_shade_window_ewmh(ps, wid);
			break;
		case CLIENTOP_CLOSE_ICCWM:
			wm_close_window_iccwm(ps, wid);
			break;
		case CLIENTOP_CLOSE_EWMH:
			wm_close_window_ewmh(ps, wid);
			break;
		case CLIENTOP_DESTROY:
			XDestroyWindow(cw->mainwin->ps->dpy, cw->client.window);
			break;
	}

	return 0;
}
