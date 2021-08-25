# libwinr
This is a library that handles the boilerplate necessary to create a window, get
its framebuffer to draw on and handle events. It can also render text properly
(which is usually difficult without a big fat library). Currently only Wayland
is supported, but X11 and Linux framebuffer backends are planned.

Features:

- very small API (15 functions) and implementation (~2k LOC)
- handles text rendering
- doesn't take control over your main loop
- very few dependencies (libwayland-client, libxkbcommon and that's it)

Font rendering is done by [SSFN][]. Fonts are only supported in SFN format and
might have to be converted.

This is alpha software. It has had literally three days of development in total.
So there will be some rough edges. Be careful.

[SSFN]: https://gitlab.com/bztsrc/scalable-font2

## Compiling

You need the `wayland-client` and `xkbcommon` headers to compile.

    make

You may want to change some parameters in `config.mk`, but it should work as-is.

## API

    typedef struct {
        int x, y;
    } Point;
    Point pointadd(Point, Point);

A 2D point.
`pointadd` adds two points.

    typedef struct {
        Point min, max;
    } Rect;
    int rectw(Rect);
    int recth(Rect);

A rectangle, represented by its top-left and bottom-right points.
It is inclusive on `min` and exclusive on `max`.
`rectw` returns the width of the rectangle.
`recth` returns the height of the rectangle.

    typedef struct {
        Rect r, damage;
        void *data;
        unsigned s;
        ...
    } Fb;
    void fbdamage(Fb *, Rect);
    void drawrect(Fb *, Rect, int color);

A framebuffer, with damage tracking.
`Fb.data` points to the framebuffer pixels, as ARGB32 row-major.
`Fb.r` is the rectangle that `Fb.data` represents; it is not guaranteed to start
at (0, 0).
`Fb.s` is the stride (how much you need to add to a pointer to go down one line).
It may be different from `rectw(Fb.r) * 4`.
`Fb.damage` is the damage rectangle for the framebuffer; it must be updated with
`fbdamage`.
`drawrect` draws a solid-colored rectangle, with no alpha blending. Its main use
is clearing the screen.

    typedef struct { ... } Font;
    void fontopen(Font *, char const *);
    void fontclose(Font *);
    void fontselect(Font *, int family, int style, int size);
    void drawtext(Fb *fb, Font *, Point pos, int color, char const *str, int len);

`Font` represents a font. It is supposed to be opaque.
`fontopen` and `fontclose` open an SFN font from a filename and close it,
respectively.
`fontselect` sets a family, style and size (in pixels) for a font.
`drawtext` draws `str` on `fb` with the top left at `pos`. `len` is the maximum
number of characters read from `str`, or negative for null-terminated strings.

    typedef enum {
        Enone,
        Eframe,
        Eresized,
        Eclosed,
        Epointer,
        Ebutton,
        Escroll,
        Eenter,
        Eleave,
        Emouse = Epointer | Ebutton | Escroll | Eenter | Eleave,
        Ekey,
        Eany
    } Ekind;

`Ekind` represents an event.
`Eframe` means a new frame is required.
`Eresized` means the window was resized. It implies `Eframe`.
`Eclosed` means the window was requested to be closed. It may be ignored, but
your users might hate you if you do that.
`Epointer` means the mouse moved.
`Ebutton` means the mouse button status was changed.
`Escroll` means the scroll amount was changed.
`Eenter` means the mouse entered the window region.
`Eleave` means the mouse left the window region.
`Emouse` means any mouse related event.
`Ekey` means a key arrived.
`Eany` means any event.

    typedef struct {
        int key;
        int pressed;
        unsigned long time;
    } Keypress;

`Keypress` represents a single keystroke.
`key` is a (currently platform-dependent) keysym.
`pressed` is whether it is pressed or not.
`time` is a timestamp, in milliseconds.

    typedef enum {
        Bnone,
        Bleft,
        Bright,
        Bmiddle,
        Bany
    } Button;

    typedef struct {
        Fb fb;
        Rect r;
        Point mouse, scroll;
        Ekind ev;
        Button but;
        ...
    } Win;
    void winopen(Win *);
    void winclose(Win *);
    void winflush(Win *, Ekind ev);
    void winaddkeypress(Win *, Keypress);
    Keypress winkeypress(Win *);
    int winkeytext(Keypress, char *, int);

`Win` represents an open window.
`Win.fb` is an `Fb` for the contents of the window.
`Win.r` is the size rectangle; there are no guarantees about `Win.r.min` being 0.
`Win.mouse` is the mouse position, window-relative; it may be outside the window.
`Win.scroll` is the accumulated scroll vector; it will continue to accumulate
until cleared manually.
`Win.ev` is the bitmap of which events happened on the last call to `winflush`.
`Win.but` is the set of mouse buttons currently pressed.

`winopen` and `winclose` open and close a window, respectively.
`winflush` flushes the frame to the screen and waits until either one of the
events listed by `ev` happen, or the window is closed, or it needs to be
rerendered. Which events actually happened are stored in `Win.ev`.

`winkeypress` takes one `Keypress` from a window's internal queue. This queue is
filled automatically as keystrokes arrive from the keyboard, but may have them
manually inserted by `winaddkeypress`.
`winkeytext` converts a `Keypress` into a sequence of UTF-8 characters. It
returns the length of the resulting sequence, or -1 if it's an invalid keycode.
It does not convert control characters, and can't handle compose and dead keys
yet.

