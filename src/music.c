#include "main.h"

static Evas_Object *music;
static Evas_Object *slider;
static Evas_Object *scroller;
static Evas_Object *playlist;
//static Ecore_Timer *playlist_scroll_timer;
static Ecore_List  *full_playlist;
static int song_active = -99;
static int playlist_top = 0;
static double click_time;
static int playpause_playing = 0;

static void music_signal(void *data, Evas_Object *obj,
		const char *signal, const char *source);
static void music_song_signal(void *data, Evas_Object *obj,
		const char *signal, const char *source);

static void music_playlist_scroll(int top);
static void music_playlist_update(mpd_Song *data);
static void music_playlist_remove_nth(int pos);

void music_init()
{
	full_playlist = ecore_list_new();

	music = edje_object_add(evas);
	edje_object_file_set(music, theme, "music");
	edje_object_signal_callback_add(music, "mouse,clicked,1", "*", music_signal, NULL);

	playlist = e_box_add(evas);
	edje_object_part_swallow(music, "list", playlist);
	e_box_align_set(playlist, 0.0, 0.0);
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

	//_music_scroller_set(0.0);
	evas_object_show(scroller);
}

void music_song_insert(mpd_Song *song)
{
	mpd_Song *old, *new;

	/* FIXME: Workaround for bug triggerd by ecore_list_remove/insert
	 * somehow after the insert full_playlist->current and ->index
	 * are out of sync, index is off by -1 */
	ecore_list_first_goto(full_playlist);

	new = mpd_songDup(song);
	if (new->pos > ecore_list_count(full_playlist)) {
		fprintf(stderr, "%s: tried to insert past end of list!\n",
				__func__);
	} else if (new->pos < 0) {
		fprintf(stderr, "%s: position was negative!\n", __func__);
	} else if (new->pos == ecore_list_count(full_playlist)) {
		ecore_list_append(full_playlist, new);
		music_playlist_update(new);
	} else {
		ecore_list_index_goto(full_playlist, new->pos);
		old = ecore_list_remove(full_playlist);
		ecore_list_insert(full_playlist, new);
		music_playlist_update(new);
		mpd_freeSong(old);
	}
}

void music_song_remove(int pos)
{
	mpd_Song *song;

	/* FIXME: Workaround for bug triggerd by ecore_list_remove/insert
	 * somehow after the insert full_playlist->current and ->index
	 * are out of sync, index is off by -1 */
	ecore_list_first_goto(full_playlist);

	ecore_list_index_goto(full_playlist, pos);
	song = ecore_list_remove(full_playlist);
	mpd_freeSong(song);

	music_playlist_remove_nth(pos);
}

int music_song_count() {
	return ecore_list_count(full_playlist);
}

/* -1 means no song is active */
void music_song_active(int pos)
{
	int count = e_box_pack_count_get(playlist);
	Evas_Object *song = NULL;

	if (pos == song_active)
		return;

	song_active = pos;

	if (pos >= 0)
		music_playlist_scroll(pos);

	if (pos >= playlist_top || pos < playlist_top + count)
		song = e_box_pack_object_nth(playlist, pos - playlist_top);

	for (int i = 0; i < count; i++) {
		Evas_Object *tmp = e_box_pack_object_nth(playlist, i);
		if (tmp != song)
			edje_object_signal_emit(tmp, "button,off", "");
	}

	if (pos >= playlist_top || pos < playlist_top + count)
		edje_object_signal_emit(song, "button,on", "");
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

#if 0
static void music_scroller_set(double scroll)
{
	Evas_Object *area;
	Evas_Coord slide_x, slide_y, slide_w, slide_h;

	area = edje_object_part_object_get(music, "scroller");
	evas_object_geometry_get(area, &slide_x, &slide_y, &slide_w, &slide_h);

	evas_object_resize(scroller, slide_w, slide_w);
	evas_object_move(scroller, slide_x, slide_h * scroll + slide_y);
}
#endif

static Evas_Object* music_playlist_new(mpd_Song *data)
{
	Evas_Object *song;
	int *pos;

	song = edje_object_add(evas);
	edje_object_file_set(song, theme, "list_item");
	edje_object_signal_callback_add(song, "mouse,clicked,1", "*",
			music_song_signal, NULL);

	pos = malloc(sizeof(int));
	*pos = data->pos;
	evas_object_data_set(song, "pos", pos);

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

	return song;
}

static void music_playlist_remove(Evas_Object *song)
{
	if (song) {
		e_box_unpack(song);
		evas_object_hide(song);
		free(evas_object_data_get(song, "pos"));
		evas_object_del(song);
	}
}

static void music_playlist_append(mpd_Song *data)
{
	Evas_Object *song = music_playlist_new(data);
	Evas_Coord height;

	edje_object_size_min_calc(song, NULL, &height);

	e_box_pack_end(playlist, song);
	e_box_pack_options_set(song,
			       1, 0, /* fill */
			       1, 0, /* expand */
			       0.5, 0.5, /* align */
			       -1, height, /* min */
			       -1, height); /* max */
	evas_object_show(song);
}

static void music_playlist_prepend(mpd_Song *data)
{
	Evas_Object *song = music_playlist_new(data);
	Evas_Coord height;

	edje_object_size_min_calc(song, NULL, &height);

	e_box_pack_start(playlist, song);
	e_box_pack_options_set(song,
			       1, 0, /* fill */
			       1, 0, /* expand */
			       0.5, 0.5, /* align */
			       -1, height, /* min */
			       -1, height); /* max */
	evas_object_show(song);
}

static void music_playlist_update(mpd_Song *data)
{
	int count = 8, pos, len;
	Evas_Object *tmp, *song;
	Evas_Coord height;

	if (data->pos < playlist_top || data->pos >= playlist_top + count)
		return;

	song = music_playlist_new(data);
	pos = data->pos - playlist_top;
	len = e_box_pack_count_get(playlist);

	if (data->pos > len) {
		fprintf(stderr, "%s: tried to insert past end of list!\n",
				__func__);
	} else if (data->pos < 0) {
		fprintf(stderr, "%s: position was negative!\n", __func__);
	} else if (data->pos == len ) {
		e_box_pack_end(playlist, song);
	} else {
		tmp = e_box_pack_object_nth(playlist, pos);
		e_box_pack_before(playlist, song, tmp);
		e_box_unpack(tmp);
		evas_object_hide(tmp);
		evas_object_del(tmp);
	}

	edje_object_size_min_calc(song, NULL, &height);
	e_box_pack_options_set(song,
			       1, 0, /* fill */
			       1, 0, /* expand */
			       0.5, 0.5, /* align */
			       -1, height, /* min */
			       -1, height); /* max */
	evas_object_show(song);
}

static void music_playlist_remove_first()
{
	music_playlist_remove(e_box_pack_object_first(playlist));
}

static void music_playlist_remove_last()
{
	music_playlist_remove(e_box_pack_object_last(playlist));
}

static void music_playlist_remove_nth(int pos)
{
	int count = 8;

	if (pos < playlist_top || pos >= playlist_top + count)
		return;

	music_playlist_remove(
		e_box_pack_object_nth(playlist, pos - playlist_top));
}

static void music_playlist_scroll(int top)
{
	int count = 8;

	/* FIXME: Workaround for bug triggerd by ecore_list_remove/insert
	 * somehow after the insert full_playlist->current and ->index
	 * are out of sync, index is off by -1 */
	ecore_list_first_goto(full_playlist);

	if (top == playlist_top)
		return;
	else if (top < 0) {
		fprintf(stderr, "%s: negative top unimplemented.\n", __func__);
		return;
	}
	else if (top < playlist_top) {
		/* Scroll up */
		for (int i = playlist_top-1; i >= top; i--) {
			mpd_Song *new;
			/* FIXME: The following will rescan the beginning */
			new = ecore_list_index_goto(full_playlist, i);
			music_playlist_remove_last();
			if (new)
				music_playlist_prepend(new);
		}
	}
	else if (top > playlist_top) {
		/* Scroll down */
		for (int i = playlist_top+count; i < top+count; i++) {
			mpd_Song *new;
			new = ecore_list_index_goto(full_playlist, i);
			music_playlist_remove_first();
			if (new)
				music_playlist_append(new);
		}
	}

	playlist_top = top;
}


#if 0
void music_playlist_autoscroll(int pos, int align)
{
	double time = ecore_time_get();

	if (click_time + 30.0 < time)
		music_playlist_scrollxxx(pos);
	else if (click_time + 15.0 < time)
		music_playlist_scrollxxx(pos);
}	
#endif

static void music_signal(void *data, Evas_Object *obj, const char *signal, const char *source)
{
	click_time = ecore_time_get();

	if (!strcmp("scroller", source)) {
#if 0
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
#endif
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

static void music_song_signal(void *data, Evas_Object *obj, const char *signal, const char *source) {
	int *pos = evas_object_data_get(obj, "pos");

	if (!pos) {
		fprintf(stderr, "%s: missing song position!", __func__);
		return;
	}

	click_time = ecore_time_get();
	music_song_active(*pos);
	mpdclient_song_play(*pos);
}

