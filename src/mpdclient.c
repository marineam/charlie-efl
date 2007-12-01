#include "main.h"

static mpd_Connection * conn;
static long long cur_playlist;
static int cur_song;

static int mpdclient_connect();
static int mpdclient_playlist_update(void *data);

void mpdclient_init() {
	if (!mpdclient_connect()) {
		mpdclient_playlist_update(NULL);
		ecore_timer_add(1.0, mpdclient_playlist_update, NULL);
		//music_playlist_scroll(0, 1, 1);
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
	int i;

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
		if(entity->type!=MPD_INFO_ENTITY_TYPE_SONG) {
			mpd_freeInfoEntity(entity);
			continue;
		}

		music_song_insert(entity->info.song);

		mpd_freeInfoEntity(entity);
	}

	for (i = status->playlistLength; i < music_song_count(); i++) {
		music_song_remove(i);
	}

	if(status->state == MPD_STATUS_STATE_PLAY ||
			status->state == MPD_STATUS_STATE_PAUSE) {
		music_song_active(status->song);
		music_slider_set((double)status->elapsedTime / status->totalTime);
	} else {
		music_song_active(-1);
		music_slider_set(0.0);
	}

	music_playing(status->state == MPD_STATUS_STATE_PLAY);

	mpd_finishCommand(conn);
	mpd_freeStatus(status);
	return 1;
}

void mpdclient_song_play(int pos) {
	mpd_sendPlayCommand(conn, pos);
	mpd_finishCommand(conn);
}

void mpdclient_pause(int state) {
	mpd_Status *status;

	if (state == 1) {
		mpd_sendPauseCommand(conn, state);
		mpd_finishCommand(conn);
		return;
	}

	/* If we are stopped, start playing */
	mpd_sendStatusCommand(conn);
	status = mpd_getStatus(conn);
	if (status->state == MPD_STATUS_STATE_STOP &&
			status->playlistLength > 0) {
		mpd_sendPlayCommand(conn, 0);
		//music_song_active(0);
		//music_playlist_autoscroll(0, 1);
	}
	else
		mpd_sendPauseCommand(conn, 0);
	
	mpd_finishCommand(conn);
}
