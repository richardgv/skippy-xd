#include "skippy.h"
#include <jpeglib.h>

pictw_t *
sjpg_read(session_t *ps, const char *path) {
	pictw_t *pictw = NULL;
	JSAMPLE *data = NULL;
	bool need_abort = false;
	struct jpeg_error_mgr jerr;
	struct jpeg_decompress_struct cinfo = {
		.err = jpeg_std_error(&jerr),
	};
	jpeg_create_decompress(&cinfo);

	FILE *fp = fopen(path, "rb");
	if (unlikely(!fp)) {
		printfef("(\"%s\"): Failed to open file.", path);
		goto sjpg_read_end;
	}
	jpeg_stdio_src(&cinfo, fp);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);
	need_abort = true;
	int width = 0, height = 0, depth = 24;
	{
		const int comps = 4;
		data = allocchk(malloc(cinfo.output_width *
					cinfo.output_height * comps * sizeof(JSAMPLE)));
		JSAMPROW rowptrs[cinfo.output_height];
		for (int i = 0; i < cinfo.output_height; ++i)
			rowptrs[i] = data + i * cinfo.output_width * comps;
		width = cinfo.output_width;
		height = cinfo.output_height;
		// libjpeg enforces a loop to read scanlines
		while (cinfo.output_scanline < cinfo.output_height) {
			if (unlikely(!jpeg_read_scanlines(&cinfo, &rowptrs[cinfo.output_scanline],
							cinfo.output_height - cinfo.output_scanline))) {
				printfef("(\"%s\"): Failed to read scanline %d.", path,
						cinfo.output_scanline);
				goto sjpg_read_end;
			}
		}

		// Expand greyscale value to RGBA
		if (1 == cinfo.output_components) {
			for (int i = 0; i < height; ++i)
				for (int j = width - 1; j >= 0; --j) {
					unsigned char a = rowptrs[i][j];
					rowptrs[i][j * comps + 0] = a;
					rowptrs[i][j * comps + 1] = a;
					rowptrs[i][j * comps + 2] = a;
					rowptrs[i][j * comps + 3] = 0xff;
				}
		}
		else if (3 == cinfo.output_components) {
			for (int i = 0; i < height; ++i) {
				simg_data24_fillalpha(&data[i * 4 * width], width);
				simg_data24_tobgr(&data[i * 4 * width], width);
			}
		}
	}
	if (unlikely(!jpeg_finish_decompress(&cinfo))) {
		printfef("(\"%s\"): Failed to finish decompression.", path);
		goto sjpg_read_end;
	}
	need_abort = false;
	pictw = simg_data_to_pictw(ps, width, height, depth, data, 0);
	free(data);
	if (unlikely(!pictw)) {
		printfef("(\"%s\"): Failed to create Picture.", path);
		goto sjpg_read_end;
	}

sjpg_read_end:
	/* if (unlikely(data && !pictw))
		free(data); */
	if (unlikely(need_abort))
		jpeg_abort_decompress(&cinfo);
	if (likely(fp))
		fclose(fp);
	jpeg_destroy_decompress(&cinfo);

	return pictw;
}
