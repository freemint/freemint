HCP ?= hcp

.PHONY: all

all: freemint.hyp
	$(MAKE) -C xaaes $@

%.hyp: %.stg
	@if command -v $(HCP) >/dev/null 2>&1 && test -f $<; then \
		$(HCP) -o $@ $<; \
	else \
		echo "HCP not found, help file not compiled" >&2; \
	fi

clean::
	$(MAKE) -C xaaes $@
	$(RM) *.hyp *.ref
