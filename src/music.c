#include "main.h"

static Evas_Object *music = NULL;
static Evas_Object *playlist = NULL;

void music_init()
{
	music = edje_object_add(evas);
	edje_object_file_set(music, theme, "music");

	playlist = e_table_add(evas);
	edje_object_part_swallow(music, "list", playlist);
	evas_object_show(playlist);
}

void music_show()
{
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
	int col, row;

	song = edje_object_add(evas);
	edje_object_file_set(song, theme, "list_item");
	edje_object_part_text_set(song, "label", label);
	edje_object_signal_callback_add(song, "mouse,clicked,1", "*", _music_song_signal, playlist);
	e_table_col_row_size_get(playlist, &col, &row);
	e_table_pack(playlist, song, 0, row, 1, 1);

	e_table_pack_options_set(song,
			       1, 0, /* fill */
			       1, 0, /* expand */
			       0.5, 0.0, /* align */
			       -1, 45, /* min */
			       -1, 45); /* max */
	evas_object_show(song);
}
