#include "ssfn.h"

/* opaque data structure */
typedef struct {
	ssfn_t ctx;
} Font;

/* open a font by name */
void fontopen(Font *, char const *);
/* set the size and style of the font */
void fontselect(Font *, int family, int style, int size);
/* draw a string onto a framebuffer, with the given font */
void drawtext(Fb *fb, Font *font, Point pos, int color, char const *str, int len);
/* close a font and free its resources */
void fontclose(Font *);

