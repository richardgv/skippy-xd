PREFIX = /usr/local
BINDIR = ${PREFIX}/bin

X11PREFIX = /usr/X11R6

CC ?= gcc
PYTHON = python

PACKAGES = x11 xft xrender xcomposite xdamage xfixes

# Disable post-processing effects
# CFLAGS += -DNOEFFECTS

# Comment these out to disable Xinerama support
ifeq "${CFG_NO_XINERAMA}" ""
	CPPFLAGS += -DXINERAMA
	PACKAGES += xext xinerama
endif

# Uncomment this for Xinerama debugging
ifeq "$(CFG_DEV)" ""
	CFLAGS ?= -DNDEBUG -O2 -D_FORTIFY_SOURCE=2
else
	CC = clang
	CFLAGS += -ggdb
	export LD_ALTEXEC = /usr/bin/ld.gold
	#CPPFLAGS += -DDEBUG
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

SOURCES = skippy.c wm.c dlist.c mainwin.c clientwin.c layout.c focus.c config.c tooltip.c
HEADERS = skippy.h wm.h dlist.h mainwin.h clientwin.h layout.h focus.h config.h tooltip.h

all: skippy-xd${EXESUFFIX}

skippy-xd${EXESUFFIX}: ${SOURCES} ${HEADERS}
	${CC} ${INCS} ${CFLAGS} ${CPPFLAGS} ${LIBS} ${LDFLAGS} -o skippy-xd${EXESUFFIX} ${SOURCES}

environment.h: configure_environment_header.py
	${PYTHON} configure_environment_header.py

clean:
	rm -f skippy-xd${EXESUFFIX} environment.h

install:
	install -d ${DESTDIR}${BINDIR}
	install -m 755 skippy-xd$(EXESUFFIX) ${DESTDIR}${BINDIR}/skippy-xd${EXESUFFIX}

version:
	@echo "${COMPTON_VERSION}"

.PHONY: uninstall clean docs version
