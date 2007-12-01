#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Ecore_Evas.h>
#include <Ecore_Data.h>
#include <Ecore.h>
#include <Edje.h>
#include <Eet.h>

#include "lib/e/e_box.h"
#include "lib/libmpdclient/libmpdclient.h"
#include "layout.h"
#include "music.h"
#include "mpdclient.h"

#define WIDTH 800
#define HEIGHT 600
#define DATA "/home/marineam/e/music/data"
#define DEF_THEME "default.edj"
#define theme DATA "/" DEF_THEME

extern Ecore_Evas *ecore_evas;
extern Evas *evas;

