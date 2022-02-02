# libwinr
This is a library that handles the boilerplate necessary to create a window, get
its framebuffer to draw on and handle events. It can also render text properly
(which is usually difficult without a big fat library). Currently only Wayland
is supported, but X11 and Linux framebuffer backends are planned.

Features:

- very small API (<20 functions) and implementation (~2k LOC)
- handles text rendering
- doesn't take control over your main loop
- very few dependencies (libwayland-client, libxkbcommon and that's it)

Font rendering is done by [SSFN][]. Fonts are only supported in SFN format and
might have to be converted (automatic conversion is a planned feature).

This is alpha software. There will be some rough edges. Be careful.

[SSFN]: https://gitlab.com/bztsrc/scalable-font2

## Example

    // draw must be included before the others
    #include <winr/draw.h>
    #include <winr/font.h>
    #include <winr/win.h>

    int
    main()
    {
        char buf[32];
        Keypress kp;
        Win *w;
        Font *f;

        w = winopen(Point(800, 600), "winr example");
        f = fontopen("monospace", 16);

        w->ev = Eframe;

        do {
            if (w->ev & Eframe) {
                drawrect(&w->fb, w->fb.r, 0xffffffff);
                drawtext(&w->fb, &f, w->fb.r.min, 0xff000000, "Hello, world!", -1);
            }
            if (w->ev & Ekey) {
                kp = winkeypress(w);
                if (winkeytext(kp, buf, sizeof(buf)) > 0)
                    drawtext(&w->fb, &f, w->fb.r.min, 0xff444444, buf, -1);
            }
            winflush(w, Eclosed | Eframe | Ekey);
        } while (!(w->ev & Eclosed));

        winclose(w);
        fontclose(f);

        return 0;
    }

## Compiling

You need the `wayland-client` and `xkbcommon` headers to compile.

    make

To install:

    make install

The PREFIX and DESTDIR variables do what you would expect.

