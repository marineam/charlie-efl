#include "main.h"

static Evas_Object *music = NULL;
static Evas_Object *playlist = NULL;
static Ecore_Timer *playlist_scroll_timer = NULL;
static int playlist_scroll_top = 0;
static double playlist_scroll_align = 1.0;
static double click_time;

struct song_data {
	int pos;
};

static int _music_scroll(void *data);
static void _music_signal(void *data, Evas_Object *obj,
		const char *signal, const char *source);
static void _music_song_signal(void *data, Evas_Object *obj,
		const char *signal, const char *source);
static void _music_song_insert_replace(const char *label, int pos, int replace);
static void _music_list_active(Evas_Object *box, Evas_Object *button);
static void _music_song_free(Evas_Object *song);

void music_init()
{
	music = edje_object_add(evas);
	edje_object_file_set(music, theme, "music");
	edje_object_signal_callback_add(music, "mouse,clicked,1", "*", _music_signal, NULL);

	playlist = e_box_add(evas);
	edje_object_part_swallow(music, "list", playlist);
	e_box_align_set(playlist, 0.0, playlist_scroll_align);
	evas_object_show(playlist);
}

void music_show()
{
	layout_swallow("main_content", music);
	evas_object_show(music);
}

void music_song_add(const char *label)
{
	_music_song_insert_replace(label, -1, 0);
}

void music_song_insert(const char *label, int pos) {
	_music_song_insert_replace(label, pos, 0);
}

void music_song_replace(const char *label, int pos) {
	_music_song_insert_replace(label, pos, 1);
}

void music_song_remove(int pos) {
	Evas_Object *tmp;
	tmp = e_box_pack_object_nth(playlist, pos);
	e_box_unpack(tmp);
	_music_song_free(tmp);
}

int music_song_count() {
	return e_box_pack_count_get(playlist);
}

/* -1 means no song is active */
void music_song_active(int pos) {
	Evas_Object *song;
	if (pos < 0)
		_music_list_active(playlist, NULL);
	else {
		song = e_box_pack_object_nth(playlist, pos);
		_music_list_active(playlist, song);
	}
}

static void _music_list_active(Evas_Object *box, Evas_Object *button) {
	Evas_Object *tmp;
	int n, count;

	count = e_box_pack_count_get(box);
	for (n = 0; n < count; n++) {
		tmp = e_box_pack_object_nth(box, n);
		if (tmp != button)
			edje_object_signal_emit(tmp, "button,off", "");
	}

	if (button)
		edje_object_signal_emit(button, "button,on", "");
}

static void _music_song_free(Evas_Object *song)
{
	evas_object_hide(song);
	free(evas_object_data_get(song, "song_data"));
	evas_object_del(song);
}

static void _music_song_insert_replace(const char *label, int pos, int replace) {
	struct song_data *data;
	Evas_Object *song;
	Evas_Object *tmp;
	Evas_Coord h;
	int len = e_box_pack_count_get(playlist);

	song = edje_object_add(evas);
	edje_object_file_set(song, theme, "list_item");
	edje_object_part_text_set(song, "label", label);
	edje_object_signal_callback_add(song, "mouse,clicked,1", "*", _music_song_signal, playlist);

	if ((data = malloc(sizeof(struct song_data)))) {
		data->pos = pos;
		evas_object_data_set(song, "song_data", data);
	} else {
		fprintf(stderr, "malloc failed!\n");
		exit(1);
	}

	if (pos > len) {
		fprintf(stderr, "music_song_insert: "
				"tried to insert past end of list!\n");
		e_box_pack_end(playlist, song);
	} else if (pos < 0 || pos == len ) {
		e_box_pack_end(playlist, song);
	} else {
		tmp = e_box_pack_object_nth(playlist, pos);
		e_box_pack_before(playlist, song, tmp);
		if (replace) {
			e_box_unpack(tmp);
			_music_song_free(tmp);
		}
	}

	edje_object_size_min_calc(song, NULL, &h);
	e_box_pack_options_set(song,
			       1, 0, /* fill */
			       1, 0, /* expand */
			       0.5, 0.5, /* align */
			       -1, h, /* min */
			       -1, h); /* max */

	evas_object_show(song);
}

void music_playlist_autoscroll(int pos, int align)
{
	double time = ecore_time_get();
	if (click_time + 30.0 < time)
		music_playlist_scroll(pos, align);
	else if (click_time + 5.0 < time)
		music_playlist_scroll(pos, 0);
}	

/* pos: song position in list align is relative to
 * align: position in screen pos should be,
 * 	negative counts up from bottom
 * 	0 is automagical
 */
void music_playlist_scroll(int pos, int align)
{
	int total_count = e_box_pack_count_get(playlist), view_count;
	double old_align = playlist_scroll_align;
	Evas_Object *tmp;
	Evas_Coord songh, songh2, listh;

	if (pos < 0 || pos >= total_count || total_count <= 0)
		return;

	/* get a item just to see how big it is
	 * check two because one might be slected (and thus bigger) */
	tmp = e_box_pack_object_nth(playlist, 0);
	edje_object_size_min_calc(tmp, NULL, &songh);
	tmp = e_box_pack_object_nth(playlist, 1);
	edje_object_size_min_calc(tmp, NULL, &songh2);
	if (songh > songh2)
		songh = songh2;

	/* get the size of the playlist's parent object */
	evas_object_geometry_get(music, NULL, NULL, NULL, &listh);
	/* FIXME: account for when list != music in height */

	view_count = ((songh/2) + listh) / songh;

	if (align < 0)
		align = align + view_count;
	else if (align == 0) {
		if (playlist_scroll_top >= pos)
			align = 1;
		else if (playlist_scroll_top+view_count-1 <= pos)
			align = view_count - 2;
		else
			return;
	} else
		align--;

	if (view_count >= total_count) {
		playlist_scroll_top = 0;
	}
	else {
		playlist_scroll_top = pos-align;
	}

	if (playlist_scroll_top < 0)
		playlist_scroll_top = 0;
	if (playlist_scroll_top > total_count-view_count)
		playlist_scroll_top = total_count-view_count;

	playlist_scroll_align =
			1.0 + 1.0/listh /*small pat at top */
			- ((double)(playlist_scroll_top) / total_count);

	if (!playlist_scroll_timer && old_align != playlist_scroll_align)
		playlist_scroll_timer =
			ecore_timer_add(1.0 / 30.0, _music_scroll, NULL);
}

	



static void _music_signal(void *data, Evas_Object *obj, const char *signal, const char *source) {

	if (!strcmp("up", source)) {
		edje_object_signal_emit(obj, "up,on", "");
		music_playlist_scroll(playlist_scroll_top-1, 1);
	}
	else if (!strcmp("down", source)) {
		edje_object_signal_emit(obj, "down,on", "");
		music_playlist_scroll(playlist_scroll_top+1, 1);
	}
}

static int _music_scroll(void *data) {
	double diff, curr;

	click_time = ecore_time_get();
	
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
	struct song_data *songd;
	
	click_time = ecore_time_get();
	
	_music_list_active((Evas_Object*)data, obj);
	if ((songd = evas_object_data_get(obj, "song_data"))) {
		music_playlist_scroll(songd->pos, 0);
		mpdclient_song_play(songd->pos);
	}

}

