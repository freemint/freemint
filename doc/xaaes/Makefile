UDO ?= udo
HCP ?= hcp

.PHONY: all

all: xaaes.html xaaes.txt xaaes.hyp

xaaes.html: xaaes.u
	@if command -v $(UDO) >/dev/null 2>&1; then \
		$(UDO) --html -o $@ $<; \
	else \
		echo "UDO not found, help file not compiled" >&2; \
	fi

xaaes.txt: xaaes.u
	@if command -v $(UDO) >/dev/null 2>&1; then \
		$(UDO) --asc --no-warningslines -o $@ $<; \
	else \
		echo "UDO not found, help file not compiled" >&2; \
	fi

xaaes.stg: xaaes.u
	@if command -v $(UDO) >/dev/null 2>&1; then \
		$(UDO) --stg --no-warningslines -o $@ $<; \
		sed -i 's/ RX / SYSTEM /' $@; \
	else \
		echo "UDO not found, help file not compiled" >&2; \
	fi

%.hyp: %.stg
	@if command -v $(HCP) >/dev/null 2>&1 && test -f $<; then \
		$(HCP) -o $@ $<; \
	else \
		echo "HCP not found, help file not compiled" >&2; \
	fi

clean::
	$(RM) *.hyp *.ref *.stg *.uls *.ulh *.uhs *.ula *.uha udo*.gif xaaes.html xaaes.txt
