#include "main.h"

static int scrollbox_view_count(Evas_Object *box);
static int scrollbox_view_pos(Evas_Object *box, int pos);
static void scrollbox_bar_update(Evas_Object *box);
static void scrollbox_view_remove_nth(Evas_Object *box, int pos);
static void scrollbox_view_update(Evas_Object *box,
	struct scrollbox_item *item);
static void scrollbox_scroll(Evas_Object *box, int top);
static void scrollbox_autoscroll(Evas_Object *box, int pos);

Evas_Object* scrollbox_new()
{
	struct scrollbox *boxinfo;
	Evas_Object *box;

	boxinfo = malloc(sizeof(struct scrollbox));
	memset(boxinfo, 0,  sizeof(struct scrollbox));

	box = evas_object_rectangle_add(evas);
	evas_object_data_set(box, "scrollbox", boxinfo);

	boxinfo->active = -2;
	boxinfo->list = ecore_list_new();
	boxinfo->scroll_box = e_box_add(evas);
	boxinfo->scroll_bar = evas_object_rectangle_add(evas);

	evas_object_clip_set(boxinfo->scroll_box, box);
	evas_object_clip_set(boxinfo->scroll_bar, box);
	evas_object_show(boxinfo->scroll_box);
	evas_object_show(boxinfo->scroll_bar);

	e_box_align_set(boxinfo->scroll_box, 0.0, 0.0);
	evas_object_color_set(boxinfo->scroll_bar, 10, 207, 233, 50);

	return box;
}

void scrollbox_show(Evas_Object *box)
{
	Evas_Coord x, y, w, h;
	struct scrollbox *boxinfo;

	boxinfo = evas_object_data_get(box, "scrollbox");

	evas_object_geometry_get(box, &x, &y, &w, &h);
	evas_object_move(boxinfo->scroll_box, x, y);
	evas_object_resize(boxinfo->scroll_box, w - UNIT, h);
	scrollbox_bar_update(box);

	evas_object_show(box);
}

void scrollbox_item_insert(Evas_Object *box, struct scrollbox_item *item)
{
	struct scrollbox *boxinfo;
	struct scrollbox_item *old;

	boxinfo = evas_object_data_get(box, "scrollbox");

	if (item->pos > ecore_list_count(boxinfo->list)) {
		fprintf(stderr, "%s: tried to insert past end of list!\n",
				__func__);
	} else if (item->pos < 0) {
		fprintf(stderr, "%s: position was negative!\n", __func__);
	} else if (item->pos == ecore_list_count(boxinfo->list)) {
		ecore_list_append(boxinfo->list, item);
		scrollbox_view_update(box, item);
	} else {
		ecore_list_index_goto(boxinfo->list, item->pos);
		old = ecore_list_remove(boxinfo->list);
		ecore_list_insert(boxinfo->list, item);
		scrollbox_view_update(box, item);
		old->free(old);

		/* FIXME: Workaround for bug triggerd by 
		 * ecore_list_remove/insert somehow after the insert 
		 * list->current and ->index are out of sync,
		 * index is off by -1 */
		ecore_list_first_goto(boxinfo->list);
	}

	scrollbox_bar_update(box);
}

void scrollbox_item_remove(Evas_Object *box, int pos)
{
	struct scrollbox *boxinfo;
	struct scrollbox_item *old;

	boxinfo = evas_object_data_get(box, "scrollbox");

	ecore_list_index_goto(boxinfo->list, pos);
	old = ecore_list_remove(boxinfo->list);
	scrollbox_view_remove_nth(box, pos);
	old->free(old);

	/* FIXME: Workaround for bug triggerd by 
	 * ecore_list_remove/insert somehow after the insert 
	 * list->current and ->index are out of sync,
	 * index is off by -1 */
	ecore_list_first_goto(boxinfo->list);

	scrollbox_bar_update(box);
}

int scrollbox_item_count(Evas_Object *box) {
	struct scrollbox *boxinfo;

	boxinfo = evas_object_data_get(box, "scrollbox");
	return ecore_list_count(boxinfo->list);
}

/* -1 means no song is active */
void scrollbox_item_active(Evas_Object *box, int pos)
{
	struct scrollbox *boxinfo;
	int count, view_pos;
	Evas_Object *view_item = NULL;

	boxinfo = evas_object_data_get(box, "scrollbox");

	if (pos == boxinfo->active)
		return;

	boxinfo->active = pos;
	view_pos = scrollbox_view_pos(box, pos);

	if (pos >= 0)
		scrollbox_autoscroll(box, pos);

	if (view_pos >= 0)
		view_item = e_box_pack_object_nth(boxinfo->scroll_box,view_pos);

	count = e_box_pack_count_get(boxinfo->scroll_box);
	for (int i = 0; i < count; i++) {
		Evas_Object *tmp;

		tmp = e_box_pack_object_nth(boxinfo->scroll_box, i);
		if (tmp != view_item) {
			struct scrollbox_item *tmp_item;

			tmp_item = evas_object_data_get(tmp, "scrollbox_item");
			tmp_item->active(tmp, 0);
		}
	}

	if (view_item) {
		struct scrollbox_item *item;

		item = evas_object_data_get(view_item, "scrollbox_item");
		item->active(view_item, 1);
	}
}

static Evas_Object* scrollbox_item_create(struct scrollbox_item *item)
{
	Evas_Object *item_view;

	item_view = item->create(item->data);
	evas_object_data_set(item_view, "scrollbox_item", item);

	return item_view;
}

static int scrollbox_view_count(Evas_Object *box)
{
	struct scrollbox *boxinfo;
	Evas_Coord h;

	boxinfo = evas_object_data_get(box, "scrollbox");

	evas_object_geometry_get(box, NULL, NULL, NULL, &h);
	return h / UNIT;
}

/* Get the position in the visible list of a given index
 * -1 means it is not visible */
static int scrollbox_view_pos(Evas_Object *box, int pos)
{
	struct scrollbox *boxinfo;

	boxinfo = evas_object_data_get(box, "scrollbox");

	if (pos < boxinfo->scroll_top || pos >= boxinfo->top +
			scrollbox_view_count(box))
		return -1;
	else
		return pos - boxinfo->scroll_top;
}

static void scrollbox_bar_update(Evas_Object *box)
{
	Evas_Coord x, y, w, h, bar_y, bar_h;
	int view_count, list_count;
	struct scrollbox *boxinfo;

	boxinfo = evas_object_data_get(box, "scrollbox");
	evas_object_geometry_get(box, &x, &y, &w, &h);

	view_count = scrollbox_view_count(box);
	list_count = ecore_list_count(boxinfo->list);
	if (list_count) {
		bar_h = h * ((double)view_count / list_count);
		bar_y = h * ((double)boxinfo->top / list_count) + y;
	}
	else {
		bar_h = h;
		bar_y = y;
	}

	evas_object_move(boxinfo->scroll_bar, x + w - UNIT, bar_y);
	evas_object_resize(boxinfo->scroll_bar, UNIT, bar_h);
}

static void scrollbox_view_remove(Evas_Object *item_view)
{
	struct scrollbox_item *item;

	if (!item_view)
		return;

	e_box_unpack(item_view);
	evas_object_hide(item_view);

	item = evas_object_data_get(item_view, "scrollbox_item");
	item->destroy(item_view);
}

static void scrollbox_view_append(struct scrollbox *boxinfo,
	struct scrollbox_item *item)
{
	Evas_Object *item_view;

	item_view = scrollbox_item_create(item);

	e_box_pack_end(boxinfo->scroll_box, item_view);
	e_box_pack_options_set(item_view,
			       1, 0, /* fill */
			       1, 0, /* expand */
			       0.5, 0.5, /* align */
			       -1, UNIT, /* min */
			       -1, UNIT); /* max */
	evas_object_show(item_view);
}

static void scrollbox_view_prepend(struct scrollbox *boxinfo,
	struct scrollbox_item *item)
{
	Evas_Object *item_view;

	item_view = scrollbox_item_create(item);

	e_box_pack_start(boxinfo->scroll_box, item_view);
	e_box_pack_options_set(item_view,
			       1, 0, /* fill */
			       1, 0, /* expand */
			       0.5, 0.5, /* align */
			       -1, UNIT, /* min */
			       -1, UNIT); /* max */
	evas_object_show(item_view);
}

static void scrollbox_view_update(Evas_Object *box, struct scrollbox_item *item)
{
	struct scrollbox *boxinfo;
	int view_pos, count;
	Evas_Object *item_view;

	boxinfo = evas_object_data_get(box, "scrollbox");

	view_pos = scrollbox_view_pos(box, item->pos);
	if (view_pos < 0)
		return;

	item_view = scrollbox_item_create(item);
	count = e_box_pack_count_get(boxinfo->scroll_box);

	if (view_pos > count) {
		fprintf(stderr, "%s: tried to update past end of view. "
			"Packing out of order?\n", __func__);
	}
	else if (view_pos == count) {
		e_box_pack_end(boxinfo->scroll_box, item_view);
	} else {
		Evas_Object *tmp;
		struct scrollbox_item *tmp_item;

		tmp = e_box_pack_object_nth(boxinfo->scroll_box, view_pos);
		e_box_pack_before(boxinfo->scroll_box, item_view, tmp);
		e_box_unpack(tmp);
		evas_object_hide(tmp);

		tmp_item = evas_object_data_get(tmp, "scrollbox_item");
		tmp_item->destroy(tmp);
	}

	e_box_pack_options_set(item_view,
			       1, 0, /* fill */
			       1, 0, /* expand */
			       0.5, 0.5, /* align */
			       -1, UNIT, /* min */
			       -1, UNIT); /* max */
	evas_object_show(item_view);
}

static void scrollbox_view_remove_first(struct scrollbox *boxinfo)
{
	scrollbox_view_remove(e_box_pack_object_first(boxinfo->scroll_box));
}

static void scrollbox_view_remove_last(struct scrollbox *boxinfo)
{
	scrollbox_view_remove(e_box_pack_object_last(boxinfo->scroll_box));
}

static void scrollbox_view_remove_nth(Evas_Object *box, int pos)
{
	struct scrollbox *boxinfo;
	int view_pos;

	boxinfo = evas_object_data_get(box, "scrollbox");

	view_pos = scrollbox_view_pos(box, pos);

	if (view_pos >= 0)
		scrollbox_view_remove(e_box_pack_object_nth(
			boxinfo->scroll_box, view_pos));

	/* FIXME: can this get called on boxinfo->top? */
	/* If so, we need to update top, scroll_top, etc */
}

static int scrollbox_scroll_helper(void *box) {
	struct scrollbox *boxinfo;
	double diff, curr;
	int ret;

	boxinfo = evas_object_data_get(box, "scrollbox");

	e_box_align_get(boxinfo->scroll_box, NULL, &curr);

	diff = boxinfo->scroll_align - curr;

	if (-0.001 < diff && diff < 0.001) {
		curr = 0.0;
		ret = 0;
		boxinfo->scroll_timer = NULL;

		for (int i = boxinfo->scroll_top; i < boxinfo->top; i++) {
			scrollbox_view_remove_first(boxinfo);
		}
		for (int i = e_box_pack_count_get(boxinfo->scroll_box);
				i > scrollbox_item_count(box); i--) {
			scrollbox_view_remove_last(boxinfo);
		}
		boxinfo->scroll_top = boxinfo->top;
	}
	else {
		ret = 1;
		curr = curr * 0.7 + boxinfo->scroll_align * 0.3;
	}

	e_box_align_set(boxinfo->scroll_box, 0.0, curr);

	return ret;
}

static void scrollbox_scroll(Evas_Object *box, int top)
{
	struct scrollbox *boxinfo;
	double old_scroll_align;
	int extra, max_top;

	boxinfo = evas_object_data_get(box, "scrollbox");

	max_top = ecore_list_count(boxinfo->list) - scrollbox_item_count(box);
	if (top > max_top)
		top = max_top;
	else if (top < 0)
		top = 0;

	if (top == boxinfo->top)
		return;
	else if (top < boxinfo->top) {
		boxinfo->scroll_top = boxinfo->top;
		/* Scroll up */
		for (int i = boxinfo->top-1; i >= top; i--) {
			struct scrollbox_item *new;

			/* FIXME: The following will rescan the beginning */
			new = ecore_list_index_goto(boxinfo->list, i);
			if (new) {
				scrollbox_view_prepend(boxinfo, new);
				boxinfo->scroll_top--;
			}
		}
	}
	else if (top > boxinfo->top) {
		boxinfo->scroll_top = boxinfo->top;
		/* Scroll down */
		for (int i = boxinfo->top + scrollbox_item_count(box);
				i < top + scrollbox_item_count(box); i++) {
			/* fixme? called scrollbox_item_count twice? */
			struct scrollbox_item *new;

			new = ecore_list_index_goto(boxinfo->list, i);
			if (new) {
				scrollbox_view_append(boxinfo, new);
			}
		}
		e_box_align_set(boxinfo->scroll_box, 0.0, 1.0);
	}

	boxinfo->top = top;
	scrollbox_bar_update(box);

	/* Update smooth scrolling alignment */
	e_box_align_get(boxinfo->scroll_box, NULL, &old_scroll_align);
	extra = e_box_pack_count_get(boxinfo->scroll_box) -
		scrollbox_item_count(box);
	if (extra)
		boxinfo->scroll_align = 1.0 -
			((double)(boxinfo->top - boxinfo->scroll_top) / extra);
	else
		boxinfo->scroll_align = 0.0;

	if (!boxinfo->scroll_timer)
		boxinfo->scroll_timer = ecore_timer_add(1.0 / 30.0,
			scrollbox_scroll_helper, box);
}

static void scrollbox_autoscroll(Evas_Object *box, int pos)
{
	/* FIXME: implement clicktime */
	//double time = ecore_time_get();

	//if (click_time + 30.0 < time)
		scrollbox_scroll(box, pos);
}

#if 0
static void scrollbox_signal(void *data, Evas_Object *obj, const char *signal, const char *source)
{
	//click_time = ecore_time_get();

	if (!strcmp("scroller", source)) {
		Evas_Coord mouse_y, scroll_y, scroll_h, scroller_h;
		Evas_Object *area;
		float scroll;
		int total = ecore_list_count(full_playlist);

		evas_pointer_canvas_xy_get(evas, NULL, &mouse_y);
		area = edje_object_part_object_get(music, "scroller");
		evas_object_geometry_get(area, NULL,&scroll_y,NULL,&scroll_h);

		scroller_h = scroll_h * ((float)playlist_count / total);
		scroll = (float)(mouse_y - scroll_y - scroller_h/2)/ scroll_h;

		music_playlist_scroll((int)(scroll * total));
	}
	else if (!strcmp("playpause", source) && playpause_playing) {
		mpdclient_pause(1);
		music_playing(0);
	}
	else if (!strcmp("playpause", source)) {
		mpdclient_pause(0);
		music_playing(1);
	}
}
#endif
