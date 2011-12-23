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

void
tooltip_destroy(Tooltip *tt)
{
	if(tt->text)
		free(tt->text);
	if(tt->font)
		XftFontClose(tt->mainwin->dpy, tt->font);
	if(tt->draw)
		XftDrawDestroy(tt->draw);
	if(tt->color.pixel != None)
		XftColorFree(tt->mainwin->dpy,
		             tt->mainwin->visual,
		             tt->mainwin->colormap,
		             &tt->color);
	if(tt->background.pixel != None)
		XftColorFree(tt->mainwin->dpy,
		             tt->mainwin->visual,
		             tt->mainwin->colormap,
		             &tt->background);
	if(tt->border.pixel != None)
		XftColorFree(tt->mainwin->dpy,
		             tt->mainwin->visual,
		             tt->mainwin->colormap,
		             &tt->border);
	if(tt->shadow.pixel != None)
		XftColorFree(tt->mainwin->dpy,
		             tt->mainwin->visual,
		             tt->mainwin->colormap,
		             &tt->shadow);
	if(tt->window != None)
		XDestroyWindow(tt->mainwin->dpy, tt->window);
	
	free(tt);
}

Tooltip *
tooltip_create(MainWin *mw, dlist *config)
{
	Tooltip *tt;
	XSetWindowAttributes attr;
	const char *tmp;
	long int tmp_l;
	
	tt = (Tooltip *)malloc(sizeof(Tooltip));
	if(! tt)
		return 0;
	
	tt->mainwin = mw;
	tt->window = None;
	tt->font = 0;
	tt->draw = 0;
	tt->text = 0;
	tt->color.pixel = tt->background.pixel = tt->border.pixel = tt->shadow.pixel = None;
	
	attr.override_redirect = True;
	attr.border_pixel = None;
	attr.background_pixel = None;
	attr.event_mask = ExposureMask;
	attr.colormap = mw->colormap;
	
	tt->window = XCreateWindow(mw->dpy, mw->root,
	                           0, 0, 1, 1, 0,
	                           mw->depth, InputOutput, mw->visual,
	                           CWBorderPixel|CWBackPixel|CWOverrideRedirect|CWEventMask|CWColormap,
	                           &attr);
	if(tt->window == None)
	{
		fprintf(stderr, "WARNING: Couldn't create tooltip window.\n");
		tooltip_destroy(tt);
		return 0;
	}
	
	tmp = config_get(config, "tooltip", "border", "#e0e0e0");
	if(! XftColorAllocName(mw->dpy, mw->visual, mw->colormap, tmp, &tt->border))
	{
		fprintf(stderr, "WARNING: Invalid color '%s'.\n", tmp);
		tooltip_destroy(tt);
		return 0;
	}
	
	tmp = config_get(config, "tooltip", "background", "#404040");
	if(! XftColorAllocName(mw->dpy, mw->visual, mw->colormap, tmp, &tt->background))
	{
		fprintf(stderr, "WARNING: Invalid color '%s'.\n", tmp);
		tooltip_destroy(tt);
		return 0;
	}
	
	tmp = config_get(config, "tooltip", "opacity", "128");
	tmp_l = MIN(MAX(0, strtol(tmp, 0, 0) * 256), 65535);
	tt->background.color.alpha = tmp_l;
	tt->border.color.alpha = tmp_l;
	
	tmp = config_get(config, "tooltip", "text", "#e0e0e0");
	if(! XftColorAllocName(mw->dpy, mw->visual, mw->colormap, tmp, &tt->color))
	{
		fprintf(stderr, "WARNING: Couldn't allocate color '%s'.\n", tmp);
		tooltip_destroy(tt);
		return 0;
	}
	
	tmp = config_get(config, "tooltip", "textShadow", "black");
	if(strcasecmp(tmp, "none") != 0)
	{
		if(! XftColorAllocName(mw->dpy, mw->visual, mw->colormap, tmp, &tt->shadow))
		{
			fprintf(stderr, "WARNING: Couldn't allocate color '%s'.\n", tmp);
			tooltip_destroy(tt);
			return 0;
		}
	}
	
	tt->draw = XftDrawCreate(mw->dpy, tt->window, mw->visual, mw->colormap);
	if(! tt->draw)
	{
		fprintf(stderr, "WARNING: Couldn't create Xft draw surface.\n");
		tooltip_destroy(tt);
		return 0;
	}
	
	tt->font = XftFontOpenName(mw->dpy, mw->screen, config_get(config, "tooltip", "font", "fixed-11:weight=bold"));
	if(! tt->font)
	{
		fprintf(stderr, "WARNING: Couldn't open Xft font.\n");
		tooltip_destroy(tt);
		return 0;
	}
	
	tt->font_height = tt->font->ascent + tt->font->descent;
	
	return tt;
}

void
tooltip_map(Tooltip *tt, int x, int y, const FcChar8 *text, int len)
{
	XUnmapWindow(tt->mainwin->dpy, tt->window);
	
	XftTextExtents8(tt->mainwin->dpy, tt->font, text, len, &tt->extents);
	
	tt->width = tt->extents.width + 8;
	tt->height = tt->font_height + 5 + (tt->shadow.pixel ? 2 : 0);
	XResizeWindow(tt->mainwin->dpy, tt->window, tt->width, tt->height);
	tooltip_move(tt, x, y);
	
	if(tt->text)
		free(tt->text);
	
	tt->text = (FcChar8 *)malloc(len);
	memcpy(tt->text, text, len);
	
	tt->text_len = len;
	
	XMapWindow(tt->mainwin->dpy, tt->window);
	XRaiseWindow(tt->mainwin->dpy, tt->window);
}

void
tooltip_move(Tooltip *tt, int x, int y)
{
	if(x + tt->extents.width + 9 > tt->mainwin->x + tt->mainwin->width)
		x = tt->mainwin->x + tt->mainwin->width - tt->extents.width - 9;
	x = MAX(0, x);
	
	if(y + tt->extents.height + 8 > tt->mainwin->y + tt->mainwin->height)
		y = tt->mainwin->height + tt->mainwin->y - tt->extents.height - 8;
	y = MAX(0, y);
	
	XMoveWindow(tt->mainwin->dpy, tt->window, x, y);
}

void
tooltip_unmap(Tooltip *tt)
{
	XUnmapWindow(tt->mainwin->dpy, tt->window);
	if(tt->text)
		free(tt->text);
	tt->text = 0;
	tt->text_len = 0;
}

void
tooltip_handle(Tooltip *tt, XEvent *ev)
{
	if(! tt->text)
		return;
	
	if(ev->type == Expose && ev->xexpose.count == 0)
	{
		XftDrawRect(tt->draw, &tt->border, 0, 0, tt->width, 1);
		XftDrawRect(tt->draw, &tt->border, 0, 1, 1, tt->height - 2);
		XftDrawRect(tt->draw, &tt->border, 0, tt->height - 1, tt->width, 1);
		XftDrawRect(tt->draw, &tt->border, tt->width - 1, 1, 1, tt->height - 2);
		XftDrawRect(tt->draw, &tt->background, 1, 1, tt->width - 2, tt->height - 2);
		if(tt->shadow.pixel)
			XftDrawString8(tt->draw, &tt->shadow, tt->font, 6, 3 + tt->extents.y + (tt->font_height - tt->extents.y) / 2, tt->text, tt->text_len);
		XftDrawString8(tt->draw, &tt->color, tt->font, 4, 1 + tt->extents.y + (tt->font_height - tt->extents.y) / 2, tt->text, tt->text_len);
	}
}
