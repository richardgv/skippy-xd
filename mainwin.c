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
    XVisualInfo		*xvi;
    XVisualInfo		template;
    int			nvi;
    int			i;
    XRenderPictFormat	*format;
    Visual		*visual;
    
    template.screen = scr;
    template.depth = 32;
    template.class = TrueColor;
    xvi = XGetVisualInfo (dpy, 
			  VisualScreenMask |
			  VisualDepthMask |
			  VisualClassMask,
			  &template,
			  &nvi);
    if (!xvi)
	return 0;
    visual = 0;
    for (i = 0; i < nvi; i++)
    {
	format = XRenderFindVisualFormat (dpy, xvi[i].visual);
	if (format->type == PictTypeDirect && format->direct.alphaMask)
	{
	    visual = xvi[i].visual;
	    break;
	}
    }

    XFree (xvi);
    return visual;
}

MainWin *
mainwin_create(Display *dpy, dlist *config)
{
	const char *tmp;
	double tmp_d;
	XColor exact_color;
	XSetWindowAttributes wattr;
	XWindowAttributes rootattr;
	XRenderPictureAttributes pa;
	XRenderColor clear;
	int error_base;
#ifdef XINERAMA
	int event_base;
#endif /* XINERAMA */
	
	MainWin *mw = (MainWin *)malloc(sizeof(MainWin));
	
	mw->dpy = dpy;
	mw->screen = DefaultScreen(dpy);
	mw->root = RootWindow(dpy, mw->screen);
	mw->lazy_trans = (strcasecmp(config_get(config, "general", "lazyTrans", "false"), "true") == 0) ? True : False;
	if(mw->lazy_trans)
	{
		mw->depth  = 32;
		mw->visual = find_argb_visual(dpy, DefaultScreen(dpy));
		if(! mw->visual)
		{
			fprintf(stderr, "WARNING: Couldn't find argb visual, disabling lazy transparency.\n");
			mw->lazy_trans = False;
		}
	}
	if(! mw->lazy_trans)
	{
		mw->depth = DefaultDepth(dpy, mw->screen);
		mw->visual = DefaultVisual(dpy, mw->screen);
	}
	mw->colormap = XCreateColormap(dpy, mw->root, mw->visual, AllocNone);
	mw->bg_pixmap = None;
	mw->background = None;
	mw->format = XRenderFindVisualFormat(dpy, mw->visual);
#ifdef XINERAMA
	mw->xin_info = mw->xin_active = 0;
	mw->xin_screens = 0;
#endif /* XINERAMA */
	
	mw->pressed = mw->focus = 0;
	mw->tooltip = 0;
	mw->cod = 0;
	mw->key_up = XKeysymToKeycode(dpy, XK_Up);
	mw->key_down = XKeysymToKeycode(dpy, XK_Down);
	mw->key_left = XKeysymToKeycode(dpy, XK_Left);
	mw->key_right = XKeysymToKeycode(dpy, XK_Right);
	mw->key_enter = XKeysymToKeycode(dpy, XK_Return);
	mw->key_space = XKeysymToKeycode(dpy, XK_space);
	mw->key_escape = XKeysymToKeycode(dpy, XK_Escape);
	mw->key_q = XKeysymToKeycode(dpy, XK_q);
	
	XGetWindowAttributes(dpy, mw->root, &rootattr);
	mw->x = mw->y = 0;
	mw->width = rootattr.width;
	mw->height = rootattr.height;
	
	wattr.colormap = mw->colormap;
	wattr.background_pixel = wattr.border_pixel = 0;
	wattr.event_mask = VisibilityChangeMask |
	                   ButtonReleaseMask;
	
	mw->window = XCreateWindow(dpy, mw->root, 0, 0, mw->width, mw->height, 0,
	                           mw->depth, InputOutput, mw->visual,
	                           CWBackPixel|CWBorderPixel|CWColormap|CWEventMask, &wattr);
	if(mw->window == None) {
		free(mw);
		return 0;
	}
	
#ifdef XINERAMA
# ifdef DEBUG
	fprintf(stderr, "--> checking for Xinerama extension... ");
# endif /* DEBUG */
	if(XineramaQueryExtension(dpy, &event_base, &error_base))
	{
# ifdef DEBUG
	    fprintf(stderr, "yes\n--> checking if Xinerama is enabled... ");
# endif /* DEBUG */
	    if(XineramaIsActive(dpy))
	    {
# ifdef DEBUG
	        fprintf(stderr, "yes\n--> fetching Xinerama info... ");
# endif /* DEBUG */
	        mw->xin_info = XineramaQueryScreens(mw->dpy, &mw->xin_screens);
# ifdef DEBUG	        
		fprintf(stderr, "done (%i screens)\n", mw->xin_screens);
# endif /* DEBUG */
	    }
# ifdef DEBUG
	    else
	        fprintf(stderr, "no\n");
# endif /* DEBUG */
	}
# ifdef DEBUG
	else
	    fprintf(stderr, "no\n");
# endif /* DEBUG */
#endif /* XINERAMA */
	
	if(! XDamageQueryExtension (dpy, &mw->damage_event_base, &error_base))
	{
		fprintf(stderr, "FATAL: XDamage extension not found.\n");
		exit(1);
	}
	
	if(! XCompositeQueryExtension(dpy, &event_base, &error_base))
	{
		fprintf(stderr, "FATAL: XComposite extension not found.\n");
		exit(1);
	}
	
	if(! XRenderQueryExtension(dpy, &event_base, &error_base))
	{
		fprintf(stderr, "FATAL: XRender extension not found.\n");
		exit(1);
	}
	
	if(! XFixesQueryExtension(dpy, &event_base, &error_base))
	{
		fprintf(stderr, "FATAL: XFixes extension not found.\n");
		exit(1);
	}
	
	XCompositeRedirectSubwindows (mw->dpy, mw->root, CompositeRedirectAutomatic);
	
	tmp_d = strtod(config_get(config, "general", "updateFreq", "10.0"), 0);
	if(tmp_d != 0.0)
		mw->poll_time = (1.0 / tmp_d) * 1000.0;
	else
		mw->poll_time = 0;
	
	tmp = config_get(config, "normal", "tint", "black");
	if(! XParseColor(mw->dpy, mw->colormap, tmp, &exact_color))
	{
		fprintf(stderr, "Couldn't look up color '%s', reverting to black", tmp);
		mw->normalTint.red = mw->normalTint.green = mw->normalTint.blue = 0;
	}
	else
	{
		mw->normalTint.red = exact_color.red;
		mw->normalTint.green = exact_color.green;
		mw->normalTint.blue = exact_color.blue;
	}
	mw->normalTint.alpha = MAX(0, MIN(strtol(config_get(config, "normal", "tintOpacity", "0"), 0, 0) * 256, 65535));
	
	tmp = config_get(config, "highlight", "tint", "#101020");
	if(! XParseColor(mw->dpy, mw->colormap, tmp, &exact_color))
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
	mw->highlightTint.alpha = MAX(0, MIN(strtol(config_get(config, "highlight", "tintOpacity", "64"), 0, 0) * 256, 65535));
	
	pa.repeat = True;
	clear.alpha = MAX(0, MIN(strtol(config_get(config, "normal", "opacity", "200"), 0, 10) * 256, 65535));
	mw->normalPixmap = XCreatePixmap(mw->dpy, mw->window, 1, 1, 8);
	mw->normalPicture = XRenderCreatePicture(mw->dpy, mw->normalPixmap, XRenderFindStandardFormat(mw->dpy, PictStandardA8), CPRepeat, &pa);
	XRenderFillRectangle(mw->dpy, PictOpSrc, mw->normalPicture, &clear, 0, 0, 1, 1);
	
	clear.alpha = MAX(0, MIN(strtol(config_get(config, "highlight", "opacity", "255"), 0, 10) * 256, 65535));
	mw->highlightPixmap = XCreatePixmap(mw->dpy, mw->window, 1, 1, 8);
	mw->highlightPicture = XRenderCreatePicture(mw->dpy, mw->highlightPixmap, XRenderFindStandardFormat(mw->dpy, PictStandardA8), CPRepeat, &pa);
	XRenderFillRectangle(mw->dpy, PictOpSrc, mw->highlightPicture, &clear, 0, 0, 1, 1);
	
	tmp = config_get(config, "general", "distance", "50");
	mw->distance = MAX(1, strtol(tmp, 0, 10));
	
	if(! strcasecmp(config_get(config, "tooltip", "show", "true"), "true"))
		mw->tooltip = tooltip_create(mw, config);
	
	return mw;
}

void
mainwin_update_background(MainWin *mw)
{
	Pixmap root = wm_get_root_pmap(mw->dpy);
	XRenderColor black = { 0, 0, 0, 65535};
	XRenderPictureAttributes pa;
	
	if(mw->bg_pixmap)
		XFreePixmap(mw->dpy, mw->bg_pixmap);
	if(mw->background)
		XRenderFreePicture(mw->dpy, mw->background);
	
	mw->bg_pixmap = XCreatePixmap(mw->dpy, mw->window, mw->width, mw->height, mw->depth);
	pa.repeat = True;
	mw->background = XRenderCreatePicture(mw->dpy, mw->bg_pixmap, mw->format, CPRepeat, &pa);
	
	if(root == None)
		XRenderFillRectangle(mw->dpy, PictOpSrc, mw->background, &black, 0, 0, mw->width, mw->height);
	else
	{
		Picture from = XRenderCreatePicture(mw->dpy, root, XRenderFindVisualFormat(mw->dpy, DefaultVisual(mw->dpy, mw->screen)), 0, 0);
		XRenderComposite(mw->dpy, PictOpSrc, from, None, mw->background, mw->x, mw->y, 0, 0, 0, 0, mw->width, mw->height);
		XRenderFreePicture(mw->dpy, from);
	}
	
	XSetWindowBackgroundPixmap(mw->dpy, mw->window, mw->bg_pixmap);
	XClearWindow(mw->dpy, mw->window);
}

void
mainwin_update(MainWin *mw)
{
#ifdef XINERAMA
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
	XQueryPointer(mw->dpy, mw->root, &dummy_w, &dummy_w, &root_x, &root_y, &dummy_i, &dummy_i, &dummy_u);
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
	XMoveResizeWindow(mw->dpy, mw->window, iter->x_org, iter->y_org, mw->width, mw->height);
	mw->xin_active = iter;
#endif /* XINERAMA */
	mainwin_update_background(mw);
}

void
mainwin_map(MainWin *mw)
{
	wm_set_fullscreen(mw->dpy, mw->window, mw->x, mw->y, mw->width, mw->height);
	mw->pressed = 0;
	XMapWindow(mw->dpy, mw->window);
	XRaiseWindow(mw->dpy, mw->window);
}

void
mainwin_unmap(MainWin *mw)
{
	if(mw->tooltip)
		tooltip_unmap(mw->tooltip);
	if(mw->bg_pixmap)
	{
		XFreePixmap(mw->dpy, mw->bg_pixmap);
		mw->bg_pixmap = None;
	}
	XUnmapWindow(mw->dpy, mw->window);
}

void
mainwin_destroy(MainWin *mw)
{
	if(mw->tooltip)
		tooltip_destroy(mw->tooltip);
	
	if(mw->background != None)
		XRenderFreePicture(mw->dpy, mw->background);
	
	if(mw->bg_pixmap != None)
		XFreePixmap(mw->dpy, mw->bg_pixmap);
	
	if(mw->normalPicture != None)
		XRenderFreePicture(mw->dpy, mw->normalPicture);
	
	if(mw->highlightPicture != None)
		XRenderFreePicture(mw->dpy, mw->highlightPicture);
	
	if(mw->normalPixmap != None)
		XFreePixmap(mw->dpy, mw->normalPixmap);
	
	if(mw->highlightPixmap != None)
		XFreePixmap(mw->dpy, mw->highlightPixmap);
	
	XDestroyWindow(mw->dpy, mw->window);
	
#ifdef XINERAMA
	if(mw->xin_info)
		XFree(mw->xin_info);
#endif /* XINERAMA */
	
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
mainwin_handle(MainWin *mw, XEvent *ev)
{
	switch(ev->type)
	{
	case KeyPress:
		if(ev->xkey.keycode == XKeysymToKeycode(mw->dpy, XK_q))
			return 2;
		break;
	case ButtonRelease:
		return 1;
		break;
	case VisibilityNotify:
		if(ev->xvisibility.state && mw->focus)
		{
			XSetInputFocus(mw->dpy, mw->focus->mini.window, RevertToParent, CurrentTime);
			mw->focus = 0;
		}
		break;
	default:
		;
	}
	return 0;
}
