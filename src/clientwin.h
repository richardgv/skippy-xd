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

#ifndef SKIPPY_CLIENT_H
#define SKIPPY_CLIENT_H

//#define SW_POS_SIZE
typedef struct {
	Window window;
#ifndef SW_POS_SIZE
	int x, y;
	unsigned int width, height;
#else
	PosSize	rect;
#endif
	XRenderPictFormat *format;
} SkippyWindow;

#define SKIPPYWINT_INIT { .window = None }

struct _MainWin;
typedef struct {
	struct _MainWin *mainwin;
	
	SkippyWindow client;
	bool redirected;
	Pixmap cpixmap;
	SkippyWindow mini;
	
	Pixmap pixmap;
	Picture origin, destination;
	Damage damage;
	
	int focused;
	
	bool damaged;
	/* XserverRegion repair; */
	
	/* These are virtual positions set by the layout routine */
	int x, y;
} ClientWin;

#define CLIENTWT_INIT { \
	.client = SKIPPYWINT_INIT, \
	.mini = SKIPPYWINT_INIT, \
	.mainwin = NULL \
}

int clientwin_validate_func(dlist *, void *);
int clientwin_sort_func(dlist *, dlist *, void *);
ClientWin *clientwin_create(struct _MainWin *, Window);
void clientwin_destroy(ClientWin *, bool destroyed);
void clientwin_move(ClientWin *, float, int, int);
void clientwin_create_scaled_image(ClientWin *cw);
void clientwin_move_and_create_scaled_image(ClientWin *cw, float f, int dx, int dy);

void clientwin_map(ClientWin *);
void clientwin_unmap(ClientWin *);
int clientwin_handle(ClientWin *, XEvent *);
int clientwin_cmp_func(dlist *, void*);
void clientwin_update(ClientWin *cw);
int clientwin_check_group_leader_func(dlist *l, void *data);
void clientwin_render(ClientWin *);
void clientwin_schedule_repair(ClientWin *cw, XRectangle *area);
void clientwin_repair(ClientWin *cw);

void 
clientwin_lerp_client_to_mini(ClientWin* cw,float t);

// accessors, less needed if code is refactored to use vec2i etc.
// bloats the headers whilst the change is in progress, can be search-replaced when done
#ifdef SW_POS_SIZE
inline Vec2i sw_pos(const SkippyWindow* w)	{ return w->rect.pos; }
inline Vec2i sw_size(const SkippyWindow* w)	{ return w->rect.size; }
inline void sw_set_pos(SkippyWindow* sw, const Vec2i p) { sw->rect.pos=p; }
inline void sw_set_size(SkippyWindow* sw, const Vec2i sz) { sw->rect.size=sz; }
inline void sw_set_rect(SkippyWindow* sw, const Rect2i* r) { sw_set_pos(sw,r->min); sw_set_size(sw, rect2i_size(r));}
inline int sw_width(const SkippyWindow* w) { return w->rect.size.x;} // => w->rect.size.x
inline int sw_height(const SkippyWindow* w) { return w->rect.size.y;} // => w->rect.size.y
inline int sw_x(const SkippyWindow* w) { return w->rect.pos.x;} // => w->rect.pos.x
inline int sw_y(const SkippyWindow* w) { return w->rect.pos.y;} // => w->rect.pos.y
#else
inline Vec2i sw_pos(const SkippyWindow* w)	{ return vec2i_mk(w->x,w->y); }
inline Vec2i sw_size(const SkippyWindow* w)	{ return vec2i_mk(w->width,w->height); }
inline void sw_set_pos(SkippyWindow* sw, const Vec2i p) { sw->x=p.x; sw->y=p.y; }
inline void sw_set_size(SkippyWindow* sw, const Vec2i sz) { sw->width=sz.x; sw->height=sz.y; }
inline void sw_set_rect(SkippyWindow* sw, const Rect2i* r) { sw_set_pos(sw,r->min); sw->width=r->max.x-r->min.x; sw->height=r->max.y-r->min.y;}
inline int sw_width(const SkippyWindow* w) { return w->width;} // => w->rect.size.x
inline int sw_height(const SkippyWindow* w) { return w->height;} // => w->rect.size.y
inline int sw_x(const SkippyWindow* w) { return w->x;} // => w->rect.pos.x
inline int sw_y(const SkippyWindow* w) { return w->y;} // => w->rect.pos.y
#endif
inline Vec2i sw_set_xy(SkippyWindow* sw,int x,int y)	{ sw_set_pos(sw,vec2i_mk(x,y)); }
inline Vec2i sw_set_width_height(SkippyWindow* sw,int w,int h)	{ sw_set_size(sw,vec2i_mk(w,h)); }
inline Rect2i sw_rect(const SkippyWindow* sw)	{ return rect2i_mk_at(sw_pos(sw),sw_size(sw));}
inline PosSize sw_pos_size(const SkippyWindow* sw)	{ return pos_size_mk(sw_pos(sw),sw_size(sw));}
inline void sw_set_pos_size(SkippyWindow* sw, const Vec2i p,const Vec2i sz) { sw_set_pos(sw,p); sw_set_size(sw,sz);}


inline Rect2i cw_client_rect(const ClientWin* w)	{ return rect2i_mk_at(sw_pos(&w->client),sw_size(&w->client));}
inline Rect2i cw_client_mini_rect(const ClientWin* w)	{ return rect2i_mk_at(sw_pos(&w->mini),sw_size(&w->mini));}
inline Vec2i cw_client_pos(const ClientWin* w)	{ return sw_pos(&w->client);}
inline Vec2i cw_client_size(const ClientWin* w)	{ return sw_size(&w->client);}
inline int cw_client_width(const ClientWin* w)	{ return sw_size(&w->client).x;}
inline int cw_client_height(const ClientWin* w)	{ return sw_size(&w->client).y;}
inline void cw_set_tmp_xy(ClientWin* w, int x, int y) { w->x = x; w->y=y;}
inline void cw_set_tmp_pos(ClientWin* w, Vec2i pos) { w->x = pos.x; w->y=pos.y;}
inline Vec2i cw_tmp_pos(ClientWin* w) { return vec2i_mk(w->x,w->y);}
inline float cw_client_aspect(const ClientWin* w) { Vec2i delta=sw_size(&w->client); return MAX(1,delta.x)/(float)MAX(1,delta.y);}
inline void cw_set_mini_size( ClientWin* w,Vec2i sz)	{  sw_set_size(&w->mini,sz);}
inline Vec2i cw_mini_centre(const ClientWin* cw) { return v2i_mad(sw_pos(&cw->mini), sw_size(&cw->mini), 1,2); }
inline Rect2i cw_mini_rect(const ClientWin* cw) { return sw_rect(&cw->mini); }
inline Vec2i cw_client_centre(const ClientWin* cw) { return v2i_mad(sw_pos(&cw->client), sw_size(&cw->client), 1,2); }
inline void cw_set_mini_rect(ClientWin* cw, const Rect2i* rc) { sw_set_rect(&cw->mini, rc);}
#endif /* SKIPPY_CLIENT_H */
