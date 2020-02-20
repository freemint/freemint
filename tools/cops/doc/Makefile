UDO = udo
HCP = hcp

LANGUAGES = en de fr

IMAGES = cops.img

.SUFFIXES: .hyp .u

ALL_HYPS = $(addprefix cops_, $(addsuffix .hyp, $(LANGUAGES)))

all: $(ALL_HYPS)

.u.hyp:
	$(UDO) --stg -o $(notdir $(basename $@)).stg $<
	$(HCP) -o $@ $(notdir $(basename $@)).stg

$(ALL_HYPS): $(IMAGES)

clean::
	$(RM) *.hyp *.ref *.stg *.uls *.uhs
