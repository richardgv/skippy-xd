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

#include "anime.h"
#include "skippy.h"

void anime(
	MainWin *mw,
	dlist *clients,
	float timeslice
) {
	clients = dlist_first(clients);
	mw->clients = animate_layout(mw, mw->clients, mw->revert_focus_win, None, timeslice);
	if (!mw->cod) {
		printfef("(): Failed to build layout.");
		return;
	}
}

dlist *
animate_layout(MainWin *mw, dlist *clients, Window focus, Window leader, float timeslice) {
	wm_get_current_desktop(mw->ps);
	
	unsigned int width = 0, height = 0;
	layout_run(mw, mw->cod, &width, &height);
	float factor = (float) (mw->width - (2 * mw->distance)) / width;
	if (factor * height > mw->height - (2 * mw->distance))
		factor = (float) (mw->height - (2 * mw->distance)) / height;
	if (!mw->ps->o.allowUpscale)
		factor = MIN(factor, 1.0f);

	int xoff = (mw->width - (float) width * factor) / 2;
	int yoff = (mw->height - (float) height * factor) / 2;
	float modified_factor = 1.0 + timeslice * (factor - 1.0);
	mainwin_transform(mw, modified_factor);
	foreach_dlist (mw->cod) {
		clientwin_move((ClientWin *) iter->data, factor, xoff, yoff, timeslice);
		clientwin_map((ClientWin*)iter->data);
	}

	return clients;
}
