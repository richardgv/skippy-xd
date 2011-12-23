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

int
clientwin_cmp_func(dlist *l, void *data)
{
	return ((ClientWin*)l->data)->client.window == (Window)data;
}

int
clientwin_validate_func(dlist *l, void *data)
{
	ClientWin *cw = (ClientWin *)l->data;
	CARD32 desktop = (*(CARD32*)data),
		w_desktop = wm_get_window_desktop(cw->mainwin->dpy, cw->client.window);
	
#ifdef XINERAMA
	if(cw->mainwin->xin_active && ! INTERSECTS(cw->client.x, cw->client.y, cw->client.width, cw->client.height,
	                                           cw->mainwin->xin_active->x_org, cw->mainwin->xin_active->y_org,
	                                           cw->mainwin->xin_active->width, cw->mainwin->xin_active->height))
		return 0;
#endif
	
	return (w_desktop == (CARD32)-1 || desktop == w_desktop) &&
	       wm_validate_window(cw->mainwin->dpy, cw->client.window);
}

int
clientwin_check_group_leader_func(dlist *l, void *data)
{
	ClientWin *cw = (ClientWin *)l->data;
	return wm_get_group_leader(cw->mainwin->dpy, cw->client.window) == *((Window*)data);
}

int
clientwin_sort_func(dlist* a, dlist* b, void* data)
{
	unsigned int pa = ((ClientWin*)a->data)->client.x * ((ClientWin*)a->data)->client.y,
	             pb = ((ClientWin*)b->data)->client.x * ((ClientWin*)b->data)->client.y;
	return (pa < pb) ? -1 : (pa == pb) ? 0 : 1;
}

ClientWin *
clientwin_create(MainWin *mw, Window client)
{
	ClientWin *cw = (ClientWin *)malloc(sizeof(ClientWin));
	XSetWindowAttributes sattr;
	XWindowAttributes attr;
	XRenderPictureAttributes pa;
	
	cw->mainwin = mw;
	cw->pixmap = None;
	cw->focused = 0;
	cw->origin = cw->destination = None;
	cw->damage = None;
	cw->damaged = False;
	/* cw->repair = None; */
	
	sattr.border_pixel = sattr.background_pixel = 0;
	sattr.colormap = mw->colormap;
	
	sattr.event_mask = ButtonPressMask |
	                   ButtonReleaseMask |
	                   KeyReleaseMask |
	                   EnterWindowMask |
	                   LeaveWindowMask |
	                   PointerMotionMask |
	                   ExposureMask |
	                   FocusChangeMask;
	
	sattr.override_redirect = mw->lazy_trans;
	
	cw->client.window = client;
	cw->mini.format = mw->format;
	cw->mini.window = XCreateWindow(mw->dpy, mw->lazy_trans ? mw->root : mw->window, 0, 0, 1, 1, 0,
	                                mw->depth, InputOutput, mw->visual,
	                                CWColormap | CWBackPixel | CWBorderPixel | CWEventMask | CWOverrideRedirect, &sattr);
	
	if(cw->mini.window == None)
	{
		free(cw);
		return 0;
	}
	
	XGetWindowAttributes(mw->dpy, client, &attr);
	cw->client.format = XRenderFindVisualFormat(mw->dpy, attr.visual);
	
	pa.subwindow_mode = IncludeInferiors;
	cw->origin = XRenderCreatePicture (cw->mainwin->dpy, cw->client.window, cw->client.format, CPSubwindowMode, &pa);
	
	XRenderSetPictureFilter(cw->mainwin->dpy, cw->origin, FilterBest, 0, 0);
	
	XSelectInput(cw->mainwin->dpy, cw->client.window, SubstructureNotifyMask | StructureNotifyMask);
	
	return cw;
}

void
clientwin_update(ClientWin *cw)
{
	Window tmpwin;
	XWindowAttributes wattr;
	
	XGetWindowAttributes(cw->mainwin->dpy, cw->client.window, &wattr);
	
	cw->client.format = XRenderFindVisualFormat(cw->mainwin->dpy, wattr.visual);
	XTranslateCoordinates(cw->mainwin->dpy, cw->client.window, wattr.root,
		                      -wattr.border_width,
		                      -wattr.border_width,
		                      &cw->client.x, &cw->client.y, &tmpwin);
	
	cw->client.width = wattr.width;
	cw->client.height = wattr.height;
	
	cw->mini.x = cw->mini.y = 0;
	cw->mini.width = cw->mini.height = 1;
}

void
clientwin_destroy(ClientWin *cw, Bool parentDestroyed)
{
	if(! parentDestroyed)
	{
		if(cw->origin != None)
			XRenderFreePicture(cw->mainwin->dpy, cw->origin);
		if(cw->damage != None)
			XDamageDestroy(cw->mainwin->dpy, cw->damage);
	}
	if(cw->destination != None)
		XRenderFreePicture(cw->mainwin->dpy, cw->destination);
	if(cw->pixmap != None)
		XFreePixmap(cw->mainwin->dpy, cw->pixmap);
	
	XDestroyWindow(cw->mainwin->dpy, cw->mini.window);
	
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
	
	if(cw->mainwin->lazy_trans)
	{
		XRenderComposite(cw->mainwin->dpy, PictOpSrc, cw->origin,
		                 cw->focused ? cw->mainwin->highlightPicture : cw->mainwin->normalPicture,
		                 cw->destination, s_x, s_y, 0, 0, s_x, s_y, s_w, s_h);
	}
	else
	{
		XRenderComposite(cw->mainwin->dpy, PictOpSrc, cw->mainwin->background, None, cw->destination, cw->mini.x + s_x, cw->mini.y + s_y, 0, 0, s_x, s_y, s_w, s_h);
		XRenderComposite(cw->mainwin->dpy, PictOpOver, cw->origin,
		                 cw->focused ? cw->mainwin->highlightPicture : cw->mainwin->normalPicture,
		                 cw->destination, s_x, s_y, 0, 0, s_x, s_y, s_w, s_h);
	}
	
	if(tint->alpha)
		XRenderFillRectangle(cw->mainwin->dpy, PictOpOver, cw->destination, tint, s_x, s_y, s_w, s_h);
	
	XClearArea(cw->mainwin->dpy, cw->mini.window, s_x, s_y, s_w, s_h, False);
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
	XserverRegion rgn = XFixesCreateRegion(cw->mainwin->dpy, 0, 0);
	
	XDamageSubtract(cw->mainwin->dpy, cw->damage, None, rgn);
	
	rects = XFixesFetchRegion(cw->mainwin->dpy, rgn, &nrects);
	XFixesDestroyRegion(cw->mainwin->dpy, rgn);
	
	for(i = 0; i < nrects; i++)
		clientwin_repaint(cw, &rects[i]);
	
	if(rects)
		XFree(rects);
	
	cw->damaged = False;
}

void
clientwin_schedule_repair(ClientWin *cw, XRectangle *area)
{
	cw->damaged = True;
}

void
clientwin_move(ClientWin *cw, float f, int x, int y)
{
	/* int border = MAX(1, (double)DISTANCE(cw->mainwin) * f * 0.25); */
	int border = 0;
	XSetWindowBorderWidth(cw->mainwin->dpy, cw->mini.window, border);
	
	cw->factor = f;
	cw->mini.x = x + (int)cw->x * f;
	cw->mini.y = y + (int)cw->y * f;
	if(cw->mainwin->lazy_trans)
	{
		cw->mini.x += cw->mainwin->x;
		cw->mini.y += cw->mainwin->y;
	}
	cw->mini.width = MAX(1, (int)cw->client.width * f);
	cw->mini.height = MAX(1, (int)cw->client.height * f);
	XMoveResizeWindow(cw->mainwin->dpy, cw->mini.window, cw->mini.x - border, cw->mini.y - border, cw->mini.width, cw->mini.height);
	
	if(cw->pixmap)
		XFreePixmap(cw->mainwin->dpy, cw->pixmap);
	
	if(cw->destination)
		XRenderFreePicture(cw->mainwin->dpy, cw->destination);
	
	cw->pixmap = XCreatePixmap(cw->mainwin->dpy, cw->mini.window, cw->mini.width, cw->mini.height, cw->mainwin->depth);
	XSetWindowBackgroundPixmap(cw->mainwin->dpy, cw->mini.window, cw->pixmap);
	
	cw->destination = XRenderCreatePicture(cw->mainwin->dpy, cw->pixmap, cw->mini.format, 0, 0);
}

void
clientwin_map(ClientWin *cw)
{
	if(cw->damage)
		XDamageDestroy(cw->mainwin->dpy, cw->damage);
	
	cw->damage = XDamageCreate(cw->mainwin->dpy, cw->client.window, XDamageReportDeltaRectangles);
	XRenderSetPictureTransform(cw->mainwin->dpy, cw->origin, &cw->mainwin->transform);
	
	clientwin_render(cw);
	
	XMapWindow(cw->mainwin->dpy, cw->mini.window);
}

void
clientwin_unmap(ClientWin *cw)
{
	if(cw->damage)
	{
		XDamageDestroy(cw->mainwin->dpy, cw->damage);
		cw->damage = None;
	}
	
	if(cw->destination)
	{
		XRenderFreePicture(cw->mainwin->dpy, cw->destination);
		cw->destination = None;
	}
	
	if(cw->pixmap)
	{
		XFreePixmap(cw->mainwin->dpy, cw->pixmap);
		cw->pixmap = None;
	}
	
	XUnmapWindow(cw->mainwin->dpy, cw->mini.window);
	XSetWindowBackgroundPixmap(cw->mainwin->dpy, cw->mini.window, None);
	
	cw->focused = 0;
}

static void
childwin_focus(ClientWin *cw)
{
	XWarpPointer(cw->mainwin->dpy, None, cw->client.window, 0, 0, 0, 0, cw->client.width / 2, cw->client.height / 2);
	XRaiseWindow(cw->mainwin->dpy, cw->client.window);
	XSetInputFocus(cw->mainwin->dpy, cw->client.window, RevertToParent, CurrentTime);
}

int
clientwin_handle(ClientWin *cw, XEvent *ev)
{
	if((ev->type == ButtonRelease && ev->xbutton.button == 1 && cw->mainwin->pressed == cw)) {
		if((ev->xbutton.x >= 0 && ev->xbutton.y >= 0 && ev->xbutton.x < cw->mini.width && ev->xbutton.y < cw->mini.height))
			childwin_focus(cw);
		cw->mainwin->pressed = 0;
		return 1;
	} else if(ev->type == KeyRelease) {
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
	} else if(ev->type == ButtonPress && ev->xbutton.button == 1) {
		cw->mainwin->pressed = cw;
	} else if(ev->type == FocusIn) {
		cw->focused = 1;
		clientwin_render(cw);
		XFlush(cw->mainwin->dpy);
	} else if(ev->type == FocusOut) {
		cw->focused = 0;
		clientwin_render(cw);
		XFlush(cw->mainwin->dpy);
	} else if(ev->type == EnterNotify) {
		XSetInputFocus(cw->mainwin->dpy, cw->mini.window, RevertToNone, CurrentTime);
		if(cw->mainwin->tooltip)
		{
			int win_title_len = 0;
			FcChar8 *win_title = wm_get_window_title(cw->mainwin->dpy, cw->client.window, &win_title_len);
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
