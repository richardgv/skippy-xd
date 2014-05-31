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
#ifdef CFG_GIFLIB
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
	static const XRenderColor XRC_TRANS = {
		.red = 0, .green = 0, .blue = 0, .alpha = 0
	};
	if (!pc) pc = &XRC_TRANS;

	const int depth = 32;
	pictw_t *dest = NULL;

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

	if (!twidth) twidth = (double) theight / src->height * src->width;
	else if (!theight) theight = (double) twidth / src->width * src->height;

	if (!(dest = create_pictw(ps, twidth, theight, depth))) {
		printfef("(): Failed to create Picture.");
		goto simg_postprocess_end;
	}

	img_composite_params_t params = IMG_COMPOSITE_PARAMS_INIT;
	simg_get_composite_params(src, twidth, theight, mode, alg, valg, &params);
	simg_composite(ps, src, dest->pict, twidth, theight, &params, pc,
			None, NULL);

simg_postprocess_end:
	free_pictw(ps, &src);
	return dest;
}

void
simg_get_composite_params(pictw_t *src, int twidth, int theight,
		enum pict_posp_mode mode, enum align alg, enum align valg,
		img_composite_params_t *pparams) {
	int width = src->width, height = src->height;
	double ratio_x = 1.0, ratio_y = 1.0;
	switch (mode) {
		case PICTPOSP_ORIG:
			break;
		case PICTPOSP_TILE:
			break;
		case PICTPOSP_SCALE:
		case PICTPOSP_SCALEK:
		case PICTPOSP_SCALEE:
		case PICTPOSP_SCALEEK:
			{
				if (twidth) ratio_x = (double) twidth / width;
				if (theight) ratio_y = (double) theight / height;
				if (PICTPOSP_SCALEK == mode || PICTPOSP_SCALEEK == mode)
					ratio_x = ratio_y = MIN(ratio_x, ratio_y);
				if (PICTPOSP_SCALEE == mode || PICTPOSP_SCALEEK == mode) {
					ratio_x = MAX(1.0f, ratio_x);
					ratio_y = MAX(1.0f, ratio_y);
				}
				width *= ratio_x;
				height *= ratio_y;
			}
							break;
		default:            assert(0); break;
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

	pparams->rect.x = x;
	pparams->rect.y = y;
	pparams->rect.width = width;
	pparams->rect.height = height;
	pparams->num_x = num_x;
	pparams->num_y = num_y;
	pparams->ratio_x = ratio_x;
	pparams->ratio_y = ratio_y;
}

void
simg_composite(session_t *ps, pictw_t *src, Picture dest,
		int twidth, int theight, const img_composite_params_t *pparams,
		const XRenderColor *pc, Picture mask, const XRectangle *pbound) {
	assert(src);
	assert(pparams);

	const double ratio_x = pparams->ratio_x, ratio_y = pparams->ratio_y;
	const int num_x = pparams->num_x;
	const int num_y = pparams->num_y;
	const int width = src->width * pparams->ratio_x;
	const int height = src->height * pparams->ratio_y;
	const int x = pparams->rect.x;
	const int y = pparams->rect.y;

	int x2 = MIN(x + width * num_x, twidth),
			y2 = MIN(y + height * num_y, theight);
	if (pc) {
		if (src->depth >= 32)
			XRenderFillRectangle_cropped(ps->dpy, PictOpSrc, dest, pc, 0, 0,
					twidth, theight, pbound);
		else {
			XRenderFillRectangle_cropped(ps->dpy, PictOpSrc, dest, pc, 0, 0,
					twidth, y, pbound);
			XRenderFillRectangle_cropped(ps->dpy, PictOpSrc, dest, pc, 0, y2,
					twidth, theight - y2, pbound);
			XRenderFillRectangle_cropped(ps->dpy, PictOpSrc, dest, pc, 0, y,
					x, y2 - y, pbound);
			XRenderFillRectangle_cropped(ps->dpy, PictOpSrc, dest, pc, x2, y,
					twidth - x2, y2 - y, pbound);
		}
	}

	bool transformed = false;
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

		XRectangle bound = { .x = x, .y = y, .width = x2 - x, .height = y2 - y};
		int op = (src->depth >= 32 || (pc && pc->alpha) || mask ? PictOpOver: PictOpSrc);
		for (int i = 0; i < num_x; ++i)
			for (int j = 0; j < num_y; ++j) {
				XRectangle reg = { .x = x + i * width, .y = y + j * height,
					.width = width, .height = height };
				int tex_x = 0, tex_y = 0;
				XRectangle reg2;
				rect_crop2(&reg2, &reg, &bound, &tex_x, &tex_y, ratio_x, ratio_y);
				if (reg2.width > 0 && reg2.height > 0) {
					XRenderComposite_cropped(ps->dpy, op, src->pict, mask,
							dest, tex_x, tex_y, 0, 0,
							reg2.x, reg2.y, reg2.width, reg2.height,
							pbound, ratio_x, ratio_y);
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
}

