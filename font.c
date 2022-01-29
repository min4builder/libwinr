#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "draw.h"
#include "font.h"
#define SSFN_IMPLEMENTATION
#include "ssfn.h"

void
fontopen(Font *f, char const *name, int size)
{
	char path[4096];
	char const *fontpath = getenv("FONTPATH");
	char const *nextpath;
	if (!fontpath)
		fontpath = ".";
	while (*fontpath) {
		nextpath = strchr(fontpath, ':');
		if (!nextpath)
			nextpath = strchr(fontpath, '\0');
		snprintf(path, sizeof(path), "%.*s/%s.sfn", (int) (nextpath - fontpath), fontpath, name);
		fontpath = *nextpath ? nextpath + 1 : nextpath;

		fprintf(stderr, "trying '%s' for '%s'...\n", path, name);
		int fd = open(path, O_RDONLY);
		if (fd < 0) {
			if (*fontpath)
				continue;
			perror("fontopen");
			return;
		}

		ssize_t len = lseek(fd, 0, SEEK_END);
		if (len < 0) {
			perror("fontopen");
			return;
		}

		char *data = malloc(len);
		size_t r = 0;
		while (r < (size_t) len)
			r += pread(fd, data + r, len - r, r);

		f->ctx = calloc(1, sizeof(ssfn_t));
		if (!f->ctx) {
			perror("fontopen");
			return;
		}

		int err = ssfn_load(f->ctx, data);
		if (err) {
			fprintf(stderr, "fontopen: %s\n", ssfn_error(err));
			return;
		}
		break;
	}
	f->size = size;
	int err = ssfn_select(f->ctx, SSFN_FAMILY_ANY, NULL, SSFN_STYLE_REGULAR, f->size);
	if (err)
		fprintf(stderr, "fontopen: %s\n", ssfn_error(err));
}

void
fontset(Font *f, int attr)
{
	int style = SSFN_STYLE_ABS_SIZE;
	if (attr & Fbold)
		style |= SSFN_STYLE_BOLD;
	if (attr & Fitalic)
		style |= SSFN_STYLE_ITALIC;
	if (attr & Funderline)
		style |= SSFN_STYLE_UNDERLINE;
	if (attr & Fstrikethrough)
		style |= SSFN_STYLE_STHROUGH;
	if (attr & Frtl)
		style |= SSFN_STYLE_RTL;
	int err = ssfn_select(f->ctx, SSFN_FAMILY_ANY, NULL, style, f->size);
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
		.x = fb->r.min.x + p.x,
		.y = fb->r.min.y + p.y + f->size
	};
	Point max = Point(0, 0);
	int prog = 0;
	while (len < 0 ? s[prog] : prog < len) {
		int n = ssfn_render(f->ctx, &dst, s + prog);
		if (max.x < dst.x)
			max.x = dst.x;
		if (max.y < dst.y)
			max.y = dst.y;
		if (!n)
			break;
		if (n < 0) {
			printf("drawtext: %s\n", ssfn_error(n));
			n = 1;
		}
		prog += n;
	}
	fbdamage(fb, Rect(p, fb->r.max));
}

void
fontclose(Font *f)
{
	ssfn_free(f->ctx);
	free(f->ctx);
}

