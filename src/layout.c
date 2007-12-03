#include "main.h"

static Evas_Object *layout;

/* create the screen layout - edje defines it with swallow regions and anything
 else the theme wants to do */
void layout_init(void)
{
	Evas_Coord w, h;

	layout = edje_object_add(evas);
	edje_object_file_set(layout, theme, "layout");
	evas_object_move(layout, 0, 0);
	edje_object_size_min_get(layout, &w, &h);
	ecore_evas_resize(ecore_evas, w, h);
	evas_object_resize(layout, w, h);
	evas_object_show(layout);
}

/* handle a window/screen resize */
void layout_resize(void)
{
	Evas_Coord w, h;

	evas_output_viewport_get(evas, NULL, NULL, &w, &h);
	evas_object_resize(layout, w, h);
}

/* swallow an object into a named layout location */
void layout_swallow(char *position, Evas_Object * obj)
{
	edje_object_part_swallow(layout, position, obj);
}
