#include "main.h"
#include <semaphore.h>

static Evas_Object *music;
static Evas_Object *slider;
static Evas_Object *playlist;
//static double click_time;
static int playpause_playing;

static Evas_Object* song_create(void *vdata);
static void song_destroy(Evas_Object *song);
static void song_active(Evas_Object *song, int active);
static void song_free(struct scrollbox_item *song);
static void song_signal(void *data, Evas_Object *obj,
		const char *signal, const char *source);
static void music_signal(void *data, Evas_Object *obj,
		const char *signal, const char *source);

void music_init()
{
	music = edje_object_add(evas);
	edje_object_file_set(music, theme, "music");
	edje_object_signal_callback_add(music, "mouse,clicked,1", "*", music_signal, NULL);

	playlist = scrollbox_new();
	edje_object_part_swallow(music, "list", playlist);
	evas_object_show(playlist);

	slider = evas_object_rectangle_add(evas);
	evas_object_color_set(slider, 10, 207, 233, 50);
}

void music_show()
{
	layout_swallow("main_content", music);
	evas_object_show(music);

	music_slider_set(0.0);
	evas_object_show(slider);

	scrollbox_show(playlist);
}

void music_resize()
{
#if 0
	if (playlist_item_height) {
		Evas_Coord listh;

		edje_object_part_geometry_get(music, "list",
			NULL, NULL, NULL, &listh);
		playlist_count = listh / playlist_item_height;

		/* TODO: actually do something to resize the playlist */
	}
#endif
}

void music_song_insert(mpd_Song *song)
{
	struct scrollbox_item *new;

	new = malloc(sizeof(struct scrollbox_item));
	new->pos = song->pos;
	new->data = mpd_songDup(song);
	new->create = song_create;
	new->destroy = song_destroy;
	new->active = song_active;
	new->free = song_free;
	scrollbox_item_insert(playlist, new);
}

void music_song_remove(int pos)
{
	scrollbox_item_remove(playlist, pos);
}

int music_song_count() {
	return scrollbox_item_count(playlist);
}

/* -1 means no song is active */
void music_song_active(int pos)
{
	scrollbox_item_active(playlist, pos);
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

static Evas_Object* song_create(void *vdata)
{
	mpd_Song *data = vdata;
	Evas_Object *song;
	int *pos;

	song = edje_object_add(evas);
	edje_object_file_set(song, theme, "list_item");
	edje_object_signal_callback_add(song, "mouse,clicked,1", "*",
			song_signal, NULL);

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

static void song_destroy(Evas_Object *song)
{
	free(evas_object_data_get(song, "pos"));
	evas_object_del(song);
}

static void song_active(Evas_Object *song, int active)
{
	if (active)
		edje_object_signal_emit(song, "button,on", "");
	else
		edje_object_signal_emit(song, "button,off", "");
}

static void song_free(struct scrollbox_item *song)
{
	mpd_freeSong(song->data);
	free(song);
}

static void song_signal(void *data, Evas_Object *obj, const char *signal, const char *source) {
	int *pos = evas_object_data_get(obj, "pos");

	if (!pos) {
		fprintf(stderr, "%s: missing song position!", __func__);
		return;
	}

	//click_time = ecore_time_get();
	music_song_active(*pos);
	mpdclient_song_play(*pos);
}

static void music_signal(void *data, Evas_Object *obj, const char *signal, const char *source)
{
	//click_time = ecore_time_get();

	if (!strcmp("playpause", source) && playpause_playing) {
		mpdclient_pause(1);
		music_playing(0);
	}
	else if (!strcmp("playpause", source)) {
		mpdclient_pause(0);
		music_playing(1);
	}
}
