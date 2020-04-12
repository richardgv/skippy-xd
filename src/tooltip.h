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

#ifndef SKIPPY_TOOLTIP_H
#define SKIPPY_TOOLTIP_H

struct _Tooltip {
	MainWin *mainwin;
	
	unsigned int width, height;
	
	Window window;
	XftFont *font;
	XftDraw *draw;
	XftColor color, background, border, shadow;
	XGlyphInfo extents;
	int font_height;
	
	FcChar8 *text;
	int text_len;
};
typedef struct _Tooltip Tooltip;

Tooltip *tooltip_create(MainWin *mw);
void tooltip_destroy(Tooltip *);
void tooltip_map(Tooltip *tt, int mouse_x, int mouse_y,
		const FcChar8 *text, int len);
void tooltip_unmap(Tooltip *);
void tooltip_handle(Tooltip *, XEvent *);
void tooltip_move(Tooltip *tt, int mouse_x, int mouse_y);

#endif /* SKIPPY_TOOLTIP_H */
