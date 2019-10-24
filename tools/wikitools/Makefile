
%.wu: %.xml
	w2u $<

%.wik: %.u
	u2w $<

%.wik: %.wu
	u2w $<

%.html: %.u
	udo -h -o $(@:.u=.html) $<
	sed -i /"udo.*\.gif"/d $(@:.wu=.html)

%.html: %.wu
	udo -h -o $(@:.wu=.html) $<
	sed -i /"udo.*\.gif"/d $(@:.wu=.html)

%.html: %.xml
	w2u $<
	udo -h -o $(@:.xml=.html) $(<:.xml=.wu)
	sed -i /"udo.*\.gif"/d $(@:.wu=.html)

%.txt: %.u
	udo -a -o $(@:.u=.txt) $<

%.txt: %.wu
#	udo -a -o $(@:.wu=.txt) $<

%.txt: %.xml
	w2u $<
	udo -a -o $(@:.xml=.txt) $(<:.xml=.wu)

%.u.n: %.u
	egrep "(^!(sub)*node|^!label)" $< >$@



clean::
	rm	-f *.ul? *.uh? udo_hm.gif udo_nohm.gif	udo_nolf.gif udo_norg.gif udo_noup.gif 0*.html indexudo.html

