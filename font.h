/* opaque data structure */
typedef struct {
	int size;
	void *ctx;
} Font;

/* font attribute constants */
enum {
	Fbold,
	Fitalic,
	Funderline,
	Fstrikethrough,
	Frtl
};

/* open a font by name */
void fontopen(Font *, char const *name, int size);
/* set the style of the font (see constants above) */
void fontset(Font *, int attr);
/* draw a string onto a framebuffer, with the given font */
void drawtext(Fb *fb, Font *font, Point pos, int color, char const *str, int len);
/* close a font and free its resources */
void fontclose(Font *);

