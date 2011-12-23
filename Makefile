PREFIX = /usr/local
BINDIR = ${PREFIX}/bin

X11PREFIX = /usr/X11R6

PYTHON = python

CFLAGS += -I${X11PREFIX}/include `pkg-config xft xrender xcomposite xdamage xfixes --cflags` -g -pedantic -Wall
LDFLAGS += -L${X11PREFIX}/lib -lX11 -lm `pkg-config xft xrender xcomposite xdamage xfixes --libs`

# Disable post-processing effects
# CFLAGS += -DNOEFFECTS

# Comment these out to disable Xinerama support
CFLAGS += -DXINERAMA
LDFLAGS += -lXext -lXinerama

# Uncomment this for Xinerama debugging
#CFLAGS += -DDEBUG

EXESUFFIX =

SOURCES = skippy.c wm.c dlist.c mainwin.c clientwin.c layout.c focus.c config.c tooltip.c
HEADERS = skippy.h wm.h dlist.h mainwin.h clientwin.h layout.h focus.h config.h tooltip.h environment.h

all: skippy-xd${EXESUFFIX}

skippy-xd${EXESUFFIX}: Makefile ${SOURCES} ${HEADERS}
	gcc ${CFLAGS} -o skippy-xd${EXESUFFIX} ${SOURCES} ${LDFLAGS}

environment.h: configure_environment_header.py
	${PYTHON} configure_environment_header.py

clean:
	rm -f skippy-xd${EXESUFFIX} environment.h

install:
	install -d ${DESTDIR}${BINDIR}
	install -m 755 skippy-xd$(EXESUFFIX) ${DESTDIR}${BINDIR}/skippy-xd${EXESUFFIX}
