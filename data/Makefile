EDJE_CC = edje_cc
EDJE_FLAGS = -v

FILES = Vera.ttf VeraBd.ttf bg.png mitem.png mitem2.png

all: default.edj

default.edj: Makefile $(FILES) default.edc ../config.h
	$(EDJE_CC) $(EDJE_FLAGS) default.edc default.edj

clean:
	rm -f *.edj
