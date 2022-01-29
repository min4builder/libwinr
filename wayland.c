#define _GNU_SOURCE /* ugh */
#include <fcntl.h>
#include <linux/input-event-codes.h> /* BTN_* constants */
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include "xdg-shell-client-protocol.h"
#include "draw.h"
#include "win.h"

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
	if (!w->config)
		return;

	if (w->buffer)
		wl_buffer_destroy(w->buffer);

	unsigned size = rectw(w->r) * 4 * recth(w->r) * 2;
	if (size > w->shmsize) {
		ftruncate(w->shmfd, size);
		wl_shm_pool_resize(w->pool, size);
		w->shmsize = size;
	}

	if (w->data) {
		if (w->data < w->backdata)
			munmap(w->data, w->fb.s * recth(w->fb.r) * 2);
		else
			munmap(w->backdata, w->fb.s * recth(w->fb.r) * 2);
	}

	w->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, w->shmfd, 0);
	w->backdata = (char *) w->data + size/2;

	w->fb.data = w->data;
	w->fb.r = w->r;
	w->fb.s = rectw(w->fb.r) * 4;

	w->buffer = wl_shm_pool_create_buffer(w->pool, 0, rectw(w->fb.r), recth(w->fb.r), w->fb.s, WL_SHM_FORMAT_ARGB8888);
	w->backbuffer = wl_shm_pool_create_buffer(w->pool, size/2, rectw(w->fb.r), recth(w->fb.r), w->fb.s, WL_SHM_FORMAT_ARGB8888);
	wl_buffer_add_listener(w->buffer, &buf_listener, w);

	xdg_surface_ack_configure(w->xdg_surf, w->configserial);
	w->config = 0;
	w->canrender = 1;
}

void
winopen(Win *w)
{
	w->kbctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	w->display = wl_display_connect(NULL);
	w->registry = wl_display_get_registry(w->display);
	wl_registry_add_listener(w->registry, &reg_listener, w);

	wl_display_roundtrip(w->display);

	w->surface = wl_compositor_create_surface(w->compositor);
	w->xdg_surf = xdg_wm_base_get_xdg_surface(w->wm, w->surface);
	xdg_surface_add_listener(w->xdg_surf, &xdg_listener, w);

	w->toplevel = xdg_surface_get_toplevel(w->xdg_surf);
	xdg_toplevel_set_title(w->toplevel, "Test");
	xdg_toplevel_add_listener(w->toplevel, &toplevel_listener, w);

	wl_surface_commit(w->surface);
	wl_display_roundtrip(w->display);

	/* linux-specific */
	w->shmfd = open(".", O_TMPFILE | O_RDWR | O_EXCL | O_CLOEXEC, 0600);
	w->shmsize = rectw(w->r) * 4 * recth(w->r) * 2;
	ftruncate(w->shmfd, w->shmsize);
	w->pool = wl_shm_create_pool(w->shm, w->shmfd, w->shmsize);

	w->fb.data = NULL;
	w->buffer = NULL;
	doresize(w);
}

void
winclose(Win *w)
{
	wl_buffer_destroy(w->buffer);
	close(w->shmfd);
	wl_shm_pool_destroy(w->pool);
	xdg_toplevel_destroy(w->toplevel);
	xdg_surface_destroy(w->xdg_surf);
	wl_surface_destroy(w->surface);
	wl_display_disconnect(w->display);
	xkb_context_unref(w->kbctx);
}

void
winflush(Win *w, Ekind wait)
{
	w->ev = Enone;
	while (!w->canrender)
		wl_display_dispatch(w->display);
	if (w->fb.damaged) {
		wl_surface_attach(w->surface, w->buffer, 0, 0);
		wl_surface_damage(w->surface, w->fb.damage.min.x, w->fb.damage.min.y, w->fb.damage.max.x, w->fb.damage.max.y);
		wl_surface_commit(w->surface);
		w->fb.damaged = 0;
		struct wl_buffer *b = w->buffer;
		w->buffer = w->backbuffer;
		w->backbuffer = b;
		w->fb.data = w->backdata;
		w->backdata = w->data;
		w->data = w->fb.data;
	}
	while (!((wait | Eclosed | Eframe) & w->ev))
		wl_display_dispatch(w->display);
	doresize(w);
}

void
winaddkeypress(Win *w, Keypress kp)
{
	w->kbuf[w->kpos++] = kp;
	w->kpos %= 32;
	w->klen++;
	if (w->klen >= 32) {
		w->klen = 32;
		w->kcur = w->kpos;
	}
}

Keypress
winkeypress(Win *w)
{
	if (!w->klen)
		return (Keypress) { 0 };
	Keypress kp = w->kbuf[w->kcur++];
	w->kcur %= 32;
	w->klen--;
	return kp;
}

int
winkeytext(Keypress kp, char *buf, int len)
{
	return xkb_keysym_to_utf8(kp.key, buf, len);
}

static void
bufdone(void *w_, struct wl_buffer *buf)
{
	Win *w = w_;
	w->canrender = 1;
}

static void
newglobal(void *w_, struct wl_registry *reg, uint32_t name, char const *interface, uint32_t ver)
{
	Win *w = w_;
	if (!strcmp(interface, "wl_compositor")) {
		w->compositor = wl_registry_bind(reg, name, &wl_compositor_interface, ver);
	} else if (!strcmp(interface, "wl_shm")) {
		w->shm = wl_registry_bind(reg, name, &wl_shm_interface, ver);
	} else if (!strcmp(interface, "wl_seat")) {
		w->seat = wl_registry_bind(reg, name, &wl_seat_interface, ver);
		wl_seat_add_listener(w->seat, &seat_listener, w);
	} else if (!strcmp(interface, "xdg_wm_base")) {
		w->wm = wl_registry_bind(reg, name, &xdg_wm_base_interface, ver);
		xdg_wm_base_add_listener(w->wm, &wm_listener, w);
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
		w->pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(w->pointer, &pointer_listener, w);
	} else {
		if (w->pointer)
			wl_pointer_destroy(w->pointer);
		w->pointer = NULL;
	}

	if (kinds & WL_SEAT_CAPABILITY_KEYBOARD) {
		w->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(w->keyboard, &kb_listener, w);
	} else {
		if (w->keyboard)
			wl_pointer_destroy(w->pointer);
		w->keyboard = NULL;
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
	w->bufmouse = Point(wl_fixed_to_int(x), wl_fixed_to_int(y));
	w->bufev |= Eenter | Epointer;
}

static void
mleave(void *w_, struct wl_pointer *m, uint32_t serial, struct wl_surface *surf)
{
	Win *w = w_;
	w->bufev |= Eleave;
}

static void
mmove(void *w_, struct wl_pointer *m, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	Win *w = w_;
	w->bufmouse = Point(wl_fixed_to_int(x), wl_fixed_to_int(y));
	w->bufev |= Epointer;
}

static void
mbutton(void *w_, struct wl_pointer *m, uint32_t serial, uint32_t time, uint32_t button, uint32_t pressed)
{
	Win *w = w_;
	switch (button) {
	case BTN_LEFT:
		if (pressed)
			w->bufbut |= Bleft;
		else
			w->bufbut &= ~Bleft;
		w->bufev |= Ebutton;
		break;
	case BTN_RIGHT:
		if (pressed)
			w->bufbut |= Bright;
		else
			w->bufbut &= ~Bright;
		w->bufev |= Ebutton;
		break;
	case BTN_MIDDLE:
		if (pressed)
			w->bufbut |= Bmiddle;
		else
			w->bufbut &= ~Bmiddle;
		w->bufev |= Ebutton;
		break;
	}
}

static void
mscroll(void *w_, struct wl_pointer *m, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	Win *w = w_;
	Point s = { 0, 0 };
	if (axis == 1)
		s.x = wl_fixed_to_int(value);
	else if (axis == 0)
		s.y = wl_fixed_to_int(value);
	w->bufscroll = pointadd(w->bufscroll, s);
	w->bufev |= Escroll;
}

static void
mend(void *w_, struct wl_pointer *m)
{
	Win *w = w_;
	w->mouse = w->bufmouse;
	w->scroll = pointadd(w->scroll, w->bufscroll);
	w->bufscroll = Point(0, 0);
	w->ev |= w->bufev;
	w->bufev = Enone;
	w->obut = w->but;
	w->but = w->bufbut;
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

		xkb_keymap_unref(w->kbmap);
		w->kbmap = xkb_keymap_new_from_string(w->kbctx, map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

		munmap(map, size);
		close(fd);

		xkb_state_unref(w->kbstate);
		w->kbstate = xkb_state_new(w->kbmap);
	}
}

static void
kbenter(void *w_, struct wl_keyboard *kb, uint32_t serial, struct wl_surface *surf, struct wl_array *keys)
{
	Win *w = w_;
	uint32_t *k;
	Keypress kp = { .pressed = 1, .time = 0 };
	wl_array_for_each(k, keys) {
		kp.key = xkb_state_key_get_one_sym(w->kbstate, *k + 8);
		winaddkeypress(w, kp);
		w->ev |= Ekey;
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
	Keypress kp = { .pressed = state, .time = time };
	kp.key = xkb_state_key_get_one_sym(w->kbstate, key + 8);
	winaddkeypress(w, kp);
	w->ev |= Ekey;
}

static void
kbmod(void *w_, struct wl_keyboard *kb, uint32_t serial, uint32_t pressed, uint32_t latched, uint32_t locked, uint32_t group)
{
	Win *w = w_;
	xkb_state_update_mask(w->kbstate, pressed, latched, locked, 0, 0, group);
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
	w->config = 1;
	w->configserial = serial;
}

static void
winconfig(void *w_, struct xdg_toplevel *toplevel, int32_t width, int32_t height, struct wl_array *states)
{
	Win *w = w_;
	if (width || height) {
		if (width != rectw(w->r) || height != recth(w->r))
			w->ev |= Eframe | Eresized;
		w->r = Rect(Point(0, 0), Point(width, height));
	}
}

static void
winclosed(void *w_, struct xdg_toplevel *toplevel)
{
	Win *w = w_;
	w->ev |= Eclosed;
}

