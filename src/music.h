
void music_init();
void music_show();
void music_song_add(mpd_Song *data);
void music_song_remove(int pos);
void music_song_active(int pos);
int music_song_count();

void music_playlist_autoscroll(int pos, int align);
void music_playlist_scroll(int pos, int align);
