#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Ecore_Evas.h>
#include <Ecore_Data.h>
#include <Ecore.h>
#include <Edje.h>

#include "../config.h"
#include "lib/e/e_box.h"
#include "lib/libmpdclient/libmpdclient.h"
#include "layout.h"
#include "music.h"
#include "mpdclient.h"
#include "scrollbox.h"

#define DATA "/home/marineam/e/music/data"
#define DEF_THEME "default.edj"
#define theme DATA "/" DEF_THEME

#define check(f) do { \
	if (!(f)) { \
		fprintf(stderr, "Failed at %s:%d\n", __FILE__, __LINE__); \
		exit(1); \
	} } while(0)

extern Ecore_Evas *ecore_evas;
extern Evas *evas;
extern double click_time;
