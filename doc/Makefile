ifeq ($(HCP),)
HCP = hcp
endif

.PHONY: all

all: freemint.hyp
	$(MAKE) -C xaaes $@

freemint.hyp: freemint.stg
	@if $(HCP) --version >/dev/null 2>&1 && test -f $<; then \
		echo $(HCP) -o $@ $<; \
		$(HCP) -o $@ $<; \
	else \
		echo "HCP not found, help file not compiled" >&2; \
	fi

clean::
	$(MAKE) -C xaaes $@
	$(RM) *.hyp *.ref
