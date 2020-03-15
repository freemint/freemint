ifeq ($(UDO),)
UDO = udo
endif
ifeq ($(HCP),)
HCP = hcp
endif

.PHONY: all

all: xaaes.html xaaes.txt xaaes.hyp

xaaes.html: xaaes.u
	@if $(UDO) --version >/dev/null 2>&1; then \
		echo $(UDO) --html -o $@ $<; \
		$(UDO) --html -o $@ $<; \
	else \
		echo "UDO not found, help file not compiled" >&2; \
	fi

xaaes.txt: xaaes.u
	@if $(UDO) --version >/dev/null 2>&1; then \
		echo $(UDO) --asc -o $@ $<; \
		$(UDO) --asc --no-warningslines -o $@ $<; \
	else \
		echo "UDO not found, help file not compiled" >&2; \
	fi

xaaes.stg: xaaes.u
	@if $(UDO) --version >/dev/null 2>&1; then \
		echo $(UDO) --stg -o $@ $<; \
		$(UDO) --stg --no-warningslines -o $@ $<; \
		sed -i 's/ RX / SYSTEM /' $@; \
	else \
		echo "UDO not found, help file not compiled" >&2; \
	fi

xaaes.hyp: xaaes.stg
	@if $(HCP) --version >/dev/null 2>&1 && test -f $<; then \
		echo $(HCP) -o $@ $<; \
		$(HCP) -o $@ $<; \
	else \
		echo "HCP not found, help file not compiled" >&2; \
	fi

clean::
	$(RM) *.hyp *.ref *.stg *.uls *.ulh *.uhs *.ula *.uha udo*.gif xaaes.html xaaes.txt
