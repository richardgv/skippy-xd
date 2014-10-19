#include "skippy.h"

typedef struct {
	XRectangle rect;
	int num_x, num_y;
	double ratio_x, ratio_y;
} img_composite_params_t;

#define XRECTANGLE_INIT { .x = 0, .y = 0, .width = 0, .height = 0 }

#define IMG_COMPOSITE_PARAMS_INIT { \
	.rect = XRECTANGLE_INIT, \
	.num_x = 0, \
	.num_y = 0, \
	.ratio_x = 0.0f, \
	.ratio_y = 0.0f, \
}

/**
 * @brief Crop a rectangle by another rectangle.
 */
static inline void
rect_crop(XRectangle *pdst, const XRectangle *psrc, const XRectangle *pbound) {
	XRectangle *pdst_original = NULL;
	XRectangle rtmp;
	if (psrc == pdst) {
		pdst_original = pdst;
		pdst = &rtmp;
	}
	assert(psrc != pdst);
	pdst->x = MAX(psrc->x, pbound->x);
	pdst->y = MAX(psrc->y, pbound->y);
	pdst->width = MAX(0,
			MIN(psrc->x + psrc->width, pbound->x + pbound->width) - pdst->x);
	pdst->height = MAX(0,
			MIN(psrc->y + psrc->height, pbound->y + pbound->height) - pdst->y);
	if (pdst_original)
		*pdst_original = *pdst;
}

/**
 * @brief Crop a rectangle by another rectangle, and adjust texture
 *        start coordinate.
 *
 * ptex_x and ptex_y must point to initialized values.
 */
static inline void
rect_crop2(XRectangle *pdst, const XRectangle *psrc, const XRectangle *pbound,
		int *ptex_x, int *ptex_y, double ratio_x, double ratio_y) {
	int original_x = psrc->x, original_y = psrc->y;
	rect_crop(pdst, psrc, pbound);
	*ptex_x += (pdst->x - original_x) / ratio_x;
	*ptex_y += (pdst->y - original_y) / ratio_y;
}

/**
 * @brief Get length of 1 pixel of data for the specified depth.
 */
static inline int
depth_to_len(int depth) {
	int l = 8;
	while (depth > l)
		l *= 2;
	return l / 8;
}

/**
 * @brief Get X Render format for a specified depth.
 */
static inline const XRenderPictFormat *
depth_to_rfmt(session_t *ps, int depth) {
	switch (depth) {
		case 32: return XRenderFindStandardFormat(ps->dpy, PictStandardARGB32);
		case 24: return XRenderFindStandardFormat(ps->dpy, PictStandardRGB24);
		case 8: return XRenderFindStandardFormat(ps->dpy, PictStandardA8);
		case 4: return XRenderFindStandardFormat(ps->dpy, PictStandardA4);
		case 1: return XRenderFindStandardFormat(ps->dpy, PictStandardA1);
	};
	assert(0);
	return NULL;
}

static inline void
free_pictw_keeppixmap(session_t *ps, pictw_t **ppictw) {
	if (*ppictw) {
		free_picture(ps, &(*ppictw)->pict);
		free(*ppictw);
	}
	*ppictw = NULL;
}

static inline void
free_pictw(session_t *ps, pictw_t **ppictw) {
	if (*ppictw) {
		free_pixmap(ps, &(*ppictw)->pxmap);
		free_pictw_keeppixmap(ps, ppictw);
	}
}

/**
 * @brief Destroy a <code>pictspec_t</code>.
 */
static inline void
free_pictspec(session_t *ps, pictspec_t *p) {
	free_pictw(ps, &p->img);
	free(p->path);
}

/**
 * @brief Build a pictw_t of specified size and depth.
 */
static inline pictw_t *
create_pictw_frompixmap(session_t *ps, int width, int height, int depth,
		Pixmap pxmap) {
	pictw_t *pictw = NULL;

	if (!pxmap) {
		printfef("(%d, %d, %d, %#010lx): Missing pixmap.", width, height, depth, pxmap);
		return NULL;
	}

	// Acquire Pixmap info
	if (!(width && height && depth)) {
		Window rroot = None;
		int rx = 0, ry = 0;
		unsigned rwidth = 0, rheight = 0, rborder_width = 0, rdepth = 0;
		if (!XGetGeometry(ps->dpy, pxmap,
				&rroot, &rx, &ry, &rwidth, &rheight, &rborder_width, &rdepth)) {
			printfef("(%d, %d, %d, %#010lx): Failed to determine pixmap size.", width, height, depth, pxmap);
			return NULL;
		}
		width = rwidth;
		height = rheight;
		depth = rdepth;
	}

	// Sanity check
	if (!(width && height && depth)) {
		printfef("(%d, %d, %d, %#010lx): Failed to get pixmap info.", width, height, depth, pxmap);
		return NULL;
	}

	// Find X Render format
	const XRenderPictFormat *rfmt = depth_to_rfmt(ps, depth);
	if (!rfmt) {
		printfef("(%d, %d, %d, %#010lx): Failed to find X Render format for depth %d.",
				width, height, depth, pxmap, depth);
		return NULL;
	}

	// Create Picture
	pictw = scalloc(1, pictw_t);
	pictw->pxmap = pxmap;
	pictw->width = width;
	pictw->height = height;
	pictw->depth = depth;

	if (!(pictw->pict = XRenderCreatePicture(ps->dpy, pictw->pxmap,
					rfmt, 0, NULL))) {
		printfef("(%d, %d, %d, %#010lx): Failed to create Picture.",
				width, height, depth, pxmap);
		free_pictw(ps, &pictw);
	}

	return pictw;
}

/**
 * @brief Build a pictw_t of specified size and depth.
 */
static inline pictw_t *
create_pictw(session_t *ps, int width, int height, int depth) {
	Pixmap pxmap = XCreatePixmap(ps->dpy, ps->root, width, height, depth);
	if (!pxmap) {
		printfef("(%d, %d, %d): Failed to create Pixmap.", width, height, depth);
		return NULL;
	}

	return create_pictw_frompixmap(ps, width, height, depth, pxmap);
}

static inline pictw_t *
clone_pictw(session_t *ps, pictw_t *pictw) {
	if (!pictw) return NULL;
	pictw_t *new_pictw = create_pictw(ps, pictw->width, pictw->height, pictw->depth);
	if (!new_pictw) return NULL;
	XRenderComposite(ps->dpy, PictOpSrc, pictw->pict, None, new_pictw->pict,
			0, 0, 0, 0, 0, 0, new_pictw->width, new_pictw->height);

	return new_pictw;
}

pictw_t *
simg_load(session_t *ps, const char *path, enum pict_posp_mode mode,
		int twidth, int theight, enum align alg, enum align valg,
		const XRenderColor *pc);

static inline pictw_t *
simg_load_s(session_t *ps, const pictspec_t *spec) {
	return simg_load(ps, spec->path, spec->mode, spec->twidth, spec->theight,
			spec->alg, spec->valg, &spec->c);
}

static inline bool
simg_cachespec(session_t *ps, pictspec_t *spec) {
	free_pictw(ps, &spec->img);
	if (spec->path
			&& !(spec->img = simg_load(ps, spec->path, PICTPOSP_ORIG, 0, 0,
					ALIGN_MID, ALIGN_MID, NULL)))
		return false;
	return true;
}

pictw_t *
simg_postprocess(session_t *ps, pictw_t *src, enum pict_posp_mode mode,
		int twidth, int theight, enum align alg, enum align valg,
		const XRenderColor *pc);

void
simg_get_composite_params(pictw_t *src, int twidth, int theight,
		enum pict_posp_mode mode, enum align alg, enum align valg,
		img_composite_params_t *pparams);

void
simg_composite(session_t *ps, pictw_t *src, Picture dest,
		int twidth, int theight, const img_composite_params_t *pparams,
		const XRenderColor *pc, Picture mask, const XRectangle *pbound);

/**
 * @brief XRenderFillRectangle() with area cropped by a rectangle.
 */
static inline void
XRenderFillRectangle_cropped(Display *dpy, int op, Picture dst,
		const XRenderColor *color,
		int x, int y, unsigned int width, unsigned int height,
		const XRectangle *pbound) {
	XRectangle r = { .x = x, .y = y, .width = width, .height = height };
	if (pbound)
		rect_crop(&r, &r, pbound);
	XRenderFillRectangle(dpy, op, dst, color,
			r.x, r.y, r.width, r.height);
}

/**
 * @brief XRenderComposite() with area cropped by a rectangle.
 */
static inline void
XRenderComposite_cropped(Display *dpy, int op, Picture src, Picture mask,
		Picture dst, int src_x, int src_y, int mask_x, int mask_y,
		int dst_x, int dst_y, unsigned int width, unsigned int height,
		const XRectangle *pbound, double ratio_x, double ratio_y) {
	XRectangle r = { .x = dst_x, .y = dst_y, .width = width, .height = height };
	if (pbound)
		rect_crop2(&r, &r, pbound, &src_x, &src_y, ratio_x, ratio_y);
	XRenderComposite(dpy, op, src, mask, dst, src_x, src_y,
			mask_x, mask_y, r.x, r.y, r.width, r.height);
}

static inline pictw_t *
simg_pixmap_to_pictw(session_t *ps, int width, int height, int depth,
		Pixmap pxmap, Pixmap mask) {
	GC gc = None;
	pictw_t *porig = create_pictw_frompixmap(ps, width, height, depth, pxmap);
	pictw_t *pmask = NULL;
	pictw_t *pictw = NULL;

	if (!porig) {
		printfef("(%d, %d, %d, %#010lx, %#010lx): Failed to create picture for pixmap.",
				width, height, depth, pxmap, mask);
		goto simg_pixmap_to_pict_end;
	}

	if (mask) {
		if (!(pmask = create_pictw_frompixmap(ps, width, height, depth, mask))) {
			printfef("(%d, %d, %d, %#010lx, %#010lx): Failed to create picture for mask.",
					width, height, depth, pxmap, mask);
			goto simg_pixmap_to_pict_end;
		}
		// Probably we should check for width/height consistency between pixmap
		// and mask...
	}

	if (!(pictw = create_pictw(ps, porig->width, porig->height, (mask ? 32: porig->depth)))) {
		printfef("(%d, %d, %d, %#010lx, %#010lx): Failed to create target picture.",
				width, height, depth, pxmap, mask);
		goto simg_pixmap_to_pict_end;
	}

	// Copy content
	static const XRenderColor XRC_TRANS = {
		.red = 0, .green = 0, .blue = 0, .alpha = 0
	};
	XRenderFillRectangle(ps->dpy, PictOpSrc, pictw->pict, &XRC_TRANS,
			0, 0, pictw->width, pictw->height);
	XRenderComposite(ps->dpy, PictOpSrc, porig->pict, (pmask ? pmask->pict: None),
			pictw->pict, 0, 0, 0, 0, 0, 0, pictw->width, pictw->height);

	// Does core Xlib handle transparency correctly?
	/*
	gc = XCreateGC(ps->dpy, pictw->pxmap, 0, 0);
	if (!gc) {
		printfef("(%#010lx, %#010lx, %d, %d, %d): Failed to create GC.",
				pxmap, mask, width, height, depth);
		free_pictw(ps, &pictw);
		goto simg_data_to_pict_end;
	}
	if (XCopyArea(ps->dpy, pxmap, pictw->pxmap, gc, 0, 0, width, height, 0, 0)) {
	}
	*/

simg_pixmap_to_pict_end:
	free_pictw_keeppixmap(ps, &porig);
	free_pictw_keeppixmap(ps, &pmask);
	if (gc)
		XFreeGC(ps->dpy, gc);

	return pictw;
}

static inline pictw_t *
simg_data_to_pictw(session_t *ps, int width, int height, int depth,
		const unsigned char *data, int bytes_per_line) {
	assert(data);
	data = mmemcpy(data, height
			* (bytes_per_line ? bytes_per_line: depth_to_len(depth) * width));
	pictw_t *pictw = NULL;
	GC gc = None;

	// Use ARGB visual if needed
	Visual *visual = DefaultVisual(ps->dpy, ps->screen);
	if (32 == depth && ps->argb_visual)
		visual = ps->argb_visual;
	XImage *img = XCreateImage(ps->dpy, visual,
			depth, ZPixmap, 0, (char *) data, width, height,
			8, bytes_per_line);
	if (!img) {
		printfef("(%d, %d, %d): Failed to create XImage.",
				width, height, depth);
		goto simg_data_to_pict_end;
	}
	if (!(pictw = create_pictw(ps, width, height, depth))) {
		printfef("(%d, %d, %d): Failed to create Picture.",
				width, height, depth);
		goto simg_data_to_pict_end;
	}
	gc = XCreateGC(ps->dpy, pictw->pxmap, 0, 0);
	if (!gc) {
		printfef("(%d, %d, %d): Failed to create GC.",
				width, height, depth);
		free_pictw(ps, &pictw);
		goto simg_data_to_pict_end;
	}
	XPutImage(ps->dpy, pictw->pxmap, gc, img, 0, 0, 0, 0, width, height);

simg_data_to_pict_end:
	if (img)
		XDestroyImage(img);
	if (gc)
		XFreeGC(ps->dpy, gc);

	return pictw;
}

static inline void
simg_data24_premultiply(unsigned char *data, int len) {
	for (; len > 0; len--, data += 4) {
		const unsigned char alpha = data[3];
		if (0xff == alpha || !alpha) continue;
		// This is slightly inaccurate because 1.0 == 255 instead of 256 (2 ** 8).
		// Of course there are "smarter" ways to perform 3 multiplications all at
		// once, but I don't feel like writing endian-specific code.
		data[0] = ((uint_fast16_t) data[0] * alpha) >> 8;
		data[1] = ((uint_fast16_t) data[1] * alpha) >> 8;
		data[2] = ((uint_fast16_t) data[2] * alpha) >> 8;
	}
}

static inline void
simg_data24_fillalpha(unsigned char *data, int len) {
	for (--len; len >= 0; --len) {
		if (len) {
			data[len * 4 + 0] = data[len * 3 + 0];
			data[len * 4 + 1] = data[len * 3 + 1];
			data[len * 4 + 2] = data[len * 3 + 2];
		}
		data[len * 4 + 3] = 0xff;
	}
}

static inline void
simg_data24_tobgr(unsigned char *data, int len) {
	for (--len; len >= 0; --len) {
		unsigned char r = data[len * 4];
		data[len * 4 + 0] = data[len * 4 + 2];
		data[len * 4 + 2] = r;
	}
}

static inline unsigned char *
simg_data32_from_long(const long *src, int len) {
	if (4 == sizeof(long))
		return (unsigned char *) src;

	uint32_t *data = smalloc(len, uint32_t);
	for (int i = 0; i < len; ++i)
		data[i] = src[i];
	return (unsigned char *) data;
}

static inline void
simg_data32_premultiply(unsigned char *data, int len) {
	for (--len; len >= 0; --len) {
		float a = (float) data[len * 4 + 3] / 0xff;
		data[len * 4 + 0] *= a;
		data[len * 4 + 1] *= a;
		data[len * 4 + 2] *= a;
	}
}
