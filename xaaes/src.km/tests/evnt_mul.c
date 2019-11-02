#include "aestests.h"

#undef SIGUSR1
#define SIGUSR1 29
#undef SIGUSR2
#define SIGUSR2 30
#undef SIGTERM
#define SIGTERM 15


static void cdecl handler(long signr)
{
	long i;

	printf("handler: got signal %ld.\n", signr);
	fflush(stdout);
	(void) Cconws("waiting...");
	for (i = 0; i < 7000000L; i++)
		;
	(void) Cconws("...OK\r\n");
}

int main(void)
{
	long ret;
	_WORD msg[8];
	_WORD ret2;
	_WORD ev_mwhich;
	_WORD ev_mbreturn;
	_WORD keycode;
	_WORD kstate;
	_WORD ev_mmobutton;
	_WORD ev_mmox;
	_WORD ev_mmoy;

	if (0 > appl_init())
		return -1;
	printf("My ProcID is %d.\n", Pgetpid());
	ret = (long) Psignal(SIGUSR1, handler);
	printf("Psignal => %ld\n", ret);
	ret = (long) Psignal(SIGUSR2, handler);
	printf("Psignal => %ld\n", ret);
	ret = (long) Psignal(SIGTERM, handler);
	printf("Psignal => %ld\n", ret);
	for (;;)
	{
		ev_mwhich = evnt_multi(MU_KEYBD + MU_BUTTON + MU_MESAG + MU_TIMER, 2,	/* reconoze double-clicks*/
							   1,		/* left mousebutton only */
							   1,		/* left mousebutton pressed */
							   0, 0, 0, 0, 0,	/* no 1st rectangle         */
							   0, 0, 0, 0, 0,	/* no 2nd rectangle         */
							   msg,
							   1000,	/* ms */
							   &ev_mmox, &ev_mmoy, &ev_mmobutton, &kstate, &keycode, &ev_mbreturn);
		printf("evnt_multi => %d\n", ev_mwhich);
		if (ev_mwhich & MU_MESAG)
			for (ret2 = 0; ret2 < 8; ret2++)
				printf("msg[%d] == %02x\n", ret2, msg[ret2]);
		printf("\n");
		fflush(stdout);

		if (((ev_mwhich & MU_MESAG)) && msg[0] == AP_TERM)
			break;
		if ((ev_mwhich & MU_KEYBD) && (keycode & 0xff) == 0x1b)
			break;
	}
	
	appl_exit();
	
	return 0;
}
