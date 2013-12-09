#include "skippy.h"
#include <png.h>
#include <zlib.h>
#define SPNG_SIGBYTES 8

// <libpng-1.0.6 compatibility
#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->png_jmpbuf)
#endif

void
spng_about(FILE *os) {
	fprintf(os, "PNG support: Yes\n"
			"  Compiled with libpng %s, using %s.\n"
			"  Compiled with zlib %s, using %s.\n",
			PNG_LIBPNG_VER_STRING, png_libpng_ver,
			ZLIB_VERSION, zlib_version);
}

pictw_t *
spng_read(session_t *ps, const char *path) {
	assert(path);

	char sig[SPNG_SIGBYTES] = "";
	pictw_t *pictw = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	FILE *fp = fopen(path, "rb");
	bool need_premultiply = false;
	if (unlikely(!fp)) {
		printfef("(\"%s\"): Failed to open file.", path);
		goto spng_read_end;
	}
	if (unlikely(SPNG_SIGBYTES != fread(&sig, 1, SPNG_SIGBYTES, fp))) {
		printfef("(\"%s\"): Failed to read %d-byte signature.",
				path, SPNG_SIGBYTES);
		goto spng_read_end;
	}
	if (unlikely(png_sig_cmp((png_bytep) sig, 0, SPNG_SIGBYTES))) {
		printfef("(\"%s\"): PNG signature invalid.", path);
		goto spng_read_end;
	}
	png_ptr = allocchk(png_create_read_struct(PNG_LIBPNG_VER_STRING,
				NULL, NULL, NULL));
	info_ptr = allocchk(png_create_info_struct(png_ptr));
	if (setjmp(png_jmpbuf(png_ptr)))
		goto spng_read_end;
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, SPNG_SIGBYTES);
	png_read_info(png_ptr, info_ptr);
	png_uint_32 width = 0, height = 0;

	// Set transformations
	int bit_depth = 0, color_type = 0;
	{
		int interlace_type = 0;
		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
				&color_type, &interlace_type, NULL, NULL);

		// Scale or strip 16-bit colors
		if (bit_depth == 16) {
			printfdf("(\"%s\"): Scaling 16-bit colors.", path);
#if PNG_LIBPNG_VER >= 10504
			png_set_scale_16(png_ptr);
#else
			png_set_strip_16(png_ptr);
#endif
			bit_depth = 8;
		}

		/* if (bit_depth < 8)
			png_set_packing(png_ptr); */

		// No idea why this is needed...
		png_set_bgr(png_ptr);

		// Convert palette to RGB
		if (color_type == PNG_COLOR_TYPE_PALETTE) {
			printfdf("(\"%s\"): Converting palette PNG to RGB.", path);
			png_set_palette_to_rgb(png_ptr);
			color_type = PNG_COLOR_TYPE_RGB;
		}

		if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
			printfdf("(\"%s\"): Converting rDNS to full alpha.", path);
			png_set_tRNS_to_alpha(png_ptr);
		}

		if (color_type == PNG_COLOR_TYPE_GRAY
				|| PNG_COLOR_TYPE_GRAY_ALPHA == color_type) {
			printfdf("(\"%s\"): Converting gray (+ alpha) PNG to RGB.", path);
			png_set_gray_to_rgb(png_ptr);
			if (PNG_COLOR_TYPE_GRAY == color_type)
				color_type = PNG_COLOR_TYPE_RGB;
			else
				color_type = PNG_COLOR_TYPE_RGB_ALPHA;
		}

		/*
		if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
			printfdf("(\"%s\"): Converting 1/2/4 bit gray PNG to 8-bit.", path);
#if PNG_LIBPNG_VER >= 10209
			png_set_expand_gray_1_2_4_to_8(png_ptr);
#else
			png_set_gray_1_2_4_to_8(png_ptr);
#endif
			bit_depth = 8;
		}
		*/

		// Somehow XImage requires 24-bit visual to use 32 bits per pixel
		if (color_type == PNG_COLOR_TYPE_RGB) {
			printfdf("(\"%s\"): Appending filler alpha values.", path);
			png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
		}

		// Premultiply alpha
		if (PNG_COLOR_TYPE_RGB_ALPHA == color_type) {
#if PNG_LIBPNG_VER >= 10504
			png_set_alpha_mode(png_ptr, PNG_ALPHA_STANDARD, 1.0);
#else
			need_premultiply = true;
#endif
		}

		/*
		int number_passes = 1;
#ifdef PNG_READ_INTERLACING_SUPPORTED
		number_passes = png_set_interlace_handling(png_ptr);
#endif */

		if (PNG_INTERLACE_NONE != interlace_type)
			png_set_interlace_handling(png_ptr);
	}

	png_read_update_info(png_ptr, info_ptr);

	int depth = 0;
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
				&color_type, NULL, NULL, NULL);
	switch (color_type) {
		case PNG_COLOR_TYPE_GRAY:       depth = 1 * bit_depth; break;
		case PNG_COLOR_TYPE_RGB:        depth = 3 * bit_depth; break;
		case PNG_COLOR_TYPE_RGB_ALPHA:  depth = 4 * bit_depth; break;
		default:                        assert(0); break;
	}

	// Read data and fill to Picture
	{
		int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
		png_bytep row_pointers[height];
		memset(row_pointers, 0, sizeof(row_pointers));
		row_pointers[0] = png_malloc(png_ptr, rowbytes * height);
		for (int row = 1; row < height; row++)
			row_pointers[row] = row_pointers[row - 1] + rowbytes;
		png_read_image(png_ptr, row_pointers);
		if (need_premultiply)
			for (int row = 0; row < height; row++)
				simg_data32_premultiply(row_pointers[row], width);
		pictw = simg_data_to_pictw(ps, width, height, depth,
				row_pointers[0], rowbytes);
		png_free(png_ptr, row_pointers[0]);
		if (unlikely(!pictw)) {
			printfef("(\"%s\"): Failed to create Picture.", path);
			goto spng_read_end;
		}
	}

spng_read_end:
	if (png_ptr)
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	if (fp)
		fclose(fp);

	return pictw;
}
