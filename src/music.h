
void music_init();
void music_show();
void music_resize();
void music_song_insert(mpd_Song *song);
void music_song_remove(int pos);
void music_song_active(int pos);
int music_song_count();

void music_playing(int state);

void music_slider_set(double progress);
