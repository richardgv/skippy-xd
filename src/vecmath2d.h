#ifndef vecmath2d_h
#define vecmath2d_h

// 2d int maths helpers
typedef struct Vec2i {	// todo: refactor to use this
	int x,y;
} Vec2i;

// rectangular region
typedef struct Rect2i {
	Vec2i min,max;
} Rect2i;

// alternative way of specifying rectangle
typedef struct PosSize {
	Vec2i pos,size;
} PosSize;


inline int invlerpi(int lo,int hi,int v,int one) {
	return ((v-lo)*one)/(hi-lo);
}
inline int lerpi(int lo,int hi,int f,int one) {
	return lo+(((hi-lo)*f)/one);
}
inline void v2i_set(Vec2i* dst, int x, int y) {
	dst->x=x;dst->y=y;
}
inline Vec2i vec2i_mk(int x, int y) {
	Vec2i v; v.x=x; v.y=y; return v;
}
inline Vec2i v2i_sub(Vec2i a,Vec2i b) {
	Vec2i ret= {a.x-b.x,a.y-b.y};return ret;
}
inline Vec2i v2i_add(Vec2i a,Vec2i b) {
	Vec2i ret= {a.x+b.x,a.y+b.y};return ret; 
}
inline Vec2i v2i_mul(Vec2i a,int f,int prec) {
	Vec2i ret= {(a.x*f)/prec,(a.y*f)/prec};return ret; 
}
inline Vec2i v2i_avr(Vec2i a,Vec2i b) {
	Vec2i ret=v2i_mul(v2i_add(a,b),1,2); return ret;
}
inline Vec2i v2i_mad(Vec2i a,Vec2i b,int f,int prec) {
	return v2i_add(a, v2i_mul(b,f,prec)); 
}
inline Vec2i v2i_lerp(Vec2i a,Vec2i b,int f,int prec) {
	return v2i_add(a, v2i_mul(v2i_sub(b,a),f,prec)); 
}
inline Vec2i v2i_invlerp(const Vec2i vmin,const Vec2i vmax, Vec2i v,int precision ) {
	Vec2i ret={invlerpi(vmin.x, vmax.x, v.x,precision), invlerpi(vmin.y, vmax.y, v.y,precision)}; return ret;
}
inline Vec2i rect2i_lerp(const Rect2i* r, Vec2i v, int precision) {
	Vec2i ret; ret.x=lerpi(r->min.x,r->max.x, v.x,precision); ret.y=lerpi(r->min.y,r->max.y, v.y,precision); return ret;
}
inline Vec2i rect2i_invlerp(const Rect2i* r, Vec2i v, int precision) {
	return v2i_invlerp(r->min,r->max, v, precision);
}
inline Vec2i v2i_min(Vec2i a,Vec2i b) {
	Vec2i ret={MIN(a.x,b.x),MIN(a.y,b.y)}; return ret;
}
inline Vec2i v2i_max(Vec2i a,Vec2i b) {
	Vec2i ret={MAX(a.x,b.x),MAX(a.y,b.y)}; return ret;
}
inline Rect2i rect2i_init(void){
	Rect2i ret={{INT_MAX,INT_MAX},{-INT_MAX,-INT_MAX}}; return ret;
}
inline Rect2i rect2i_mk_at(Vec2i pos, Vec2i size){
	Rect2i ret; ret.min=pos; ret.max=v2i_add(pos,size);  return ret;
}
inline Rect2i rect2i_mk_at_centre(Vec2i centre, Vec2i half_size){
	Rect2i ret; ret.min=v2i_sub(centre,half_size); ret.max=v2i_add(centre,half_size);  return ret;
}
inline Rect2i rect2i_mk(Vec2i a, Vec2i b){
	Rect2i ret; ret.min=a; ret.max=b;  return ret;
}
inline void rect2i_set_init(Rect2i* ret){ 
	v2i_set(&ret->min,INT_MAX,INT_MAX);v2i_set(&ret->max,-INT_MAX,-INT_MAX);
}
inline void rect2i_include(Rect2i* r,Vec2i v){
	r->min=v2i_min(r->min,v); r->max=v2i_max(r->max,v);
}
inline void rect2i_include_rect2i(Rect2i* r,const Rect2i* src){
	r->min=v2i_min(r->min,src->min);r->max=v2i_max(r->max,src->max);
}
inline Vec2i rect2i_size(const Rect2i* r) {
	return v2i_sub(r->max,r->min);
}
inline int rect2i_aspect(const Rect2i* r,int precision) {
	Vec2i sz=rect2i_size(r); return (sz.x*precision)/sz.y;
}
inline Rect2i rect2i_add(const Rect2i* a, Vec2i ofs) {
	Rect2i ret={v2i_add(a->min,ofs),v2i_add(a->max,ofs)}; return ret;
}
inline int rect2i_area( const Rect2i* r,int denom) {
	Vec2i sz=rect2i_size(r); return (sz.x*sz.y/denom);
}
inline Rect2i rect2i_intersect( const Rect2i* a,const Rect2i* b ) {
	Rect2i r; r.min=v2i_max(a->min,b->min); r.max=v2i_min(a->max,b->max); return r;
}
inline int rect2i_overlap( const Rect2i* a,const Rect2i* b) {
	Rect2i ri=rect2i_intersect(a,b);Vec2i sz= rect2i_size(&ri);if (sz.x>0 && sz.y>0) return sz.x*sz.y; else return 0;
}
inline bool rect2i_contains( const Rect2i* rc, const Vec2i v){
	return v.x>=rc->min.x && v.x<rc->max.x && v.y>=rc->min.y && v.y<rc->max.y;
}
inline bool rect2i_valid(const Rect2i* a) {
	return a->max.x>=a->min.x && a->max.y >= a->min.y;
}
inline void vec2i_print(const Vec2i v) {
	printf("(%d %d)", v.x,v.y);
}
inline void rect2i_print(const Rect2i* a) {
	printf("("); vec2i_print(a->min); printf(" "); vec2i_print(a->max); printf(")");
}
inline Vec2i rect2i_centre(const Rect2i* r) {
	return v2i_avr(r->min,r->max);
}
inline PosSize pos_size_mk(const Vec2i pos, const Vec2i size) {
	PosSize psz;psz.pos=pos; psz.size=size; return psz;
}
inline PosSize pos_size_from_rect(const Rect2i* r) {
	PosSize psz;psz.pos=r->min; psz.size=rect2i_size(r); return psz;
}
inline Rect2i rect2i_from_pos_size(const PosSize* psz)
{	rect2i_mk_at(psz->pos, psz->size);
}

#endif
