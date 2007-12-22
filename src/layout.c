#include "main.h"

static Evas_Object *layout;
static Evas_Object *volume;
int playpause_playing;
float cur_vol;

static void layout_signal(void *data, Evas_Object *obj, const char *signal, const char *source);
static void volume_signal(void *data, Evas *evas, Evas_Object *obj, void *event_info);

/* create the screen layout - edje defines it with swallow regions and anything
 else the theme wants to do */
void layout_init(void)
{
	layout = edje_object_add(evas);
	edje_object_file_set(layout, theme, "layout");
	evas_object_move(layout, 0, 0);
	layout_resize();
	edje_object_signal_callback_add(layout, "mouse,clicked,1", "*", layout_signal, NULL);

	volume = evas_object_rectangle_add(evas);
	evas_object_color_set(volume, 10, 207, 233, 50);
	evas_object_event_callback_add(volume,
		EVAS_CALLBACK_MOUSE_DOWN, volume_signal, NULL);

	evas_object_show(layout);
	evas_object_show(volume);
}

/* handle a window/screen resize */
void layout_resize(void)
{
	Evas_Coord w, h;

	evas_output_viewport_get(evas, NULL, NULL, &w, &h);
	evas_object_resize(layout, w, h);
}

/* swallow an object into a named layout location */
void layout_swallow(char *position, Evas_Object * obj)
{
	edje_object_part_swallow(layout, position, obj);
}

static void layout_vol_update(int new_vol)
{
	Evas_Coord x, y, w, h;
	float vol = (float)new_vol/100.0;

	cur_vol = vol;
	edje_object_part_geometry_get(layout, "volume", &x, &y, &w, &h);
	y = y + (h - vol*h);
	h = vol*h;
	x += PAD;
	y += PAD;
	w -= 2*PAD;
	h -= 2*PAD;
	evas_object_move(volume, x, y);
	evas_object_resize(volume, w, h);
}

void layout_update(int playing, int vol)
{
	if (playing != playpause_playing) {
		playpause_playing = playing;
		if (playing)
			edje_object_signal_emit(layout, "play", "");
		else
			edje_object_signal_emit(layout, "pause", "");
	}

	if (vol != cur_vol) {
		layout_vol_update(vol);
	}
}

static void layout_vol_change()
{
	Evas_Coord mouse_y, vol_h, vol_y;
	int vol;

	evas_pointer_canvas_xy_get(evas, NULL, &mouse_y);
	edje_object_part_geometry_get(layout, "volume",
		NULL, &vol_y, NULL, &vol_h);
	vol_y += 2*PAD;
	vol_h -= 2*PAD;

	vol = ((float)(vol_h - (mouse_y - vol_y)) / vol_h) * 100;
	mpdclient_volume(vol);
	layout_vol_update(vol);
}

static void layout_signal(void *data, Evas_Object *obj, const char *signal, const char *source)
{
	click_time = ecore_time_get();

	if (!strcmp("playpause", source) && playpause_playing) {
		mpdclient_pause(1);
		edje_object_signal_emit(layout, "pause", "");
	}
	else if (!strcmp("playpause", source)) {
		mpdclient_pause(0);
		edje_object_signal_emit(layout, "play", "");
	}
	else if (!strcmp("volume", source)) {
		layout_vol_change();
	}
}

static void volume_signal(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	layout_vol_change();
}
