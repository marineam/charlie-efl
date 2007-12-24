
struct scrollbox {
	int count;
	int top;
	int active;
	int scroll_top;
	double scroll_align;
	Ecore_Timer *scroll_timer;
	Evas_Object *scroll_box;
	Evas_Object *scroll_bar;
	Evas_Object *base_box;
	Evas_Object *base_bar;
	Ecore_List *list;
};

struct scrollbox_item {
	int pos;
	void *data;
	Evas_Object* (*create)(void *data);
	void (*destroy)(Evas_Object *item);
	void (*active)(Evas_Object *item, int active);
	void (*free)(struct scrollbox_item *item);
};

Evas_Object* scrollbox_new();
void scrollbox_show(Evas_Object *box);
void scrollbox_item_insert(Evas_Object *box, struct scrollbox_item *item);
void scrollbox_item_remove(Evas_Object *box, int pos);
int scrollbox_item_count(Evas_Object *box);
void scrollbox_item_active(Evas_Object *box, int pos);
struct scrollbox_item* scrollbox_item_get(Evas_Object *box, int pos);
