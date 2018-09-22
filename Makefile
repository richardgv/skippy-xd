PREFIX ?= /usr
BINDIR ?= ${PREFIX}/bin

ifeq ($(shell command -v clang 2>&1 | grep -c "clang"), 1)
	CC = clang
else
	CC ?= gcc
endif

CPPFLAGS += -std=c99 -Wall -I/usr/include/freetype2

SRCS_RAW = skippy wm dlist mainwin clientwin layout focus config tooltip img img-xlib
PACKAGES = x11 xft xrender xcomposite xdamage xfixes

# === Options ===
ifeq "${CFG_NO_XINERAMA}" ""
	CPPFLAGS += -DCFG_XINERAMA
	PACKAGES += xext xinerama
endif

ifeq "${CFG_NO_PNG}" ""
	CPPFLAGS += -DCFG_LIBPNG
	SRCS_RAW += img-png
	PACKAGES += libpng zlib
endif

ifeq "${CFG_NO_JPEG}" ""
	CPPFLAGS += -DCFG_JPEG
	SRCS_RAW += img-jpeg
	LIBS += -ljpeg
endif

ifeq "${CFG_NO_GIF}" ""
	CPPFLAGS += -DCFG_GIFLIB
	SRCS_RAW += img-gif
	LIBS += -lgif
endif

ifeq "$(CFG_DEV)" ""
	CFLAGS ?= -DNDEBUG -O2 -D_FORTIFY_SOURCE=2

	ifeq "$(CC)" "clang"
		CPPFLAGS += -Wno-unused-function
	else # gcc
		CPPFLAGS += -Wno-unused-but-set-variable
	endif
else
	CC = clang
	CFLAGS += -ggdb -Wshadow -Weverything -Wno-unused-parameter -Wno-conversion -Wno-sign-conversion -Wno-gnu -Wno-disabled-macro-expansion -Wno-padded -Wno-c11-extensions -Wno-sign-compare -Wno-vla -Wno-cast-align
	export LD_ALTEXEC = /usr/bin/ld.gold
	# Xinerama debugging
	CPPFLAGS += -DDEBUG_XINERAMA
endif

CFLAGS += -std=c99 -Wall
LDFLAGS ?= -Wl,-O1 -Wl,--as-needed
INCS = $(shell pkg-config --cflags $(PACKAGES))
LIBS += -lm $(shell pkg-config --libs $(PACKAGES))

# === Version string ===
SKIPPYXD_VERSION = "v0.5.2~pre (2018.09.09) - \\\"Puzzlebox\\\" Edition"
CPPFLAGS += -DSKIPPYXD_VERSION=\"${SKIPPYXD_VERSION}\"

# === Recipes ===
EXESUFFIX =
BINS = skippy-xd${EXESUFFIX}
SRCS = $(foreach name,$(SRCS_RAW),src/$(name).c)
HDRS = $(foreach name,$(SRCS_RAW),src/$(name).h)
OBJS = $(foreach name,$(SRCS_RAW),$(name).o)

.DEFAULT_GOAL := skippy-xd${EXESUFFIX}

%.o: src/%.c ${HDRS}
	${CC} ${INCS} ${CFLAGS} ${CPPFLAGS} -c src/$*.c

skippy-xd${EXESUFFIX}: ${OBJS}
	${CC} ${LDFLAGS} -o skippy-xd${EXESUFFIX} ${OBJS} ${LIBS}

clean:
	rm -f ${BINS} ${OBJS} src/.clang_complete

install-check:
	@echo "'make install' target folders:"
	@echo "PREFIX=${PREFIX} DESTDIR=${DESTDIR} BINDIR=${BINDIR}"
	@echo "skippy executables will be installed into: ${DESTDIR}${BINDIR}"
	@echo "skippy's config file will be installed to: ${DESTDIR}/etc/xdg/skippy-xd.rc"

install: ${BINS} skippy-xd.sample.rc
	install -d "${DESTDIR}${BINDIR}/" "${DESTDIR}/etc/xdg/"
	install -m 755 ${BINS} "${DESTDIR}${BINDIR}/"
	install -m 644 skippy-xd.sample.rc "${DESTDIR}/etc/xdg/skippy-xd.rc"
	install -m 755 skippy-xd-toggle "${DESTDIR}${BINDIR}/"

uninstall:
	# Should configuration file be removed?
	rm -f $(foreach bin,$(BINS),"${DESTDIR}${BINDIR}/$(bin)")

src/.clang_complete: Makefile
	@(for i in $(filter-out -O% -DNDEBUG, $(CPPFLAGS) $(CFLAGS) $(INCS)); do echo "$$i"; done) > $@

version:
	@echo "${COMPTON_VERSION}"

.PHONY: uninstall clean docs version
