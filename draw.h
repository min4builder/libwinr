#include "ssfn.h"

typedef struct {
	int x, y;
} Point;
#define Point(x, y) (Point) { (x), (y) }

inline Point
pointadd(Point a, Point b)
{
	return Point(a.x + b.x, a.y + b.y);
}

typedef struct {
	Point min, max;
} Rect;
#define Rect(min, max) (Rect) { (min), (max) }

inline int
rectw(Rect r)
{
	return r.max.x - r.min.x;
}

inline int
recth(Rect r)
{
	return r.max.y - r.min.y;
}

typedef struct {
	Rect r, damage;
	void *data;
	unsigned s;
	int damaged;
} Fb;

void fbdamage(Fb *, Rect);
void drawrect(Fb *, Rect, int color);

typedef struct {
	ssfn_t ctx;
} Font;

void fontopen(Font *, char const *);
void fontselect(Font *, int family, int style, int size);
void drawtext(Fb *, Font *, Point, int color, char const *, int);
void fontclose(Font *);

typedef enum {
	Enone = 0,
	Eframe = 1,
	Eresized = 2,
	Eclosed = 4,
	Epointer = 8,
	Ebutton = 16,
	Escroll = 32,
	Eenter = 64,
	Eleave = 128,
	Emouse = Epointer | Ebutton | Escroll | Eenter | Eleave,
	Ekey = 256,
	Eany = ~0
} Ekind;

typedef enum {
	Bnone = 0,
	Bleft = 1,
	Bright = 2,
	Bmiddle = 4,
	Bany = ~0
} Button;

typedef struct {
	int key;
	int pressed;
	unsigned long time;
} Keypress;

typedef struct {
	Fb fb;
	Rect r;
	Point mouse, scroll;
	Ekind ev;
	Button but;
	int klen;
	Keypress kbuf[32];
	int kpos, kcur;

	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_seat *seat;
	struct wl_shm *shm;
	struct xdg_wm_base *wm;

	struct wl_surface *surface;
	struct xdg_surface *xdg_surf;
	struct xdg_toplevel *toplevel;
	int config;
	unsigned configserial;

	struct wl_pointer *pointer;
	Point bufmouse, bufscroll;
	Ekind bufev;
	Button bufbut;
	struct wl_keyboard *keyboard;
	struct xkb_state *kbstate;
	struct xkb_context *kbctx;
	struct xkb_keymap *kbmap;

	int shmfd;
	unsigned shmsize;
	struct wl_shm_pool *pool;
	struct wl_buffer *buffer;
} Win;

void winopen(Win *);
void winclose(Win *);

void winflush(Win *, Ekind);

void winaddkeypress(Win *, Keypress);
Keypress winkeypress(Win *);
int winkeytext(Keypress, char *, int);

