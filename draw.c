#include "draw.h"

extern inline Point pointadd(Point, Point);
extern inline int rectw(Rect);
extern inline int recth(Rect);

void
fbdamage(Fb *fb, Rect r)
{
	if (!fb->damaged) {
		fb->damage = r;
		fb->damaged = 1;
		return;
	}
	if (r.min.x < fb->damage.min.x)
		fb->damage.min.x = r.min.x;
	if (r.min.y < fb->damage.min.y)
		fb->damage.min.y = r.min.y;
	if (r.max.x < fb->damage.max.x)
		fb->damage.max.x = r.max.x;
	if (r.max.y < fb->damage.max.y)
		fb->damage.max.y = r.max.y;
}

void
drawrect(Fb *fb, Rect r, int color)
{
	if (rectw(fb->r) < r.max.x)
		r.max.x = rectw(fb->r);
	if (recth(fb->r) < r.max.y)
		r.max.y = rectw(fb->r);
	int y = r.min.y;
	uint32_t *data = (char *) fb->data + y * fb->s;
	while (y < r.max.y) {
		int x = r.min.x;
		while (x < r.max.x)
			data[x++] = color;
		y++;
		data += fb->s / sizeof(uint32_t);
	}
	fbdamage(fb, r);
}

