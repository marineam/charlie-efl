
void music_init();
void music_show();
void music_hide();
void music_resize();
void music_song_insert(mpd_Song *song);
void music_song_remove(int pos);
void music_song_update(int pos, int time);
int music_song_count();
