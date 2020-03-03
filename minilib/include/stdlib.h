/*
 * stdlib.h
 *
 *  Created on: 31.05.2013
 *      Author: mfro
 */

#ifndef _STDLIB_H_
#define _STDLIB_H_

typedef unsigned long size_t;

int atoi(const char *c);
long atol(const char *c);
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

char *ltoa(long value, char *buffer, int radix);
char *ultoa(unsigned long value, char *buffer, int radix);
char *itoa(int value, char *buffer, int radix);
#define _ltoa(a, b, c) ltoa((a), (b), (c))
#define _ultoa(a, b, c) ultoa((a), (b), (c))
#define _itoa(a, b, c) itoa((a), (b), (c))

char *getenv(const char *name);

#ifndef __COMPAR_FN_T
# define __COMPAR_FN_T
typedef int (*__compar_fn_t) (__const void*, __const void*);

# ifdef	__USE_GNU
typedef __compar_fn_t comparison_fn_t;
# endif
#endif

void qsort (void* __base, size_t __total_elems, 
                             size_t __size, 
                             __compar_fn_t __compar);

int atexit(void (*func)(void));
void exit(int status);
void abort(void);

#ifdef __MSHORT__
#define	RAND_MAX (0x7FFF)
#else
#define	RAND_MAX (0x7FFFFFFFL)
#endif

#define	EXIT_FAILURE	1	/* Failing exit status.  */
#define	EXIT_SUCCESS	0	/* Successful exit status.  */

void srand (unsigned int __seed);
int rand(void);
long lrand(void);
void srand48(long int seed);

long strtol(const char*, char**, int);
unsigned long strtoul(const char*, char**, int);
long long strtoll(const char*, char**, int);
unsigned long long strtoull(const char*, char**, int);

#define abs(j)  ((j) > 0 ? (j) : -(j))

#ifndef NULL
#define NULL	((void *) 0)
#endif

/* Returned by `div'.  */
typedef struct {
	int quot;			/* Quotient.  */
    int rem;			/* Remainder.  */
} div_t;

/* Returned by `ldiv'.  */
#ifndef __ldiv_t_defined
typedef struct {
	long int quot;		/* Quotient.  */
	long int rem;		/* Remainder.  */
} ldiv_t;
# define __ldiv_t_defined	1
#endif

div_t div (int __numer, int __denom) __attribute__ ((__const__));
ldiv_t ldiv (long int __numer, long int __denom) __attribute__ ((__const__));

void *bsearch(const void *key, const void *base, size_t num, size_t size, int (*cmp)(const void *, const void *));

int setenv(const char *name, const char *value, int replace);
int unsetenv(const char *name);

#endif /* _STDLIB_H_ */
