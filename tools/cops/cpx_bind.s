
IMPORT	malloc
IMPORT	free

IMPORT	cpx_desc_list		;CPX_DESC *cpx_desc_list

IMPORT	cpx_main_loop

IMPORT	open_cpx_context	;void open_cpx_context(CPX_DESC *cpx_desc)

IMPORT	cpx_fix_rsh		;void cdecl cpx_fix_rsh(CPX_DESC *,...
IMPORT	cpx_rsh_obfix		;void cdecl cpx_rsh_obfix(CPX_DESC *cpx_desc, OBJECT *tree, WORD ob)
IMPORT	cpx_form_do		;CPX_DESC *cpx_form_do(CPX_DESC *cpx_desc, OBJECT *tree, WORD edit_obj, WORD *msg);
IMPORT	cpx_get_first_rect	;GRECT *cpx_get_first_rect(CPX_DESC *cpx_desc, GRECT *redraw_area);
IMPORT	cpx_get_next_rect	;GRECT *cpx_get_next_rect(CPX_DESC *cpx_desc);
IMPORT	cpx_set_evnt_mask	;void cpx_set_evnt_mask(CPX_DESC *cpx_desc, WORD mask, MOBLK *m1, MOBLK *m2, LONG time)
IMPORT	cpx_get_tmp_buffer	;void *cpx_get_tmp_buffer(CPX_DESC *cpx_desc);
IMPORT	cpx_save_data		;WORD cpx_save_data(CPX_DESC *cpx_desc, void *ptr, long bytes);

IMPORT	get_cookie		;WORD get_cookie(LONG id, LONG *value)

EXPORT	get_cpx_desc

EXPORT	a_call_main
EXPORT	new_context		;void new_context(CPX_DESC *cpx_desc);
EXPORT	switch_context		;void switch_context(CPX_DESC *cpx_desc);

EXPORT	get_mouse_form		;void get_mouse_form(MFORM *mf);

EXPORT	rsh_fix
EXPORT	rsh_obfix
EXPORT	Xform_do		;WORD cdecl Xform_do(OBJECT *tree, WORD edit_obj, WORD *msg);
EXPORT	GetFirstRect		;GRECT *cdecl GetFirstRect(GRECT *redraw_area);
EXPORT	GetNextRect		;GRECT *GetNextRect(void);
EXPORT	Set_Evnt_Mask		;void cdecl Set_Evnt_Mask(WORD mask, MOBLK *m1, MOBLK *m2, long time);
EXPORT	Get_Buffer		;void *cdecl Get_Buffer(void);

EXPORT	CPX_Save		;WORD cdecl CPX_Save(void *ptr, long bytes)


;Funktionen aus der CPXINFO-Struktur aufrufen
EXPORT	cpx_init		;CPXINFO *cpx_init(CPX_DESC *cpx_desc, XCPB *xcpb);
EXPORT	cpx_call		;WORD cpx_call(CPX_DESC *cpx_desc, GRECT *work);
EXPORT	cpx_draw		;void cpx_draw(CPX_DESC *cpx_desc, GRECT *clip);
EXPORT	cpx_wmove		;void cpx_wmove(CPX_DESC *cpx_desc, GRECT *work);
EXPORT	cpx_timer		;WORD cpx_timer(CPX_DESC *cpx_desc);
EXPORT	cpx_key			;WORD cpx_key(CPX_DESC *cpx_desc, WORD kstate, WORD key);
EXPORT	cpx_button		;WORD cpx_button(CPX_DESC *cpx_desc, MRETS *mrets, WORD nclicks);
EXPORT	cpx_m1			;WORD cpx_m1(CPX_DESC *cpx_desc, MRETS *mrets);
EXPORT	cpx_m2			;WORD cpx_m2(CPX_DESC *cpx_desc, MRETS *mrets);
EXPORT	cpx_hook		;WORD cpx_hook(CPX_DESC *cpx_desc, WORD event, WORD *msg, MRETS *mrets, WORD *key, WORD *nclicks);
EXPORT	cpx_close		;void cpx_close(CPX_DESC *cpx_desc, WORD flag);

EXPORT	cpx_userdef		;void cpx_userdef(void (*cpx_userdef)(void));

EXPORT	clear_cpu_caches	;LONG clear_cpu_caches(void);


AC_CLOSE	EQU	41


OFFSET				;typedef struct
				;{
CPXINFO_call:	DS.l	1	;	cpx_call    WORD cdecl  (*cpx_call)(GRECT *rect);
CPXINFO_draw:	DS.l	1	;	void cdecl  (*cpx_draw)(GRECT *clip);
CPXINFO_wmove:	DS.l	1	;	void cdecl  (*cpx_wmove)(GRECT *work);
CPXINFO_timer:	DS.l	1	;	void cdecl  (*cpx_timer)(WORD *quit);
CPXINFO_key:	DS.l	1	;	void cdecl  (*cpx_key)(WORD kstate, WORD key, WORD *quit);
CPXINFO_button:	DS.l	1	;	void cdecl  (*cpx_button)(MRETS *mrets, WORD nclicks, WORD *quit);
CPXINFO_m1:	DS.l	1	;	void cdecl  (*cpx_m1)(MRETS *mrets, WORD *quit);
CPXINFO_m2:	DS.l	1	;	void cdecl  (*cpx_m2)(MRETS *mrets, WORD *quit);
CPXINFO_hook:	DS.l	1	;	WORD cdecl  (*cpx_hook)(WORD event, WORD *msg, MRETS *mrets, WORD *key, WORD *nclicks);
CPXINFO_close:	DS.l	1	;	void cdecl  (*cpx_close)(WORD flag);
				;} CPXINFO;


OFFSET				;typedef struct _cpx_desc
				;{
CPXD_next:	DS.l	1	;	struct _cpx_desc *next; /* Zeiger auf die naechste CPX-Beschreibung oder 0L */

CPXD_start:	DS.l	1	;	void	*start_of_cpx; /* Startadresse des CPX im Speicher */
CPXD_end:	DS.l	1	;	void	*end_of_cpx; /* Endadresse des CPX im Speicher */
CPXD_sp_memory:	DS.l	1	;	void	*sp_memory;

CPXD_context:	DS.l	16	;	void	*context[16];
CPXD_return_addr:DS.l	1	;	void	*return_addr; /* temporaere Ruecksprungadresse */

CPXD_info:	DS.l	1	;	CPXINFO	*info; /* Zeiger auf CPXINFO-Struktur die bei cpx_call() zurueckgeliefert wird */

CPXD_dialog:	DS.l	1	;	DIALOG	*dialog; /* Zeiger auf die Dialogbeschreibung oder 0L, wenn das CPX nicht offen ist */
CPXD_tree:	DS.l	1	;	OBJECT	*tree; /* ist bei Form-CPX 0L, bis cpx_form_do() aufgerufen wird */
CPXD_whdl:	DS.w	1	;	WORD	whdl; /* Handle des Dialogfensters */
CPXD_window_x:	DS.w	1	;	WORD	window_x; /* Position des Dialogfensters */
CPXD_window_y:	DS.w	1	;	WORD	window_y;
	
CPXD_is_evnt_cpx:DS.w	1	;	WORD	is_evnt_cpx; /* 0: Form-CPX 1: Event-CPX */
CPXD_button:	DS.w	1	;	WORD	button;
CPXD_msg:	DS.l	1


M_POS_HX 	EQU -$0358	;x-Hot-Spot
M_POS_HY 	EQU -$0356	;y-Hot-Spot
M_PLANES 	EQU -$0354	;Anzahl der Ebenen (immer 1)
M_CDB_BG 	EQU -$0352	;Hintergrundfarbe
M_CDB_FG 	EQU -$0350	;Vordergrundfarbe
MASK_FORM	EQU -$034e	;Maskendaten


OFFSET				;typedef struct mfstr
				;{
MFORM_xhot:	DS.w	1	;	int  mf_xhot;
MFORM_yhot:	DS.w	1	;	int  mf_yhot;
MFORM_nplanes:	DS.w	1	;	int  mf_nplanes;
MFORM_fg:	DS.w	1	;	int  mf_fg;
MFORM_bg:	DS.w	1	;	int  mf_bg;
MFORM_mask:	DS.w	16	;	int  mf_mask[16];
MFORM_data:	DS.w	16	;	int  mf_data[16];
sizeof_MFORM:			;} MFORM;

TEXT

;CPX_DESC *get_cpx_desc(void *return_address);
;Anhand der Ruecksprungadresse CPX_DESC-Struktur heraussuchen
;Eingaben:
;a0.l Zeiger auf Ruecksprungadresse
;Ausgaben:
;a0.l Zeiger auf CPX_DESC-Struktur
get_cpx_desc:
	movea.l	cpx_desc_list(pc),a1
					
get_cpxd_loop:
	cmpa.l	CPXD_start(a1),a0
	blo.s	get_cpxd_next		;Adresse ist vor dem CPX-Bereich
	cmpa.l	CPXD_end(a1),a0
	bls.s	get_cpxd_found		;Adresse ist innerhalb des CPX-Bereichs
get_cpxd_next:
	move.l	(a1),a1			;next
	move.l	a1,d0
	bne.s	get_cpxd_loop

	suba.l	a0,a0			;Fehler
	rts

get_cpxd_found:
	movea.l	a1,a0			;Zeiger auf CPX_DESC-Struktur
	rts

;void cdecl rsh_fix(WORD num_objs, WORD num_frstr, WORD num_frimg, WORD num_tree,
;		OBJECT *rs_object, TEDINFO *rs_tedinfo, char *rs_strings[], ICONBLK *rs_iconblk,
;	 	BITBLK *rs_bitblk, long *rs_frstr, long *rs_frimg, long *rs_trindex, struct foobar *rs_imdope);
;Eingaben:
;siehe oben
;Ausgaben:
;-
rsh_fix:
	movea.l	(sp),a0			;Ruecksprungadresse
	bsr	get_cpx_desc		;CPX_DESC suchen
	move.l	a0,d0
	beq.s	rsh_fix_err		;kein passendes CPX gefunden

	move.l	(sp)+,CPXD_return_addr(a0)
	move.l	a0,-(sp)		;CPX_DESC *
	bsr	cpx_fix_rsh		;void cdecl cpx_fix_rsh(CPX_DESC *,...
	movea.l	(sp)+,a0
	move.l	CPXD_return_addr(a0),-(sp)
	rts
	
rsh_fix_err:
	moveq	#0,d0			;Behandlung fuer schwerwiegenden Fehler
	rts

;void cdecl rsh_obfix(OBJECT *tree, WORD ob)
rsh_obfix:
	movea.l	(sp),a0			;Ruecksprungadresse
	bsr	get_cpx_desc		;CPX_DESC suchen
	move.l	a0,d0
	beq.s	rsh_obfix_err		;kein passendes CPX gefunden

	move.l	(sp)+,CPXD_return_addr(a0)
	move.l	a0,-(sp)		;CPX_DESC *
	bsr	cpx_rsh_obfix		;void cdecl cpx_rsh_obfix(CPX_DESC *,...
	movea.l	(sp)+,a0
	move.l	CPXD_return_addr(a0),-(sp)
	rts
	
rsh_obfix_err:
	moveq		#0,d0		;Behandlung fuer schwerwiegenden Fehler
	rts

;WORD cdecl Xform_do(OBJECT *tree, WORD edit_obj, WORD *msg);
;form_do() fuer ein CPX ausfuehren
;Eingaben:
;4(sp).l OBJECT	*tree
;8(sp).w WORD		edit_obj
;10(sp).l WORD		*msg
;Ausgaben:
;-
Xform_do:
	movea.l	(sp),a0			;Ruecksprungadresse
	bsr	get_cpx_desc		;CPX_DESC suchen
	move.l	a0,d0
	beq.s	Xform_do_err		;kein passendes CPX gefunden

	movem.l	d0-d7/a1-a7,CPXD_context(a0)

	movea.l	4(sp),a1		;tree
	move.w	8(sp),d0		;edit_obj
	move.l	10(sp),d1		;msg
	
	movea.l	kernel_stack(pc),sp
	move.l	d1,-(sp)		;msg
	bsr	cpx_form_do		;CPX_DESC *cpx_form_do(CPX_DESC *cpx_desc,
					;		       OBJECT *tree, WORD edit_obj, WORD *msg)
	addq.l	#4,sp			;Stack korrigieren
	move.l	a0,d0
	beq	a_call_return		;Hauptfenster wurde geschlossen
	bra	switch_context		;dieser Fall darf nicht auftreten!
	
;Behandlung fuer schwerwiegenden Fehler
Xform_do_err:
	move.l	10(sp),a0		;msg
	move.w	#AC_CLOSE,(a0)		;CPX wieder schliessen
	moveq	#-1,d0
	rts

;GRECT * cdecl	GetFirstRect(GRECT *redraw_area);
;Eingaben:
;4(sp).l GRECT *redraw_area
;Ausgaben
;d0.l GRECT *
GetFirstRect:
	movea.l	(sp),a0			;Ruecksprungadresse
	bsr	get_cpx_desc		;CPX_DESC suchen
	move.l	a0,d0
	beq.s	GetFirstRect_err	;kein passendes CPX gefunden

	movea.l	4(sp),a1		;prect
	bsr	cpx_get_first_rect	;GRECT	*cpx_get_first_rect(CPX_DESC *cpx_desc, GRECT *redraw_area)
	move.l	a0,d0			;GRECT *
	rts

GetFirstRect_err:
	moveq	#0,d0			;Behandlung fuer schwerwiegenden Fehler
	rts

;GRECT *GetNextRect(void);
;Eingaben:
;-
;Ausgaben
;d0.l GRECT *
GetNextRect:
	movea.l	(sp),a0			;Ruecksprungadresse
	bsr	get_cpx_desc		;CPX_DESC suchen
	move.l	a0,d0
	beq.s	GetNextRect_err		;kein passendes CPX gefunden

	bsr	cpx_get_next_rect	;GRECT	*cpx_get_next_rect(CPX_DESC *cpx_desc)
	move.l	a0,d0			;GRECT *
	rts

GetNextRect_err:
	moveq	#0,d0			;Behandlung fuer schwerwiegenden Fehler
	rts

;void cdecl Set_Evnt_Mask(WORD mask, MOBLK *m1, MOBLK *m2, long time);
;Eingaben:
;4(sp).w WORD mask
;6(sp).l MOBLK *m1
;10(sp).l MOBLK *m2
;14(sp).l LONG time
;Ausgaben:
;-
Set_Evnt_Mask:
	movea.l	(sp),a0			;Ruecksprungadresse
	bsr	get_cpx_desc		;CPX_DESC suchen
	move.l	a0,d0
	beq.s	a_set_evmask_err	;kein passendes CPX gefunden

	move.w	4(sp),d0		;mask
	move.l	14(sp),d1		;time
	movea.l	6(sp),a1		;m1
	move.l	10(sp),-(sp)		;m2
	bsr	cpx_set_evnt_mask	;void cpx_set_evnt_mask(CPX_DESC *cpx_desc, WORD mask,
					;			MOBLK *m1, MOBLK *m2, LONG time)
	addq.l	#4,sp
	rts

a_set_evmask_err:
	moveq	#0,d0			;Behandlung fuer schwerwiegenden Fehler
	rts

;void *cdecl Get_Buffer(void);
;Eingaben:
;-
;Ausgaben
;d0.l void *
Get_Buffer:
	movea.l	(sp),a0			;Ruecksprungadresse
	bsr	get_cpx_desc		;CPX_DESC suchen
	move.l	a0,d0
	beq.s	Get_Buffer_err		;kein passendes CPX gefunden

	bsr	cpx_get_tmp_buffer	;void	*cpx_get_tmp_buffer(CPX_DESC *cpx_desc);
	move.l	a0,d0			;void *
	rts

Get_Buffer_err:
	moveq	#0,d0			;Behandlung fuer schwerwiegenden Fehler
	rts

;WORD cdecl CPX_Save(void *ptr, long bytes)
;Eingaben:
;4(sp).l void *ptr
;8(sp).l long bytes
;Ausgaben:
;d0.w
CPX_Save:
	movea.l	(sp),a0			;Ruecksprungadresse
	bsr	get_cpx_desc		;CPX_DESC suchen
	move.l	a0,d0
	beq.s	CPX_save_err		;kein passendes CPX gefunden

	movea.l	4(sp),a1
	move.l	8(sp),d0
	bsr	cpx_save_data		;WORD cpx_save_data(CPX_DESC *cpx_desc, void *ptr, long bytes)
	rts

CPX_save_err:
	moveq	#0,d0
	rts
	
a_call_main:
	movem.l	d0-d7/a0-a7,alpha_context	;Kontext des Hauptprogramms
	move.l	#a_call_return,-(sp)	;Rueckkehrfunktion ins Hauptprogramm
	move.l	sp,kernel_stack
	bsr	cpx_main_loop		;Eventschleife

a_call_return:
	movem.l	alpha_context(pc),d0-d7/a0-a7
	rts

;void new_context(CPX_DESC *cpx_desc);
;Neuen Kontext anlegen, CPX oeffnen, cpx_init() und cpx_call() aufrufen
;Eingaben:
;a0.l CPX_DESC *cpx_desc
;Ausgaben:
;-
new_context:
	movem.l	d0-d1/a0-a1,-(sp)
	move.l	#16384,d0
	bsr	malloc			;16384 Bytes fuer temporaeren Stack anfordern
	move.l	a0,d2
	movem.l	(sp)+,d0-d1/a0-a1

	move.l	d2,CPXD_sp_memory(a0)	;Speicherbereich fuer Stack merken
	beq.s	new_context_err

	add.l	#16384,d2
	move.l	d2,sp			;neuer Stack fuer Eventloop
	
	movem.l	d0-d7/a0-a7,-(sp)	;CPX oeffnen, ggf. Eventloop aufrufen
	bsr	open_cpx_context	;void	open_cpx_context(CPX_DESC *cpx_desc)
	movem.l	(sp)+,d0-d7/a0-a7

	movea.l	kernel_stack(pc),sp
	
	movea.l	CPXD_sp_memory(a0),a0	;Speicherbereich fuer den Stack freigeben
	bsr	free			;Stack freigeben

	bra	cpx_main_loop		;Hauptschleife aufrufen
	
new_context_err:
	moveq	#0,d0
	rts

;void switch_context(CPX_DESC *cpx_desc)
;Kontext wechseln, CPX anspringen
;Eingaben:
;a0.l CPX_DESC *cpx_desc
;Ausgaben:
;-
switch_context:
	movem.l	CPXD_context(a0),d0-d7/a1-a7
	move.w	CPXD_button(a0),d0	;Objektnummer oder Message
	rts


;CPXINFO *cpx_init(CPX_DESC *cpx_desc, XCPB *xcpb)
;cpx_init() aufrufen, davor Register a2 sichern
;Eingaben:
;a0.l CPX_DESC *cpx_desc
;a1.l XCPB *xcpb
;Ausgaben:
;a0.l CPXINFO *
cpx_init:
	move.l	a2,-(sp)
	movea.l	CPXD_start(a0),a0	;CPXINFO * cdecl (*cpx_init)(XCPB *xcpb)

	move.l	#$10001,-(sp)
	move.l	#'COPS',-(sp)		;Kennung
	move.l	a1,-(sp)		;XCPB *xcpb
	jsr	(a0)
	lea	12(sp),sp		;Stack korrigieren
	movea.l	d0,a0			;Rueckgabewert

	movea.l	(sp)+,a2
	rts

;WORD cpx_call(CPX_DESC *cpx_desc, GRECT *work)
;cpx_call() aufrufen, davor Register a2 sichern
;Eingaben:
;a0.l CPX_DESC *cpx_desc
;a1.l GRECT *work
;Ausgaben:
;d0.w 0: CPX schliessen 1: weitermachen
cpx_call:
	move.l	a2,-(sp)

	move.l	CPXD_dialog(a0),-(sp)	;DIALOG *dialog
	move.l	a1,-(sp)		;GRECT *work

	movea.l	CPXD_info(a0),a0	;Zeiger auf CPXINFO-Struktur
	movea.l	CPXINFO_call(a0),a0	;WORD cdecl (*cpx_call)(GRECT *work)
	jsr	(a0)
	addq.l	#8,sp			;Stack korrigieren

	movea.l	(sp)+,a2
	rts

;void cpx_draw(CPX_DESC *cpx_desc, GRECT *clip)
;cpx_draw() aufrufen, davor Register a2 sichern
;Eingaben:
;a0.l CPX_DESC *cpx_desc
;a1.l GRECT *clip
;Ausgaben:
;-
cpx_draw:
	move.l	a2,-(sp)
	movea.l	CPXD_info(a0),a0	;Zeiger auf CPXINFO-Struktur
	move.l	CPXINFO_draw(a0),d2
	beq.s	cpx_draw_exit

	movea.l	d2,a0			;void cdecl (*cpx_draw)(GRECT *clip)
	move.l	a1,-(sp)		;GRECT *clip
	jsr	(a0)
	addq.l	#4,sp			;Stack korrigieren

cpx_draw_exit:
	movea.l	(sp)+,a2
	rts

;void cpx_wmove(CPX_DESC *cpx_desc, GRECT *work)
;cpx_wmove() aufrufen, davor Register a2 sichern
;Eingaben:
;a0.l CPX_DESC *cpx_desc
;a1.l GRECT *work
;Ausgaben:
;-
cpx_wmove:
	move.l	a2,-(sp)
	movea.l	CPXD_info(a0),a0	;Zeiger auf CPXINFO-Struktur
	move.l	CPXINFO_wmove(a0),d0
	beq.s	cpx_wmove_exit

	movea.l	d0,a0			;void cdecl (*cpx_wmove)(GRECT *clip)
	move.l	a1,-(sp)		;GRECT *wmove
	jsr	(a0)
	addq.l	#4,sp			;Stack korrigieren

cpx_wmove_exit:
	movea.l	(sp)+,a2
	rts

;WORD cpx_timer(CPX_DESC *cpx_desc)
;cpx_timer() aufrufen, davor Register a2 sichern
;Eingaben:
;a0.l CPX_DESC *cpx_desc
;Ausgaben:
;d0.w 0: weitermachen 1: CPX schliessen
cpx_timer:
	move.l	a2,-(sp)
	movea.l	CPXD_info(a0),a0	;Zeiger auf CPXINFO-Struktur
	move.l	CPXINFO_timer(a0),d2
	beq.s	cpx_continue

	movea.l	d2,a0			;void cdecl (*cpx_timer)(WORD *event)
	clr.w	-(sp)
	move.l	sp,-(sp)		;WORD *event
	jsr	(a0)
	addq.l	#4,sp			;Stack korrigieren
	move.w	(sp)+,d0		;WORD	event

	movea.l	(sp)+,a2
	rts

cpx_continue:
	moveq	#0,d0			;weitermachen, CPX nicht schliessen
	movea.l	(sp)+,a2
	rts

;WORD cpx_key(CPX_DESC *cpx_desc, WORD kstate, WORD key)
;cpx_key() aufrufen, davor Register a2 sichern
;Eingaben:
;d0.w kstate
;d1.w key
;a0.l CPX_DESC *cpx_desc
;Ausgaben:
;d0.w 0: weitermachen 1: CPX schliessen
cpx_key:
	move.l	a2,-(sp)
	movea.l	CPXD_info(a0),a0	;Zeiger auf CPXINFO-Struktur
	move.l	CPXINFO_key(a0),d2
	beq.s	cpx_continue

	movea.l	d2,a0			;void cdecl (*cpx_key)(WORD kstate, WORD key, WORD *event)
	clr.w	-(sp)			;WORD event
	move.l	sp,-(sp)		;WORD *event
	move.w	d1,-(sp)		;WORD key
	move.w	d0,-(sp)		;WORD kstate
	jsr	(a0)
	addq.l	#8,sp			;Stack korrigieren
	move.w	(sp)+,d0		;WORD event

	movea.l	(sp)+,a2
	rts

;WORD	cpx_button(CPX_DESC *cpx_desc, MRETS *mrets, WORD nclicks)
;cpx_button() aufrufen, davor Register a2 sichern
;Eingaben:
;d0.w nclicks
;a0.l CPX_DESC *cpx_desc
;a1.l MRETS *mrets
;Ausgaben:
;d0.w 0: weitermachen 1: CPX schliessen
cpx_button:
	move.l	a2,-(sp)
	movea.l	CPXD_info(a0),a0	;Zeiger auf CPXINFO-Struktur
	move.l	CPXINFO_button(a0),d2
	beq.s	cpx_continue

	movea.l	d2,a0			;void cdecl (*cpx_button)(MRETS *mrets, WORD nclicks, WORD *event)
	clr.w	-(sp)			;WORD event
	move.l	sp,-(sp)		;WORD *event
	move.w	d0,-(sp)		;WORD nclicks
	move.l	a1,-(sp)		;MRETS *mrets
	jsr	(a0)
	lea	10(sp),sp		;Stack korrigieren
	move.w	(sp)+,d0		;WORD event

	movea.l	(sp)+,a2
	rts

;WORD cpx_m1(CPX_DESC *cpx_desc, MRETS *mrets)
;cpx_m1() aufrufen, davor Register a2 sichern
;Eingaben:
;a0.l CPX_DESC *cpx_desc
;a1.l MRETS *mrets
;Ausgaben:
;d0.w 0: weitermachen 1: CPX schliessen
cpx_m1:
	move.l	a2,-(sp)
	movea.l	CPXD_info(a0),a0	;Zeiger auf CPXINFO-Struktur
	move.l	CPXINFO_m1(a0),d2
	beq.s	cpx_continue

	movea.l	d2,a0			;void cdecl (*cpx_m1)(MRETS *mrets, WORD *event)
	clr.w	-(sp)			;WORD event
	move.l	sp,-(sp)		;WORD *event
	move.l	a1,-(sp)		;MRETS *mrets
	jsr	(a0)
	addq.l	#8,sp			;Stack korrigieren
	move.w	(sp)+,d0		;WORD event

	movea.l	(sp)+,a2
	rts

;WORD cpx_m2(CPX_DESC *cpx_desc, MRETS *mrets)
;cpx_m2() aufrufen, davor Register a2 sichern
;Eingaben:
;a0.l CPX_DESC *cpx_desc
;a1.l MRETS *mrets
;Ausgaben:
;d0.w 0: weitermachen 1: CPX schliessen
cpx_m2:
	move.l	a2,-(sp)
	movea.l	CPXD_info(a0),a0	;Zeiger auf CPXINFO-Struktur
	move.l	CPXINFO_m2(a0),d2
	beq.s	cpx_continue

	movea.l	d2,a0			;void cdecl (*cpx_m2)(MRETS *mrets, WORD *event)
	clr.w	-(sp)			;WORD event
	move.l	sp,-(sp)		;WORD *event
	move.l	a1,-(sp)		;MRETS *mrets
	jsr	(a0)
	addq.l	#8,sp			;Stack korrigieren
	move.w	(sp)+,d0		;WORD event

	movea.l	(sp)+,a2
	rts

;WORD cpx_hook(CPX_DESC *cpx_desc, WORD event, WORD *msg, MRETS *mrets, WORD *key, WORD *nclicks)
;cpx_button() aufrufen, davor Register a2 sichern
;Eingaben:
;d0.w event
;a0.l CPX_DESC *cpx_desc
;a1.l WORD *msg
;4(sp).l MRETS *mrets
;8(sp).l WORD *key
;12(sp).l WORD *nclicks
;Ausgaben:
;d0.w 0: weitermachen 1: CPX schliessen
cpx_hook:
	move.l	a2,-(sp)
	movea.l	CPXD_info(a0),a0	;Zeiger auf CPXINFO-Struktur
	move.l	CPXINFO_hook(a0),d2
	beq	cpx_continue

	movea.l	d2,a0			;WORD cdecl (*cpx_hook)(WORD event, WORD *msg, MRETS *mrets,
					;			WORD *key, WORD *nclicks)
	move.l	16(sp),-(sp)		;WORD *nclicks
	move.l	16(sp),-(sp)		;WORD *key
	move.l	16(sp),-(sp)		;MRETS *mrets
	move.l	a1,-(sp)		;WORD *msg
	move.w	d0,-(sp)		;WORD event
	jsr	(a0)
	lea	18(sp),sp		;Stack korrigieren

	movea.l	(sp)+,a2
	rts

;void	cpx_close(CPX_DESC *cpx_desc, WORD flag)
;cpx_close() aufrufen, davor Register a2 sichern
;Eingaben:
;d0.w flag
;a0.l CPX_DESC *cpx_desc
;Ausgaben:
;-
cpx_close:
	move.l	a2,-(sp)
	movea.l	CPXD_info(a0),a0	;Zeiger auf CPXINFO-Struktur
	move.l	CPXINFO_close(a0),d2
	beq.s	cpx_close_exit

	movea.l	d2,a0			;void cdecl (*cpx_close)(WORD flag)
	move.w	d0,-(sp)		;WORD flag
	jsr	(a0)
	addq.l	#2,sp			;Stack korrigieren

cpx_close_exit:
	movea.l	(sp)+,a2
	rts

;void cpx_userdef(void (*cpx_userdef)(void));
;Eingaben:
;a0.l void (*cpx_userdef)(void)
;Ausgaben:
;-
cpx_userdef:
	move.l	a2,-(sp)
	move.l	a0,d0
	beq.s	cpx_userdef_exit
	jsr	(a0)
cpx_userdef_exit:
	movea.l	(sp)+,a2
	rts

;void get_mouse_form(MFORM *mf);
;Eingaben:
;a0.l MFORM *mf
;Ausgaben:
;-
get_mouse_form:
	move.l	a2,-(sp)
	move.l	a0,-(sp)
	DC.w	$a000
	movea.l	(sp)+,a1
	lea	M_POS_HX(a0),a0		;Zeiger auf die LineA-Mausform
	move.l	(a0)+,(a1)+		;mf_xhot / mf_yhot
	move.w	(a0)+,(a1)+		;mf_nplanes
	move.l	(a0)+,(a1)+		;mf_fg / mf_bg
	lea	MFORM_data-MFORM_mask(a1),a2
	moveq	#15,d0

get_mform_loop:
	move.w	(a0)+,(a1)+		;Maske
	move.w	(a0)+,(a1)+		;Vordergrund
	dbra	d0,get_mform_loop

	movea.l	(sp)+,a2
	rts

	MC68040


;LONG clear_cpu_caches(void);
clear_cpu_caches:
	clr.l	-(sp)
	move.l	#'_CPU',d0		;CPU-Typ suchen
	movea.l	sp,a0
	bsr	get_cookie		;WORD get_cookie(LONG id, LONG *value)
	move.l	(sp)+,d1		;Inhalt des Cookies
	tst.w	d0
	bne.s	clear_cpu040
	moveq	#0,d1
clear_cpu040:
	move.w	sr,-(sp)
	ori	#$700,sr		;Interrupts sperren

	cmpi.w	#40,d1			;68040?
	blt.s	clear_cpu030

	cpusha	bc			;beide Caches loeschen
	bra.s	clear_cpu_exit

clear_cpu030:
	cmp.w	#20,d1			;68020 oder 68030?
	blt.s	clear_cpu_exit

	movec	cacr,d0
	or.l	#$808,d0		;Daten- und Instruktionscache loeschen
	movec	d0,cacr

clear_cpu_exit:
	move.w	(sp)+,sr		;Statusregister zurueck
	rts	

	MC68000

	BSS
	
kernel_stack:
	DS.l		1		;Zeiger auf den Kernelstack von ungefaehr 4K
alpha_context:
	DS.l		16		;Kontext um wieder ins Hauptprogramm zu wechseln

	END
