#include "skippy.h"

/**
 * @brief Crop a rectangle by another rectangle.
 *
 * psrc and pdst cannot be the same.
 */
static inline void
rect_crop(XRectangle *pdst, const XRectangle *psrc, const XRectangle *pbound) {
  assert(psrc != pdst);
  pdst->x = MAX(psrc->x, pbound->x);
  pdst->y = MAX(psrc->y, pbound->y);
  pdst->width = MAX(0,
		  MIN(psrc->x + psrc->width, pbound->x + pbound->width) - pdst->x);
  pdst->height = MAX(0,
		  MIN(psrc->y + psrc->height, pbound->y + pbound->height) - pdst->y);
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
free_pictw(session_t *ps, pictw_t **ppictw) {
	if (*ppictw) {
		free_pixmap(ps, &(*ppictw)->pxmap);
		free_picture(ps, &(*ppictw)->pict);
		free(*ppictw);
	}
	*ppictw = NULL;
}

/**
 * @brief Build a pictw_t of specified size and depth.
 */
static inline pictw_t *
create_pictw(session_t *ps, int width, int height, int depth) {
	pictw_t *pictw = allocchk(calloc(1, sizeof(pictw_t)));
	
	if (!(pictw->pxmap =
				XCreatePixmap(ps->dpy, ps->root, width, height, depth))) {
		printfef("(%d, %d, %d): Failed to create Pixmap.",
				width, height, depth);
		goto create_pictw_err;
	}
	if (!(pictw->pict = XRenderCreatePicture(ps->dpy, pictw->pxmap,
					depth_to_rfmt(ps, depth), 0, NULL))) {
		printfef("(%d, %d, %d): Failed to create Picture.",
				width, height, depth);
		goto create_pictw_err;
	}
	pictw->width = width;
	pictw->height = height;
	pictw->depth = depth;

	return pictw;

create_pictw_err:
	free_pictw(ps, &pictw);

	return NULL;
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

pictw_t *
simg_postprocess(session_t *ps, pictw_t *src, enum pict_posp_mode mode,
		int twidth, int theight, enum align alg, enum align valg,
		const XRenderColor *pc);

static inline pictw_t *
simg_data_to_pictw(session_t *ps, int width, int height, int depth,
		const unsigned char *data, int bytes_per_line) {
	assert(data);
	pictw_t *pictw = NULL;
	GC gc = None;
	XImage *img = XCreateImage(ps->dpy, DefaultVisual(ps->dpy, ps->screen),
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
	for (; len >= 0; --len) {
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
	for (; len >= 0; --len) {
		unsigned char r = data[len * 4];
		data[len * 4 + 0] = data[len * 4 + 2];
		data[len * 4 + 2] = r;
	}
}

static inline void
simg_data32_premultiply(unsigned char *data, int len) {
	for (; len > 0; --len) {
		float a = (float) data[len * 4 + 3] / 0xff;
		data[len * 4 + 0] *= a;
		data[len * 4 + 1] *= a;
		data[len * 4 + 2] *= a;
	}
}
