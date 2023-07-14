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

// calculate and populate windows destination positions
// switches to different layout algorithms based on user/default config
void layout_run(MainWin *, dlist *, unsigned int *, unsigned int *, enum layoutmode);
void layout_xd(MainWin *, dlist *, unsigned int *, unsigned int *);
void layout_boxy(MainWin *, dlist *, unsigned int *, unsigned int *);
int boxy_affinity(ClientWin *, int, int, int, int, int, int);

int middleOfThree(int a, int b, int c);

#endif /* SKIPPY_LAYOUT_H */
