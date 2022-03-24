typedef enum {
	Enone = 0, /* nothing happened */
	Eframe = 1, /* redraw required */
	Eresized = 2, /* resized, implies Eframe; updates win.r */
	Eclosed = 4, /* window close requested */
	Epointer = 8, /* mouse moved; uses pt */
	Ebutton = 16, /* mouse button event; uses key */
	Escroll = 32, /* scrolled; uses pt */
	Eenter = 64, /* mouse enter; uses pt for window-relative coordinates */
	Eleave = 128, /* mouse leave */
	Emouse = Epointer | Ebutton | Escroll | Eenter | Eleave, /* any mouse event; Event.kind will have the specific one */
	Ekey = 256, /* key event; uses key */
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

/* event structure */
typedef struct {
	Ekind kind; /* kind of event; one of Ekind */
	unsigned long time; /* timestamp */
	struct {
		int key; /* key or button pressed */
		int press; /* whether the key is pressed or not */
	} key;
	Point pos;
} Event;

typedef struct Display Display;

/* window structure */
typedef struct {
	Fb fb; /* contents of the window */
	Rect r; /* rectangle the size of the window; not the same as fb.r */
	Ekind listen; /* events that will be received */

	Display *d;
} Win;

/* open and close windows; window size is a hint and may not be respected */
Win *winopen(Point size, char const *name);
void winclose(Win *win);

/* flush frame */
void winflush(Win *win);
/* return one event from the queue; wait if queue empty */
Event winevent(Win *win);

/* returns a file descriptor that can be used to poll, if necessary */
int winpollfd(Win *win);

/* add event to queue */
void winaddevent(Win *win, Event e);
/* convert keypress into UTF-8 characters; returns length of sequence, -1 if invalid */
int winkeytext(Event e, char *str, int len);

