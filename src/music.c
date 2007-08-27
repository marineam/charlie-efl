#include "main.h"

static Evas_Object *music = NULL;
static Evas_Object *playlist = NULL;
static Ecore_Timer *playlist_scroll_timer = NULL;
static double playlist_scroll_align = 0.5;

static int _music_scroll(void *data);
static void _music_signal(void *data, Evas_Object *obj, const char *signal, const char *source);
static void _music_song_signal(void *data, Evas_Object *obj, const char *signal, const char *source);

void music_init()
{
	music = edje_object_add(evas);
	edje_object_file_set(music, theme, "music");
	edje_object_signal_callback_add(music, "mouse,clicked,1", "*", _music_signal, NULL);

	playlist = e_box_add(evas);
	edje_object_part_swallow(music, "list", playlist);
	evas_object_show(playlist);
}

void music_show()
{
	layout_swallow("main_content", music);
	evas_object_show(music);
}

void music_song_add(const char *label)
{
	Evas_Object *song;

	song = edje_object_add(evas);
	edje_object_file_set(song, theme, "list_item");
	edje_object_part_text_set(song, "label", label);
	edje_object_signal_callback_add(song, "mouse,clicked,1", "*", _music_song_signal, playlist);
	e_box_pack_end(playlist, song);

	e_box_pack_options_set(song,
			       1, 0, /* fill */
			       1, 0, /* expand */
			       0.5, 0.5, /* align */
			       -1, 40, /* min */
			       -1, 40); /* max */
	evas_object_show(song);
}

static void _music_signal(void *data, Evas_Object *obj, const char *signal, const char *source) {

	if (!strcmp("up", source)) {
		edje_object_signal_emit(obj, "up,on", "");
		playlist_scroll_align += 0.05;
		if (!playlist_scroll_timer)
			playlist_scroll_timer = ecore_timer_add(1.0 / 30.0, _music_scroll, NULL);
	}
	else if (!strcmp("down", source)) {
		edje_object_signal_emit(obj, "down,on", "");
		playlist_scroll_align -= 0.05;
		if (!playlist_scroll_timer)
			playlist_scroll_timer = ecore_timer_add(1.0 / 30.0, _music_scroll, NULL);
	}
}

static int _music_scroll(void *data) {
	double diff, curr;

	e_box_align_get(playlist, NULL, &curr);

	diff = playlist_scroll_align - curr;

	if (-0.001 < diff && diff < 0.001) {
		e_box_align_set(playlist, 0.0, playlist_scroll_align);
		playlist_scroll_timer = NULL;
		edje_object_signal_emit(music, "up,off", "");
		edje_object_signal_emit(music, "down,off", "");
		return 0;
	}
	else
		e_box_align_set(playlist, 0.0, curr * 0.7 + playlist_scroll_align * 0.3);

	return 1;
}

static void _music_song_signal(void *data, Evas_Object *obj, const char *signal, const char *source) {
	Evas_Object *box = (Evas_Object *)data, *button;
	int n, count;

	count = e_box_pack_count_get(box);
	for (n = 0; n < count; n++) {
		button = e_box_pack_object_nth(box, n);
		if (obj != button)
			edje_object_signal_emit(button, "button,off", "");
	}

	edje_object_signal_emit(obj, "button,on", ""); 
}

