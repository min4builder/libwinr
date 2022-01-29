typedef struct {
	int x, y;
} Point;
#define Point(x, y) (Point) { (x), (y) }

/* rectangle represented by top-left and bottom-right corners */
typedef struct {
	Point min, max;
} Rect;
#define Rect(min, max) (Rect) { (min), (max) }

/* add two points */
inline Point
pointadd(Point a, Point b)
{
	return Point(a.x + b.x, a.y + b.y);
}

/* clamp a point within a rectangle */
inline Point
pointclamp(Point p, Rect r)
{
	if (p.x < r.min.x)
		p.x = r.min.x;
	if (p.y < r.min.y)
		p.y = r.min.y;
	if (p.x > r.max.x)
		p.x = r.max.x;
	if (p.y > r.max.y)
		p.y = r.max.y;
	return p;
}

/* width of a rectangle */
inline int
rectw(Rect r)
{
	return r.max.x - r.min.x;
}

/* height of a rectangle */
inline int
recth(Rect r)
{
	return r.max.y - r.min.y;
}

/* intersection of two rectangles */
inline Rect
rectintersect(Rect a, Rect b)
{
	return Rect(pointclamp(a.min, b), pointclamp(a.max, b));
}

/* framebuffer; data, if not NULL, is the pixels in ARGB32 row-major.
   r is the rectangle; it might not start at (0, 0).
   s is the stride; it might not be rectw(.r) * 4.
   damage is the damage rectangle; it must be updated by fbdamage. */
typedef struct {
	Rect r, damage;
	void *data;
	unsigned s;
	int damaged;
} Fb;

/* damage the framebuffer with the rectangle;
   must be called if fb.data is changed directly */
void fbdamage(Fb *, Rect);
/* fill a rectangle with a solid color.
   r is absolute, not relative to fb.r */
void drawrect(Fb *, Rect, int color);
/* blit a portion of a framebuffer onto another */
void drawblit(Fb *, Point, Fb const *, Rect);

