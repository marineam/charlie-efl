/* libmpdclient
   (c)2003-2006 by Warren Dukes (warren.dukes@gmail.com)
   This project's homepage is: http://www.musicpd.org

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of the Music Player Daemon nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "main.h"
#include "lib/libmpdclient/libmpdclient.h"

static mpd_Connection * conn;

static int mpdclient_connect();

void mpdclient_init() {
	if (!mpdclient_connect())
		mpdclient_playlist_load();
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

void mpdclient_playlist_load() {
	mpd_Status * status;
	mpd_InfoEntity *entity;
	char *label;
	int label_len;

	mpd_sendCommandListOkBegin(conn);
	mpd_sendStatusCommand(conn);
	mpd_sendPlaylistInfoCommand(conn, -1);
	mpd_sendCommandListEnd(conn);

	if((status = mpd_getStatus(conn))==NULL) {
		fprintf(stderr,"%s\n",conn->errorStr);
		mpd_closeConnection(conn);
		return;
	}

	printf("volume: %i\n",status->volume);
	printf("repeat: %i\n",status->repeat);
	printf("playlist: %lli\n",status->playlist);
	printf("playlistLength: %i\n",status->playlistLength);
	if(status->error) printf("error: %s\n",status->error);

	if(status->state == MPD_STATUS_STATE_PLAY || 
			status->state == MPD_STATUS_STATE_PAUSE) {
		printf("song: %i\n",status->song);
		printf("elaspedTime: %i\n",status->elapsedTime);
		printf("totalTime: %i\n",status->totalTime);
		printf("bitRate: %i\n",status->bitRate);
		printf("sampleRate: %i\n",status->sampleRate);
		printf("bits: %i\n",status->bits);
		printf("channels: %i\n",status->channels);
	}

	if(conn->error) {
		fprintf(stderr,"%s\n",conn->errorStr);
		mpd_closeConnection(conn);
		return;
	}

	mpd_nextListOkCommand(conn);

	while((entity = mpd_getNextInfoEntity(conn))) {
		mpd_Song * song = entity->info.song;

		if(entity->type!=MPD_INFO_ENTITY_TYPE_SONG) {
			mpd_freeInfoEntity(entity);
			continue;
		}

		printf("file: %s\n",song->file);
		if(song->artist && song->title) {
			label_len = strlen(song->artist) + strlen(song->title) + 4;

			if ((label = malloc(label_len))) {
				snprintf(label, label_len, "%s - %s", song->artist, song->title);
				music_song_add(label);
				free(label);
			}
		}

		mpd_freeInfoEntity(entity);
	}

	if(conn->error) {
		fprintf(stderr,"%s\n",conn->errorStr);
		mpd_closeConnection(conn);
		return;
	}

	mpd_finishCommand(conn);
	if(conn->error) {
		fprintf(stderr,"%s\n",conn->errorStr);
		mpd_closeConnection(conn);
		return;
	}

	mpd_freeStatus(status);

	return;
}
