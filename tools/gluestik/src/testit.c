#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osbind.h>
#include <mintbind.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#include "include/sockerr.h"
#include "portab.h"
#include "include/transprt.h"

# define __KERNEL__
# include "errno.h"

DRV_LIST *drivers = (DRV_LIST *)NULL;
TPL *tpl = (TPL *)NULL;

/* Put 'STIK' cookie value into drivers */

typedef struct {
    long cktag;
    long ckvalue;
} ck_entry;

static long init_drivers(void)
{
  long i = 0;
  ck_entry *jar = *((ck_entry **) 0x5a0);

  while (jar[i].cktag) {
    if (!strncmp((char *)&jar[i].cktag, CJTAG, 4)) {
      drivers = (DRV_LIST *)jar[i].ckvalue;
      return (0);
    }
    ++i;
  }
  return (0);  /* Pointless return value...  */
}

void errtext_test(void)
{
  static int errs[] = {E_NORMAL, E_EOF, E_BADHANDLE, E_CONNECTFAIL, E_LOCKED,
		       -1000 + EWOULDBLOCK, -1000 + ENOMEM, E_NOMEM,
		       -999};
  int n;
  char *s;

  printf("\nTesting error messages...\n");
  printf("\t[get_err_text = %p]\n", (void *)(tpl->get_err_text));

  for (n = 0; n < sizeof(errs)/sizeof(int); n++) {
    s = get_err_text(errs[n]);
    printf("get_err_text(%d) returned \"%s\"\n", errs[n], (s?s:"<<<NULL>>>"));
  }
}

void stikvar_test(void)
{
  static char *vars[] = {"ALLOCMEM", "CDVALID", "BOGUSVAR", "GLUESTIK_TRACE",
			 "MULTITHREAD"};
  int n;
  char *s;

  printf("\nTesting STiK config vars...\n");
  printf("\t[getvstr = %p]\n", (void *)(tpl->getvstr));
  printf("\t[setvstr = %p]\n", (void *)(tpl->setvstr));

  printf("Before setvstr()'s:\n");
  for (n = 0; n < sizeof(vars)/sizeof(char *); n++) {
    s = getvstr(vars[n]);
    printf("getvstr(\"%s\") returns \"%s\"\n", vars[n], (s?s:"<<<NULL>>>"));
  }

  printf("setvstr(\"ALLOCMEM\", \"1000000\") returned %d\n",
	 setvstr("ALLOCMEM", "1000000"));
  printf("setvstr(\"BOGUSVAR\", \"not bogus\") returned %d\n",
	 setvstr("BOGUSVAR", "not bogus"));

  printf("After setvstr()'s:\n");
  for (n = 0; n < sizeof(vars)/sizeof(char *); n++) {
    s = getvstr(vars[n]);
    printf("getvstr(\"%s\") returns \"%s\"\n", vars[n], (s?s:"<<<NULL>>>"));
  }
}

void mem_test(void)
{
  char *mem1, *mem2, *mem3, *mem0;
  static char data[] = "Some test data";

  printf("\nTesting the memory pool...\n");
  printf("\t[KRmalloc = %p]\n", (void *)(tpl->KRmalloc));
  printf("\t[KRfree = %p]\n", (void *)(tpl->KRfree));
  printf("\t[KRgetfree = %p]\n", (void *)(tpl->KRgetfree));
  printf("\t[KRrealloc = %p]\n", (void *)(tpl->KRrealloc));

  printf("%ld bytes free, largest block %ld bytes\n",
	 KRgetfree(0), KRgetfree(1));
  mem1 = KRmalloc(50);
  printf("First KRmalloc(50) returns %p\n", (void *)mem1);
  mem2 = KRmalloc(50);
  printf("Second KRmalloc(50) returns %p\n", (void *)mem2);
  mem3 = KRmalloc(50);
  printf("Third KRmalloc(50) returns %p\n", (void *)mem3);
  printf("%ld bytes free, largest block %ld bytes\n",
	 KRgetfree(0), KRgetfree(1));

  KRfree(mem2);
  printf("Freed second block; now %ld bytes free, largest block %ld bytes\n",
	 KRgetfree(0), KRgetfree(1));

  strcpy(mem1, data);
  mem0 = KRrealloc(mem1, 80);
  printf("KRrealloc(80) on first block returns %p (should not have "
	 "changed%s)\n",
	 mem0, (mem0 == mem1 ? "" : " --- ERROR"));
  printf("Contents of first block has%s changed\n",
	 (strcmp(mem0, data) ? "" : " not"));
  printf("Now %ld bytes free, largest block %ld bytes\n",
	 KRgetfree(0), KRgetfree(1));
  mem1 = mem0;

  mem0 = KRrealloc(mem1, 150);
  printf("KRrealloc(150) on first block returns %p (should have "
	 "changed%s)\n",
	 mem0, (mem0 != mem1 ? "" : " --- ERROR"));
  printf("Contents of first block has %s\n",
	 (strcmp(mem0, data) ? "changed --- ERROR" : "not changed"));
  printf("Now %ld bytes free, largest block %ld bytes\n",
	 KRgetfree(0), KRgetfree(1));

  mem2 = KRrealloc(NULL, 85);
  printf("KRrealloc(NULL, 85) returns %p (should be where first block "
	 "originally was%s)\n",
	 mem2, (mem2 == mem1 ? "" : " --- ERROR"));
  printf("New block was %s\n", (*mem2 ? "not erased --- ERROR" : "erased"));
  printf("Now %ld bytes free, largest block %ld bytes\n",
	 KRgetfree(0), KRgetfree(1));

  KRfree(mem0);
  KRfree(mem2);
  KRfree(mem3);
  printf("All blocks freed; now %ld bytes free, largest block %ld bytes\n",
	 KRgetfree(0), KRgetfree(1));
}

void resolver_test(void)
{
  char *realname;
  unsigned long addrs[5];
  int n, i, j;
  static char *names[] = {"localhost", "FreeMiNT",
			"www.w3.org", "bogus.address"};

  printf("\nTesting the DNS resolver...\n");
  printf("\t[resolve = %p]\n", (void *)(tpl->resolve));

  for (j = 0; j < sizeof(names)/sizeof(char *); j++) {
    n = resolve(names[j], &realname, addrs, 5);
    if (n < 0) {
      printf("resolve(\"%s\") returned error code %d: %s\n",
	     names[j], n, get_err_text(n));
    } else {
      printf("resolve(\"%s\") returned %d addresses: ", 
	     names[j], n);
      for (i = 0; i < n; i++)
	printf(" %lu.%lu.%lu.%lu", addrs[i]>>24, (addrs[i]>>16)&0xFFL,
	       (addrs[i]>>8)&0xFFL, addrs[i]&0xFFL);
      printf("\n\tReal hostname was %s\n", realname);
      KRfree(realname);
    }
  }
}

void tcp_test(void)
{
  unsigned long hostaddr;
  int fd, ret, nchars = 0;
#if 0
  int waiting = 0;
#endif
  NDB *N;
  static char finger_host[] = "FreeMiNT",
	      finger_name[] = "dsb\r\n";
  char buf[10];

  printf("\nTesting the TCP functions...\n");
  printf("\t[TCP_open = %p]\n", (void *)(tpl->TCP_open));
  printf("\t[TCP_close = %p]\n", (void *)(tpl->TCP_close));
  printf("\t[TCP_send = %p]\n", (void *)(tpl->TCP_send));
  printf("\t[CNbyte_count = %p]\n", (void *)(tpl->CNbyte_count));
  printf("\t[CNget_char = %p]\n", (void *)(tpl->CNget_char));
  printf("\t[CNget_block = %p]\n", (void *)(tpl->CNget_block));
  printf("\t[CNget_NDB = %p]\n", (void *)(tpl->CNget_NDB));

  ret = resolve(finger_host, NULL, &hostaddr, 1);
  if (ret < 0) {
    printf("resolve(\"%s\") returned error code %d: %s\n",
	   finger_host, ret, get_err_text(ret));
    return;
  }

  fd = TCP_open(hostaddr, 79, 0, 0);
  if (fd < 0) {
    printf("TCP_open() returned error code %d: %s\n", fd, get_err_text(fd));
    return;
  } else {
    printf("TCP_open() returns socket handle %d\n", fd);
  }

#if 0
  /* E_OBUFFULL means the connection hasn't established yet; loop until it
   * goes away. */
  for (;;) {
    ret = TCP_send(fd, finger_name, strlen(finger_name));
    if (ret == E_OBUFFULL) {
      if (!waiting) {
	waiting = 1;
	printf("TCP_send() returned %d (\"%s\"); waiting...\n",
	       ret, get_err_text(ret));
      }
      continue;
    }
    if (ret < 0) {
      printf("TCP_send() returned error code %d: %s\n", ret,
	     get_err_text(ret));
      TCP_close(fd, 0);
      return;
    }
    /* We finally sent the thing... */
    break;
  }
#else
  ret = TCP_send(fd, finger_name, strlen(finger_name));
  if (ret < 0) {
    printf("TCP_send() returned error code %d: %s\n", ret,
	   get_err_text(ret));
    TCP_close(fd, 0);
    return;
  }
#endif

  ret = CNbyte_count(fd);
  if (ret < 0) {
    printf("CNbyte_count() returned error code %d: %s\n", ret, get_err_text(ret));
  } else {
    printf("CNbyte_count() reports %d bytes waiting on socket %d\n",
	   ret, fd);
  }

  for (;;) {
    ret = CNget_block(fd, buf, 10);
    if (ret == E_NODATA)
      continue;
    if (ret < 0) {
      printf("CNget_block() returned error code %d: %s\n", ret,
	     get_err_text(ret));
    }
    break;
  }
  if (ret >= 0) {
    printf("CNget_block() read %d bytes from socket %d\n", ret, fd);
  }

  {
    int ret2 = CNbyte_count(fd);

    if (ret2 < 0) {
      printf("After CNget_block(), "
	     "CNbyte_count() returned error code %d: %s\n",
	     ret2, get_err_text(ret2));
    } else {
      printf("After CNget_block(), "
	     "CNbyte_count() reports %d bytes waiting on socket %d\n",
	     ret2, fd);
    }
  }

  N = CNget_NDB(fd);
  if (!N) {
    printf("CNget_NDB() read nothing\n");
  } else {
    printf("CNget_NDB() read %u bytes from socket %d\n", N->len, fd);
  }

  {
    int ret2 = CNbyte_count(fd);

    if (ret2 < 0) {
      printf("After CNget_NDB(), "
	     "CNbyte_count() returned error code %d: %s\n",
	     ret2, get_err_text(ret2));
    } else {
      printf("After CNget_NDB(), "
	     "CNbyte_count() reports %d bytes waiting on socket %d\n",
	     ret2, fd);
    }
  }

  /* Now, print whatever these two managed to read and get the rest with
     CNget_char(). */
  if (ret > 0)
    printf("%.*s", ret, buf);
  if (N) {
    printf("%.*s", (int)N->len, N->ndata);
    if (N->ptr)
      KRfree(N->ptr);
    KRfree((char *)N);
  }

  for (;;) {
    ret = CNget_char(fd);
    if (ret == E_EOF)
      break;
    if (ret == E_NODATA)
      continue;
    if (ret < 0) {
      printf("\nCNget_char() returned error code %d: %s\n",
	     ret, get_err_text(ret));
      break;
    }
    nchars++;
    putchar(ret);
  }
  printf("\nRead %d bytes via CNget_char()\n", nchars);

  ret = CNbyte_count(fd);
  if (ret < 0) {
    printf("After EOF, CNbyte_count() returned error code %d: %s\n",
	   ret, get_err_text(ret));
  } else {
    printf("After EOF, CNbyte_count() reports %d bytes waiting on socket %d\n",
	   ret, fd);
  }
  ret = Finstat(fd);
  if (ret < 0) {
    printf("After EOF, Finstat() returned error code %d: %s\n",
	   ret, strerror(-ret));
  } else {
    printf("After EOF, Finstat() reports %d bytes waiting on socket %d\n",
	   ret, fd);
  }

  ret = TCP_close(fd, 0);
  if (ret < 0) {
    printf("TCP_close(%d) returned error code %d: %s\n",
	   fd, ret, get_err_text(ret));
  }
}

void NDB_test(void)
{
  unsigned long hostaddr;
  int fd, ret;
  NDB *N;
  static char finger_host[] = "FreeMiNT",
	      finger_name[] = "dsb\r\n";

  printf("\nTesting the TCP functions, with CNget_NDB() reads...\n");
  printf("\t[TCP_open = %p]\n", (void *)(tpl->TCP_open));
  printf("\t[TCP_close = %p]\n", (void *)(tpl->TCP_close));
  printf("\t[TCP_wait_state = %p]\n", (void *)(tpl->TCP_wait_state));
  printf("\t[TCP_send = %p]\n", (void *)(tpl->TCP_send));
  printf("\t[CNbyte_count = %p]\n", (void *)(tpl->CNbyte_count));
  printf("\t[CNget_NDB = %p]\n", (void *)(tpl->CNget_NDB));

  ret = resolve(finger_host, NULL, &hostaddr, 1);
  if (ret < 0) {
    printf("resolve(\"%s\") returned error code %d: %s\n",
	   finger_host, ret, get_err_text(ret));
    return;
  }

  fd = TCP_open(hostaddr, 79, 0, 0);
  if (fd < 0) {
    printf("TCP_open() returned error code %d: %s\n", fd, get_err_text(fd));
    return;
  } else {
    printf("TCP_open() returns socket handle %d\n", fd);
  }

#if 0
  ret = TCP_wait_state(fd, TESTABLISH, 5);
  if (ret < 0) {
    printf("TCP_wait_state() returned error code %d: %s\n", ret,
	   get_err_text(ret));
  }
#endif

  ret = TCP_send(fd, finger_name, strlen(finger_name));
  if (ret < 0) {
    printf("TCP_send() returned error code %d: %s\n", ret, get_err_text(ret));
    TCP_close(fd, 0);
    return;
  }

  ret = CNbyte_count(fd);
  if (ret < 0) {
    printf("CNbyte_count() returned error code %d: %s\n", ret, get_err_text(ret));
  } else {
    printf("CNbyte_count() reports %d bytes waiting on socket %d\n",
	   ret, fd);
  }

  for (;;) {
    N = CNget_NDB(fd);
    if (N) {
      printf("%.*s", (int)N->len, N->ndata);
      if (N->ptr)
	KRfree(N->ptr);
      KRfree((char *)N);
    }
    ret = CNbyte_count(fd);
    if (ret < 0 && ret != E_NODATA) {
      break;
    }
  }

  if (ret < 0) {
    printf("After EOF, CNbyte_count() returned error code %d: %s\n",
	   ret, get_err_text(ret));
  } else {
    printf("After EOF, CNbyte_count() reports %d bytes waiting on socket %d\n",
	   ret, fd);
  }

  ret = TCP_close(fd, 0);
  if (ret < 0) {
    printf("TCP_close(%d) returned error code %d: %s\n",
	   fd, ret, get_err_text(ret));
  }
}

void CIB_test(void)
{
  CIB *C;
  unsigned long hostaddr;
  int fd, ret;
  static char host_name[] = "FreeMiNT";

  printf("\nTesting CNgetinfo()...\n");
  printf("\t[CNgetinfo = %p]\n", (void *)(tpl->CNgetinfo));

  ret = resolve(host_name, NULL, &hostaddr, 1);
  if (ret < 0) {
    printf("resolve(\"%s\") returned error code %d: %s\n",
	   host_name, ret, get_err_text(ret));
    return;
  }

  fd = TCP_open(hostaddr, 23, 0, 0);
  if (fd < 0) {
    printf("TCP_open() returned error code %d: %s\n", fd, get_err_text(fd));
    return;
  } else {
    printf("TCP_open() returns socket handle %d\n", fd);
  }

  printf("Before TCP_wait_state():\n");
  C = CNgetinfo(fd);
  if (!C) {
    printf("CNgetinfo(%d) returned NULL\n", fd);
  } else {
    printf("CNgetinfo(%d) returned {%u, %u, %u, %lu.%lu.%lu.%lu, "
	   "%lu.%lu.%lu.%lu}\n",
	   fd, C->protocol, C->lport, C->rport,
	   (C->rhost>>24), (C->rhost>>16)&0xFFL,
	   (C->rhost>>8)&0xFFL, C->rhost&0xFFL,
	   (C->lhost>>24), (C->lhost>>16)&0xFFL,
	   (C->lhost>>8)&0xFFL, C->lhost&0xFFL
	   );
  }

  printf("Waiting for connection to establish...");
  fflush(stdout);
  ret = TCP_wait_state(fd, TESTABLISH, 10);
  if (ret < 0) {
    printf("\nTCP_wait_state(%d, %d, 0) returned error code %d: %s\n",
	   fd, TESTABLISH, ret, get_err_text(ret));
  } else {
    printf(" done\n");
  }

  printf("After TCP_wait_state():\n");
  C = CNgetinfo(fd);
  if (!C) {
    printf("CNgetinfo(%d) returned NULL\n", fd);
  } else {
    printf("CNgetinfo(%d) returned {%u, %u, %u, %lu.%lu.%lu.%lu, "
	   "%lu.%lu.%lu.%lu}\n",
	   fd, C->protocol, C->lport, C->rport,
	   (C->rhost>>24), (C->rhost>>16)&0xFFL,
	   (C->rhost>>8)&0xFFL, C->rhost&0xFFL,
	   (C->lhost>>24), (C->lhost>>16)&0xFFL,
	   (C->lhost>>8)&0xFFL, C->lhost&0xFFL
	   );
  }

  ret = TCP_close(fd, 0);
  if (ret < 0) {
    printf("TCP_close(%d) returned error code %d: %s\n",
	   fd, ret, get_err_text(ret));
  }
}

#if 0
void dummy_test(void)
{
  int ret;

  printf("\nTesting the dummy functions...\n");
  ret = dummy1();
  printf("dummy1() returned %d: \"%s\"\n", ret, get_err_text(ret));
  ret = dummy8();
  printf("dummy8() returned %d: \"%s\"\n", ret, get_err_text(ret));
}
#endif

void listen_port_test(void)
{
  int fd;
  int ret;
  CIB *C;

  printf("Test with specified listen port 4269:\n");
  fd = TCP_open(0, 4269, 0, 0);
  if (fd < 0) {
    printf("TCP_open() returned error code %d: %s\n", fd, get_err_text(fd));
    return;
  } else {
    printf("TCP_open() returns socket handle %d\n", fd);
  }

  C = CNgetinfo(fd);
  if (!C) {
    printf("CNgetinfo(%d) returned NULL\n", fd);
  } else {
    printf("CNgetinfo(%d) returns {%d, lport=%d, rport=%d, "
	   "rhost=%d.%d.%d.%d(%lu), lhost=%d.%d.%d.%d(%lu)} @%p\n",
	   fd, C->protocol, C->lport, C->rport,
	   (int)(C->rhost>>24)&0xFF, (int)(C->rhost>>16)&0xFF,
	   (int)(C->rhost>>8)&0xFF, (int)(C->rhost&0xFF), C->rhost,
	   (int)(C->lhost>>24)&0xFF, (int)(C->lhost>>16)&0xFF,
	   (int)(C->lhost>>8)&0xFF, (int)(C->lhost&0xFF), C->lhost,
	   (void *)C);
  }

  ret = CNbyte_count(fd);
  if (ret < 0) {
    printf("CNbyte_count(%d) returned error code %d: %s\n", fd, ret,
	   get_err_text(ret));
  } else {
    printf("CNbyte_count(%d) reports %d bytes waiting \n", fd, ret);
  }

  ret = TCP_close(fd, 0);
  if (ret < 0) {
    printf("TCP_close(%d) returned error code %d: %s\n",
	   fd, ret, get_err_text(ret));
  }

  printf("Test with unspecified listen port:\n");
  fd = TCP_open(0, 0, 0, 0);
  if (fd < 0) {
    printf("TCP_open() returned error code %d: %s\n", fd, get_err_text(fd));
    return;
  } else {
    printf("TCP_open() returns socket handle %d\n", fd);
  }

  C = CNgetinfo(fd);
  if (!C) {
    printf("CNgetinfo(%d) returned NULL\n", fd);
  } else {
    printf("CNgetinfo(%d) returns {%d, lport=%d, rport=%d, "
	   "rhost=%d.%d.%d.%d(%lu), lhost=%d.%d.%d.%d(%lu)} @%p\n",
	   fd, C->protocol, C->lport, C->rport,
	   (int)(C->rhost>>24)&0xFF, (int)(C->rhost>>16)&0xFF,
	   (int)(C->rhost>>8)&0xFF, (int)(C->rhost&0xFF), C->rhost,
	   (int)(C->lhost>>24)&0xFF, (int)(C->lhost>>16)&0xFF,
	   (int)(C->lhost>>8)&0xFF, (int)(C->lhost&0xFF), C->lhost,
	   (void *)C);
  }

  ret = CNbyte_count(fd);
  if (ret < 0) {
    printf("CNbyte_count(%d) returned error code %d: %s\n", fd, ret,
	   get_err_text(ret));
  } else {
    printf("CNbyte_count(%d) reports %d bytes waiting \n", fd, ret);
  }

  ret = TCP_close(fd, 0);
  if (ret < 0) {
    printf("TCP_close(%d) returned error code %d: %s\n",
	   fd, ret, get_err_text(ret));
  }
}


struct {
  void (*func)(void);
  const char *descr;
} tests[] = {
  {errtext_test, "Test get_err_text()"},
  {stikvar_test, "Test getvstr() and setvstr()"},
  {mem_test, "Test the memory pool"},
  {resolver_test, "Test the DNS resolver"},
  {tcp_test, "Test the TCP functions"},
  {NDB_test, "Test the TCP functions, with CNget_NDB() reads only"},
  {CIB_test, "Test CNgetinfo()"},
#if 0
  {dummy_test, "Test the dummy functions"},
#endif
  {listen_port_test, "Test basic listen port functionality"},
};
#define NTESTS (sizeof(tests)/sizeof(*tests))

int main(int argc, char *argv[])
{
  int i, n;

  Supexec(init_drivers);
  printf("STiK cookie returned a drivers value of %p\n",
	 (void *)drivers);
  if (!drivers) {
    printf("Hmph.  That didn't work...\n");
    exit(1);
  }
  printf("\ndrivers = {\"%s\", %p, %p, %p, %p}\n", drivers->magic,
	 (void *)(drivers->get_dftab), (void *)(drivers->ETM_exec),
	 (void *)(drivers->cfg), (void *)(drivers->sting_basepage));
  printf("drivers->cfg->client_ip = 0x%08lx\n",
	 *(unsigned long *)(drivers->cfg));

  if (strcmp(MAGIC, drivers->magic)) {
    printf("Hmm... drivers->magic should have been \"%s\"\n", MAGIC);
    return 1;
  }

  tpl = (TPL *)get_dftab(TRANSPORT_DRIVER);
  printf("get_dftab(\"%s\") returns %p\n", TRANSPORT_DRIVER, (void *)tpl);
  if (!tpl) {
    printf("Well, durnit, what is this about?\n");
    return 1;
  }
  printf("tpl = {\"%s\", \"%s\", \"%s\"}\n", tpl->module, tpl->author,
	 tpl->version);

  if (argc <= 1) {
    printf("\nYou didn't select any tests.  The available tests are:\n");
    for (i = 0; i < NTESTS; i++)
      printf("\t%d:  %s\n", i + 1, tests[i].descr);
  } else {
    for (i = 1; i < argc; i++) {
      n = atoi(argv[i]);
      if (n == 0 || n > NTESTS) {
	printf("Unknown test \"%s\"\n", argv[i]);
	continue;
      }
      (*tests[n - 1].func)();
    }
  }

  return 0;
}
