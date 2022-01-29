PREFIX = /usr
INCLUDEDIR = $(PREFIX)/include
LIBDIR = $(PREFIX)/lib
PKGCONFDIR = $(PREFIX)/lib/pkgconfig
DEPENDS = wayland-client xkbcommon

.POSIX:
.PHONY: all install clean

OBJS = draw.o font.o wayland.o xdg-shell-protocol.o
CFLAGS = -g -O3 -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter

all: libwinr.a libwinr.pc

install: libwinr.a libwinr.pc
	install -Dm644 libwinr.a $(DESTDIR)$(LIBDIR)/libwinr.a
	install -Dm644 -t $(DESTDIR)$(INCLUDEDIR)/winr draw.h ssfn.h font.h win.h
	install -Dm644 libwinr.pc $(DESTDIR)$(PKGCONFDIR)/libwinr.pc

libwinr.pc: libwinr.pc.in
	sed "s|@prefix@|$(PREFIX)|g;s|@includedir@|$(INCLUDEDIR)|g;s|@libdir@|$(LIBDIR)|g;s|@depends@|$(DEPENDS)|g" libwinr.pc.in > $@

libwinr.a: $(OBJS)
	$(AR) rsc $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

draw.o: draw.h
font.o: font.c draw.h font.h ssfn.h
	$(CC) $(CFLAGS) -O1 -c -o $@ font.c
wayland.o: draw.h win.h xdg-shell-client-protocol.h

xdg-shell-protocol.c:
	wayland-scanner private-code > $@ < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml

xdg-shell-client-protocol.h:
	wayland-scanner client-header > $@ < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml

clean:
	-$(RM) libwinr.a libwinr.pc $(OBJS)
