PREFIX ?= /usr/local
BINDIR ?= ${PREFIX}/bin

CC ?= gcc

PACKAGES = x11 xft xrender xcomposite xdamage xfixes

# === Options ===
ifeq "${CFG_NO_XINERAMA}" ""
	CPPFLAGS += -DCFG_XINERAMA
	PACKAGES += xext xinerama
endif

ifeq "$(CFG_DEV)" ""
	CFLAGS ?= -DNDEBUG -O2 -D_FORTIFY_SOURCE=2
else
	CC = clang
	CFLAGS += -ggdb # -Weverything -Wno-gnu -Wno-disabled-macro-expansion -Wno-padded
	export LD_ALTEXEC = /usr/bin/ld.gold
	# Xinerama debugging
	CPPFLAGS += -DDEBUG_XINERAMA
endif

CFLAGS += -std=c99 -Wall
LDFLAGS ?= -Wl,-O1 -Wl,--as-needed
INCS = $(shell pkg-config --cflags $(PACKAGES))
LIBS = -lm $(shell pkg-config --libs $(PACKAGES))

# === Version string ===
SKIPPYXD_VERSION ?= git-$(shell git describe --always --dirty)-$(shell git log -1 --date=short --pretty=format:%cd)
CPPFLAGS += -DSKIPPYXD_VERSION="\"${SKIPPYXD_VERSION}\""

# === Recipes ===
EXESUFFIX =
BINS = skippy-xd${EXESUFFIX}
SRCS_RAW = skippy wm dlist mainwin clientwin layout focus config tooltip
SRCS = $(foreach name,$(SRCS_RAW),src/$(name).c)
HDRS = $(foreach name,$(SRCS_RAW),src/$(name).h)

all: skippy-xd${EXESUFFIX}

skippy-xd${EXESUFFIX}: ${SRCS} ${HDRS}
	${CC} ${INCS} ${CFLAGS} ${CPPFLAGS} ${LIBS} ${LDFLAGS} -o skippy-xd${EXESUFFIX} ${SRCS}

clean:
	rm -f ${BINS}

install: ${BINS} skippy-xd.rc-default
	install -d "${DESTDIR}${BINDIR}" "${DESTDIR}/etc/xdg/"
	install -m 755 ${BINS} "${DESTDIR}${BINDIR}/"
	install -m 644 skippy-xd.rc-default "${DESTDIR}/etc/xdg/skippy-xd.rc"

uninstall:
	rm -f $(foreach bin,$(BINS),"${DESTDIR}${BINDIR}/$(bin)")

version:
	@echo "${COMPTON_VERSION}"

.PHONY: uninstall clean docs version
