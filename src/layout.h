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

#ifndef SKIPPY_LAYOUT_H
#define SKIPPY_LAYOUT_H

typedef enum LAYOUT_MODE {
	LAYOUT_ORIGINAL,
	LAYOUT_DESKTOP,
	LAYOUT_GRID
}
LAYOUT_MODE;

void layout_run(MainWin *, LAYOUT_MODE m, dlist *, unsigned int *, unsigned int *);
float layout_factor(const MainWin*,unsigned int width,unsigned int height, unsigned int extra_border);


#endif /* SKIPPY_LAYOUT_H */
