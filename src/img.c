#include "skippy.h"
#include <float.h>

pictw_t *
simg_load(session_t *ps, const char *path, enum pict_posp_mode mode,
		int twidth, int theight, enum align alg, enum align valg,
		const XRenderColor *pc) {
	pictw_t *result = NULL;
	bool processed = false;

	if (!path || !path[0]) { }
#ifdef CFG_LIBPNG
	else if (str_endwith(path, ".png"))
		result = spng_read(ps, path);
#endif
#ifdef CFG_JPEG
	else if (str_endwith(path, ".jpg") || str_endwith(path, ".jpeg")
			|| str_endwith(path, ".jpe"))
		result = sjpg_read(ps, path);
#endif
#ifdef CFG_JPEG
	else if (str_endwith(path, ".gif"))
		result = sgif_read(ps, path);
#endif
	if (!processed)
		result = simg_postprocess(ps, result, mode, twidth, theight,
				alg, valg, pc);

	return result;
}

pictw_t *
simg_postprocess(session_t *ps, pictw_t *src, enum pict_posp_mode mode,
		int twidth, int theight, enum align alg, enum align valg,
		const XRenderColor *pc) {
	const int depth = 32;
	pictw_t *dest = NULL;
	bool transformed = false;

	if (!src) {
		if (twidth && theight) {
			if (!(dest = create_pictw(ps, twidth, theight, depth)))
				printfef("(): Failed to create Picture.");
			else
				XRenderFillRectangle(ps->dpy, PictOpSrc, dest->pict, pc, 0, 0,
						twidth, theight);
		}
		goto simg_postprocess_end;
	}

	if (!(twidth || theight)
			|| (twidth == src->width && theight == src->height))
		return src;

	// Determine composite paramaters. We have to do this before create
	// Picture because the width/height may need to be calculated.
	int width = src->width, height = src->height;
	if (!twidth) twidth = width;
	if (!theight) theight = height;
	double ratio_x = 1.0, ratio_y = 1.0;
	switch (mode) {
		case PICTPOSP_ORIG: break;
		case PICTPOSP_TILE: break;
		case PICTPOSP_SCALE:
		case PICTPOSP_SCALEK:
							{
								if (twidth) ratio_x = (double) twidth / width;
								if (theight) ratio_y = (double) theight / height;
								if (PICTPOSP_SCALEK == mode)
									ratio_x = ratio_y = MIN(ratio_x, ratio_y);
								width *= ratio_x;
								height *= ratio_y;
							}
							break;
		default:            assert(0); break;
	}

	if (!(dest = create_pictw(ps, twidth, theight, depth))) {
		printfef("(): Failed to create Picture.");
		goto simg_postprocess_end;
	}

	int x = 0, y = 0;
	int num_x = 1, num_y = 1;
	if (PICTPOSP_TILE == mode) {
	    // num_x = ceil((float) twidth / width);
	    // num_y = ceil((float) theight / height);
	    num_x = twidth / width;
	    num_y = theight / height;
	}
	switch (alg) {
		case ALIGN_LEFT:  break;
		case ALIGN_RIGHT: x = twidth - width * num_x; break;
		case ALIGN_MID:   x = (twidth - width * num_x) / 2; break;
	};
	switch (valg) {
		case ALIGN_LEFT:  break;
		case ALIGN_RIGHT: y = theight - height * num_y; break;
		case ALIGN_MID:   y = (theight - height * num_y) / 2; break;
	};
	x = MAX(x, 0);
	y = MAX(y, 0);
	int x2 = MIN(x + width * num_x, twidth),
			y2 = MIN(y + height * num_y, theight);
	/* if (pc->alpha) */ {
		if (src->depth >= 32)
			XRenderFillRectangle(ps->dpy, PictOpSrc, dest->pict, pc, 0, 0,
					twidth, theight);
		else {
			XRenderFillRectangle(ps->dpy, PictOpSrc, dest->pict, pc, 0, 0,
					twidth, y);
			XRenderFillRectangle(ps->dpy, PictOpSrc, dest->pict, pc, 0, y2,
					twidth, theight - y2);
			XRenderFillRectangle(ps->dpy, PictOpSrc, dest->pict, pc, 0, y,
					x, y2 - y);
			XRenderFillRectangle(ps->dpy, PictOpSrc, dest->pict, pc, x2, y,
					twidth - x2, y2 - y);
		}
	}
	if (src->pict) {
		if (1.0 != ratio_x || 1.0 != ratio_y) {
			XTransform transform = { .matrix = {
				{ XDoubleToFixed(1.0 / ratio_x), XDoubleToFixed(0.0), XDoubleToFixed(0.0) },
				{ XDoubleToFixed(0.0), XDoubleToFixed(1.0 / ratio_y), XDoubleToFixed(0.0) },
				{ XDoubleToFixed(0.0), XDoubleToFixed(0.0), XDoubleToFixed(1.0) },
			} };
			transformed = true;
			XRenderSetPictureTransform(ps->dpy, src->pict, &transform);
		}

		XRectangle bound = { .x = x, .y = y, .width = x2, .height = y2 };
		int op = (src->depth >= 32 && pc->alpha ? PictOpOver: PictOpSrc);
		for (int i = 0; i < num_x; ++i)
			for (int j = 0; j < num_y; ++j) {
				XRectangle reg = { .x = x + i * width, .y = y + j * height,
					.width = width, .height = height };
				XRectangle reg2;
				rect_crop(&reg2, &reg, &bound);
				if (reg2.width && reg2.height) {
					XRenderComposite(ps->dpy, op, src->pict, None,
							dest->pict, 0, 0, 0, 0,
							reg2.x, reg2.y, reg2.width, reg2.height);
				}
			}
	}

	if (transformed) {
		static XTransform transform = { .matrix = {
			{ 1.0, 0.0, 0.0 },
			{ 0.0, 1.0, 0.0 },
			{ 0.0, 0.0, 1.0 },
		} };
		XRenderSetPictureTransform(ps->dpy, src->pict, &transform);
	}

simg_postprocess_end:
	free_pictw(ps, &src);
	return dest;
}
