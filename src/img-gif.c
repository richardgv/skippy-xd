#include "skippy.h"
#include <gif_lib.h>

// Global error flag on >=giflib-4.2 before giflib-5.0
#if defined(GIFLIB_MAJOR) && GIFLIB_MAJOR == 4 && defined(GIFLIB_MINOR) && GIFLIB_MINOR >= 2
#define SGIF_HAS_ERROR
#endif

// Thread-safe error flag on >=giflib-5.0
#if defined(GIFLIB_MAJOR) && GIFLIB_MAJOR >= 5
#define SGIF_THREADSAFE
#endif

// More thread-safe error flag on >=giflib-5.1
#if defined(GIFLIB_MAJOR) && GIFLIB_MAJOR >= 5 && defined(GIFLIB_MINOR) && GIFLIB_MINOR >= 1
#define SGIF_THREADSAFE_510
#endif

pictw_t *
sgif_read(session_t *ps, const char *path) {
	assert(path);
	pictw_t *pictw = NULL;
	GifPixelType *data = NULL;
	unsigned char *tdata = NULL;

	GifRecordType rectype;
	int ret = 0, err = 0;
	const char *errstr = NULL;
#ifdef SGIF_THREADSAFE
	GifFileType *f = DGifOpenFileName(path, &err);
#else
	GifFileType *f = DGifOpenFileName(path);
#endif
	if (unlikely(!f)) {
#ifdef SGIF_HAS_ERROR
		err = GifError();
		errstr = GifErrorString();
#endif
#ifdef SGIF_THREADSAFE
		errstr = GifErrorString(err);
#endif
		printfef("(\"%s\"): Failed to open file: %d (%s)", path, err, errstr);
		goto sgif_read_end;
	}

	int width = 0, height = 0, transp = -1;
	{
		int i = 0;
		while (GIF_OK == (ret = DGifGetRecordType(f, &rectype))
				&& TERMINATE_RECORD_TYPE != rectype) {
			++i;
			switch (rectype) {
				case UNDEFINED_RECORD_TYPE:
					printfef("(\"%s\"): %d: Encountered a record of unknown type.",
							path, i);
					break;
				case SCREEN_DESC_RECORD_TYPE:
					printfef("(\"%s\"): %d: Encountered a record of "
							"ScreenDescRecordType. This shouldn't happen!",
							path, i);
					break;
				case IMAGE_DESC_RECORD_TYPE:
					if (data) {
						printfef("(\"%s\"): %d: Extra image section ignored.",
								path, i);
						break;
					}
					if (GIF_ERROR == DGifGetImageDesc(f)) {
						printfef("(\"%s\"): %d: Failed to read GIF image info.",
								path, i);
						break;
					}
					width = f->Image.Width;
					height = f->Image.Height;
					if (width <= 0 || height <= 0) {
						printfef("(\"%s\"): %d: Width/height invalid.", path, i);
						break;
					}
					assert(!data);
					data = allocchk(malloc(width * height * sizeof(GifPixelType)));
					// FIXME: Interlace images may need special treatments
					for (int j = 0; j < height; ++j)
						if (GIF_OK != DGifGetLine(f, &data[j * width], width)) {
							printfef("(\"%s\"): %d: Failed to read line %d.", path, i, j);
							goto sgif_read_end;
						}
					break;
				case EXTENSION_RECORD_TYPE:
					{
						int code = 0;
						GifByteType *pbytes = NULL;
						if (GIF_OK != DGifGetExtension(f, &code, &pbytes) || !pbytes) {
							printfef("(\"%s\"): %d: Failed to read extension block.",
									path, i);
							break;
						}
						do {
							// Transparency
							if (0xf9 == code && (pbytes[1] & 1))
								transp = pbytes[4];
						} while (GIF_OK == DGifGetExtensionNext(f, &pbytes) && pbytes);
					}
					break;
				case TERMINATE_RECORD_TYPE:
					assert(0);
					break;
			}
		}
		if (unlikely(!data)) {
			printfef("(\"%s\"): No valid data found.", path);
			goto sgif_read_end;
		}
	}

	// Colormap translation
	int depth = 32;
	{
		ColorMapObject *cmap = f->Image.ColorMap;
		if (!cmap) cmap = f->SColorMap;
		if (unlikely(!cmap)) {
			printfef("(\"%s\"): No colormap found.", path);
			goto sgif_read_end;
		}
		tdata = allocchk(malloc(width * height * depth / 8));
		{
			GifPixelType *pd = data;
			unsigned char *end = tdata + width * height * depth / 8;
			for (unsigned char *p = tdata; p < end; p += depth / 8, ++pd) {
				// When the alpha is 0, X seemingly wants all color channels
				// to be 0 as well.
				if (transp >= 0 && transp == *pd) {
					p[0] = p[1] = p[2] = 0;
					if (32 == depth) p[3] = 0;
					continue;
				}
				p[0] = cmap->Colors[*pd].Blue;
				p[1] = cmap->Colors[*pd].Green;
				p[2] = cmap->Colors[*pd].Red;
				p[3] = 0xff;
			}
		}
	}
	pictw = simg_data_to_pictw(ps, width, height, depth, tdata, 0);
	free(tdata);
	if (unlikely(!pictw)) {
		printfef("(\"%s\"): Failed to create Picture.", path);
		goto sgif_read_end;
	}

sgif_read_end:
	if (data)
		free(data);
	if (likely(f)) {
#ifdef SGIF_THREADSAFE_510
		int error_code = 0;
		DGifCloseFile(f, &error_code);
#else
		DGifCloseFile(f);
#endif
	}

	return pictw;
}
