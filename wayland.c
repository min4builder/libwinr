#define _GNU_SOURCE /* ugh */
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "xdg-shell-client-protocol.h"
#include "draw.h"
#include "win.h"

#include "wayland.h"

static void bufdone(void *, struct wl_buffer *);
static struct wl_buffer_listener buf_listener = { .release = bufdone };

static void newglobal(void *, struct wl_registry *, uint32_t, char const *, uint32_t);
static void delglobal(void *, struct wl_registry *, uint32_t);
static struct wl_registry_listener reg_listener = { .global = newglobal, .global_remove = delglobal };

static void inputkinds(void *, struct wl_seat *, uint32_t);
static void inputname(void *, struct wl_seat *, char const *);
static struct wl_seat_listener seat_listener = { .capabilities = inputkinds, .name = inputname };

static void menter(void *, struct wl_pointer *, uint32_t, struct wl_surface *, wl_fixed_t, wl_fixed_t);
static void mleave(void *, struct wl_pointer *, uint32_t, struct wl_surface *);
static void mmove(void *, struct wl_pointer *, uint32_t, wl_fixed_t, wl_fixed_t);
static void mbutton(void *, struct wl_pointer *, uint32_t, uint32_t, uint32_t, uint32_t);
static void mscroll(void *, struct wl_pointer *, uint32_t, uint32_t, wl_fixed_t);
static void mend(void *, struct wl_pointer *);
static void mscrolltype(void *, struct wl_pointer *, uint32_t);
static void minertia(void *, struct wl_pointer *, uint32_t, uint32_t);
static void mclicks(void *, struct wl_pointer *, uint32_t, int32_t);
static struct wl_pointer_listener pointer_listener = { .enter = menter, .leave = mleave, .motion = mmove, .button = mbutton, .axis = mscroll, .frame = mend, .axis_source = mscrolltype, .axis_stop = minertia, .axis_discrete = mclicks };

static void kbmap(void *, struct wl_keyboard *, uint32_t, int32_t, uint32_t);
static void kbenter(void *, struct wl_keyboard *, uint32_t, struct wl_surface *, struct wl_array *);
static void kbleave(void *, struct wl_keyboard *, uint32_t, struct wl_surface *);
static void kbkey(void *, struct wl_keyboard *, uint32_t, uint32_t, uint32_t, uint32_t);
static void kbmod(void *, struct wl_keyboard *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
static void kbrepeat(void *, struct wl_keyboard *, int32_t, int32_t);
static struct wl_keyboard_listener kb_listener = { .keymap = kbmap, .enter = kbenter, .leave = kbleave, .key = kbkey, .modifiers = kbmod, .repeat_info = kbrepeat};

static void dopong(void *, struct xdg_wm_base *, uint32_t);
static struct xdg_wm_base_listener wm_listener = { .ping = dopong };

static void setconfig(void *, struct xdg_surface *, uint32_t);
static struct xdg_surface_listener xdg_listener = { .configure = setconfig };

static void winconfig(void *, struct xdg_toplevel *, int32_t, int32_t, struct wl_array *);
static void winclosed(void *, struct xdg_toplevel *);
static struct xdg_toplevel_listener toplevel_listener = { .configure = winconfig, .close = winclosed };

static void
doresize(Win *w)
{
	if (!w->d->config)
		return;

	if (w->d->buffer)
		wl_buffer_destroy(w->d->buffer);

	unsigned size = rectw(w->r) * 4 * recth(w->r) * 2;
	if (size > w->d->shmsize) {
		ftruncate(w->d->shmfd, size);
		wl_shm_pool_resize(w->d->pool, size);
		w->d->shmsize = size;
	}

	if (w->d->data) {
		if (w->d->data < w->d->backdata)
			munmap(w->d->data, w->fb.s * recth(w->fb.r) * 2);
		else
			munmap(w->d->backdata, w->fb.s * recth(w->fb.r) * 2);
	}

	w->d->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, w->d->shmfd, 0);
	w->d->backdata = (char *) w->d->data + size/2;

	w->fb.data = w->d->data;
	w->fb.r = w->r;
	w->fb.s = rectw(w->fb.r) * 4;

	w->d->buffer = wl_shm_pool_create_buffer(w->d->pool, 0, rectw(w->fb.r), recth(w->fb.r), w->fb.s, WL_SHM_FORMAT_ARGB8888);
	w->d->backbuffer = wl_shm_pool_create_buffer(w->d->pool, size/2, rectw(w->fb.r), recth(w->fb.r), w->fb.s, WL_SHM_FORMAT_ARGB8888);
	wl_buffer_add_listener(w->d->buffer, &buf_listener, w);

	xdg_surface_ack_configure(w->d->xdg_surf, w->d->configserial);
	w->d->config = 0;
	w->d->canrender = 1;
}

Win *
winopen(Point size, char const *title)
{
	Win *w = calloc(1, sizeof(*w));

	w->d = calloc(1, sizeof(*w->d));

	w->r.max = size;

	w->d->kbctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	w->d->display = wl_display_connect(NULL);
	w->d->registry = wl_display_get_registry(w->d->display);
	wl_registry_add_listener(w->d->registry, &reg_listener, w);

	wl_display_roundtrip(w->d->display);

	w->d->surface = wl_compositor_create_surface(w->d->compositor);
	w->d->xdg_surf = xdg_wm_base_get_xdg_surface(w->d->wm, w->d->surface);
	xdg_surface_add_listener(w->d->xdg_surf, &xdg_listener, w);

	w->d->toplevel = xdg_surface_get_toplevel(w->d->xdg_surf);
	xdg_toplevel_set_title(w->d->toplevel, title);
	xdg_toplevel_add_listener(w->d->toplevel, &toplevel_listener, w);

	wl_surface_commit(w->d->surface);
	wl_display_roundtrip(w->d->display);

	/* linux-specific */
	w->d->shmfd = open(".", O_TMPFILE | O_RDWR | O_EXCL | O_CLOEXEC, 0600);
	w->d->shmsize = rectw(w->r) * 4 * recth(w->r) * 2;
	ftruncate(w->d->shmfd, w->d->shmsize);
	w->d->pool = wl_shm_create_pool(w->d->shm, w->d->shmfd, w->d->shmsize);

	w->fb.data = NULL;
	w->d->buffer = NULL;
	doresize(w);

	Event e = { .kind = Eframe };
	winaddevent(w, e);

	return w;
}

void
winclose(Win *w)
{
	wl_buffer_destroy(w->d->buffer);
	close(w->d->shmfd);
	wl_shm_pool_destroy(w->d->pool);
	xdg_toplevel_destroy(w->d->toplevel);
	xdg_surface_destroy(w->d->xdg_surf);
	wl_surface_destroy(w->d->surface);
	wl_display_disconnect(w->d->display);
	xkb_context_unref(w->d->kbctx);
}

void
winflush(Win *w)
{
	if (w->fb.damaged) {
		while (!w->d->canrender)
			wl_display_dispatch(w->d->display);
		wl_surface_attach(w->d->surface, w->d->buffer, 0, 0);
		wl_surface_damage(w->d->surface, w->fb.damage.min.x, w->fb.damage.min.y, w->fb.damage.max.x, w->fb.damage.max.y);
		wl_surface_commit(w->d->surface);
		w->fb.damaged = 0;
		struct wl_buffer *b = w->d->buffer;
		w->d->buffer = w->d->backbuffer;
		w->d->backbuffer = b;
		w->fb.data = w->d->backdata;
		w->d->backdata = w->d->data;
		w->d->data = w->fb.data;
	}
	doresize(w);
}

int
winpollfd(Win *w)
{
	return wl_display_get_fd(w->d->display);
}

void
winaddevent(Win *w, Event e)
{
	w->d->ebuf[w->d->epos++] = e;
	w->d->epos %= 32;
	w->d->elen++;
	if (w->d->elen >= 32) {
		w->d->elen = 32;
		w->d->ecur = w->d->epos;
	}
}

Event
winevent(Win *w)
{
	while (!w->d->elen)
		wl_display_dispatch(w->d->display);
	Event e = w->d->ebuf[w->d->ecur++];
	w->d->ecur %= 32;
	w->d->elen--;
	return e;
}

int
winkeytext(Event e, char *buf, int len)
{
	return xkb_keysym_to_utf8(e.key.key, buf, len);
}

static void
bufdone(void *w_, struct wl_buffer *buf)
{
	Win *w = w_;
	w->d->canrender = 1;
}

static void
newglobal(void *w_, struct wl_registry *reg, uint32_t name, char const *interface, uint32_t ver)
{
	Win *w = w_;
	if (!strcmp(interface, "wl_compositor")) {
		w->d->compositor = wl_registry_bind(reg, name, &wl_compositor_interface, ver);
	} else if (!strcmp(interface, "wl_shm")) {
		w->d->shm = wl_registry_bind(reg, name, &wl_shm_interface, ver);
	} else if (!strcmp(interface, "wl_seat")) {
		w->d->seat = wl_registry_bind(reg, name, &wl_seat_interface, ver);
		wl_seat_add_listener(w->d->seat, &seat_listener, w);
	} else if (!strcmp(interface, "xdg_wm_base")) {
		w->d->wm = wl_registry_bind(reg, name, &xdg_wm_base_interface, ver);
		xdg_wm_base_add_listener(w->d->wm, &wm_listener, w);
	}
}

static void
delglobal(void *w, struct wl_registry *reg, uint32_t name)
{
	return;
}

static void
inputkinds(void *w_, struct wl_seat *seat, uint32_t kinds)
{
	Win *w = w_;
	if (kinds & WL_SEAT_CAPABILITY_POINTER) {
		w->d->pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(w->d->pointer, &pointer_listener, w);
	} else {
		if (w->d->pointer)
			wl_pointer_destroy(w->d->pointer);
		w->d->pointer = NULL;
	}

	if (kinds & WL_SEAT_CAPABILITY_KEYBOARD) {
		w->d->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(w->d->keyboard, &kb_listener, w);
	} else {
		if (w->d->keyboard)
			wl_keyboard_destroy(w->d->keyboard);
		w->d->keyboard = NULL;
	}
}

static void
inputname(void *w, struct wl_seat *seat, char const *name)
{
	return;
}

static void
menter(void *w_, struct wl_pointer *m, uint32_t serial, struct wl_surface *surf, wl_fixed_t x, wl_fixed_t y)
{
	Win *w = w_;
	Event e = { .kind = Eenter };
	e.pos = Point(wl_fixed_to_int(x), wl_fixed_to_int(y));
	winaddevent(w, e);
}

static void
mleave(void *w_, struct wl_pointer *m, uint32_t serial, struct wl_surface *surf)
{
	Win *w = w_;
	Event e = { .kind = Eleave };
	winaddevent(w, e);
}

static void
mmove(void *w_, struct wl_pointer *m, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	Win *w = w_;
	Event e = { .kind = Epointer, .time = time };
	e.pos = Point(wl_fixed_to_int(x), wl_fixed_to_int(y));
	winaddevent(w, e);
}

static void
mbutton(void *w_, struct wl_pointer *m, uint32_t serial, uint32_t time, uint32_t button, uint32_t pressed)
{
	Win *w = w_;
	Event e = { .kind = Ebutton, .time = time };
	e.key.press = pressed;
	switch (button) {
	case 0x110:
		e.key.key = Bleft;
		break;
	case 0x111:
		e.key.key = Bright;
		break;
	case 0x112:
		e.key.key = Bmiddle;
		break;
	}
	winaddevent(w, e);
}

static void
mscroll(void *w_, struct wl_pointer *m, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	Win *w = w_;
	Event e = { .kind = Escroll, .time = time };
	if (axis == 1)
		e.pos.x = wl_fixed_to_int(value);
	else if (axis == 0)
		e.pos.y = wl_fixed_to_int(value);
	winaddevent(w, e);
}

static void
mend(void *w_, struct wl_pointer *m)
{
	return;
}

static void
mscrolltype(void *w, struct wl_pointer *m, uint32_t type)
{
	return;
}

static void
minertia(void *w, struct wl_pointer *m, uint32_t time, uint32_t axis)
{
	return;
}

static void
mclicks(void *w, struct wl_pointer *m, uint32_t axis, int32_t clicks)
{
	return;
}

static void
kbmap(void *w_, struct wl_keyboard *kb, uint32_t format, int32_t fd, uint32_t size)
{
	Win *w = w_;
	if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		char *map = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

		xkb_keymap_unref(w->d->kbmap);
		w->d->kbmap = xkb_keymap_new_from_string(w->d->kbctx, map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

		munmap(map, size);
		close(fd);

		xkb_state_unref(w->d->kbstate);
		w->d->kbstate = xkb_state_new(w->d->kbmap);
	}
}

static void
kbenter(void *w_, struct wl_keyboard *kb, uint32_t serial, struct wl_surface *surf, struct wl_array *keys)
{
	Win *w = w_;
	uint32_t *k;
	Event e = { .kind = Ekey, .key = { .press = 1 } };
	wl_array_for_each(k, keys) {
		e.key.key = xkb_state_key_get_one_sym(w->d->kbstate, *k + 8);
		winaddevent(w, e);
	}
}

static void
kbleave(void *w_, struct wl_keyboard *kb, uint32_t serial, struct wl_surface *surf)
{
	return;
}

static void
kbkey(void *w_, struct wl_keyboard *kb, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	Win *w = w_;
	Event e = { .kind = Ekey, .time = time };
	e.key.press = state;
	e.key.key = xkb_state_key_get_one_sym(w->d->kbstate, key + 8);
	winaddevent(w, e);
}

static void
kbmod(void *w_, struct wl_keyboard *kb, uint32_t serial, uint32_t pressed, uint32_t latched, uint32_t locked, uint32_t group)
{
	Win *w = w_;
	xkb_state_update_mask(w->d->kbstate, pressed, latched, locked, 0, 0, group);
}

static void
kbrepeat(void *w_, struct wl_keyboard *kb, int32_t rate, int32_t delay)
{
	return;
}


static void
dopong(void *w, struct xdg_wm_base *wm, uint32_t serial)
{
	xdg_wm_base_pong(wm, serial);
}

static void
setconfig(void *w_, struct xdg_surface *xdg, uint32_t serial)
{
	Win *w = w_;
	w->d->config = 1;
	w->d->configserial = serial;
}

static void
winconfig(void *w_, struct xdg_toplevel *toplevel, int32_t width, int32_t height, struct wl_array *states)
{
	Win *w = w_;
	if (width || height) {
		if (width != rectw(w->r) || height != recth(w->r)) {
			Event e = { .kind = Eframe | Eresized };
			winaddevent(w, e);
		}
		w->r = Rect(Point(0, 0), Point(width, height));
	}
}

static void
winclosed(void *w_, struct xdg_toplevel *toplevel)
{
	Win *w = w_;
	Event e = { .kind = Eclosed };
	winaddevent(w, e);
}

