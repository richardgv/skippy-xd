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

extern bool debuglog;

/// @brief Possible return values.

enum {
	LAYOUT_XD,
	LAYOUT_BOXY
};

enum {
	RET_SUCCESS = 0,
	RET_UNKNOWN,
	RET_BADARG,
	RET_XFAIL,
	RET_BADALLOC,
};

enum progmode {
	PROGMODE_NORMAL,
	PROGMODE_ACTV_EXPOSE,
	PROGMODE_TGG_EXPOSE,
	PROGMODE_ACTV_PAGING,
	PROGMODE_TGG_PAGING,
	PROGMODE_DEACTV,
	PROGMODE_RELOAD_CONFIG,
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
	CLIDISP_ZOMBIE,
	CLIDISP_ZOMBIE_ICON,
	CLIDISP_THUMBNAIL,
	CLIDISP_THUMBNAIL_ICON,
} client_disp_mode_t;

typedef enum {
	FI_PREV = -1,
	FI_CURR =  0,
	FI_NEXT = +1,
} focus_initial;

/// @brief Option structure.
typedef struct {
	char *config_path;
	enum progmode mode;
	bool runAsDaemon;
	int focus_initial;

	int layout;
	bool sortByColumn;
	int distance;
	bool useNetWMFullscreen;
	bool ignoreSkipTaskbar;
	bool acceptOvRedir;
	bool acceptWMWin;
	double updateFreq;
	int animationDuration;;
	bool lazyTrans;
	bool includeFrame;
	char *pipePath;
	bool movePointerOnStart;
	bool movePointerOnSelect;
	bool movePointerOnRaise;
	bool switchDesktopOnActivate;
	bool allowUpscale;
	bool showAllDesktops;
	int cornerRadius;
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

	char *shadow_tint;
	int shadow_tintOpacity;
	int shadow_opacity;

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
	char *bindings_keysUp;
	char *bindings_keysDown;
	char *bindings_keysLeft;
	char *bindings_keysRight;
	char *bindings_keysPrev;
	char *bindings_keysNext;
	char *bindings_keysExitCancelOnPress;
	char *bindings_keysExitCancelOnRelease;
	char *bindings_keysExitSelectOnPress;
	char *bindings_keysExitSelectOnRelease;
	char *bindings_keysReverseDirection;
	char *bindings_modifierKeyMasksReverseDirection;
} options_t;

#define OPTIONST_INIT { \
	.config_path = NULL, \
	.mode = PROGMODE_NORMAL, \
	.runAsDaemon = false, \
\
	.layout = LAYOUT_XD, \
	.sortByColumn = true, \
	.distance = 50, \
	.useNetWMFullscreen = true, \
	.ignoreSkipTaskbar = false, \
	.acceptOvRedir = false, \
	.acceptWMWin = false, \
	.updateFreq = 60.0, \
	.animationDuration = 200, \
	.lazyTrans = false, \
	.includeFrame = false, \
	.pipePath = NULL, \
	.movePointerOnStart = true, \
	.movePointerOnSelect = true, \
	.movePointerOnRaise = true, \
	.switchDesktopOnActivate = false, \
	.allowUpscale = true, \
	.cornerRadius = 0, \
	.preferredIconSize = 48, \
	.clientDisplayModes = NULL, \
	.iconFillSpec = PICTSPECT_INIT, \
	.fillSpec = PICTSPECT_INIT, \
	.showAllDesktops = false, \
	.buttonImgs = { NULL }, \
	.background = NULL, \
	.xinerama_showAll = true, \
	.normal_tint = NULL, \
	.normal_tintOpacity = 0, \
	.normal_opacity = 200, \
	.highlight_tint = NULL, \
	.highlight_tintOpacity = 64, \
	.highlight_opacity = 255, \
	.shadow_tint = NULL, \
	.shadow_tintOpacity = 164, \
	.shadow_opacity = 200, \
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

/// @brief Print out a debug message with function name.
#define printfdf(alwaysprint, format, ...) \
  ((alwaysprint) || (debuglog))? fprintf(stdout, "%s" format "\n", __func__, ## __VA_ARGS__):true;

/// @brief Print out an error message with function name.
#define printfef(alwaysprint, format, ...) \
  ((alwaysprint) || (debuglog))? fprintf(stderr, "%s" format "\n", __func__, ## __VA_ARGS__):true;

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
    printfef(true, "%s(): Failed to allocate memory.", func_name);
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

	printfef(false, "() [ %5ld.%02ld ] ", diff.tv_sec, diff.tv_usec / 10000);
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
 * @brief Convert a string into an integer.
 */
static inline int
str_to_int(const char *s)
{
	 return (int)strtol(s,NULL,0);
}

/**
 * @brief Count the number of words in a string.
 */
static inline int
str_count_words(const char *s)
{
  if (!s) return 0;

  int count = 0;
  while (*s != '\0')
  {
    while (*s != '\0' && isspace(*s)) // remove all spaces between words
      s++;

    if(*s != '\0')
      count++;

    while (*s != '\0' && !isspace(*s)) // loop past the found word
      s++;
  }
  return count;
}

/**
 * @brief Get first word.
 *
 * @param dest place to store pointer to a copy of the word
 * @return start of next word
 */
static inline const char *
str_get_word(const char *s, char **dest) {
	*dest = NULL;
	if (!s) return NULL;
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
 * @brief Get first alphanumeric word [a-zA-Z0-9] only.
 * everything else is treated as a word boundary (space)
 *
 * @param dest place to store pointer to a copy of the word
 * @return start of next word
 */
static inline const char *
str_get_word_alnum(const char *s, char **dest) {
	*dest = NULL;
	if (!s) return NULL;

	// advance until we hit an alnum
	int i = 0;
	while (s[i] && !isalnum(s[i])) ++i;

	// set start of word
	int start = i;

	// advance until we no longer are alnum
	while (s[i] && isalnum(s[i])) ++i;

	// if we advanced any chars
	if (i - start)
		// then copy into a new null terminated string
		*dest = mstrncpy(s + start, i - start);

	// keep advancing to the next word, while we are no longer alnum
	while (s[i] && !isalnum(s[i])) ++i;

	// return NULL if the next char is null
	if (!s[i]) return NULL;

	// otherwise return the pointer position, for the beginning of the next word
	return &s[i];
}

/**
 * @brief Get first alphanumeric word [a-zA-Z0-9_] including underscores.
 * everything else is treated as a word boundary (space)
 * This function INDLUDES _ underscores as being part of the words
 * i.e. it does not break, when encountering underscores
 * Otherwise the same as str_get_word_alnum() function (above)
 *
 * @param dest place to store pointer to a copy of the word
 * @return start of next word
 */
static inline const char *
str_get_word_alnum_(const char *s, char **dest) {
	*dest = NULL;
	if (!s) return NULL;

	// advance until we hit an alnum
	int i = 0;
	while (s[i] && !isalnum(s[i]) && s[i] != '_') ++i;

	// set start of word
	int start = i;

	// advance until we no longer are alnum
	while (s[i] && (isalnum(s[i]) || s[i] == '_')) ++i;

	// if we advanced any chars
	if (i - start)
		// then copy into a new null terminated string
		*dest = mstrncpy(s + start, i - start);

	// keep advancing to the next word, while we are no longer alnum
	while (s[i] && !isalnum(s[i]) && s[i] != '_') ++i;

	// return NULL if the next char is null
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

/**
 * @brief Convert a string into a KeySym.
 */
static inline KeySym
key_str_sym(char *str) {
	return XStringToKeysym(str);
}

/**
 * @brief Print an error message for each word in the string that is not recognized as a valid keysym
 *
 * @param str_err_prefix1 filename of the user's config file, where the setting is written
 * @param str_err_prefix2 [section] of the rc config file where the setting is (ini format)
 * @param str_syms a list of words, each word is suppsed to represent a valid KeySym
 */
static inline void
check_keysyms(const char *str_err_prefix1, const char *str_err_prefix2, const char *str_syms)
{
  if (!str_syms) return;
  int count = str_count_words(str_syms);

  const char* next = str_syms;
  for (int i = 0; i < count; i++)
  {
		char *word = NULL;
		next = str_get_word(next, &word);

		if (!word)
			break;

		KeySym keysym = key_str_sym(word);

		if (!keysym)
			printfef(true, "(): %s%s \"%s\" was not recognized as a valid KeySym. Run the program 'xev' to find the correct value.", str_err_prefix1, str_err_prefix2, word);

		free(word);
  }
}

/**
 * @brief convert a string of words into an array of KeySyms
 * if a word in the string is not recognized as a valid KeySym,
 * then skip over it. Finally write 0x00 to the last array entry (+1)
 * which denotes the end of the array of KeySyms
 *
 * @param s the string of words to parse / split / convert
 * @param dest place to store pointer to the new array of KeySyms
 * @return size of the array
 */
static inline int
keys_str_syms(const char *s, KeySym **dest)
{
  *dest = NULL;
  if (!s) return 0;

  int num_words = str_count_words(s);
  if(num_words == 0) return 0;

  KeySym *arr_keysyms = (KeySym *)malloc(sizeof(KeySym)*(num_words+1));
  memset(arr_keysyms, 0, sizeof(KeySym)*(num_words+1));

  int count = 0;
  for (int i = 0; i < num_words; i++)
  {
	char *word = NULL;

	s = str_get_word(s, &word);
	// s = str_get_word_alnum_(s, &word);

	if (!word)
	  break;

	KeySym keysym = key_str_sym(word);

	// only increment the array counter if the keysym is valid
	if(keysym)
	{
	  arr_keysyms[count] = keysym;
	  count++;
	}
	free(word);

	if (!s)
	  break;
  }

  *dest = arr_keysyms;
  return count;
}

/**
 * @brief Count the number of KeySyms, in an array of KeySyms
 */
static inline int
arr_keysyms_size(KeySym *arr_keysyms)
{
  if (!arr_keysyms) return 0;

  int count = 0;
  while(arr_keysyms[count] != 0x00)
    count++;

  return count;
}

/**
 * @brief convert an array of KeySyms into an array of KeyCodes
 * if a KeySym in the array is not recognized as a valid KeyCode,
 * then skip over it. Finally write 0x00 to the last array entry (+1)
 * which denotes the end of the array of KeyCodes
 *
 * @param keysyms the input array of KeySyms
 * @param dest place to store pointer to the new array of KeyCodes
 * @return size of the new array
 */
static inline int
keysyms_arr_keycodes(Display *display, KeySym *keysyms, KeyCode **dest)
{
  // fputs("keysyms_arr_keycodes(Display *display, KeySym *keysyms, KeyCode **dest)\n", stdout); fflush(stdout);
  *dest = NULL;
  if (!display) return 0;
  if (!keysyms) return 0;

  int num_keysyms = arr_keysyms_size(keysyms);
  if (num_keysyms == 0) return 0;

  KeyCode *arr_keycodes = (KeyCode *)malloc(sizeof(KeyCode)*(num_keysyms+1));
  memset(arr_keycodes, 0, sizeof(KeyCode)*(num_keysyms+1));

  int count = 0;
  for (int i = 0; i < num_keysyms; i++)
  {
    arr_keycodes[i] = XKeysymToKeycode(display, keysyms[i]);
    printfdf(false, "(): i=%i, keysym=%lu, keycode=(0x%02i)", i, keysyms[i], arr_keycodes[i]);
    count++;
  }
  // fputs("\n", stdout); fflush(stdout);

  *dest = arr_keycodes;
  return count;
}

/**
 * @brief Count the number of KeyCodes, in an array of KeyCodes
 */
static inline int
arr_keycodes_size(KeyCode *arr_keycodes)
{
  int count = 0;
  if (!arr_keycodes) return 0;

  while (arr_keycodes[count] != 0x00)
    count++;

  return count;
}

/**
 * @brief Check to see if there is a specific KeyCode in an array of KeyCodes
 */
static inline bool
arr_keycodes_includes(KeyCode *arr_keycodes, KeyCode keycode)
{
  if (!arr_keycodes) return false;
  if (!keycode) return false;

  int count = 0;
  while (arr_keycodes[count] != 0x00)
  {
    if (arr_keycodes[count] == keycode)
      return true;
    count++;
  }
  return false;
}

/**
 * @brief take 2 arrays of KeySyms, and generate a new array
 * containing only the KeySyms that exist in both arrays
 * Finally write 0x00 to the last array entry (+1)
 * which denotes the end of the array of KeyCodes
 *
 * @param arr1 the 1st input array of KeySyms
 * @param arr2 the 2nd input array of KeySyms
 * @param dest place to store pointer to the new array of KeyCodes
 * @return size of the new array
 */
static inline int
keysyms_arr_intersect(KeySym *arr1, KeySym *arr2, KeySym **dest)
{
  *dest = NULL;
  if (!arr1) return 0;
  if (!arr2) return 0;

  int arr1_size = arr_keysyms_size(arr1);
  int arr2_size = arr_keysyms_size(arr2);

  int dest_size = MIN(arr1_size, arr2_size) + 1;
  if (dest_size == 1) return 0;

  KeySym *intersect = (KeySym *)malloc(sizeof(KeySym)*dest_size);
  memset(intersect, 0, sizeof(KeySym)*dest_size);

  int count = 0;
  for (int i = 0; i < arr1_size; i++)
  {
    for (int j = 0; j < arr2_size; j++)
    {
      if (arr1[i] == arr2[j])
      {
        KeySym keysym = arr1[i];
        intersect[count] = keysym;
        count++;
      }
    }
  }
  *dest = intersect;
  return count;
}

/**
 * @brief Print an error message each time the same KeySym is found in 2 different keybindings settings
 *
 * @param config_path filename of the user's config file, where the setting is written
 * @param arr1_str the name of the 1st config setting
 * @param arr1 the 1st input array of KeySyms
 * @param arr2 the 2nd input array of KeySyms
 * @param arr2_str name of the 2nd config setting
 * @return true if any keybindings conflicts were found
 */
static inline bool
check_keybindings_conflict(const char *config_path, const char *arr1_str, KeySym *arr1, const char *arr2_str, KeySym *arr2)
{
  if (!arr1) return false;
  if (!arr2) return false;

  KeySym *conflicts = NULL;
  int num_conflicts = keysyms_arr_intersect(arr1, arr2, &conflicts);

  if (num_conflicts)
  {
    for (int i = 0; i < num_conflicts; i++)
    {
      KeySym keysym = conflicts[i];
      printfdf(true, "(): %s: [bindings] Conflict detected. Remove the duplicate setting. Keybinding: '%s' found in both '%s' and '%s'.", config_path, XKeysymToString(keysym), arr1_str, arr2_str);
      printfdf(true, "%s   (0x%02lx)\n", XKeysymToString(keysym), keysym);
    }
    free(conflicts);
    return true;
  }
  free(conflicts);
  return false;
}

#define report_key(ev) \
	printfdf(false, "(): Key %u (%s) occured.", (ev)->xkey.keycode, \
			ev_key_str(&(ev)->xkey))

#define report_key_ignored(ev) \
	printfdf(false, "(): KeyRelease %u (%s) ignored.", (ev)->xkey.keycode, \
			ev_key_str(&(ev)->xkey))

#define report_key_unbinded(ev) \
	printfdf(false, "(): KeyRelease %u (%s) not binded to anything.", \
			(ev)->xkey.keycode, ev_key_str(&(ev)->xkey))


/**
 * @brief Checks if a key event matches particular key and modifier combination.
 */
static inline bool
ev_key_modifier(XKeyEvent *ev, int key_modifier)
{
	return ev->state & key_modifier;
}

/**
 * @brief A static lookup table of enums for X modifier key masks declared in /usr/include/X11/X.h
 */
static const int ev_modifier_mask[] = \
{
	ShiftMask,
	LockMask,
	ControlMask,
	Mod1Mask,
	Mod2Mask,
	Mod3Mask,
	Mod4Mask,
	Mod5Mask,
	Button1Mask,
	Button2Mask,
	Button3Mask,
	Button4Mask,
	Button5Mask,
	AnyModifier,
	0x00
};

/**
 * @brief A static lookup table of strings for X modifier key masks declared in /usr/include/X11/X.h
 */
static const char *ev_modifier_mask_str[] = \
{
	"ShiftMask",
	"LockMask",
	"ControlMask",
	"Mod1Mask",
	"Mod2Mask",
	"Mod3Mask",
	"Mod4Mask",
	"Mod5Mask",
	"Button1Mask",
	"Button2Mask",
	"Button3Mask",
	"Button4Mask",
	"Button5Mask",
	"AnyModifier",
	0x00
};

/**
 * @brief Convert a modifier key mask (enum / bitmask) into a string representation
 */
static inline const char *
int_key_modifier_str(unsigned int key_modifier)
{
    if (!key_modifier) return NULL;

	int i = 0;
	while (ev_modifier_mask[i])
	{
		if (key_modifier == ev_modifier_mask[i])
			return ev_modifier_mask_str[i];
		i++;
	}
	return NULL;
}

/**
 * @brief Convert a string representation of a modifier key mask into an enum / bitmask
 */
static inline unsigned int
str_key_modifier_int(char *str)
{
    if (!str) return 0;

	int i = 0;
	while (ev_modifier_mask_str[i])
	{
		if (strcmp(str, ev_modifier_mask_str[i]) == 0)
			return ev_modifier_mask[i];
		i++;
	}
	return 0x00;
}

/**
 * @brief Print an error message if a word in the string is not recognized as a valid X Modifier Key Mask
 */
static inline void
check_modmasks(const char *str_err_prefix1, const char *str_err_prefix2, const char *str_modkeymasks)
{
  // fputs("str_check_modkeymasks(const char *str_err_prefix1, const char *str_err_prefix2, const char *str_modkeymasks)\n", stdout);
  if (!str_modkeymasks) return;

  int count = str_count_words(str_modkeymasks);
  const char* next = str_modkeymasks;

  for (int i = 0; i < count; i++)
  {
		char *word = NULL;
		next = str_get_word(next, &word);

		if (!word)
			break;

		int modkeymask = str_key_modifier_int(word);
		printfdf(false, "(): %s  %i (0x%02i)", word, modkeymask, modkeymask);

		if (!modkeymask)
			printfef(true, "(): %s%s \"%s\" not recognized as a Modifier Key Mask. See: /usr/include/X11/X.h", str_err_prefix1, str_err_prefix2, word);

		free(word);
  }
  // fputs("\n", stdout);
}

/**
 * @brief convert a string of words into an array modifier key bitmasks
 * if a word in the string is not recognized as a valid modifier key bitmask
 * then skip over it. Finally write 0x00 to the last array entry (+1)
 * which denotes the end of the array of modifier key bitmasks
 *
 * @param s the string of words to parse / split / convert
 * @param dest place to store pointer to the new array of modifier key bitmasks
 * @return size of the array
 */
static inline int
modkeymasks_str_enums(const char *s, int** dest)
{
  // fputs("modkeymasks_str_enums(char *str, int** dest)\n", stdout);
  *dest = NULL;
  if (!s) return 0;

  int num_words = str_count_words(s);
  if(num_words == 0) return 0;

  int *arr_modkeymasks = (int *)malloc(sizeof(int)*(num_words+1));
  memset(arr_modkeymasks, 0, sizeof(int)*(num_words+1));

  int count = 0;
  for (int i = 0; i < num_words; i++)
  {
	char *word = NULL;

	s = str_get_word(s, &word);
	// s = str_get_word_alnum_(s, &word);

	if (!word)
	  break;

	int modkeymask = str_key_modifier_int(word);
	printfdf(false, "(): %s  %i (0x%02i)", word, modkeymask, modkeymask);

	// only increment the array counter if the modifier key mask is valid
	if(modkeymask)
	{
	  arr_modkeymasks[count] = modkeymask;
	  count++;
	}
	free(word);

	if (!s)
	  break;
  }
  *dest = arr_modkeymasks;
  return count;
}

/**
 * @brief Check to see if there is a specific modifier key bitmask in an array of modifier key bitmasks
 */
static inline bool
arr_modkeymasks_includes(int *arr_modkeymasks, int modkeymask)
{
  // fputs("arr_keycodes_includes(KeyCode *arr_keycodes, KeyCode keycode)\n", stdout);
  if (!arr_modkeymasks) return false;
  if (!modkeymask) return false;

  int i = 0;
  while (arr_modkeymasks[i] != 0x00)
  {
    if (arr_modkeymasks[i] & modkeymask)
      return true;
    i++;
  }

  return false;
}

/**
 * @brief Print a log message if the keyboard event has a modifier key held down
 */
static inline void
report_key_modifiers(XKeyEvent *evk)
{
  // fputs("report_key_modifiers(XKeyEvent *evk)\n", stdout);
  if (!evk) return;

  int i = 0;
  while (ev_modifier_mask[i])
  {
    if (evk->state & ev_modifier_mask[i])
      printfdf(false,"(): Key modifier (%s) is held for key (%s).", ev_modifier_mask_str[i], ev_key_str(evk));
    i++;
  }
  // fputs("\n", stdout);
}

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

static inline int
sort_cw_by_row(dlist* dlist1, dlist* dlist2, void* data)
{
	ClientWin *cw1 = (ClientWin *) dlist1->data;
	ClientWin *cw2 = (ClientWin *) dlist2->data;
	if (cw1->y < cw2->y)
		return -1;
	else if (cw1->y > cw2->y)
		return 1;
	else if (cw1->x < cw2->x)
		return -1;
	else if (cw1->x > cw2->x)
		return 1;
	else
		return 0;
}

static inline int
sort_cw_by_column(dlist* dlist1, dlist* dlist2, void* data)
{
	ClientWin *cw1 = (ClientWin *) dlist1->data;
	ClientWin *cw2 = (ClientWin *) dlist2->data;

	if (cw1->x + cw1->src.width / 2
			< cw2->x + cw2->src.width / 2)
		return -1;
	else if (cw1->x + cw1->src.width / 2
			> cw2->x + cw2->src.width / 2)
		return 1;
	else if (cw1->y + cw1->src.height / 2
			< cw2->y + cw2->src.height / 2)
		return -1;
	else if (cw1->y + cw1->src.height / 2
			> cw2->y + cw2->src.height / 2)
		return 1;
	else
		return 0;
}

extern session_t *ps_g;

int load_config_file(session_t *ps);

#endif /* SKIPPY_H */
