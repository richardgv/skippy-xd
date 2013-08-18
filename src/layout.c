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

void layout_run_scale_all(MainWin *mw, dlist *windows, unsigned int total_width, unsigned int total_height);

void 
layout_dump(const dlist* windows,unsigned int *total_width,unsigned int* total_height) {
	const dlist* iter;
	for(iter = windows; iter; iter = iter->next){
		const ClientWin *cw = (const ClientWin*)iter->data;
		printf("mini pos:%d %d / %d %d\n",cw->x,cw->y,*total_width,*total_height);
	}

}

void
windows_get_rect2i(Rect2i* ret,const dlist* windows) {
	const dlist* iter;
	*ret=rect2i_init();
	for (iter=windows; iter; iter=iter->next) {
		const ClientWin* cw=(const ClientWin*) iter->data;
		Rect2i cwr=clientwin_rect(cw);
		rect2i_include_rect2i(ret, &cwr);
	}
}


void
layout_run_desktop(MainWin *mw, dlist *windows, unsigned int *total_width, unsigned int *total_height)
{
	// dumb grid layout; TODO: Find aspect ratios, use to calculate w/hoptimal
	dlist* iter;
	int	num_win=dlist_len(windows);

	Rect2i rect_all;
	windows_get_rect2i(&rect_all ,windows);
	for (iter=windows; iter; iter=iter->next) {
		ClientWin* cw=(ClientWin*) iter->data;
		//printf("%d %d %d %d\n",cw->client.x,cw->client.y,cw->client.width,cw->client.height);
		//Vec2i vp=clientwin_pos(&cw->client);
		//vec2i_print(vp);
		//rect2i_print(&cwr);
//		cw->x = cw->client.x-rect_all.min.x; cw->y=cw->client.y-rect_all.min.y;

		cw->x = cw->client.x-rect_all.min.x;
		cw->y = cw->client.y- rect_all.min.y;
//		cw->x=rect_all.min.x; cw->y=0;
	}
	Vec2i size_all=rect2i_size(&rect_all);
	printf("total bounds:-");
	rect2i_print(&rect_all);
	printf("\n");
	vec2i_print(size_all);

	*total_width=size_all.x; *total_height=size_all.y;
	layout_dump(windows,total_width,total_height);


}


void
layout_run_original(MainWin *mw, dlist *windows, unsigned int *total_width, unsigned int *total_height)
{
	mw->distance=8;
	int sum_w = 0, max_h = 0, max_w = 0, max_row_w = 0;
	int row_y = 0, y = 0, x = 0, row_h = 0;
	
	dlist *iter, *slots = 0, *slot_iter, *rows;
	
	rows = dlist_add(0, 0);
	
	windows = dlist_first(windows);
	*total_width = *total_height = 0;
	
	for(iter = windows; iter; iter = iter->next)
	{
		ClientWin *cw = (ClientWin *)iter->data;
		sum_w += cw->client.width;
		max_w = MAX(max_w, cw->client.width);
		max_h = MAX(max_h, cw->client.height);
		logd("win %d,%d %dx%d\n ",  cw->client.x,cw->client.y, cw->client.width, cw->client.height);
	}
	
	for(iter = windows; iter; iter = iter->next)
	{
		ClientWin *cw = (ClientWin*)iter->data;
		dlist *slot_iter = dlist_first(slots);
		for(; slot_iter; slot_iter = slot_iter->next)
		{
			dlist *slot = (dlist *)slot_iter->data;
			int slot_h = -mw->distance;
			REDUCE(slot_h = slot_h + ((ClientWin*)iter->data)->client.height + mw->distance, slot);
			if(slot_h + mw->distance + cw->client.height < max_h)
			{
				slot_iter->data = dlist_add(slot, cw);
				break;
			}
		}
		if(! slot_iter)
			slots = dlist_add(slots, dlist_add(0, cw));
	}
	
	max_row_w = sqrt(sum_w * max_h);
	for(slot_iter = dlist_first(slots); slot_iter; slot_iter = slot_iter->next)
	{
		dlist *slot = (dlist *)slot_iter->data;
		int slot_w = 0;
		REDUCE(slot_w = MAX(slot_w, ((ClientWin*)iter->data)->client.width), slot);
		y = row_y;
		for(iter = dlist_first(slot); iter; iter = iter->next)
		{
			ClientWin *cw = (ClientWin *)iter->data;
			cw->x = x + (slot_w - cw->client.width) / 2;
			cw->y = y;
			y += cw->client.height + mw->distance;
			rows->data = dlist_add((dlist *)rows->data, cw);
		}
		row_h = MAX(row_h, y - row_y);
		*total_height = MAX(*total_height, y);
		x += slot_w + mw->distance;
		*total_width = MAX(*total_width, x);
		if(x > max_row_w)
		{
			x = 0;
			row_y += row_h;
			row_h = 0;
			rows = dlist_add(rows, 0);
		}
		dlist_free(slot);
	}
	dlist_free(slots);
	
	*total_width -= mw->distance;
	*total_height -= mw->distance;
	
	for(iter = dlist_first(rows); iter; iter = iter->next)
	{
		dlist *row = (dlist *)iter->data;
		int row_w = 0, xoff;
		REDUCE(row_w = MAX(row_w, ((ClientWin*)iter->data)->x + ((ClientWin*)iter->data)->client.width), row);
		xoff = (*total_width - row_w) / 2;
		REDUCE(((ClientWin*)iter->data)->x += xoff, row);
		dlist_free(row);
	}

	layout_dump(windows,total_width,total_height);

	layout_run_scale_all(mw,windows,*total_width,*total_height);
	
	dlist_free(rows);
}

float layout_factor(const MainWin *mw,unsigned int width,unsigned int height, unsigned int extra_border) {
	float factor = (float)(mw->width - extra_border) / width;
	if(factor * height > mw->height - extra_border)
		factor = (float)(mw->height - extra_border) / height;
	return factor;
}
void layout_run_scale_all(MainWin *mw, dlist *windows, unsigned int total_width, unsigned int total_height)
{
	dlist* iter;
	
	float factor=layout_factor(mw,total_width,total_height, mw->distance);

	int xoff = (mw->width - (float)total_width * factor) / 2;
	int yoff = (mw->height - (float)total_height * factor) / 2;

	int dx = (mw->width - (float)total_width * factor) / 2;
	int dy = (mw->height - (float)total_height * factor) / 2;

	for(iter = windows; iter; iter = iter->next)
	{
		ClientWin *cw = (ClientWin*)iter->data;

		cw->factor=factor;

		cw->mini.x = dx + (int)cw->x * factor;
		cw->mini.y = dy + (int)cw->y * factor;

		cw->mini.width = MAX(1, (int)cw->client.width * factor);
		cw->mini.height = MAX(1, (int)cw->client.height * factor);
		printf("%d %d %d %d\n",cw->mini.x,cw->mini.y, cw->mini.width,cw->mini.height);
	}
}


void
layout_run(MainWin *mw,LAYOUT_MODE mode, dlist *windows, unsigned int *total_width, unsigned int *total_height) 
{
	if (mode==LAYOUT_DESKTOP){ 
		layout_run_desktop(mw,windows,total_width,total_height);
	} else
		layout_run_original(mw,windows,total_width,total_height);

	layout_run_scale_all(mw, windows, *total_width, *total_height);
}

