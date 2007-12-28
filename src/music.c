#include "main.h"

static Evas_Object *music;
static Evas_Object *playlist;
static int playpause_playing;

static Evas_Object* song_create(void *vdata);
static void song_destroy(Evas_Object *song);
static void song_active(Evas_Object *song, int active);
static void song_free(struct scrollbox_item *song);
static void song_signal(void *data, Evas_Object *obj,
		const char *signal, const char *source);

void music_init()
{
	playlist = scrollbox_new();
	evas_object_show(playlist);
}

void music_show()
{
	layout_swallow("main_content", playlist);
	scrollbox_show(playlist);
}

void music_hide()
{
	scrollbox_hide(playlist);
}

void music_resize()
{
	/* TODO: Signal the playlist that it resized */
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
void music_song_update(int pos, int time)
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

static void song_signal(void *data, Evas_Object *obj, const char *signal, const char *source)
{
	int *pos = evas_object_data_get(obj, "pos");

	click_time = ecore_time_get();

	if (!pos) {
		fprintf(stderr, "%s: missing song position!", __func__);
		return;
	}

	scrollbox_item_active(playlist, *pos);
	mpdclient_song_play(*pos);
}
