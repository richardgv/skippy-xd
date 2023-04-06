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

dlist *
dlist_last(dlist *l)
{
	if (! l)
		return NULL;

	while(l && l->next)
		l = l->next;
	return l;
}

dlist *
dlist_first(dlist *l)
{
	while(l && l->prev)
		l = l->prev;
	return l;
}

dlist *
dlist_add(dlist *l, void *d)
{
	dlist *new_elem;
	
	l = dlist_last(l);
	new_elem = malloc(sizeof(dlist));
	new_elem->prev = l;
	new_elem->next = 0;
	new_elem->data = d;
	if(l)
		l->next = new_elem;
	return new_elem;
}

dlist *
dlist_prepend(dlist *l, void *d)
{
	dlist *new_elem;
	l = dlist_first(l);
	new_elem = malloc(sizeof(dlist));
	new_elem->prev = 0;
	new_elem->next = l;
	new_elem->data = d;
	if(l)
		l->prev = new_elem;
	return new_elem;
}

dlist *
dlist_insert_before(dlist *elem, dlist *new_elem)
{
	if (!elem)
		return NULL;

	dlist *first_elem = dlist_first(elem);

	if (!new_elem)
		return first_elem;

	dlist *prev = elem->prev;
	if (prev)
	{
		prev->next = new_elem;
		new_elem->prev = prev;
	}
	else
	{
		new_elem->prev = 0;
		first_elem = new_elem;
	}
	new_elem->next = elem;
	elem->prev = new_elem;

	return first_elem;
}

dlist *
dlist_insert_after(dlist *elem, dlist *new_elem)
{
	if (!elem)
		return NULL;

	dlist *first_elem = dlist_first(elem);

	if (!new_elem)
		return first_elem;

	dlist *next = elem->next;
	if (next)
	{
		next->prev = new_elem;
		new_elem->next = next;
	}
	else
	{
		new_elem->next = 0;
	}
	new_elem->prev = elem;
	elem->next = new_elem;

	return first_elem;
}

dlist *
dlist_insert_nth(dlist *l, dlist *new_elem, unsigned int n)
{
	if (!l)
		return NULL;

	if (!new_elem)
		return l;

	dlist *first_elem = dlist_first(l);
	dlist *nth = dlist_nth(l, n);

	if (nth)
	{
		first_elem = dlist_insert_before(nth, new_elem);
	}
	else
	{
		// there is no nth element
		printfef(false, "(%p,%p,%i): list has fewer than %i elements. appending as the last element.", l, new_elem, n, n);
		first_elem = dlist_insert_after( dlist_last(l), new_elem);
	}

	return first_elem;
}

dlist *
dlist_cycle_prev(dlist *l)
{
	if (! l)
		return NULL;

	dlist *first_elem = dlist_first(l);
	dlist *last_elem = dlist_last(l);
	dlist *prev = last_elem->prev;

	if (dlist_len(first_elem) == 1)
		return first_elem;

	if (prev)
		prev->next = NULL;

	first_elem->prev = last_elem;
	last_elem->prev = NULL;
	last_elem->next = first_elem;

	first_elem = last_elem;
	return first_elem;
}

dlist *
dlist_cycle_next(dlist *l)
{
	if (! l)
		return NULL;

	dlist *first_elem = dlist_first(l);
	dlist *last_elem = dlist_last(l);
	dlist *next = first_elem->next;

	if (dlist_len(first_elem) == 1)
		return first_elem;

	if (next)
		next->prev = NULL;

	last_elem->next = first_elem;
	first_elem->prev = last_elem;
	first_elem->next = NULL;

	first_elem = next;
	return first_elem;
}

dlist *
dlist_cycle(dlist *l, int n)
{
	if (! l)
		return NULL;

	dlist *first_elem = dlist_first(l);

	if (n < 0)
	{
		while (n != 0)
		{
			first_elem = dlist_cycle_prev(first_elem);
			n++;
		}
	}

	if (n > 0)
	{
		while (n != 0)
		{
			first_elem = dlist_cycle_next(first_elem);
			n--;
		}
	}
	return first_elem;
}

dlist *
dlist_extract(dlist *elem)
{
	if (! elem)
		return NULL;

	dlist *first_elem = dlist_first(elem);

	dlist *prev = elem->prev;
	dlist *next = elem->next;

	if (prev)
	{
		if (next)
		{
			prev->next = next;
			next->prev = prev;
		}
		else
		{
			prev->next = NULL;
		}
	}
	else if (next)
	{
		next->prev = NULL;
		first_elem = next;
	}

	elem->prev = NULL;
	elem->next = NULL;

	return first_elem;
}

dlist *
dlist_remove(dlist *l)
{
	dlist *n = 0;
	
	if (! l)
		return NULL;
	
	if(l->prev) {
		n = l->prev;
		l->prev->next = l->next;
	}
	if(l->next) {
		n = l->next;
		l->next->prev = l->prev;
	}
	free(l);
	return n;
}

dlist *
dlist_remove_free_data(dlist *l)
{
	if(l && l->data)
		free(l->data);
	return dlist_remove(l);
}

dlist *
dlist_remove_nth(dlist *l, unsigned int n)
{
	return dlist_remove(dlist_nth(l, n));
}

dlist *
dlist_remove_nth_free_data(dlist *l, unsigned int n)
{
	return dlist_remove_free_data(dlist_nth(l, n));
}

int
dlist_same(dlist *l1, dlist *l2)
{
	if (! l2)
		return 0;

	l1 = dlist_first(l1);
	while(l1) {
		if(l1 == l2)
			return 1;
		l1 = l1->next;
	}
	return 0;
}

void
dlist_reverse(dlist *l)
{
	if (! l)
		return;

	dlist *iter1 = dlist_first(l),
		*iter2 = dlist_last(l);
	
	while(iter1 != iter2) {
		dlist_swap(iter1, iter2);
		if(iter1->next == iter2)
			break;
		iter1 = iter1->next;
		iter2 = iter2->prev;
	}
}

dlist *
dlist_free(dlist *l)
{
	if (! l)
		return NULL;

	l = dlist_first(l);
	
	while(l) {
		dlist *c = l;
		l = l->next;
		free(c);
	}
	
	return NULL;
}

dlist *
dlist_dup(dlist *l)
{
	if (! l)
		return NULL;

	dlist *n = NULL;
	l = dlist_first(l);
	
	while(l) {
		n = dlist_add(n, l->data);
		l = l->next;
	}
	
	return n;
}

dlist *
dlist_split_nth(dlist *l, unsigned int n)
{
	dlist *first_elem = dlist_first(l);
	dlist *dlist_n = dlist_nth(first_elem, n);

	if (! dlist_n)
		return NULL;

	dlist *prev = dlist_n->prev;
	if (prev)
	{
		prev->next = NULL;
		dlist_n->prev = NULL;
	}

	return dlist_n;
}

dlist *
dlist_join(dlist *l, dlist *l2) {
	if (!l) return l2;
	if (!l2) return l;
	if (l == l2) return NULL;

	dlist *first = dlist_first(l);
	dlist *last = dlist_last(l);
	dlist *first2 = dlist_first(l2);


	// check that no elements are the same
	l = first; l2 = first2;
	while (l)
	{
		while (l2)
		{
			if (l == l2)
				return NULL;

			l2 = l2->next;
		}
		l = l->next;
	}

	last->next = first2;
	first2->prev = last;
	return first;
}

int
dlist_index_of(dlist *l, dlist *elem)
{
	if (! l)
		return -1;

	if (! elem)
		return -1;

	int n = 0;
	dlist *nth_elem = dlist_first(l);

	while ((nth_elem) && (nth_elem != elem))
	{
		nth_elem = nth_elem->next;
		n++;
	}

	if (!nth_elem)
		return -1;

	return n;
}

dlist *
dlist_find_all(dlist *l, dlist_match_func match, void *data)
{
	if (! l)
		return NULL;

	if (! data)
		return NULL;

	dlist *n = 0;
	l = dlist_first(l);
	
	while(l) {
		if(match(l, data))
			n = dlist_add(n, l->data);
		l = l->next;
	}
	
	return n;
}

dlist *
dlist_find(dlist *l, dlist_match_func func, void *data)
{
	if (! l)
		return NULL;

	if (! data)
		return NULL;

	for(l = dlist_first(l); l; l = l->next)
		if(func(l, data))
			break;
	return l;
}

dlist *
dlist_find_data(dlist *l, void *data)
{
	for(l = dlist_first(l); l; l = l->next)
		if(l->data == data)
			break;
	return l;
}

void
dlist_free_data(dlist *l)
{
	l = dlist_first(l);
	
	while(l) {
		if(l->data)
			free(l->data);
		l->data = 0;
		l = l->next;
	}
}

dlist *
dlist_free_with_data(dlist *l)
{
	l = dlist_first(l);
	
	while(l) {
		dlist *c = l;
		if(l->data)
			free(l->data);
		l = l->next;
		free(c);
	}
	
	return NULL;
}

dlist *
dlist_free_with_func(dlist *l, dlist_free_func func)
{
	l = dlist_first(l);
	
	while(l) {
		dlist *c = l;
		if(l->data)
			func(l->data);
		l = l->next;
		free(c);
	}
	
	return NULL;
}

unsigned int
dlist_len(dlist *l)
{
	unsigned int n = 0;
	
	l = dlist_first(l);
	while(l) {
		n++;
		l = l->next;
	}
	
	return n;
}

dlist *
dlist_nth(dlist *l, unsigned int n)
{
	unsigned int i = 0;
	l = dlist_first(l);
	while(l && i != n) {
		i++;
		l = l->next;
	}
	
	return l;
}

void
dlist_swap(dlist *l1, dlist *l2)
{
	if (! l1)
		return;

	if (! l2)
		return;

	void *tmp = l1->data;
	l1->data = l2->data;
	l2->data = tmp;
}

void
dlist_sort(dlist *l, dlist_cmp_func cmp, void *data)
{
	dlist *start = dlist_first(l);
	while(start) {
		dlist *iter = start;
		start = 0;
		while(iter) {
			if(iter->next && cmp(iter, iter->next, data) == 1) {
				dlist_swap(iter, iter->next);
				if(! start)
					start = iter->prev;
			}
			iter = iter->next;
		}
	}
}

