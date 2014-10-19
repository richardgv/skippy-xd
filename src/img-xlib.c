#include "skippy.h"

#define ICON_PROP_MAXLEN 1048576

pictw_t *
simg_load_icon(session_t *ps, Window wid, int desired_size) {
	pictw_t *pictw = NULL;
	bool processed = false;

	{
		// _NET_WM_ICON
		int best_width = 0, best_height = 0;
		float best_scale = 1.0f, best_area = 0.0f;
		const unsigned char *best_data = NULL;
		winprop_t prop = wid_get_prop_adv(ps, wid, _NET_WM_ICON, 0, ICON_PROP_MAXLEN, XA_CARDINAL, 32);
		if (prop.nitems) {
			int width = 0, height = 0;
			const long *end = prop.data32 + prop.nitems;
			// Format: WIDTH HEIGHT DATA (32-bit)
			int wanted_bytes = 0;
			for (const long *p = prop.data32; p < end; p += wanted_bytes) {
				if (p + 2 >= end) {
					printfef("(%#010lx): %d trailing byte(s).", wid, (int) (end - p));
					break;
				}
				width = p[0];
				height = p[1];
				if (width <= 0 || height <= 0) {
					printfef("(%#010lx): (offset %d, width %d, height %d) Invalid width/height.",
							wid, (int) (p - prop.data32), width, height);
					break;
				}
				wanted_bytes = 2 + width * height;
				if ((end - p) < wanted_bytes) {
					printfef("(%#010lx): (offset %d, width %d, height %d) Not enough bytes (%d/%d).",
							wid, (int) (p - prop.data32), width, height, (int) (end - p), wanted_bytes);
					break;
				}
				// Prefer larger ones if possible
				if (best_width >= desired_size && best_height >= desired_size
						&& (width < desired_size || height < desired_size))
					continue;
				float scale = MAX(1.0f,
						MIN((float) best_height / height, (float) best_width / width));
				float area = width * height * scale * scale;
				if (area > best_area) {
					best_width = width;
					best_height = height;
					best_scale = scale;
					best_area = area;
					best_data = (const unsigned char *) (p + 2);
				}
			}
		}
		if (best_data) {
			{
				unsigned char *converted_data = simg_data32_from_long(
						(const long *) best_data, best_width * best_height);
				simg_data32_premultiply(converted_data, best_width * best_height);
				pictw = simg_data_to_pictw(ps, best_width, best_height, 32, converted_data, 0);
				if (converted_data != best_data)
					free(converted_data);
			}
			if (!pictw)
				printfef("(%#010lx): Failed to create picture.", wid);
			/* if (pictw)
				printfdf("(%#010lx): (offset %d, width %d, height %d) Loaded.",
						wid, (int) (best_data - prop.data8), pictw->width, pictw->height); */
		}
		free_winprop(&prop);
	}

	if (pictw) goto simg_load_icon_end;

	// WM_HINTS
	// Our method probably fills 1-8 bit pixmaps as black instead of using
	// "suitable background/foreground". I hope it isn't a problem, though.
	{
		XWMHints *h = XGetWMHints(ps->dpy, wid);
		if (h && (IconPixmapHint & h->flags) && h->icon_pixmap)
			pictw = simg_pixmap_to_pictw(ps, 0, 0, 0, h->icon_pixmap, h->icon_mask);
		sxfree(h);
	}

	if (pictw) goto simg_load_icon_end;

	// KWM_WIN_ICON
	// Same issue as above.
	{
		winprop_t prop = wid_get_prop_adv(ps, wid, KWM_WIN_ICON, 0, 2, KWM_WIN_ICON, 32);
		if (prop.nitems) {
			Pixmap pxmap = prop.data32[0],
						 mask = (prop.nitems >= 2 ? prop.data32[1]: None);
			if (pxmap)
				pictw = simg_pixmap_to_pictw(ps, 0, 0, 0, pxmap, mask);
		}
		free_winprop(&prop);
	}

	if (pictw) goto simg_load_icon_end;

simg_load_icon_end:
	// Post-processing
	if (pictw && !processed) {
		pictw = simg_postprocess(ps, pictw, PICTPOSP_SCALEK,
				desired_size, desired_size, ALIGN_MID, ALIGN_MID, NULL);
		/* printfdf("(%#010lx): (width %d, height %d) Processed.",
				wid, pictw->width, pictw->height); */
	}

	return pictw;
}
