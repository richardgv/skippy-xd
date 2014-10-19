/* Skippy - Seduces Kids Into Perversion
 *
 * Copyright (C) 2004 Hyriand <hyriand@thegraveyard.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @file skippy.h
 *
 * @brief Global header.
 */

#ifndef SKIPPY_H
#define SKIPPY_H

#define BUF_LEN 128

#define _GNU_SOURCE

#include <stdbool.h>
#include <strings.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>

#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>

#ifdef CFG_XINERAMA
# include <X11/extensions/Xinerama.h>
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

#include "dlist.h"

#define MAX_MOUSE_BUTTONS 4

/**
 * @brief Dump raw bytes in HEX format.
 *
 * @param data pointer to raw data
 * @param len length of data
 */
static inline void
hexdump(const char *data, int len) {
  static const int BYTE_PER_LN = 16;

  if (len <= 0)
    return;

  // Print header
  printf("%10s:", "Offset");
  for (int i = 0; i < BYTE_PER_LN; ++i)
    printf(" %2d", i);
  putchar('\n');

  // Dump content
  for (int offset = 0; offset < len; ++offset) {
    if (!(offset % BYTE_PER_LN))
      printf("0x%08x:", offset);

    printf(" %02hhx", data[offset]);

    if ((BYTE_PER_LN - 1) == offset % BYTE_PER_LN)
      putchar('\n');
  }
  if (len % BYTE_PER_LN)
    putchar('\n');

  fflush(stdout);
}

/// @brief Possible return values.
enum {
	RET_SUCCESS = 0,
	RET_UNKNOWN,
	RET_BADARG,
	RET_XFAIL,
	RET_BADALLOC,
};

enum progmode {
	PROGMODE_NORMAL,
	PROGMODE_ACTV_PICKER,
	PROGMODE_DEACTV_PICKER,
	PROGMODE_TOGGLE_PICKER,
	PROGMODE_DM_STOP,
};

enum cliop {
	CLIENTOP_NO,
	CLIENTOP_FOCUS,
	CLIENTOP_ICONIFY,
	CLIENTOP_SHADE_EWMH,
	CLIENTOP_CLOSE_ICCCM,
	CLIENTOP_CLOSE_EWMH,
	CLIENTOP_DESTROY,
};

enum align {
	ALIGN_LEFT,
	ALIGN_MID,
	ALIGN_RIGHT,
};

enum buttons {
	BUTN_CLOSE,
	BUTN_MINIMIZE,
	BUTN_SHADE,
	NUM_BUTN,
};

enum pict_posp_mode {
	PICTPOSP_ORIG,
	PICTPOSP_SCALE,
	PICTPOSP_SCALEK,
	PICTPOSP_SCALEE,
	PICTPOSP_SCALEEK,
	PICTPOSP_TILE,
};

typedef enum {
	WMPSN_NONE,
	WMPSN_EWMH,
	WMPSN_GNOME,
} wmpsn_t;

typedef struct {
	Pixmap pxmap;
	Picture pict;
	int height;
	int width;
	int depth;
} pictw_t;

typedef struct {
	char *path;
	pictw_t *img;
	enum pict_posp_mode mode;
	int twidth;
	int theight;
	enum align alg;
	enum align valg;
	XRenderColor c;
} pictspec_t;

#define PICTSPECT_INIT { \
	.path = NULL, \
}

typedef struct {
	unsigned int key;
	enum {
		KEYMOD_CTRL = 1 << 0,
		KEYMOD_SHIFT = 1 << 1,
		KEYMOD_META = 1 << 2,
	} mod;
} keydef_t;

typedef enum {
	CLIDISP_NONE,
	CLIDISP_FILLED,
	CLIDISP_ICON,
	CLIDISP_THUMBNAIL,
	CLIDISP_THUMBNAIL_ICON,
} client_disp_mode_t;

/// @brief Option structure.
typedef struct {
	char *config_path;
	enum progmode mode;
	bool runAsDaemon;
	bool synchronize;

	int distance;
	bool useNetWMFullscreen;
	bool ignoreSkipTaskbar;
	bool acceptOvRedir;
	bool acceptWMWin;
	double updateFreq;
	bool lazyTrans;
	bool useNameWindowPixmap;
	bool forceNameWindowPixmap;
	bool includeFrame;
	char *pipePath;
	bool movePointerOnStart;
	bool movePointerOnSelect;
	bool movePointerOnRaise;
	bool switchDesktopOnActivate;
	bool allowUpscale;
	bool includeAllScreens;
	bool avoidThumbnailsFromOtherScreens;
	bool showAllDesktops;
	bool showUnmapped;
	int preferredIconSize;
	client_disp_mode_t *clientDisplayModes;
	pictspec_t iconFillSpec;
	pictw_t *iconDefault;
	pictspec_t fillSpec;
	char *buttonImgs[NUM_BUTN];
	pictw_t *background;

	bool xinerama_showAll;

	char *normal_tint;
	int normal_tintOpacity;
	int normal_opacity;

	char *highlight_tint;
	int highlight_tintOpacity;
	int highlight_opacity;

	bool tooltip_show;
	bool tooltip_followsMouse;
	int tooltip_offsetX;
	int tooltip_offsetY;
	enum align tooltip_align;
	char *tooltip_border;
	char *tooltip_background;
	int tooltip_opacity;
	char *tooltip_text;
	char *tooltip_textShadow;
	char *tooltip_font;

	enum cliop bindings_miwMouse[MAX_MOUSE_BUTTONS];
} options_t;

#define OPTIONST_INIT { \
	.config_path = NULL, \
	.mode = PROGMODE_NORMAL, \
	.runAsDaemon = false, \
	.synchronize = false, \
\
	.distance = 50, \
	.useNetWMFullscreen = true, \
	.ignoreSkipTaskbar = false, \
	.acceptOvRedir = false, \
	.acceptWMWin = false, \
	.updateFreq = 10.0, \
	.lazyTrans = false, \
	.useNameWindowPixmap = false, \
	.forceNameWindowPixmap = false, \
	.includeFrame = false, \
	.pipePath = NULL, \
	.movePointerOnStart = true, \
	.movePointerOnSelect = true, \
	.movePointerOnRaise = true, \
	.switchDesktopOnActivate = false, \
	.allowUpscale = true, \
	.includeAllScreens = false, \
	.avoidThumbnailsFromOtherScreens = true, \
	.preferredIconSize = 48, \
	.clientDisplayModes = NULL, \
	.iconFillSpec = PICTSPECT_INIT, \
	.fillSpec = PICTSPECT_INIT, \
	.showAllDesktops = false, \
	.showUnmapped = true, \
	.buttonImgs = { NULL }, \
	.background = NULL, \
	.xinerama_showAll = true, \
	.normal_tint = NULL, \
	.normal_tintOpacity = 0, \
	.normal_opacity = 200, \
	.highlight_tint = NULL, \
	.highlight_tintOpacity = 64, \
	.highlight_opacity = 255, \
	.tooltip_show = true, \
	.tooltip_followsMouse = true, \
	.tooltip_offsetX = 20, \
	.tooltip_offsetY = 20, \
	.tooltip_align = ALIGN_LEFT, \
	.tooltip_border = NULL, \
	.tooltip_background = NULL, \
	.tooltip_opacity = 128, \
	.tooltip_text = NULL, \
	.tooltip_textShadow = NULL, \
	.tooltip_font = NULL, \
}

/// @brief X information structure.
typedef struct {
	int damage_ev_base;
	int damage_err_base;
	int composite_ev_base;
	int composite_err_base;
	int render_ev_base;
	int render_err_base;
	int fixes_ev_base;
	int fixes_err_base;

	bool xinerama_exist;
	int xinerama_err_base;
	int xinerama_ev_base;
} xinfo_t;

#define XINFOT_INIT { \
	.xinerama_exist = false, \
}

typedef struct _clientwin_t ClientWin;
typedef struct _mainwin_t MainWin;

/// @brief Session global info structure.
typedef struct {
	/// @brief Program options.
	options_t o;
	/// @brief X display.
	Display *dpy;
	/// @brief Current screen.
	int screen;
	/// @brief Root window ID.
	Window root;
	/// @brief Information about X.
	xinfo_t xinfo;
	/// @brief Time the program was started, in milliseconds.
	struct timeval time_start;
	/// @brief WM personality.
	wmpsn_t wmpsn;
	/// @brief Whether we have EWMH fullscreen support.
	bool has_ewmh_fullscreen;
	/// @brief ARGB visual of the current screen.
	Visual *argb_visual;
	/// @brief File descriptor of command pipe, in daemon mode.
	int fd_pipe;
	/// @brief Main window.
	MainWin *mainwin;
} session_t;

#define SESSIONT_INIT { \
	.o = OPTIONST_INIT, \
	.xinfo = XINFOT_INIT, \
	.time_start = { .tv_sec = 0, .tv_usec = 0 }, \
	.fd_pipe = -1, \
}

/// @brief Print out a debug message.
#define printfd(format, ...) \
  (fprintf(stdout, format "\n", ## __VA_ARGS__), fflush(stdout))

/// @brief Print out a debug message with function name.
#define printfdf(format, ...) \
  (fprintf(stdout, "%s" format "\n", __func__, ## __VA_ARGS__), fflush(stdout))

/// @brief Print out an error message.
#define printfe(format, ...) \
  (fprintf(stderr, format "\n", ## __VA_ARGS__), fflush(stderr))

/// @brief Print out an error message with function name.
#define printfef(format, ...) \
  printfe("%s" format, __func__, ## __VA_ARGS__)

/// @brief Return a value if it's true.
#define retif(x) do { if (x) return (x); } while (0)

/// @brief Wrapper for gcc branch prediction builtin, for likely branch.
#define likely(x)    __builtin_expect(!!(x), 1)
/// @brief Wrapper for gcc branch prediction builtin, for unlikely branch.
#define unlikely(x)  __builtin_expect(!!(x), 0)

/**
 * @brief Quit with RETV_BADALLOC if the passed-in pointer is empty.
 */
static inline void *
allocchk_(void *ptr, const char *func_name) {
  if (unlikely(!ptr)) {
    printfe("%s(): Failed to allocate memory.", func_name);
	exit(RET_BADALLOC);
  }

  return ptr;
}

/// @brief Wrapper of allocchk_().
#define allocchk(ptr) allocchk_(ptr, __func__)

/// @brief Wrapper of malloc().
#define smalloc(nmemb, type) ((type *) allocchk(malloc((nmemb) * sizeof(type))))

/// @brief Wrapper of calloc().
#define scalloc(nmemb, type) ((type *) allocchk(calloc((nmemb), sizeof(type))))

/// @brief Wrapper of ralloc().
#define srealloc(ptr, nmemb, type) ((type *) allocchk(realloc((ptr), (nmemb) * sizeof(type))))

/// @brief Return the case string.
/// Use #s here to prevent macro expansion
#define CASESTRRET(s)   case s: return #s

/// @brief Return number of elements in a constant array.
#define CARR_LEN(a) sizeof(a) / sizeof(a[0])

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) > (b)) ? (b) : (a))

#define foreach_dlist_vn(itervar, l) \
	for (dlist *(itervar) = dlist_first(l); (itervar); (itervar) = (itervar)->next)
#define foreach_dlist(l) foreach_dlist_vn(iter, l)
#define REDUCE(statement, l) \
	do { \
		foreach_dlist(l) \
			statement; \
	} while (0)

/**
 * @brief Get current time, in milliseconds.
 */
static inline long
time_in_millis(void) {
	struct timeval tp;

	gettimeofday(&tp, NULL);

	return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

/**
 * @brief Subtracting two struct timeval values.
 *
 * Taken from glibc manual.
 *
 * Subtract the `struct timeval' values X and Y,
 * storing the result in RESULT.
 * Return 1 if the difference is negative, otherwise 0.
 */
static inline int
timeval_subtract(struct timeval *result,
		struct timeval *x, struct timeval *y) {
	// Perform the carry for the later subtraction by updating y
	if (x->tv_usec < y->tv_usec) {
		long nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}

	if (x->tv_usec - y->tv_usec > 1000000) {
		long nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	// Compute the time remaining to wait.
	// tv_usec is certainly positive.
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	// Return 1 if result is negative.
	return x->tv_sec < y->tv_sec;
}

/**
 * @brief Print time passed since program starts execution.
 */
static inline void
print_timestamp(session_t *ps) {
	struct timeval tm, diff;

	if (gettimeofday(&tm, NULL)) return;

	timeval_subtract(&diff, &tm, &ps->time_start);

	printf("[ %5ld.%02ld ] ", diff.tv_sec, diff.tv_usec / 10000);
}

/**
 * @brief Allocate the space and copy some data.
 */
static inline unsigned char *
mmemcpy(const unsigned char *data, int len) {
	unsigned char *d = smalloc(len, unsigned char);
	memcpy(d, data, len);
	return d;
}

/**
 * @brief Allocate the space and join two strings.
 */
static inline char *
mstrjoin(const char *src1, const char *src2) {
  char *str = allocchk(calloc((strlen(src1) + strlen(src2) + 1), sizeof(char)));

  strcpy(str, src1);
  strcat(str, src2);

  return str;
}

/**
 * @brief Allocate the space and join two strings;
 */
static inline char *
mstrjoin3(const char *src1, const char *src2, const char *src3) {
  char *str = allocchk(calloc((strlen(src1) + strlen(src2)
        + strlen(src3) + 1), sizeof(char)));

  strcpy(str, src1);
  strcat(str, src2);
  strcat(str, src3);

  return str;
}

/**
 * @brief Wrapper of strdup().
 */
static inline char *
mstrdup(const char *src) {
	return allocchk(strdup(src));
}

/**
 * @brief Copy a number of characters to a newly allocated string.
 */
static inline char *
mstrncpy(const char *src, unsigned len) {
	char *str = allocchk(malloc(sizeof(char) * (len + 1)));
	strncpy(str, src, len);
	str[len] = '\0';

	return str;
}

/**
 * @brief Copy and place a string to somewhere.
 */
static inline void
strplace(char **dst, const char *src) {
	free(*dst);
	*dst = mstrdup(src);
}

/**
 * @brief Check if a character symbolizes end of a word.
 */
static inline bool
isspace0(char c) {
	return !c || isspace(c);
}

/**
 * @brief Check if a string ends with something, ignore case.
 */
static inline bool
str_endwith(const char *haystick, const char *needle) {
	int haystick_len = strlen(haystick);
	int needle_len = strlen(needle);
	return haystick_len >= needle_len
		&& !strcasecmp(haystick + haystick_len - needle_len, needle);
}

/**
 * @brief Check if a string starts with some words.
 */
static inline bool
str_startswithword(const char *haystick, const char *needle) {
	const int needle_len = strlen(needle);
	return !strncmp(haystick, needle, needle_len)
		&& isspace0(haystick[needle_len]);
}

/**
 * @brief Check if a string starts with some words, ignore case.
 */
static inline bool
str_startswithwordi(const char *haystick, const char *needle) {
	const int needle_len = strlen(needle);
	return !strncasecmp(haystick, needle, needle_len)
		&& isspace0(haystick[needle_len]);
}

/**
 * @brief Get first word.
 *
 * @param dest place to store pointer to a copy of the first word
 * @return start of next word
 */
static inline const char *
str_get_word(const char *s, char **dest) {
	*dest = NULL;
	int i = 0;
	while (isspace(s[i])) ++i;
	int start = i;
	while (!isspace0(s[i])) ++i;
	if (i - start)
		*dest = mstrncpy(s + start, i - start);
	while (isspace(s[i])) ++i;
	if (!s[i]) return NULL;
	return &s[i];
}

/**
 * @brief Destroy a <code>Pixmap</code>.
 */
static inline void
free_pixmap(session_t *ps, Pixmap *p) {
	if (*p) {
		XFreePixmap(ps->dpy, *p);
		*p = None;
	}
}

/**
 * @brief Destroy a <code>Picture</code>.
 */
static inline void
free_picture(session_t *ps, Picture *p) {
	if (*p) {
		XRenderFreePicture(ps->dpy, *p);
		*p = None;
	}
}

/**
 * @brief Destroy a <code>Damage</code>.
 */
static inline void
free_damage(session_t *ps, Damage *p) {
	if (*p) {
		// BadDamage will be thrown if the window is destroyed
		XDamageDestroy(ps->dpy, *p);
		*p = None;
	}
}

/**
 * @brief Destroy a <code>XserverRegion</code>.
 */
static inline void
free_region(session_t *ps, XserverRegion *p) {
	if (*p) {
		XFixesDestroyRegion(ps->dpy, *p);
		*p = None;
	}
}

static inline unsigned short
alphaconv(int alpha) {
	return MIN(alpha * 256, 65535);
}

/**
 * @brief Wrapper of XFree() for convenience.
 *
 * Because a NULL pointer cannot be passed to XFree(), its man page says.
 */
static inline void
sxfree(void *data) {
  if (data)
    XFree(data);
}

/**
 * @brief Wrapper to sxfree() to turn the pointer NULL as well.
 */
static inline void
spxfree(void *data) {
	sxfree(*(void **) data);
	*(void **) data = NULL;
}

/**
 * @brief Checks if a key event matches particular key and modifier
 * combination.
 */
static inline bool
ev_iskey(XKeyEvent *ev, const keydef_t *k) {
	unsigned int mask = 0;
	if (KEYMOD_CTRL & k->mod) mask |= ControlMask;
	if (KEYMOD_SHIFT & k->mod) mask |= ShiftMask;
	if (KEYMOD_META & k->mod) mask |= Mod1Mask;
	return k->key == ev->keycode
		&& (ev->state & (ControlMask | ShiftMask | Mod1Mask)) == mask;
}

/**
 * @brief Return a string representation for the keycode in a KeyEvent.
 */
static inline const char *
ev_key_str(XKeyEvent *ev) {
	return XKeysymToString(XLookupKeysym(ev, 0));
}

#define report_key_ignored(ev) \
	printfef("(): KeyRelease %u (%s) ignored.", (ev)->xkey.keycode, \
			ev_key_str(&(ev)->xkey))

#define report_key_unbinded(ev) \
	printfef("(): KeyRelease %u (%s) not binded to anything.", \
			(ev)->xkey.keycode, ev_key_str(&(ev)->xkey))

#include "img.h"
#include "wm.h"
#include "mainwin.h"
#include "clientwin.h"
#include "layout.h"
#include "focus.h"
#include "config.h"
#include "tooltip.h"
#include "img-xlib.h"
#ifdef CFG_LIBPNG
// FreeType uses setjmp.h and libpng-1.2 feels crazy about this...
#define PNG_SKIP_SETJMP_CHECK 1
#include "img-png.h"
#endif
#ifdef CFG_JPEG
#include "img-jpeg.h"
#endif
#ifdef CFG_GIFLIB
#include "img-gif.h"
#endif

extern session_t *ps_g;

#endif /* SKIPPY_H */
