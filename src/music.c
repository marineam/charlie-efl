#include "main.h"

static Evas_Object *music;
static Evas_Object *slider;
static Evas_Object *scroller;
static Evas_Object *playlist;
static Ecore_Timer *playlist_scroll_timer;
static int playlist_scroll_top = 0;
static double playlist_scroll_align = 0.0;
static double click_time;
static int playpause_playing = 0;

static int _music_scroll(void *data);
static void _music_signal(void *data, Evas_Object *obj,
		const char *signal, const char *source);
static void _music_song_signal(void *data, Evas_Object *obj,
		const char *signal, const char *source);
static void _music_list_active(Evas_Object *box, Evas_Object *button);
static void _music_song_free(Evas_Object *song);
static void _music_scroller_set(double scroll);

void music_init()
{
	music = edje_object_add(evas);
	edje_object_file_set(music, theme, "music");
	edje_object_signal_callback_add(music, "mouse,clicked,1", "*", _music_signal, NULL);

	playlist = e_box_add(evas);
	edje_object_part_swallow(music, "list", playlist);
	e_box_align_set(playlist, 0.0, playlist_scroll_align);
	evas_object_show(playlist);

	slider = evas_object_rectangle_add(evas);
	evas_object_color_set(slider, 10, 207, 233, 50);

	//scroller = edje_object_add(evas);
	//edje_object_file_set(scroller, theme, "button");
	
	scroller = evas_object_rectangle_add(evas);
	evas_object_color_set(scroller, 10, 207, 233, 50);
}

void music_show()
{
	layout_swallow("main_content", music);
	evas_object_show(music);

	music_slider_set(0.0);
	evas_object_show(slider);
	
	_music_scroller_set(0.0);
	evas_object_show(scroller);
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

void music_playing(int state) {
	if (state == playpause_playing)
		return;

	playpause_playing = state;
	if (state)
		edje_object_signal_emit(music, "play", "");
	else
		edje_object_signal_emit(music, "pause", "");
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
	mpd_freeSong(evas_object_data_get(song, "song_data"));
	evas_object_del(song);
}

void music_slider_set(double progress)
{
	Evas_Object *area;
	Evas_Coord slide_x, slide_y, slide_w, slide_h;

	area = edje_object_part_object_get(music, "slider");
	evas_object_geometry_get(area, &slide_x, &slide_y, &slide_w, &slide_h);

	evas_object_resize(slider, slide_h * 0.2, slide_h * 0.5);
	evas_object_move(slider, slide_x - slide_h * 0.1 +
		slide_w * progress, slide_y + slide_h * .25);
}

static void _music_scroller_set(double scroll)
{
	Evas_Object *area;
	Evas_Coord slide_x, slide_y, slide_w, slide_h;

	area = edje_object_part_object_get(music, "scroller");
	evas_object_geometry_get(area, &slide_x, &slide_y, &slide_w, &slide_h);

	evas_object_resize(scroller, slide_w, slide_w);
	evas_object_move(scroller, slide_x, slide_h * scroll + slide_y);
}

void music_song_add(mpd_Song *data)
{
	Evas_Object *song;
	Evas_Object *tmp;
	Evas_Coord height;
	int len = e_box_pack_count_get(playlist);

	song = edje_object_add(evas);
	edje_object_file_set(song, theme, "list_item");
	edje_object_size_min_calc(song, NULL, &height);
	edje_object_signal_callback_add(song, "mouse,clicked,1", "*",
			_music_song_signal, playlist);

	evas_object_data_set(song, "song_data", data);
	if (data->title)
		edje_object_part_text_set(song, "title", data->title);
	else
		edje_object_part_text_set(song, "title", data->file);

	if (data->artist && data->album) {
		int len = strlen(data->artist) + strlen(data->album) + 9;
		char label[len];

		snprintf(label, len, "by %s, on %s",
			data->artist, data->album);
		edje_object_part_text_set(song, "artist", label);
	}
	else if (data->artist) {
		int len = strlen(data->artist) + 4;
		char label[len];

		snprintf(label, len, "by %s", data->artist);
		edje_object_part_text_set(song, "artist", label);
	}
	else
		edje_object_part_text_set(song, "artist", "");

	if (data->pos > len) {
		fprintf(stderr, "%s: tried to insert past end of list!\n",
				__func__);
		e_box_pack_end(playlist, song);
	} else if (data->pos < 0 || data->pos == len ) {
		e_box_pack_end(playlist, song);
	} else {
		tmp = e_box_pack_object_nth(playlist, data->pos);
		e_box_pack_before(playlist, song, tmp);
		e_box_unpack(tmp);
		_music_song_free(tmp);
	}

	e_box_pack_options_set(song,
			       1, 0, /* fill */
			       1, 0, /* expand */
			       0.5, 0.5, /* align */
			       -1, height, /* min */
			       -1, height); /* max */

	evas_object_show(song);
}

void music_playlist_autoscroll(int pos, int align)
{
	double time = ecore_time_get();

	if (click_time + 30.0 < time)
		music_playlist_scroll(pos, align, 0);
	else if (click_time + 15.0 < time)
		music_playlist_scroll(pos, 0, 0);
}	

/* pos: song position in list align is relative to
 * align: position in screen pos should be,
 * 	negative counts up from bottom
 * 	0 is automagical
 */
void music_playlist_scroll(int pos, int align, int force)
{
	int total = e_box_pack_count_get(playlist);
	double old_align = playlist_scroll_align, view;
	Evas_Coord listh, boxh;

	if (pos < 0 || pos >= total || total <= 0)
		return;

	/* get the size of the playlist and parent object */
	e_box_min_size_get(playlist, NULL, &boxh);
	edje_object_part_geometry_get(music, "list", NULL, NULL, NULL, &listh);

	view = (double)listh / (boxh/total);

	if (align < 0)
		align = align + view;
	else if (align == 0) {
		if (playlist_scroll_top >= pos)
			align = 1;
		else if (playlist_scroll_top + (int)view - 1 <= pos)
			align = view - 2;
		else
			return;
	} else
		align--;

	if ((int)view >= total) {
		playlist_scroll_top = 0;
		playlist_scroll_align = 0.0;
	}
	else {
		playlist_scroll_top = pos-align;

		if (playlist_scroll_top < 0)
			playlist_scroll_top = 0;
		if (playlist_scroll_top > total - (int)(view))
			playlist_scroll_top = total - view + .95;
	
		playlist_scroll_align =
			1.0 - ((double)playlist_scroll_top / (total - view));
	}
	
	_music_scroller_set((double)playlist_scroll_top / total);

	if (force)
		e_box_align_set(playlist, 0.0, playlist_scroll_align);
	else if (!playlist_scroll_timer && old_align != playlist_scroll_align)
		playlist_scroll_timer =
			ecore_timer_add(1.0 / 30.0, _music_scroll, NULL);
}

	



static void _music_signal(void *data, Evas_Object *obj, const char *signal, const char *source)
{
	click_time = ecore_time_get();

	if (!strcmp("scroller", source)) {
		Evas_Coord mouse_y, scroll_y, scroll_w, scroll_h;
		Evas_Object *area;
		double scroll;
		int total;
		
		evas_pointer_canvas_xy_get(evas, NULL, &mouse_y);
		area = edje_object_part_object_get(music, "scroller");
		total = e_box_pack_count_get(playlist);
		evas_object_geometry_get(area, NULL, &scroll_y, &scroll_w, &scroll_h);
		
		scroll = (double)(mouse_y - scroll_y - scroll_w/2) / scroll_h;

		music_playlist_scroll(scroll * total, 1, 0);
	}
	else if (!strcmp("playpause", source) && playpause_playing) {
		mpdclient_pause(1);
		music_playing(0);
	}
	else if (!strcmp("playpause", source)) {
		mpdclient_pause(0);
		music_playing(1);
	}
}

static int _music_scroll(void *data) {
	double diff, curr;

	e_box_align_get(playlist, NULL, &curr);

	diff = playlist_scroll_align - curr;

	if (-0.001 < diff && diff < 0.001) {
		e_box_align_set(playlist, 0.0, playlist_scroll_align);
		playlist_scroll_timer = NULL;
		return 0;
	}
	else
		e_box_align_set(playlist, 0.0, curr * 0.7 + playlist_scroll_align * 0.3);

	return 1;
}

static void _music_song_signal(void *data, Evas_Object *obj, const char *signal, const char *source) {
	mpd_Song *songd;
	
	click_time = ecore_time_get();
	
	_music_list_active((Evas_Object*)data, obj);
	if ((songd = evas_object_data_get(obj, "song_data"))) {
		music_playlist_scroll(songd->pos, 0, 0);
		mpdclient_song_play(songd->pos);
	}

}

