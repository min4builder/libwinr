struct Display {
	int elen;
	Event ebuf[32];
	int epos, ecur;

	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_seat *seat;
	struct wl_shm *shm;
	struct xdg_wm_base *wm;

	struct wl_surface *surface;
	struct xdg_surface *xdg_surf;
	struct xdg_toplevel *toplevel;
	int config;
	unsigned configserial;

	struct wl_pointer *pointer;
	struct wl_keyboard *keyboard;
	struct xkb_state *kbstate;
	struct xkb_context *kbctx;
	struct xkb_keymap *kbmap;

	int shmfd;
	unsigned shmsize;
	struct wl_shm_pool *pool;
	struct wl_buffer *buffer, *backbuffer;
	void *data, *backdata;
	int canrender;
};

