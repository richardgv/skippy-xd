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

#ifndef SKIPPY_MAINWIN_H
#define SKIPPY_MAINWIN_H

struct _Tooltip;

struct _MainWin
{
	session_t *ps;
	Visual *visual;
	Colormap colormap;
	int depth;
	
	/// @brief Whether the KeyRelease events should already be acceptable.
	bool pressed_key;
	/// @brief Whether the ButtonRelease events should already be acceptable.
	bool pressed_mouse;
	int poll_time;
	
	Window window;
	Picture background;
	Pixmap bg_pixmap;
	int x, y;
	unsigned int width, height;
	//PosSize	rect;
	unsigned int distance;
	XRenderPictFormat *format;
	XTransform transform;
	
	XRenderColor normalTint, highlightTint;
	Pixmap normalPixmap, highlightPixmap;
	Picture normalPicture, highlightPicture;
	
	ClientWin *pressed, *focus;
	dlist *cod;
	struct _Tooltip *tooltip;
	
	KeyCode key_act, key_up, key_down, key_left, key_right,
		key_h, key_j, key_k, key_l,
		key_enter, key_space, key_q, key_escape,key_page_up,key_page_down;
	
#ifdef CFG_XINERAMA
	int xin_screens;
	XineramaScreenInfo *xin_info, *xin_active;
#endif /* CFG_XINERAMA */
};
#ifdef MW_POS_SIZE
#define mw_pos(mw) ((mw)->rect.pos)
#define mw_size(mw) ((mw)->rect.max)
#define mw_max(mw) (vec2i_add(mw_pos(mw),mw_size(mw)))
#else
#define mw_pos(mw) (vec2i_mk((mw)->x,(mw)->y))
#define mw_size(mw) (vec2i_mk((mw)->width,(mw)->height))
#endif
#define mw_min(mw) ((mw)->rect.pos)
#define mw_max(mw) (v2i_add((mw)->rect.pos,(mw)->rect.size))
#define mw_rect(mw) (rect2i_mk_at(mw_pos(mw),mw_size(mw)))
#define mw_width(mw) (mw_size(mw).x)
#define mw_height(mw) (mw_size(mw).y)

typedef struct _MainWin MainWin;

MainWin *mainwin_create(session_t *ps);
void mainwin_destroy(MainWin *);
void mainwin_map(MainWin *);
void mainwin_unmap(MainWin *);
int mainwin_handle(MainWin *, XEvent *);
void mainwin_update_background(MainWin *mw);
void mainwin_update(MainWin *mw);
void mainwin_transform(MainWin *mw, float f);
inline  Vec2i mainwin_pos(const MainWin* mw) 	{ vec2i_mk( mw->x,mw->y);}
inline  Vec2i mainwin_size(const MainWin* mw) 	{ vec2i_mk( mw->width,mw->height);}
inline  Rect2i mainwin_rect(const MainWin* mw) 	{ rect2i_mk_at( mainwin_pos(mw), mainwin_size(mw) );}

#endif /* SKIPPY_MAINWIN_H */
