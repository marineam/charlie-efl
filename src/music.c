#include "main.h"

static Evas_Object *music = NULL;
static Evas_Object *playlist = NULL;

void music_show()
{
	if (music == NULL) {
		music = edje_object_add(evas);
		edje_object_file_set(music, theme, "music");
	}

	if (playlist == NULL) {
		playlist = e_box_add(evas);
		e_box_orientation_set(playlist, 0);
		edje_object_part_swallow(music, "playlist", playlist);
		evas_object_show(playlist);
	}

	
	layout_swallow("main_content", music);
	evas_object_show(music);
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

void music_song_add(const char *label)
{
	Evas_Object *song;

	song = edje_object_add(evas);
	edje_object_file_set(song, theme, "playlist_item");
	edje_object_part_text_set(song, "label", label);
	edje_object_signal_callback_add(song, "mouse,clicked,1", "*", _music_song_signal, playlist);
	e_box_pack_end(playlist, song);

	e_box_pack_options_set(song,
			       1, 1, /* fill */
			       1, 1, /* expand */
			       0.5, 0.0, /* align */
			       -1, -1, /* min */
			       -1, 50); /* max */
	evas_object_show(song);
}
