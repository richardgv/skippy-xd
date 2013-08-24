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

#ifndef SKIPPY_DLIST_H
#define SKIPPY_DLIST_H

struct dlist_element {
	void *data;
	struct dlist_element *next;
	struct dlist_element *prev;
};

typedef struct dlist_element dlist;

/* first element in list */
dlist *dlist_first(dlist *);

/* last element in list */
dlist *dlist_last(dlist *);

/* add element to the end of the list, returns new element */
dlist *dlist_add(dlist *, void *);

/* add element to the start of the list, returns new element (and thus, start of list) */
dlist *dlist_prepend(dlist *, void *);

/* remove an element from the list, returns another element in the list or 0 */
dlist *dlist_remove(dlist *);
dlist *dlist_remove_free_data(dlist *);
dlist *dlist_remove_nth(dlist *, unsigned int n);
dlist *dlist_remove_nth_free_data(dlist *, unsigned int n);

/* free the list (not the data), returns 0 */
dlist *dlist_free(dlist *);

/* delete a list calling func on each data item */
typedef void (*dlist_free_func)(void *);
dlist *dlist_free_with_func(dlist *, dlist_free_func);

/* free the data (not the list) */
void dlist_free_data(dlist *);

/* free both list and data, returns 0 */
dlist *dlist_free_with_data(dlist *);

/* return the length of the list */
unsigned int dlist_len(dlist *);

/* check if l1 and l2 are elements of the same list */
int dlist_same(dlist *, dlist *);

/* reverse a list (swaps data) */
void dlist_reverse(dlist *);

/* duplicate the list (not the data), returns new end */
dlist *dlist_dup(dlist *);

dlist * dlist_join(dlist *l, dlist *l2);

/* find all matching elements (returns new list or 0) */
typedef int (*dlist_match_func)(dlist*, void *);
dlist *dlist_find_all(dlist *, dlist_match_func, void *);

/* find an element (returns element or 0) */
dlist *dlist_find(dlist *, dlist_match_func, void *);

/* find an element whose data pointer matches */
dlist *dlist_find_data(dlist *, void *);

/* return nth element or 0 */
dlist *dlist_nth(dlist *l, unsigned int n);

/* swap the data fields of 2 elements */
void dlist_swap(dlist *, dlist *);

/* sort a list (not very efficient, uses bubble-sort) */
typedef int (*dlist_cmp_func)(dlist *, dlist *, void *);
void dlist_sort(dlist *, dlist_cmp_func, void *);

#endif /* SKIPPY_DLIST_H */
