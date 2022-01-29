#include <stdint.h>
#include <string.h>
#include "draw.h"

extern inline Point pointadd(Point, Point);
extern inline Point pointclamp(Point, Rect);
extern inline int rectw(Rect);
extern inline int recth(Rect);
extern inline Rect rectintersect(Rect, Rect);

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
	if (r.max.x > fb->damage.max.x)
		fb->damage.max.x = r.max.x;
	if (r.max.y > fb->damage.max.y)
		fb->damage.max.y = r.max.y;
	fb->damage = rectintersect(fb->damage, fb->r);
}

void
drawrect(Fb *fb, Rect r, int color)
{
	r = rectintersect(fb->r, r);
	int y = r.min.y;
	uint32_t *data = (uint32_t *) ((char *) fb->data + y * fb->s);
	while (y < r.max.y) {
		int x = r.min.x;
		while (x < r.max.x)
			data[x++] = color;
		y++;
		data += fb->s / sizeof(uint32_t);
	}
	fbdamage(fb, r);
}

void
drawblit(Fb *d, Point dp, Fb const *s, Rect sr)
{
	/* TODO clamp stuff */
	uint32_t *ddata = (uint32_t *) ((char *) d->data + dp.y * d->s + dp.x * sizeof(uint32_t));
	int y = sr.min.y;
	uint32_t *sdata = (uint32_t *) ((char *) s->data + y * s->s + sr.min.x * sizeof(uint32_t));
	while (y < sr.max.y) {
		memcpy(ddata, sdata, rectw(sr) * sizeof(uint32_t));
		ddata += d->s / sizeof(uint32_t);
		sdata += s->s / sizeof(uint32_t);
		y++;
	}
	fbdamage(d, Rect(dp, pointadd(dp, Point(rectw(sr), recth(sr)))));
}

