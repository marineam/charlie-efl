#include "main.h"

Ecore_Evas *ecore_evas;
Evas *evas;
double click_time;

static int main_signal_exit(void *data, int ev_type, void *ev);
static void main_resize(Ecore_Evas *ee);

int main()
{
	check(ecore_init());
	check(ecore_evas_init());
	check(edje_init());

	ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, main_signal_exit,
				NULL);

	ecore_evas = ecore_evas_software_x11_new(NULL, 0, 0, 0, WIDTH, HEIGHT);
	check(ecore_evas);
	ecore_evas_callback_resize_set(ecore_evas, main_resize);
	ecore_evas_title_set(ecore_evas, "Charlie!");
	ecore_evas_show(ecore_evas);
	evas = ecore_evas_get(ecore_evas);

	layout_init();
	music_init();

	music_show();
	mpdclient_init();

	ecore_main_loop_begin();

	ecore_evas_shutdown();
	ecore_shutdown();
	return 0;
}

static void main_resize(Ecore_Evas *ee)
{
	layout_resize();
	music_resize();
}


static int main_signal_exit(void *data, int ev_type, void *ev)
{
	ecore_main_loop_quit();
	return 1;
}
