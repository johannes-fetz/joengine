/* string.h : for SH */
#if	!defined(_STRING_H)
#define	_STRING_H

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

size_t	strlen		__P((const char *str));
char	*strcpy		__P((char *dst, const char *src));
char	*strncpy	__P((char *dst, const char *src, size_t len));
char	*strcat		__P((char *dst, const char *src));
char	*strncat	__P((char *dst, const char *src, size_t len));
int	strcmp		__P((const char *s1, const char *s2));
int	strncmp		__P((const char *s1, const char *s2, size_t len));
char	*strchr		__P((const char *str, int ch));
char	*strrchr	__P((const char *, int));
char	*strpbrk	__P((const char *str, const char *set));
size_t	strspn		__P((const char *str, const char *set));
size_t	strcspn		__P((const char *str, const char *set));
char	*strstr		__P((const char *str, const char *sub));

int	memcmp		__P((const void *p1, const void *p2, size_t len));
void	*memcpy		__P((void *dst, const void *src, size_t len));
void	*memmove	__P((void *dst, const void *src, size_t len));
void	*memset		__P((void *dst, int ptn, size_t len));
void	*memchr		__P((const void *mem, int ptn, size_t len));

#endif		/* _STRING_H */
