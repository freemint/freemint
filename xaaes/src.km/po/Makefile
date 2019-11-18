MSGFMT = msgfmt
MSGFMT_FLAGS = -c -v --statistics --endianness=big

LINGUAS := $(shell grep -v '^\#' "LINGUAS" | tr '\n' ' ')
GMO := $(addsuffix .gmo,$(LINGUAS))
PO := $(addsuffix .po,$(LINGUAS))


all: $(GMO) gentab

.SUFFIXES: .po .gmo

.po.gmo:
	@echo "MSGFMT  $@"
	@$(MSGFMT) $(MSGFMT_FLAGS) -o $@ $< || { $(RM) $@; false; }

gentab: gentab.c
	gcc -O2 -Wall -o $@ $<

clean::
	$(RM) *.gmo gentab
