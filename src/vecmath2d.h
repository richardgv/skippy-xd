#ifndef vecmath2d_h
#define vecmath2d_h

// 2d int maths  - types and inline helper functions/accessors
//
// type_op_(optional types, usually same as type if not mentioned)
// common type vec2i has abreviation v2i_
// type_set -- setter 
// type_noun/adj -- getter 
// type_verb/question -- calculate something

typedef int VScalar;	// Scalar value used here. TODO: int? i32?  i64? :)
typedef struct Vec2i {	// todo: refactor to use this
	VScalar x,y;
} Vec2i;

// rectangular region, aka RECT, AABB,   'Extents<Vec2i>'
typedef struct Rect2i {
	Vec2i min,max;
} Rect2i;

// alternative way of specifying rectanglular region
typedef struct PosSize {
	Vec2i pos,size;
} PosSize;

#ifndef CLAMP
#define CLAMP(V,LO,HI) (MIN(HI,(MAX(LO,V))))
#endif

inline VScalar invlerpi(VScalar lo,VScalar hi,VScalar v,int one) {
	return ((v-lo)*one)/(hi-lo);
}
inline int lerpi(VScalar lo,VScalar hi,VScalar f,VScalar one) {
	return lo+(((hi-lo)*f)/one);
}
inline void v2i_set(Vec2i* dst, VScalar x, VScalar y) {
	dst->x=x;dst->y=y;
}
inline Vec2i vec2i_mk(VScalar x, VScalar y) {
	Vec2i v; v.x=x; v.y=y; return v;
}
inline Vec2i vec2i_splat(VScalar v) {
	return	vec2i_mk(v,v);
}
inline Vec2i v2i_sub(Vec2i a,Vec2i b) {
	Vec2i ret= {a.x-b.x,a.y-b.y};return ret;
}
inline Vec2i v2i_add(Vec2i a,Vec2i b) {
	Vec2i ret= {a.x+b.x,a.y+b.y};return ret; 
}
inline Vec2i v2i_mul(Vec2i a,VScalar f,VScalar prec) {
	Vec2i ret= {(a.x*f)/prec,(a.y*f)/prec};return ret; 
}
inline Vec2i v2i_half(Vec2i a) {
	return vec2i_mk(a.x/2,a.y/2);
}
inline Vec2i v2i_avr(Vec2i a,Vec2i b) {
	Vec2i ret=v2i_mul(v2i_add(a,b),1,2); return ret;
}
inline Vec2i v2i_mad(Vec2i a,Vec2i b,VScalar f,VScalar prec) {
	return v2i_add(a, v2i_mul(b,f,prec)); 
}
inline Vec2i v2i_lerp(Vec2i a,Vec2i b,VScalar f,VScalar prec) {
	return v2i_add(a, v2i_mul(v2i_sub(b,a),f,prec)); 
}
inline Vec2i v2i_invlerp(const Vec2i vmin,const Vec2i vmax, Vec2i v,VScalar precision ) {
	Vec2i ret={invlerpi(vmin.x, vmax.x, v.x,precision), invlerpi(vmin.y, vmax.y, v.y,precision)}; return ret;
}
// turn a fractional position into a position within a rect
inline Vec2i rect2i_lerp(const Rect2i* r, Vec2i v, VScalar precision) {
	Vec2i ret; ret.x=lerpi(r->min.x,r->max.x, v.x,precision); ret.y=lerpi(r->min.y,r->max.y, v.y,precision); return ret;
}
// get a vector which is the fractional position of a point in a rect
inline Vec2i rect2i_invlerp(const Rect2i* r, Vec2i v, VScalar precision) {
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
// expand a rectangle to include a vector
inline void rect2i_include(Rect2i* r,Vec2i v){
	r->min=v2i_min(r->min,v); r->max=v2i_max(r->max,v);
}
// expand a rect to include a rect
inline void rect2i_include_rect2i(Rect2i* r,const Rect2i* src){
	r->min=v2i_min(r->min,src->min);r->max=v2i_max(r->max,src->max);
}
inline Vec2i rect2i_size(const Rect2i* r) {
	return v2i_sub(r->max,r->min);
}
inline int rect2i_aspect(const Rect2i* r,VScalar precision) {
	Vec2i sz=rect2i_size(r); return (sz.x*precision)/sz.y;
}
inline Rect2i rect2i_add(const Rect2i* a, Vec2i ofs) {
	Rect2i ret={v2i_add(a->min,ofs),v2i_add(a->max,ofs)}; return ret;
}
inline int rect2i_area( const Rect2i* r,VScalar denom) {
	Vec2i sz=rect2i_size(r); return (sz.x*sz.y/denom);
}
inline PosSize rect2i_pos_size(const Rect2i* r) {
	PosSize psz; psz.pos=r->min; psz.size=v2i_sub(r->max,r->min); return psz;
}
inline void rect2i_set(Rect2i* rc, const Vec2i a, const Vec2i b) { rc->min=a; rc->max=b;}
inline void rect2i_set_pos_size(Rect2i* rc, const Vec2i pos, const Vec2i size) { rc->min=pos;rc->max=v2i_add(pos,size);}
inline VScalar rect2i_width(const Rect2i* r) { return rect2i_size(r).x;}
inline VScalar rect2i_height(const Rect2i* r) { return rect2i_size(r).y;}
inline Rect2i rect2i_intersect( const Rect2i* a,const Rect2i* b ) {
	Rect2i r; r.min=v2i_max(a->min,b->min); r.max=v2i_min(a->max,b->max); return r;
}
inline VScalar rect2i_overlap_area( const Rect2i* a,const Rect2i* b) {
	Rect2i ri=rect2i_intersect(a,b);Vec2i sz= rect2i_size(&ri);if (sz.x>0 && sz.y>0) return sz.x*sz.y; else return 0;
}
inline bool rect2i_contains( const Rect2i* rc, const Vec2i v){
	return v.x>=rc->min.x && v.x<rc->max.x && v.y>=rc->min.y && v.y<rc->max.y;
}
inline bool rect2i_overlap( const Rect2i* ra, const Rect2i* rb){
	//if boundaries touch, area of overlap is still zero
	if (ra->max.x<=rb->min.x || ra->min.x>=rb->max.x) return false;
	if (ra->max.y<=rb->min.y || ra->min.y>=rb->max.y) return false;
	return 	true;
}
inline void rect2i_set_centre(Rect2i* rc, const Vec2i new_centre) {
	Vec2i size=rect2i_size(rc);
	rect2i_set_pos_size(rc, v2i_sub(new_centre, v2i_half(size)), size);
}
inline void rect2i_split_x(Rect2i* out0, Rect2i* out1,VScalar split, const Rect2i* rc ) {
	out0->min=rc->min;
	out0->max.x=split;
	out0->max.y=rc->max.y;

	out1->min.x=split;
	out1->min.y=rc->min.y;
	out1->max=rc->max;
}
inline void rect2i_split_y(Rect2i* out0, Rect2i* out1,VScalar split, const Rect2i* rc ) {
	out0->min=rc->min;
	out0->max.y=split;
	out0->max.x=rc->max.x;

	out1->min.y=split;
	out1->min.x=rc->min.x;
	out1->max=rc->max;
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
inline void pos_size_set_rect(PosSize* psz, const Rect2i* r) {
	psz->pos=r->min;
	psz->size=rect2i_size(r);
}
inline PosSize pos_size_from_rect(const Rect2i* r) {
	PosSize psz;pos_size_set_rect(&psz,r); return psz;
}
inline Rect2i rect2i_from_pos_size(const PosSize* psz)
{	rect2i_mk_at(psz->pos, psz->size);
}

#endif
