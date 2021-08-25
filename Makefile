OBJS = main.o draw.o font.o wayland.o xdg-shell-protocol.o
LIBS = -lwayland-client -lxkbcommon
CFLAGS = -g -O3 -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter

all: seedir

seedir: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: draw.h config.h
draw.o: draw.h
font.o: font.c draw.h
	$(CC) $(CFLAGS) -O1 -c -o $@ font.c
wayland.o: draw.h ssfn.h xdg-shell-client-protocol.h

xdg-shell-protocol.c:
	wayland-scanner private-code > $@ < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml

xdg-shell-client-protocol.h:
	wayland-scanner client-header > $@ < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml

clean:
	-$(RM) seedir $(OBJS) core
