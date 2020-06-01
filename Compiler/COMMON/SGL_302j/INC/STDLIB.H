
#if	!defined(_STDLIB_H)
#define	_STDLIB_H

#ifndef	__SIZE_TYPE__DEF
#define	__SIZE_TYPE__DEF	unsigned int
typedef	__SIZE_TYPE__DEF	size_t;
#endif

#ifndef	__P
#if	defined(__STDC__)
#define	__P(args)	args
#else
#define	__P(args)	()
#endif
#endif

#ifndef	NULL
#define	NULL	0
#endif

int	atoi	__P((const char *str));
long	atol	__P((const char *str));
int	abs	__P((int val));
void	qsort	__P((void *base, size_t nelem, size_t size,
			int (*compar)(const void *p1, const void *p2)));

#endif		/* _STDLIB_H */
