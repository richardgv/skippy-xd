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
	session_t * const ps = tt->mainwin->ps;

	if(tt->text)
		free(tt->text);
	if(tt->font)
		XftFontClose(ps->dpy, tt->font);
	if(tt->draw)
		XftDrawDestroy(tt->draw);
	if(tt->color.pixel != None)
		XftColorFree(ps->dpy,
		             tt->mainwin->visual,
		             tt->mainwin->colormap,
		             &tt->color);
	if(tt->background.pixel != None)
		XftColorFree(ps->dpy,
		             tt->mainwin->visual,
		             tt->mainwin->colormap,
		             &tt->background);
	if(tt->border.pixel != None)
		XftColorFree(ps->dpy,
		             tt->mainwin->visual,
		             tt->mainwin->colormap,
		             &tt->border);
	if(tt->shadow.pixel != None)
		XftColorFree(ps->dpy,
		             tt->mainwin->visual,
		             tt->mainwin->colormap,
		             &tt->shadow);
	if(tt->window != None)
		XDestroyWindow(ps->dpy, tt->window);
	
	free(tt);
}

Tooltip *
tooltip_create(MainWin *mw) {
	session_t * const ps = mw->ps;
	const char *tmp;
	long int tmp_l;
	
	Tooltip *tt = allocchk(malloc(sizeof(Tooltip)));
	
	tt->mainwin = mw;
	tt->window = None;
	tt->font = 0;
	tt->draw = 0;
	tt->text = 0;
	tt->color.pixel = tt->background.pixel = tt->border.pixel = tt->shadow.pixel = None;
	
	{
		XSetWindowAttributes attr = {
			.override_redirect = True,
			.border_pixel = None,
			.background_pixel = None,
			.event_mask = ExposureMask,
			.colormap = mw->colormap,
		};
		
		tt->window = XCreateWindow(ps->dpy, ps->root,
		                           0, 0, 1, 1, 0,
		                           mw->depth, InputOutput, mw->visual,
		                           CWBorderPixel|CWBackPixel|CWOverrideRedirect|CWEventMask|CWColormap,
		                           &attr);
	}

	if (!tt->window) {
		printfef("(): WARNING: Couldn't create tooltip window.");
		tooltip_destroy(tt);
		return 0;
	}
	wm_wid_set_info(ps, tt->window, "tooltip", _NET_WM_WINDOW_TYPE_TOOLTIP);
	
	tmp = ps->o.tooltip_border;
	if(! XftColorAllocName(ps->dpy, mw->visual, mw->colormap, tmp, &tt->border))
	{
		fprintf(stderr, "WARNING: Invalid color '%s'.\n", tmp);
		tooltip_destroy(tt);
		return 0;
	}
	
	tmp = ps->o.tooltip_background;
	if(! XftColorAllocName(ps->dpy, mw->visual, mw->colormap, tmp, &tt->background))
	{
		fprintf(stderr, "WARNING: Invalid color '%s'.\n", tmp);
		tooltip_destroy(tt);
		return 0;
	}
	
	tmp_l = alphaconv(ps->o.tooltip_opacity);
	tt->background.color.alpha = tmp_l;
	tt->border.color.alpha = tmp_l;
	
	tmp = ps->o.tooltip_text;
	if(! XftColorAllocName(ps->dpy, mw->visual, mw->colormap, tmp, &tt->color))
	{
		fprintf(stderr, "WARNING: Couldn't allocate color '%s'.\n", tmp);
		tooltip_destroy(tt);
		return 0;
	}
	
	tmp = ps->o.tooltip_textShadow;
	if(strcasecmp(tmp, "none") != 0)
	{
		if(! XftColorAllocName(ps->dpy, mw->visual, mw->colormap, tmp, &tt->shadow))
		{
			fprintf(stderr, "WARNING: Couldn't allocate color '%s'.\n", tmp);
			tooltip_destroy(tt);
			return 0;
		}
	}
	
	tt->draw = XftDrawCreate(ps->dpy, tt->window, mw->visual, mw->colormap);
	if(! tt->draw)
	{
		fprintf(stderr, "WARNING: Couldn't create Xft draw surface.\n");
		tooltip_destroy(tt);
		return 0;
	}
	
	tt->font = XftFontOpenName(ps->dpy, ps->screen, ps->o.tooltip_font);
	if(! tt->font)
	{
		fprintf(stderr, "WARNING: Couldn't open Xft font.\n");
		tooltip_destroy(tt);
		return 0;
	}
	
	tt->font_height = tt->font->ascent + tt->font->descent;
	
	// Set tooltip window input region to empty to prevent disgusting
	// racing situations
	{
		XserverRegion region = XFixesCreateRegion(ps->dpy, NULL, 0);
		XFixesSetWindowShapeRegion(ps->dpy, tt->window, ShapeInput, 0, 0, region);
		XFixesDestroyRegion(ps->dpy, region);
	}

	return tt;
}

void
tooltip_map(Tooltip *tt, int mouse_x, int mouse_y,
		const FcChar8 *text, int len) {
	session_t * const ps = tt->mainwin->ps;

	XUnmapWindow(ps->dpy, tt->window);
	
	XftTextExtentsUtf8(ps->dpy, tt->font, text, len, &tt->extents);
	
	tt->width = tt->extents.width + 8;
	tt->height = tt->font_height + 5 + (tt->shadow.pixel ? 2 : 0);
	XResizeWindow(ps->dpy, tt->window, tt->width, tt->height);
	tooltip_move(tt, mouse_x, mouse_y);
	
	if(tt->text)
		free(tt->text);
	
	tt->text = (FcChar8 *)malloc(len);
	memcpy(tt->text, text, len);
	
	tt->text_len = len;
	
	XMapWindow(ps->dpy, tt->window);
	XRaiseWindow(ps->dpy, tt->window);
}

void
tooltip_move(Tooltip *tt, int mouse_x, int mouse_y) {
	session_t *ps = tt->mainwin->ps;
	int x = ps->o.tooltip_offsetX, y = ps->o.tooltip_offsetY;
	if (ps->o.tooltip_followsMouse) {
		x += mouse_x;
		y += mouse_y;
	}
	switch (ps->o.tooltip_align) {
		case ALIGN_LEFT: break;
		case ALIGN_MID:
						 x -= tt->width / 2;
						 break;
		case ALIGN_RIGHT:
						 x -= tt->width;
						 break;
	}
	x = MIN(MAX(0, x), tt->mainwin->x + tt->mainwin->width - tt->width);
	y = MIN(MAX(0, y), tt->mainwin->y + tt->mainwin->height - tt->height);
	
	XMoveWindow(tt->mainwin->ps->dpy, tt->window, x, y);
}

void
tooltip_unmap(Tooltip *tt)
{
	XUnmapWindow(tt->mainwin->ps->dpy, tt->window);
	if(tt->text)
		free(tt->text);
	tt->text = 0;
	tt->text_len = 0;
}

void
tooltip_handle(Tooltip *tt, XEvent *ev)
{
	if (!tt->text)
		return;
	
	if (ev->type == Expose && ev->xexpose.count == 0) {
		XftDrawRect(tt->draw, &tt->border, 0, 0, tt->width, 1);
		XftDrawRect(tt->draw, &tt->border, 0, 1, 1, tt->height - 2);
		XftDrawRect(tt->draw, &tt->border, 0, tt->height - 1, tt->width, 1);
		XftDrawRect(tt->draw, &tt->border, tt->width - 1, 1, 1, tt->height - 2);
		XftDrawRect(tt->draw, &tt->background, 1, 1, tt->width - 2, tt->height - 2);
		if(tt->shadow.pixel)
			XftDrawStringUtf8(tt->draw, &tt->shadow, tt->font, 6, 3 + tt->extents.y + (tt->font_height - tt->extents.y) / 2, tt->text, tt->text_len);
		XftDrawStringUtf8(tt->draw, &tt->color, tt->font, 4, 1 + tt->extents.y + (tt->font_height - tt->extents.y) / 2, tt->text, tt->text_len);
	}
}
