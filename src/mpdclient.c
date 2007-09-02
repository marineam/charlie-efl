#include "main.h"
#include "lib/libmpdclient/libmpdclient.h"

static mpd_Connection * conn;
static long long cur_playlist;
static int cur_song;

static int mpdclient_connect();
static int mpdclient_playlist_update(void *data);

void mpdclient_init() {
	if (!mpdclient_connect()) {
		mpdclient_playlist_update(NULL);
		ecore_timer_add(1.0, mpdclient_playlist_update, NULL);
	}
	//mpd_closeConnection(conn); put this somewhere
}

static int mpdclient_connect() {
	char *hostname = getenv("MPD_HOST");
	char *port = getenv("MPD_PORT");

	if(hostname == NULL)
		hostname = "localhost";
	if(port == NULL)
		port = "6600";

	conn = mpd_newConnection(hostname,atoi(port),10);

	if(conn->error) {
		fprintf(stderr,"%s\n",conn->errorStr);
		mpd_closeConnection(conn);
		return -1;
	}

	return 0;
}

static int mpdclient_playlist_update(void *data) {
	mpd_Status * status;
	mpd_InfoEntity *entity;
	char *label;
	int label_len, i;

	mpd_sendCommandListOkBegin(conn);
	mpd_sendStatusCommand(conn);
	mpd_sendPlChangesCommand(conn, cur_playlist);
	mpd_sendCommandListEnd(conn);

	if((status = mpd_getStatus(conn))==NULL) {
		fprintf(stderr,"%s\n",conn->errorStr);
		mpd_closeConnection(conn);
		return 1;
	}

	cur_playlist = status->playlist;
	cur_song = status->songid;

	mpd_nextListOkCommand(conn);

	while((entity = mpd_getNextInfoEntity(conn))) {
		mpd_Song * song = entity->info.song;

		if(entity->type!=MPD_INFO_ENTITY_TYPE_SONG) {
			mpd_freeInfoEntity(entity);
			continue;
		}

		if(song->artist && song->title) {
			label_len = strlen(song->artist) +
				strlen(song->title) + 4;

			if ((label = malloc(label_len))) {
				snprintf(label, label_len, "%s - %s",
						song->artist, song->title);
				music_song_replace(label, song->pos);
				free(label);
			}
		}

		mpd_freeInfoEntity(entity);
	}

	for (i = status->playlistLength; i < music_song_count(); i++) {
		music_song_remove(i);
	}

	if(status->state == MPD_STATUS_STATE_PLAY ||
			status->state == MPD_STATUS_STATE_PAUSE) {
		music_song_active(status->song);
		music_playlist_autoscroll(status->song, 4);

	} else
		music_song_active(-1);

	mpd_finishCommand(conn);
	mpd_freeStatus(status);
	return 1;
}

void mpdclient_song_play(int pos) {
	mpd_sendPlayCommand(conn, pos);
	mpd_finishCommand(conn);
}
