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

// TODO - layout which biases desktop sizes according to recent use.
// or grid mode including desktops placed as individual windows similar to gnomeshell
void layout_scale_all(const MainWin *mw, dlist *windows, Vec2i total_size);
void layout_original(int separation, dlist *windows, Vec2i* total_size);
void layout_desktop(MainWin *mw, dlist *windows, Vec2i* total_size);
void layout_grid(int separation, Vec2i size, dlist* windows);

void
layout_run(MainWin *mw,LAYOUT_MODE mode, dlist *windows, Vec2i* total_size) 
{
	Rect2i main_rect=rect2i_mk(vec2i_mk(0,0), mw_size(mw));
	if (mode==LAYOUT_DESKTOP){ 
		layout_desktop(mw,windows,total_size);
		layout_scale_all(mw, windows, *total_size);
		//fill_within_region(windows,  &main_rect,false);
	} else if (mode==LAYOUT_GRID) {
		layout_grid(mw->distance, mw_size(mw),windows);
		*total_size=mw_size(mw);
	} else {
		layout_original(mw->distance,windows,total_size);
		layout_scale_all(mw, windows, *total_size);
	}

}



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
layout_desktop(MainWin *mw, dlist *windows, Vec2i* total_size)
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
	layout_scale_all(mw, windows, *total_size);


}

// Skippy Original layout - Slot-together into rows adaptively
//
void
layout_original(int separation, dlist *windows, Vec2i* total_size)
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

void sort_objects_by_fn(void** pp,int count, int(*f_sort_val)(void*)) {
	// yikes, do we have ptr-to-member in C? would take templates to write safe/generic
	int	i,ii=count-1;
	for (ii=count-1; ii>0;ii--) {
		bool	ok=true;
		for (i=0; i<ii; i++) {
			void* p0=pp[i];
			void* p1=pp[i+1];
			int	v0=f_sort_val(p0);
			int	v1=f_sort_val(p1);
			if (v0<v1)
				continue;
			pp[i+1]=p0; pp[i]=p1;
			ok=false;
		}
		if (ok) 
			break;		
	}
}
int	cw_centre_x(void* p) {
	return cw_client_centre((ClientWin*)p).x;
}
int	cw_centre_y(void* p) {
	return cw_client_centre((ClientWin*)p).y;
}
int	cw_neg_centre_y(void* p) {
	return -cw_client_centre((ClientWin*)p).y;
}

void dump_row_col_xy(ClientWin** wins,int num,int num_cols,int num_rows) {
	//printf("col %d :%d %d sort..\n",j, col_i_start,col_i_end);
	for (int j=0; j<num_rows; j++) {
		for (int i=0; i<num_cols; i++) {
			int	ii=j*num_cols+i;
			if (ii>num) continue;
			printf("win(%d/%d)/(i,j=%d,%d)\tx=y=\t%d\t%d\n",ii,num,i,j, wins[ii]->client.x,wins[ii]->client.y);
		}
	}
}


void layout_grid(int separation, Vec2i size, dlist* windows) {
	int num= dlist_len(windows);
	if (!num) return;
	int denom=1<<12;
	// duplicate the pointer array-for sorting
	// sort by x TODO - pick major sort axis based on outer aspect ratio
	ClientWin** wins = (ClientWin*) malloc(sizeof(ClientWin)*num);
	int	i=0;
	DLIST_FOREACH(ClientWin, cw,/*IN*/ windows ) 
		wins[i++]=cw;
	DLIST_NEXT
//	sort_objects_by_fn((void**)wins,num,cw_centre_x);

	// get min,max aspect ratios. if the windows are all landscape or portrait it changes numx/numy
	float min_aspect=100000.0f,max_aspect=0.f;
	float sum_aspect=0.f;
	float total_area=0.f;
	DLIST_FOREACH(ClientWin, cw,/*IN*/ windows )
		float aspect=cw_client_aspect(cw);
		min_aspect=MIN(min_aspect,aspect); max_aspect=MAX(max_aspect,aspect);	
		Vec2i wsize=cw_client_size(cw);
		float area=wsize.x*wsize.y;
		total_area += area;
		sum_aspect+=aspect;//*area;
	DLIST_NEXT
	float win_aspect=(float)size.x/(float)size.y;
	float av_aspect = sum_aspect/(float)num;// total_area;

	printf("row0=%.3f row1=%.3f row2=%.3f avasp%.3f main_asp%.3f\n",
		sqrt((num*win_aspect)/av_aspect),
		sqrt((num*av_aspect)/win_aspect),
		sqrt(num)*(av_aspect/win_aspect),
		av_aspect,
		win_aspect);
//	int num_rows = MAX(1,sqrt((num/win_aspect)*	 av_aspect  ));
	//printf("aspect=");
//	av_aspect=1.0f;
	float d=sqrt(num*(av_aspect/win_aspect)) ;
	int num_rows = MAX(1,d+0.5f);
	int num_cols = (num+(num_rows-1))/num_rows;// todo: per row, for last one..
	

	printf("%.3f d=%.3f num=%d rows=%d cols=%d \n",av_aspect,d, num,num_rows,num_cols);

	
	printf("row0=%.3f row1=%.3f row2=%.3f avasp%.3f main_asp%.3f\n",
		sqrt((num*win_aspect)/av_aspect),
		sqrt((num*av_aspect)/win_aspect),
		sqrt(num)*(av_aspect/win_aspect),
		av_aspect,
		win_aspect);

	// TODO Sort horizontally. 	
	// todo: pick major axis. widescreen monitor, its usually horiz, but we might be placing in subwindows.

	sort_objects_by_fn((void**)wins,num,cw_centre_x);
	// within each column, sort by Y
	int	row=0,col=0;
	Vec2i pos=vec2i_mk(0,0);
	int	j=0;
	for (j=0; j<num_cols; j++) {
		int col_i_start=j*num_rows;
		int col_i_end=MIN(num,col_i_start+num_rows);
		sort_objects_by_fn((void**)(wins+col_i_start), col_i_end-col_i_start, cw_centre_y);
		int	ii;		
	}
	
//	DLIST_FOREACH(ClientWin, cw, windows )
	for (i=0; i<num; i++) {
		ClientWin* cw=wins[i];
		// create grid cell to hold the window
		Rect2i cell=rect2i_mk(
			vec2i_mk((col*size.x)/num_cols,(row*size.y)/num_rows),
			vec2i_mk(((col+1)*size.x)/num_cols,((row+1)*size.y)/num_rows));

		Vec2i cell_size=//rect2i_size(&cell);
			v2i_sub(cell.max,cell.min);
		float client_aspect=cw_client_aspect(cw);
		float cell_aspect=cell_size.x/(float)cell_size.y;
		Vec2i mini_size=v2i_sub(cell_size,vec2i_mk(separation,separation));
		//printf("%.3f\n",client_aspect);
		float f=client_aspect/cell_aspect;
		if (f>1.0) {
			mini_size.y*=(1.0/f);
		} else {
			mini_size.x*=f;
		}
		cw_set_mini_size(cw,mini_size);

		cw_set_tmp_pos(cw, v2i_sub(rect2i_centre(&cell),v2i_half(mini_size)));
		cw->mini.x=cw->x; cw->mini.y=cw->y;
		row++; 
		if (row>=num_rows) {
			row=0; col++; 
		}
		// todo: size per row.
	}

	
	//layout_dump(windows,size);
	free(wins);
}


void fill_within_region(dlist* windows, const Rect2i* region,bool preserve_win_aspect) {
	int denom=1<<12;
	if (!rect2i_area(region,denom)) {
		printf("divide by zero,can't do this");
		return;
	}
	// find all the windows in a region (eg desktop?), expand their mini's to fill it
	// [.1] Find the extents of everything within this rect
	rect2i_print(region);
	Rect2i	region_contents_rc=rect2i_init();
	DLIST_FOREACH(ClientWin, cw, windows) 
		if (!rect2i_contains(region, cw_mini_centre(cw)))
			continue;
		Rect2i mini_rect = cw_mini_rect(cw);
		rect2i_include_rect2i(&region_contents_rc, &mini_rect);
	DLIST_NEXT
	printf("region contents rect=");
	rect2i_print(&region_contents_rc);
	// [.2] rescale those to fill the region, preserving individual aspect
	// this will give a slight spread in whatever axis has the most unused space
	// todo: calculate a transformation 
	printf("lerpi test %d %d %d \n", 
		lerpi(100, 200, 1024,denom),
		invlerpi(100,200, 125,denom), 
		lerpi(100,200,invlerpi(100,200,125,denom),denom));

	if (!rect2i_area(&region_contents_rc,denom)) {
		printf("divide by zero,can't do this");
		return;
	}

	DLIST_FOREACH(ClientWin, cw, windows)
		Rect2i mini_rect = cw_mini_rect(cw);
		int aspect=rect2i_aspect(&mini_rect,denom);
		//rect2i_print(&mini_rect);
		Rect2i new_rect;
		vec2i_print(rect2i_invlerp(&region_contents_rc, mini_rect.min,denom));
		Rect2i rect_frac;
		rect_frac.min=rect2i_invlerp(&region_contents_rc, mini_rect.min,denom);
		rect_frac.max=rect2i_invlerp(&region_contents_rc, mini_rect.max,denom);
		//printf("fractional subrect=");		rect2i_print(&rect_frac);
		new_rect.min = rect2i_lerp(region, rect_frac.min,denom);
		new_rect.max = rect2i_lerp(region, rect_frac.max,denom);
		//rect2i_print(&new_rect);printf("\n");

		if (preserve_win_aspect){
			Vec2i new_centre=rect2i_centre(&new_rect);
			Vec2i half_size=v2i_sub(new_rect.max,new_centre);
			// correct the aspect ratio..
			if (aspect>denom) {
				half_size.y=(half_size.x*denom)/aspect;
			} else {
				half_size.x=(half_size.y*aspect)/denom;
			}
			new_rect=rect2i_mk_at_centre(new_centre,half_size);
		}
		cw_set_mini_rect(cw, &new_rect);
	DLIST_NEXT
}

float layout_factor(const MainWin *mw,Vec2i size, unsigned int extra_border) {

	float factor = (float)(mw_width(mw) - extra_border) / size.x;
	if(factor * size.y > mw_height(mw) - extra_border)
		factor = (float)(mw_height(mw) - extra_border) / size.y;
	return factor;
}
void layout_scale_all(const MainWin *mw, dlist *windows, Vec2i total_size)
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


