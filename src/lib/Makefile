CFLAGS=
DIRS= libmpdclient

all: dirs

.PHONY: dirs
dirs:
	$(MAKE) CFLAGS="$(CFLAGS)" -C libmpdclient
	$(MAKE) CFLAGS="$(CFLAGS)" -C e

clean:
	$(MAKE) -C libmpdclient clean
	$(MAKE) -C e clean
