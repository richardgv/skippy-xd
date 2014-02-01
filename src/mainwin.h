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

struct _mainwin_t {
	session_t *ps;
	Visual *visual;
	Colormap colormap;
	int depth;
	dlist *clients;
	
	/// @brief Whether the KeyRelease events should already be acceptable.
	bool pressed_key;
	/// @brief Whether the ButtonRelease events should already be acceptable.
	bool pressed_mouse;
	int poll_time;
	
	Window window;
	Picture background;
	Pixmap bg_pixmap;
	int x, y;
	unsigned int width, height, distance;
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
		key_enter, key_space, key_q, key_escape;
	
#ifdef CFG_XINERAMA
	int xin_screens;
	XineramaScreenInfo *xin_info, *xin_active;
#endif /* CFG_XINERAMA */

	/// @brief Window ID to revert focus to when the main window is unmapped.
	Window revert_focus_win;
	/// @brief The client window to eventually focus.
	ClientWin *client_to_focus;
};

MainWin *mainwin_create(session_t *ps);
void mainwin_destroy(MainWin *);
void mainwin_map(MainWin *);
void mainwin_unmap(MainWin *);
int mainwin_handle(MainWin *, XEvent *);
void mainwin_update_background(MainWin *mw);
void mainwin_update(MainWin *mw);
void mainwin_transform(MainWin *mw, float f);

#endif /* SKIPPY_MAINWIN_H */
