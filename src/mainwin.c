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

/* from 'uncover': */
static Visual *
find_argb_visual (Display *dpy, int scr)
{
	XVisualInfo template = {
		.screen = scr, .depth = 32, .class = TrueColor,
	};
	int	nvi = 0;
	XVisualInfo *xvi = XGetVisualInfo (dpy,
			VisualScreenMask | VisualDepthMask | VisualClassMask,
			&template, &nvi);
	if (!xvi) return NULL;

	Visual *visual = NULL;;
	for (int i = 0; i < nvi; ++i) {
		XRenderPictFormat *format =
			XRenderFindVisualFormat (dpy, xvi[i].visual);
		if (format->type == PictTypeDirect && format->direct.alphaMask) {
			visual = xvi[i].visual;
			break;
		}
	}

	XFree (xvi);
	return visual;
}

MainWin *
mainwin_create(session_t *ps) {
	Display * const dpy = ps->dpy;

	const char *tmp;
	double tmp_d;
	XColor exact_color;
	XSetWindowAttributes wattr;
	XWindowAttributes rootattr;
	XRenderPictureAttributes pa;
	XRenderColor clear;

	// Get ARGB visual.
	// FIXME: Move this to skippy.c?
	if (!ps->argb_visual)
		ps->argb_visual = find_argb_visual(dpy, ps->screen);

	// calloc() makes sure it's filled with zero
	MainWin *mw = allocchk(calloc(1, sizeof(MainWin)));

	mw->ps = ps;
	if (ps->o.lazyTrans) {
		mw->depth  = 32;
		mw->visual = ps->argb_visual;
		if (!mw->visual) {
			printfef("(): Couldn't find ARGB visual, lazy transparency can't work.");
			goto mainwin_create_err;
		}
	}
	if (!ps->o.lazyTrans) {
		mw->depth = DefaultDepth(dpy, ps->screen);
		mw->visual = DefaultVisual(dpy, ps->screen);
	}
	mw->colormap = XCreateColormap(dpy, ps->root, mw->visual, AllocNone);
	mw->bg_pixmap = None;
	mw->background = None;
	mw->format = XRenderFindVisualFormat(dpy, mw->visual);
#ifdef CFG_XINERAMA
	mw->xin_info = mw->xin_active = 0;
	mw->xin_screens = 0;
#endif /* CFG_XINERAMA */
	
	mw->pressed = mw->focus = 0;
	mw->tooltip = 0;
	mw->cod = 0;
	mw->key_up = XKeysymToKeycode(dpy, XK_Up);
	mw->key_down = XKeysymToKeycode(dpy, XK_Down);
	mw->key_left = XKeysymToKeycode(dpy, XK_Left);
	mw->key_right = XKeysymToKeycode(dpy, XK_Right);
	mw->key_h = XKeysymToKeycode(dpy, XK_h);
	mw->key_j = XKeysymToKeycode(dpy, XK_j);
	mw->key_k = XKeysymToKeycode(dpy, XK_k);
	mw->key_l = XKeysymToKeycode(dpy, XK_l);
	mw->key_enter = XKeysymToKeycode(dpy, XK_Return);
	mw->key_space = XKeysymToKeycode(dpy, XK_space);
	mw->key_escape = XKeysymToKeycode(dpy, XK_Escape);
	mw->key_q = XKeysymToKeycode(dpy, XK_q);
	
	XGetWindowAttributes(dpy, ps->root, &rootattr);
	mw->x = mw->y = 0;
	mw->width = rootattr.width;
	mw->height = rootattr.height;
	
	wattr.colormap = mw->colormap;
	wattr.background_pixel = wattr.border_pixel = 0;
	wattr.override_redirect = True;
	// I have no idea why, but seemingly without ButtonPressMask, we can't
	// receive ButtonRelease events in some cases
	wattr.event_mask = VisibilityChangeMask | ButtonPressMask
		| ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
	
	mw->window = XCreateWindow(dpy, ps->root, 0, 0, mw->width, mw->height, 0,
			mw->depth, InputOutput, mw->visual,
			CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect, &wattr);
	if (!mw->window) {
		free(mw);
		return 0;
	}
	wm_wid_set_info(ps, mw->window, "main window", None);

#ifdef CFG_XINERAMA
	if (ps->xinfo.xinerama_exist && XineramaIsActive(dpy)) {
		mw->xin_info = XineramaQueryScreens(ps->dpy, &mw->xin_screens);
# ifdef DEBUG_XINERAMA
		printfef("(): Xinerama is enabled (%d screens).", mw->xin_screens);
# endif /* DEBUG_XINERAMA */
	}
#endif /* CFG_XINERAMA */

	XCompositeRedirectSubwindows (ps->dpy, ps->root, CompositeRedirectAutomatic);
	
	tmp_d = ps->o.updateFreq;
	if(tmp_d != 0.0)
		mw->poll_time = (1.0 / tmp_d) * 1000.0;
	else
		mw->poll_time = 0;
	
	if(!XParseColor(ps->dpy, mw->colormap, ps->o.normal_tint, &exact_color)) {
		printfef("(): Couldn't look up color '%s', reverting to black.", ps->o.normal_tint);
		mw->normalTint.red = mw->normalTint.green = mw->normalTint.blue = 0;
	}
	else
	{
		mw->normalTint.red = exact_color.red;
		mw->normalTint.green = exact_color.green;
		mw->normalTint.blue = exact_color.blue;
	}
	mw->normalTint.alpha = alphaconv(ps->o.normal_tintOpacity);
	
	tmp = ps->o.highlight_tint;
	if(! XParseColor(ps->dpy, mw->colormap, tmp, &exact_color))
	{
		fprintf(stderr, "Couldn't look up color '%s', reverting to #101020", tmp);
		mw->highlightTint.red = mw->highlightTint.green = 0x10;
		mw->highlightTint.blue = 0x20;
	}
	else
	{
		mw->highlightTint.red = exact_color.red;
		mw->highlightTint.green = exact_color.green;
		mw->highlightTint.blue = exact_color.blue;
	}
	mw->highlightTint.alpha = alphaconv(ps->o.highlight_tintOpacity);
	
	pa.repeat = True;
	clear.alpha = alphaconv(ps->o.normal_opacity);
	mw->normalPixmap = XCreatePixmap(ps->dpy, mw->window, 1, 1, 8);
	mw->normalPicture = XRenderCreatePicture(ps->dpy, mw->normalPixmap, XRenderFindStandardFormat(ps->dpy, PictStandardA8), CPRepeat, &pa);
	XRenderFillRectangle(ps->dpy, PictOpSrc, mw->normalPicture, &clear, 0, 0, 1, 1);
	
	clear.alpha = alphaconv(ps->o.highlight_opacity);
	mw->highlightPixmap = XCreatePixmap(ps->dpy, mw->window, 1, 1, 8);
	mw->highlightPicture = XRenderCreatePicture(ps->dpy, mw->highlightPixmap, XRenderFindStandardFormat(ps->dpy, PictStandardA8), CPRepeat, &pa);
	XRenderFillRectangle(ps->dpy, PictOpSrc, mw->highlightPicture, &clear, 0, 0, 1, 1);
	
	mw->distance = ps->o.distance;
	
	if (ps->o.tooltip_show)
		mw->tooltip = tooltip_create(mw);
	
	return mw;

mainwin_create_err:
	if (mw)
		free(mw);
	return NULL;
}

void
mainwin_update_background(MainWin *mw) {
	session_t *ps = mw->ps;

	Pixmap root = wm_get_root_pmap(ps->dpy);
	XRenderPictureAttributes pa;
	
	if(mw->bg_pixmap)
		XFreePixmap(ps->dpy, mw->bg_pixmap);
	if(mw->background)
		XRenderFreePicture(ps->dpy, mw->background);
	
	mw->bg_pixmap = XCreatePixmap(ps->dpy, mw->window, mw->width, mw->height, mw->depth);
	pa.repeat = True;
	mw->background = XRenderCreatePicture(ps->dpy, mw->bg_pixmap, mw->format, CPRepeat, &pa);
	
	if (ps->o.background) {
		XRenderComposite(ps->dpy, PictOpSrc, ps->o.background->pict, None, mw->background, mw->x, mw->y, 0, 0, 0, 0, mw->width, mw->height);
	}
	else if (!root) {
		static const XRenderColor black = { 0, 0, 0, 0xffff};
		XRenderFillRectangle(ps->dpy, PictOpSrc, mw->background, &black, 0, 0, mw->width, mw->height);
	}
	else {
		Picture from = XRenderCreatePicture(ps->dpy, root, XRenderFindVisualFormat(ps->dpy, DefaultVisual(ps->dpy, ps->screen)), 0, 0);
		XRenderComposite(ps->dpy, PictOpSrc, from, None, mw->background, mw->x, mw->y, 0, 0, 0, 0, mw->width, mw->height);
		XRenderFreePicture(ps->dpy, from);
	}
	
	XSetWindowBackgroundPixmap(ps->dpy, mw->window, mw->bg_pixmap);
	XClearWindow(ps->dpy, mw->window);
}

void
mainwin_update(MainWin *mw)
{
	session_t * const ps = mw->ps;

#ifdef CFG_XINERAMA
	XineramaScreenInfo *iter;
	int i;
	Window dummy_w;
	int root_x, root_y, dummy_i;
	unsigned int dummy_u;
	
	if(! mw->xin_info || ! mw->xin_screens)
	{
		mainwin_update_background(mw);
		return;
	}
	
# ifdef DEBUG
	fprintf(stderr, "--> querying pointer... ");
# endif /* DEBUG */
	XQueryPointer(ps->dpy, ps->root, &dummy_w, &dummy_w, &root_x, &root_y, &dummy_i, &dummy_i, &dummy_u);
# ifdef DEBUG	
	fprintf(stderr, "+%i+%i\n", root_x, root_y);
	
	fprintf(stderr, "--> figuring out which screen we're on... ");
# endif /* DEBUG */
	iter = mw->xin_info;
	for(i = 0; i < mw->xin_screens; ++i)
	{
		if(root_x >= iter->x_org && root_x < iter->x_org + iter->width &&
		   root_y >= iter->y_org && root_y < iter->y_org + iter->height)
		{
# ifdef DEBUG
			fprintf(stderr, "screen %i %ix%i+%i+%i\n", iter->screen_number, iter->width, iter->height, iter->x_org, iter->y_org);
# endif /* DEBUG */
			break;
		}
		iter++;
	}
	if(i == mw->xin_screens)
	{
# ifdef DEBUG 
		fprintf(stderr, "unknown\n");
# endif /* DEBUG */
		return;
	}
	mw->x = iter->x_org;
	mw->y = iter->y_org;
	mw->width = iter->width;
	mw->height = iter->height;
	XMoveResizeWindow(ps->dpy, mw->window, iter->x_org, iter->y_org, mw->width, mw->height);
	mw->xin_active = iter;
#endif /* CFG_XINERAMA */
	mainwin_update_background(mw);
}

void
mainwin_map(MainWin *mw) {
	session_t *ps = mw->ps;

	wm_set_fullscreen(ps, mw->window, mw->x, mw->y, mw->width, mw->height);
	mw->pressed = NULL;
	mw->pressed_key = mw->pressed_mouse = false;
	XMapWindow(ps->dpy, mw->window);
	XRaiseWindow(ps->dpy, mw->window);

	// Might because of WM reparent, XSetInputFocus() doesn't work here
	// Focus is already on mini window?
	XSetInputFocus(ps->dpy, mw->window, RevertToParent, CurrentTime);
	/* {
		int ret = XGrabKeyboard(ps->dpy, mw->window, True, GrabModeAsync,
				GrabModeAsync, CurrentTime);
		if (Success != ret)
			printfef("(): Failed to grab keyboard (%d), troubles ahead.", ret);
	} */
}

void
mainwin_unmap(MainWin *mw)
{
	if(mw->tooltip)
		tooltip_unmap(mw->tooltip);
	if(mw->bg_pixmap)
	{
		XFreePixmap(mw->ps->dpy, mw->bg_pixmap);
		mw->bg_pixmap = None;
	}
	XUngrabKeyboard(mw->ps->dpy, CurrentTime);
	XUnmapWindow(mw->ps->dpy, mw->window);
}

void
mainwin_destroy(MainWin *mw) {
	session_t *ps = mw->ps; 

	// Free all clients associated with this main window
	dlist_free_with_func(mw->clients, (dlist_free_func) clientwin_destroy);

	if(mw->tooltip)
		tooltip_destroy(mw->tooltip);
	
	if(mw->background != None)
		XRenderFreePicture(ps->dpy, mw->background);
	
	if(mw->bg_pixmap != None)
		XFreePixmap(ps->dpy, mw->bg_pixmap);
	
	if(mw->normalPicture != None)
		XRenderFreePicture(ps->dpy, mw->normalPicture);
	
	if(mw->highlightPicture != None)
		XRenderFreePicture(ps->dpy, mw->highlightPicture);
	
	if(mw->normalPixmap != None)
		XFreePixmap(ps->dpy, mw->normalPixmap);
	
	if(mw->highlightPixmap != None)
		XFreePixmap(ps->dpy, mw->highlightPixmap);
	
	XDestroyWindow(ps->dpy, mw->window);
	
#ifdef CFG_XINERAMA
	if(mw->xin_info)
		XFree(mw->xin_info);
#endif /* CFG_XINERAMA */
	
	free(mw);
}

void
mainwin_transform(MainWin *mw, float f)
{
	mw->transform.matrix[0][0] = XDoubleToFixed(1.0 / f);
	mw->transform.matrix[0][1] = 0.0;
	mw->transform.matrix[0][2] = 0.0;
	mw->transform.matrix[1][0] = 0.0;
	mw->transform.matrix[1][1] = XDoubleToFixed(1.0 / f);
	mw->transform.matrix[1][2] = 0.0;
	mw->transform.matrix[2][0] = 0.0;
	mw->transform.matrix[2][1] = 0.0;
	mw->transform.matrix[2][2] = XDoubleToFixed(1.0);
}

int
mainwin_handle(MainWin *mw, XEvent *ev) {
	session_t *ps = mw->ps;

	switch(ev->type) {
		case EnterNotify:
			XSetInputFocus(ps->dpy, mw->window, RevertToParent, CurrentTime);
			break;
		case KeyPress:
			mw->pressed_key = true;
			break;
		case KeyRelease:
			if (mw->pressed_key)
				report_key_unbinded(ev);
			else
				report_key_ignored(ev);
			break;
		case ButtonPress:
			mw->pressed_mouse = true;
			break;
		case ButtonRelease:
			if (mw->pressed_mouse) {
				printfef("(): Detected mouse button release on main window, "
						"exiting.");
				return 1;
			}
			else
				printfef("(): ButtonRelease %u ignored.", ev->xbutton.button);
			break;
	}

	return 0;
}
