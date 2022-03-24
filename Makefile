PREFIX = /usr
INCLUDEDIR = $(PREFIX)/include
LIBDIR = $(PREFIX)/lib
PKGCONFDIR = $(PREFIX)/lib/pkgconfig
BACKEND = wayland

OBJS = draw.o font.o
CFLAGS = -g -O3 -std=c99 -fPIC -pedantic -Wall -Wextra -Wno-unused-parameter

DEPENDS = wayland-client xkbcommon
OBJS += wayland.o xdg-shell-protocol.o

.POSIX:
.PHONY: all install clean

all: libwinr-$(BACKEND).a libwinr-$(BACKEND).so libwinr-$(BACKEND).pc

install: libwinr-$(BACKEND).a libwinr-$(BACKEND).so libwinr-$(BACKEND).pc
	install -Dm644 libwinr-$(BACKEND).a $(DESTDIR)$(LIBDIR)/libwinr-$(BACKEND).a
	install -Dm644 libwinr-$(BACKEND).so $(DESTDIR)$(LIBDIR)/libwinr-$(BACKEND).so
	install -Dm644 -t $(DESTDIR)$(INCLUDEDIR)/winr draw.h font.h win.h
	install -Dm644 libwinr-$(BACKEND).pc $(DESTDIR)$(PKGCONFDIR)/libwinr-$(BACKEND).pc

libwinr-$(BACKEND).pc: libwinr.pc.in
	sed "s|@variant@|$(BACKEND)|g;s|@prefix@|$(PREFIX)|g;s|@includedir@|$(INCLUDEDIR)|g;s|@libdir@|$(LIBDIR)|g;s|@depends@|$(DEPENDS)|g" libwinr.pc.in > $@

libwinr-$(BACKEND).a: $(OBJS)
	$(AR) rsc $@ $(OBJS)

libwinr-$(BACKEND).so: $(OBJS)
	$(CC) -shared -o $@ $(OBJS) `pkg-config --libs $(DEPENDS)`

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

draw.o: draw.h
font.o: font.c draw.h font.h ssfn.h
	$(CC) $(CFLAGS) -O1 -c -o $@ font.c
wayland.o: draw.h wayland.h win.h xdg-shell-client-protocol.h

xdg-shell-protocol.c:
	wayland-scanner private-code > $@ < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml

xdg-shell-client-protocol.h:
	wayland-scanner client-header > $@ < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml

clean:
	-$(RM) libwinr-*.a libwinr-*.so libwinr-*.pc $(OBJS)
