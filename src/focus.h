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

#ifndef SKIPPY_FOCUS_H
#define SKIPPY_FOCUS_H

static inline void
printfefXFocusChangeEvent(session_t *ps, XFocusChangeEvent *evf)
{
	printfefWindowName(ps, "(): event window = ", evf->window);
	printfef("(): event window id = %#010lx", evf->window);

	if(evf->mode == NotifyNormal)
		printfef("(): evf->mode = NotifyNormal");
	else if(evf->mode == NotifyGrab)
		printfef("(): evf->mode = NotifyGrab");
	else if(evf->mode == NotifyUngrab)
		printfef("(): evf->mode = NotifyUngrab");
	else if(evf->mode == NotifyWhileGrabbed)
		printfef("(): evf->mode = NotifyWhileGrabbed");
	else
		printfef("(): evf->mode = %i (not recognized)", evf->mode);

	if(evf->detail == NotifyAncestor)
		printfef("(): evf->detail = NotifyAncestor");
	else if(evf->detail == NotifyVirtual)
		printfef("(): evf->detail = NotifyVirtual");
	else if(evf->detail == NotifyInferior)
		printfef("(): evf->detail = NotifyInferior");
	else if(evf->detail == NotifyNonlinear)
		printfef("(): evf->detail = NotifyNonlinear");
	else if(evf->detail == NotifyNonlinearVirtual)
		printfef("(): evf->detail = NotifyNonlinearVirtual");
	else if(evf->detail == NotifyPointer)
		printfef("(): evf->detail = NotifyPointer");
	else if(evf->detail == NotifyPointerRoot)
		printfef("(): evf->detail = NotifyPointerRoot");
	else if(evf->detail == NotifyDetailNone)
		printfef("(): evf->detail = NotifyDetailNone");
	else
		printfef("(): evf->detail = %i (not recognized)", evf->detail);
}

static inline void
clear_focus_all(dlist *cod)
{
	foreach_dlist (cod)
	{
		ClientWin *cw = (ClientWin *)iter->data;
		cw->focused = 0;
	}

}

/**
 * @brief Focus the mini window of a client window.
 */
static inline void
focus_miniw_adv(session_t *ps, ClientWin *cw, bool move_ptr) {
	// printfef("(): ");
	clear_focus_all(cw->mainwin->cod);

	printfefWindowName(ps, "(): window = ", cw->wid_client);

	if (unlikely(!cw))
	{
		// printfef("(): if (unlikely(!cw))");
		return;
	}
	assert(cw->mini.window);
	if (move_ptr)
	{
		// printfef("(): if (move_ptr)");
		XWarpPointer(ps->dpy, None, cw->mini.window, 0, 0, 0, 0, cw->mini.width / 2, cw->mini.height / 2);
	}
	XSetInputFocus(ps->dpy, cw->mini.window, RevertToParent, CurrentTime);
	XFlush(ps->dpy);

	ps->mainwin->client_to_focus = cw;
	ps->mainwin->client_to_focus->focused = 1;

}

static inline void
focus_miniw(session_t *ps, ClientWin *cw) {
	focus_miniw_adv(ps, cw, ps->o.movePointerOnSelect);
}

/**
 * @brief Focus the mini window of next client window in list.
 */
static inline void
focus_miniw_next(session_t *ps, ClientWin *cw) {
	dlist *e = dlist_find_data(cw->mainwin->cod, cw);
	if (!e) {
		printfef("(%#010lx): Client window not found in list.", cw->src.window);
		return;
	}
	if (e->next)
		focus_miniw(ps, e->next->data);
	else
		focus_miniw(ps, dlist_first(e)->data);
}

/**
 * @brief Focus the mini window of previous client window in list.
 */
static inline void
focus_miniw_prev(session_t *ps, ClientWin *cw) {
	dlist *cwlist = dlist_first(cw->mainwin->cod);
	dlist *tgt = NULL;

	if (cw == cwlist->data)
		tgt = dlist_last(cwlist);
	else
		foreach_dlist (cwlist) {
			if (iter->next && cw == iter->next->data) {
				tgt = iter;
				break;
			}
		}

	if (!tgt) {
		printfef("(%#010lx): Client window not found in list.", cw->src.window);
		return;
	}

	focus_miniw(ps, (ClientWin *) tgt->data);
}

void focus_up(ClientWin *cw);
void focus_down(ClientWin *cw);
void focus_left(ClientWin *cw);
void focus_right(ClientWin *cw);

#endif /* SKIPPY_FOCUS_H */
