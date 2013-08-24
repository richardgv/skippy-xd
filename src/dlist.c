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
dlist_remove(dlist *l)
{
	dlist *n = 0;
	
	if(! l)
		return 0;
	
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
	l = dlist_first(l);
	
	while(l) {
		dlist *c = l;
		l = l->next;
		free(c);
	}
	
	return 0;
}

dlist *
dlist_dup(dlist *l)
{
	dlist *n = 0;
	l = dlist_first(l);
	
	while(l) {
		n = dlist_add(n, l->data);
		l = l->next;
	}
	
	return n;
}

dlist *
dlist_join(dlist *l, dlist *l2) {
	if (!l) return l2;
	if (!l2) return l;

	dlist *last = dlist_last(l);
	dlist *first2 = dlist_first(l2);
	last->next = first2;
	first2->prev = last;

	return l;
}

dlist *
dlist_find_all(dlist *l, dlist_match_func match, void *data)
{
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
	
	return 0;
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
	
	return 0;
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

