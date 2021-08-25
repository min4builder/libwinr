#define _POSIX_C_SOURCE 200809L
#define SSFN_IMPLEMENTATION
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "draw.h"

void
fontopen(Font *f, char const *filename)
{
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
		perror("fontopen");
	ssize_t len = lseek(fd, 0, SEEK_END);
	if (len < 0)
		perror("fontopen");
	char *data = malloc(len);
	size_t r = 0;
	while(r < len)
		r += pread(fd, data + r, len - r, r);
	int err = ssfn_load(&f->ctx, data);
	if (err)
		printf("fontopen: %s\n", ssfn_error(err));
}

void
fontselect(Font *f, int family, int style, int size)
{
	int err = ssfn_select(&f->ctx, family, NULL, style, size);
	if (err)
		printf("fontselect: %s\n", ssfn_error(err));
}

void
drawtext(Fb *fb, Font *f, Point p, int color, char const *s, int len)
{
	ssfn_buf_t dst = {
		.ptr = fb->data,
		.p = fb->s,
		.w = rectw(fb->r), .h = recth(fb->r),
		.fg = color,
		.bg = 0,
		.x = p.x,
		.y = p.y + f->ctx.size /* XXX undocumented, font size as passed to select */
	};
	Point max = Point(0, 0);
	int prog = 0;
	while (len < 0 ? s[prog] : prog < len) {
		int n = ssfn_render(&f->ctx, &dst, s + prog);
		if (max.x < dst.x)
			max.x = dst.x;
		if (max.y < dst.y)
			max.y = dst.y;
		if (!n)
			break;
		if (n < 0)
			printf("drawtext: %s\n", ssfn_error(n));
		prog += n;
	}
	fbdamage(fb, Rect(p, fb->r.max));
}

void
fontclose(Font *f)
{
	ssfn_free(&f->ctx);
}

