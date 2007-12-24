#include "main.h"

Ecore_Evas *ecore_evas;
Evas *evas;
double click_time;

static int main_signal_exit(void *data, int ev_type, void *ev);
static void main_resize(Ecore_Evas *ee);
static void main_menu_add(const char* label, int active);

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

	//main_menu_add("Playlist", 1);
	//main_menu_add("Library", 0);
	//main_menu_add("Radio", 0);
	//main_menu_add("Status", 0);

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

static void main_menu_signal(void *data, Evas_Object *obj, const char *signal, const char *source) {
	Evas_Object *box = (Evas_Object *)data, *button;
	int n, count;

	click_time = ecore_time_get();

	count = e_box_pack_count_get(box);
	for (n = 0; n < count; n++) {
		button = e_box_pack_object_nth(box, n);
		if (obj != button)
			edje_object_signal_emit(button, "button,off", "");
	}

	edje_object_signal_emit(obj, "button,on", ""); 
}


static void main_menu_add(const char* label, int active)
{
	static Evas_Object *box = NULL;
	Evas_Object *button;

	if (box == NULL) {
		box = e_box_add(evas);
		e_box_orientation_set(box, 1);
		layout_swallow("main_menu", box);
		evas_object_show(box);
	}

	button = edje_object_add(evas);
	edje_object_file_set(button, theme, "menu_item");
	edje_object_part_text_set(button, "label", label);
	edje_object_signal_callback_add(button, "mouse,clicked,1", "*", main_menu_signal, box);
	e_box_pack_end(box, button);

	e_box_pack_options_set(button,
			       1, 1, /* fill */
			       1, 1, /* expand */
			       0.5, 0.5, /* align */
			       -1, -1, /* min */
			       -1, -1); /* max */

	if (active)
		main_menu_signal(box, button, NULL, NULL);

	evas_object_show(button);
}
