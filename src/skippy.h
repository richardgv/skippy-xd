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
#include <regex.h>
#include <string.h>

#include "dlist.h"

#define MAX_MOUSE_BUTTONS 4

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
	PROGMODE_DM_STOP,
};

enum cliop {
	CLIENTOP_NO,
	CLIENTOP_FOCUS,
	CLIENTOP_ICONIFY,
	CLIENTOP_SHADE_EWMH,
	CLIENTOP_CLOSE_ICCWM,
	CLIENTOP_CLOSE_EWMH,
	CLIENTOP_DESTROY,
};

enum align {
	ALIGN_LEFT,
	ALIGN_MID,
	ALIGN_RIGHT,
};

/// @brief Option structure.
typedef struct {
	char *config_path;
	enum progmode mode;
	bool runAsDaemon;
	bool synchronize;

	int distance;
	bool useNetWMFullscreen;
	bool ignoreSkipTaskbar;
	double updateFreq;
	bool lazyTrans;
	bool useNameWindowPixmap;
	char *pipePath;
	bool movePointerOnStart;

	bool xinerama_showAll;
	bool layout_desktop;
	bool layout_grid;
	bool layout_smart;

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
	.distance = 8, \
	.useNetWMFullscreen = true, \
	.ignoreSkipTaskbar = false, \
	.updateFreq = 10.0, \
	.lazyTrans = false, \
	.useNameWindowPixmap = false, \
	.pipePath = NULL, \
	.movePointerOnStart = true, \
	.xinerama_showAll = false, \
	.normal_tint = NULL, \
	.normal_tintOpacity = 0, \
	.normal_opacity = 248, \
	.highlight_tint = NULL, \
	.highlight_tintOpacity = 32, \
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
} session_t;

#define SESSIONT_INIT { \
	.o = OPTIONST_INIT, \
	.xinfo = XINFOT_INIT, \
	.time_start = { .tv_sec = 0, .tv_usec = 0 }, \
}

/// @brief Print out a debug message.
#define printfd(format, ...) \
  (fprintf(stdout, format "\n", ## __VA_ARGS__), fflush(stdout))

/// @brief Print out an error message.
#define printfe(format, ...) \
  (fprintf(stderr, format "\n", ## __VA_ARGS__), fflush(stderr))

/// @brief Print out an error message with function name.
#define printfef(format, ...) \
  printfe("%s" format,  __func__, ## __VA_ARGS__)

/**
 * @brief Quit with RETV_BADALLOC if the passed-in pointer is empty.
 */
static inline void *
allocchk_(void *ptr, const char *func_name) {
  if (!ptr) {
    printfe("%s(): Failed to allocate memory.", func_name);
	exit(RET_BADALLOC);
  }

  return ptr;
}

/// @brief Wrapper of allocchk_().
#define allocchk(ptr) allocchk_(ptr, __func__)

/// @brief Return the case string.
/// Use #s here to prevent macro expansion
#define CASESTRRET(s)   case s: return #s

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) > (b)) ? (b) : (a))
#define REDUCE(statement, l) \
{ \
	dlist *iter = dlist_first(l); \
	for(; iter; iter = iter->next) \
		statement; \
}

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
 * @brief Copy and place a string to somewhere.
 */
static inline void
strplace(char **dst, const char *src) {
	free(*dst);
	*dst = mstrdup(src);
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

#include "vecmath2d.h"
#include "wm.h"
#include "clientwin.h"
#include "mainwin.h"
#include "layout.h"
#include "focus.h"
#include "config.h"
#include "tooltip.h"

extern session_t *ps_g;

#define logd //

#define ACTIVATE_WINDOW_PICKER 1
#define EXIT_RUNNING_DAEMON 2

#endif /* SKIPPY_H */
