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

void layout_run_scale_all(const MainWin *mw, dlist *windows, Vec2i total_size);

void 
layout_dump(const dlist* windows,Vec2i total_size) {
	const dlist* iter;
	for(iter = windows; iter; iter = iter->next){
		const ClientWin *cw = (const ClientWin*)iter->data;
		printf("mini pos:%d,%d size= %d,%d/ %d %d\n",cw->x,cw->y,cw->mini.width,cw->mini.height, total_size.x, total_size.y);
	}

}

void
windows_get_rect2i(Rect2i* ret,const dlist* windows) {
	const dlist* iter;
	*ret=rect2i_init();
	for (iter=windows; iter; iter=iter->next) {
		const ClientWin* cw=(const ClientWin*) iter->data;
		Rect2i cwr=cw_client_rect(cw);
		rect2i_include_rect2i(ret, &cwr);
	}
}



void
layout_run_desktop(MainWin *mw, dlist *windows, Vec2i* total_size)
{
	// dumb grid layout; TODO: Find aspect ratios, use to calculate w/hoptimal
	dlist* iter;
	int	num_win=dlist_len(windows);

	Rect2i rect_all;
	windows_get_rect2i(&rect_all ,windows);
	for (iter=windows; iter; iter=iter->next) {
		ClientWin* cw=(ClientWin*) iter->data;

		Vec2i ofs_pos = v2i_sub(sw_pos(&cw->client), rect_all.min);
		cw_set_tmp_pos(cw,ofs_pos);
	}
	Vec2i size_all=rect2i_size(&rect_all);

	*total_size=size_all;
	//layout_dump(windows,total_width,total_height);
	layout_run_scale_all(mw, windows, *total_size);


}

// Skippy Original layout - Slot-together into rows adaptively
//
void
layout_run_original(int separation, dlist *windows, Vec2i* total_size)
{
//	mw->distance=8;
	int sum_w = 0, max_row_w = 0;
	int row_y = 0, y = 0, x = 0, row_h = 0;
	Vec2i max_win_size=vec2i_mk(0,0);
	
	dlist *iter, *slots = 0, *slot_iter, *rows;
	
	rows = dlist_add(0, 0);
	
	windows = dlist_first(windows);
	*total_size=vec2i_mk(0,0);
	
	for(iter = windows; iter; iter = iter->next)
	{
		ClientWin *cw = (ClientWin *)iter->data;
		sum_w += cw_client_width(cw);
		max_win_size = v2i_max(max_win_size, cw_client_size(cw));
	}
	
	for(iter = windows; iter; iter = iter->next)
	{
		ClientWin *cw = (ClientWin*)iter->data;
		dlist *slot_iter = dlist_first(slots);
		for(; slot_iter; slot_iter = slot_iter->next)
		{
			dlist *slot = (dlist *)slot_iter->data;
			int slot_h = -separation;
			REDUCE(slot_h = slot_h + ((ClientWin*)iter->data)->client.height + separation, slot);
			if(slot_h + separation + cw->client.height < max_win_size.y)
			{
				slot_iter->data = dlist_add(slot, cw);
				break;
			}
		}
		if(! slot_iter)
			slots = dlist_add(slots, dlist_add(0, cw));
	}
	
	max_row_w = sqrt(sum_w * max_win_size.y);
	for(slot_iter = dlist_first(slots); slot_iter; slot_iter = slot_iter->next)
	{
		dlist *slot = (dlist *)slot_iter->data;
		int slot_w = 0;
		REDUCE(slot_w = MAX(slot_w, ((ClientWin*)iter->data)->client.width), slot);
		y = row_y;
		for(iter = dlist_first(slot); iter; iter = iter->next)
		{
			ClientWin *cw = (ClientWin *)iter->data;
			cw_set_tmp_xy(cw,x + (slot_w - cw->client.width) / 2,y);
			y += cw_client_height(cw) + separation;
			rows->data = dlist_add((dlist *)rows->data, cw);
		}
		row_h = MAX(row_h, y - row_y);
		total_size->y = MAX(total_size->y, y);
		x += slot_w + separation;
		total_size->x = MAX(total_size->x, x);
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
	
	//*total_width -= mw->distance;
	//*total_height -= mw->distance;
	*total_size=v2i_sub(*total_size, vec2i_mk(separation,separation));
	
	for(iter = dlist_first(rows); iter; iter = iter->next)
	{
		dlist *row = (dlist *)iter->data;
		int row_w = 0, xoff;
		REDUCE(row_w = MAX(row_w, ((ClientWin*)iter->data)->x + ((ClientWin*)iter->data)->client.width), row);
		xoff = (total_size->x - row_w) / 2;
		REDUCE(((ClientWin*)iter->data)->x += xoff, row);
		dlist_free(row);
	}

	//layout_dump(windows,total_width,total_height);

	//layout_run_scale_all(mw,windows,*total_size);
	
	dlist_free(rows);
}

// simple grid layout

#define DLIST_ITER_BEGIN(list, type, ptr) for (dlist* iter=list; iter;iter=iter->next){ type* ptr=(type*)(iter->data);
#define DLIST_ITER_END }

void layout_run_grid(int separation, Vec2i size, dlist* windows) {
	int num= dlist_len(windows);
	if (!num) return;
	int denom=1<<12;
	// get min,max aspect ratios. if the windows are all landscape or portrait it changes numx/numy
	float min_aspect=100000.0f,max_aspect=0.f;
	float sum_aspect=0.f;
	DLIST_ITER_BEGIN(windows, ClientWin, cw)
		float aspect=cw_client_aspect(cw);
		min_aspect=MIN(min_aspect,aspect); max_aspect=MAX(max_aspect,aspect);	
		sum_aspect+=aspect;
	DLIST_ITER_END	
	float win_aspect=(float)size.x/(float)size.y;
	float av_aspect = sum_aspect/(float) num;

//	int num_cols = MAX(1,sqrt(num *win_aspect/av_aspect  ));
//	int num_rows = (num+(num_cols-1))/num_cols;// todo: per row, for last one..
	printf("rowum=%.3f %.3f %.3f\n",(num*win_aspect)/av_aspect, av_aspect,win_aspect);
	int num_rows = MAX(1,sqrt((num*win_aspect)/ av_aspect  ));
	int num_cols = (num+(num_rows-1))/num_rows;// todo: per row, for last one..
	

// rows*cols = num
	printf("%.3f num=%d rows=%d cols=%d \n",av_aspect, num,num_rows,num_cols);
	
	// todo: placement for minimum movement.
	int	row=0,col=0;
	Vec2i pos=vec2i_mk(0,0);
	DLIST_ITER_BEGIN(windows, ClientWin, cw)
		// create grid cell to hold the window
//		Vec2i cell_min=vec2i_(size.x*denom/ num_cols, size.y*denom / num_rows);
		Rect2i cell=rect2i_mk(
			vec2i_mk((col*size.x)/num_cols,(row*size.y)/num_rows),
			vec2i_mk(((col+1)*size.x)/num_cols,((row+1)*size.y)/num_rows));

		Vec2i cell_size=//rect2i_size(&cell);
			v2i_sub(cell.max,cell.min);
		float client_aspect=cw_client_aspect(cw);
		float cell_aspect=cell_size.x/(float)cell_size.y;
		Vec2i mini_size;
		printf("%.3f\n",client_aspect);
		float f=client_aspect/cell_aspect;
		if (f>1.0) {
			
			mini_size.x=cell_size.x ;
			mini_size.y=(cell_size.y/f);
		} else {
			mini_size.x=(cell_size.x*f);
			mini_size.y=cell_size.y;
		}
		//mini_size=v2i_sub(cell_size,vec2i_mk(separation,separation));
		//mini_size=v2i_mul(mini_size,1,2);
		cw_set_mini_size(cw,mini_size);
//		cw->factor = 0.5f*mini_size.x/(float)cw_client_size(cw).x;
		
//		cw_set_tmp_pos(cw, v2i_sub(rect2i_centre(&cell), v2i_mul(mini_size,-1,2)));
		cw_set_tmp_pos(cw, v2i_mad(rect2i_centre(&cell),mini_size,-1,2));
		cw->mini.x=cw->x; cw->mini.y=cw->y;
		col++; 
		if (col>=num_cols) {
			col=0; row++; 
		}
		// todo: size per row.
	DLIST_ITER_END

	layout_dump(windows,size);

}

float layout_factor(const MainWin *mw,Vec2i size, unsigned int extra_border) {

	float factor = (float)(mw_width(mw) - extra_border) / size.x;
	if(factor * size.y > mw_height(mw) - extra_border)
		factor = (float)(mw_height(mw) - extra_border) / size.y;
	return factor;
}
void layout_run_scale_all(const MainWin *mw, dlist *windows, Vec2i total_size)
{
	dlist* iter;
	
	float factor=layout_factor(mw,total_size, mw->distance);
	int	 denom=1<<12; int ifactor=(int)(factor*(float)denom);


	Vec2i delta= v2i_mul(v2i_mad(mw_size(mw), total_size, -ifactor,denom),1,2);

	for(iter = windows; iter; iter = iter->next)
	{
		ClientWin *cw = (ClientWin*)iter->data;

//		cw->factor=factor;

		sw_set_pos_size(&cw->mini,
				v2i_mad(delta, cw_tmp_pos(cw), ifactor,denom),
				v2i_max(vec2i_mk(1,1),  v2i_mul(cw_client_size(cw),ifactor,denom)));

//		printf("%d %d %d %d\n",cw->mini.x,cw->mini.y, cw->mini.width,cw->mini.height);
	}
}


void
layout_run(MainWin *mw,LAYOUT_MODE mode, dlist *windows, Vec2i* total_size) 
{
	if (mode==LAYOUT_DESKTOP){ 
		layout_run_desktop(mw,windows,total_size);
		layout_run_scale_all(mw, windows, *total_size);
	} else if (mode==LAYOUT_GRID) {
		layout_run_grid(mw->distance, mw_size(mw),windows);
		*total_size=mw_size(mw);
	} else {
		layout_run_original(mw->distance,windows,total_size);
		layout_run_scale_all(mw, windows, *total_size);
	}

}

