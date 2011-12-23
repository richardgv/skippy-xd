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

#ifndef SKIPPY_H
#define SKIPPY_H

#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xatom.h>
#include <X11/Xmu/WinUtil.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>

#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>

#ifdef XINERAMA
# include <X11/extensions/Xinerama.h>
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <regex.h>
#include <string.h>


#define MAX(a,b) ((a > b) ? a : b)
#define MIN(a,b) ((a > b) ? b : a)
#define REDUCE(statement, l) \
{ \
	dlist *iter = dlist_first(l); \
	for(; iter; iter = iter->next) \
		statement; \
}


#include "dlist.h"
#include "wm.h"
#include "clientwin.h"
#include "mainwin.h"
#include "layout.h"
#include "focus.h"
#include "config.h"
#include "tooltip.h"

#endif /* SKIPPY_H */
