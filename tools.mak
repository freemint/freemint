
ts = distclean clean all

$(ts):
	cd tools; $(MAKE) CROSS=yes $@
