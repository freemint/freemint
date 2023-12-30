ifeq ($(HCP),)
HCP = hcp
endif

.PHONY: all

all: freemint.hyp

freemint.hyp: freemint.stg
	@if $(HCP) --version >/dev/null 2>&1 && test -f $<; then \
		echo $(HCP) -o $@ $<; \
		$(HCP) -o $@ $<; \
	else \
		echo "HCP not found, help file not compiled" >&2; \
	fi

clean::
	$(RM) *.hyp *.ref
