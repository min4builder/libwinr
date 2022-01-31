typedef enum {
	Enone = 0, /* nothing happened */
	Eframe = 1, /* redraw required */
	Eresized = 2, /* resized, implies Eframe */
	Eclosed = 4, /* window close requested */
	Epointer = 8, /* mouse moved */
	Ebutton = 16, /* mouse button status changed */
	Escroll = 32, /* scroll amount updated */
	Eenter = 64, /* mouse enter */
	Eleave = 128, /* mouse leave */
	Emouse = Epointer | Ebutton | Escroll | Eenter | Eleave, /* any mouse event */
	Ekey = 256, /* key event */
	Eany = ~0 /* any event */
} Ekind;

/* mouse buttons */
typedef enum {
	Bnone = 0,
	Bleft = 1,
	Bright = 2,
	Bmiddle = 4,
	Bany = ~0
} Button;

typedef struct {
	int key; /* platform-dependent keysym */
	int mods; /* platform-dependent modifiers */
	int pressed; /* currently pressed */
	unsigned long time; /* timestamp */
} Keypress;

/* window structure */
typedef struct {
	Fb fb; /* contents of the window */
	Rect r; /* rectangle the size of the window; not the same as fb.r */
	/* mouse, window-relative; can be outside window */
	/* scroll amount, cumulative; will accumulate until cleared manually */
	Point mouse, scroll;
	Ekind ev; /* events since last winflush() call */
	/* set of mouse buttons currently pressed */
	Button but;

	/* opaque */
	Button obut;
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
	struct wl_buffer *buffer, *backbuffer;
	void *data, *backdata;
	int canrender;
} Win;

/* open and close windows; window size is a hint and may not be respected */
void winopen(Win *win, Point size, char const *name);
void winclose(Win *win);

/* flush frame, wait for events in what */
void winflush(Win *win, Ekind what);

/* returns a file descriptor that can be used to poll, if necessary */
int winpollfd(Win *win);

/* add keypress to queue */
void winaddkeypress(Win *win, Keypress key);
/* get next keypress from window queue */
Keypress winkeypress(Win *win);
/* convert keypress into UTF-8 characters. returns length of sequence, -1 if invalid */
int winkeytext(Keypress, char *str, int len);

