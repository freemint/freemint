# entry point

MAKE = make
RM = rm -f
CAT = cat
TOUCH = touch


TARGET = __target
TARGETS = 000 030 040 deb mil my1 my2 my3

#default: my1
default: my2

help:
	@echo '# targets:'
	@echo '# --------'
	@echo '# - $(TARGETS)'
	@echo '# - bakclean'
	@echo '# - clean'
	@echo '# - distclean'

all: 000 030 040 deb mil


bakclean:
	$(MAKE) bakclean -f Makefile.kernel

clean:
	$(MAKE) clean -f Makefile.kernel

distclean:
	$(MAKE) distclean -f Makefile.kernel


000: $(TARGET).000
	@echo '# building a normal kernel ...'
	$(MAKE) all -f Makefile.kernel

030: $(TARGET).030
	@echo '# building a 030 kernel ...'
	$(MAKE) all -f Makefile.kernel

040: $(TARGET).040
	@echo '# building a 040 kernel ...'
	$(MAKE) all -f Makefile.kernel

deb: $(TARGET).deb
	@echo '# building a debug kernel ...'
	$(MAKE) all -f Makefile.kernel

mil: $(TARGET).mil
	@echo '# building a MILAN kernel ...'
	$(MAKE) all -f Makefile.kernel

my1: $(TARGET).my1
	@echo '# building a special kernel ...'
	$(MAKE) all -f Makefile.kernel

my2: $(TARGET).my2
	@echo '# building a special kernel ...'
	$(MAKE) all -f Makefile.kernel

my3: $(TARGET).my3
	@echo '# building a special kernel ...'
	$(MAKE) all -f Makefile.kernel


$(TARGET).000:
	$(MAKE) clean
	$(RM) $(TARGET).*
	echo "target=000" > $@

$(TARGET).030:
	$(MAKE) clean
	$(RM) $(TARGET).*
	echo "target=030" > $@

$(TARGET).040:
	$(MAKE) clean
	$(RM) $(TARGET).*
	echo "target=040" > $@

$(TARGET).deb:
	$(MAKE) clean
	$(RM) $(TARGET).*
	echo "target=deb" > $@

$(TARGET).mil:
	$(MAKE) clean
	$(RM) $(TARGET).*
	echo "target=mil" > $@

$(TARGET).my1:
	$(MAKE) clean
	$(RM) $(TARGET).*
	echo "target=my1" > $@

$(TARGET).my2:
	$(MAKE) clean
	$(RM) $(TARGET).*
	echo "target=my2" > $@

$(TARGET).my3:
	$(MAKE) clean
	$(RM) $(TARGET).*
	echo "target=my3" > $@
