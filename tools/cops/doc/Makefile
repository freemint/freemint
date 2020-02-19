UDO = udo
HCP = hcp

LANGUAGES = en de

IMAGES = cops.img

all: cops_en.hyp cops_de.hyp

cops_en.hyp: cops_en.u $(IMAGES)
	$(UDO) --stg -o cops_en.stg $<
	$(HCP) -o$@ cops_en.stg

cops_de.hyp: cops_de.u $(IMAGES)
	$(UDO) --stg -o cops_de.stg $<
	$(HCP) -o$@ cops_de.stg

clean::
	$(RM) *.hyp *.ref *.stg *.uls *.uhs
