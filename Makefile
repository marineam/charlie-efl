all:
	$(MAKE) -C src
	$(MAKE) -C data

clean:
	$(MAKE) -C src clean
	$(MAKE) -C data clean
