#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 66.3 $";
#endif

/*
 *	primes  [ start [ stop ] ]
 *
 *	Print all primes >=start and <=stop (or number(s) read from stdin).
 *
 *	A free translation of 'primes.s'
 *
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define	TABSIZE	1000		/* size of sieve table */
#define	BIG	2147483647.	/* largest signed integer */

unsigned char	table[TABSIZE];		/* table for sieve of Eratosthenes */
int	tabbits	= 8*TABSIZE;	/* number of bits in table */

double	fstart, fstop;
static	long	start;		/* lowest number to test for prime */
static	long	stop;		/* highest number to test for prime */
unsigned char	bittab[] = {		/* bit positions (to save shifting) */
	01, 02, 04, 010, 020, 040, 0100, 0200
};

long pt[] =	{		/* primes < 100 */
	2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43,
	47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97
};

long factab[] = {		/* difference between succesive trial factors */
	10, 2, 4, 2, 4, 6, 2, 6, 4, 2, 4, 6, 6, 2, 6, 4,
	2, 6, 4, 6, 8, 4, 2, 4, 2, 4, 8, 6, 4, 6, 2, 4,
	6, 2, 6, 6, 4, 2, 4, 6, 2, 6, 4, 2, 4, 2, 10, 2
};

main(argc, argv)
int	argc;
char	**argv;
{
	register long	*fp;
	register char	*p;
	register int	i;
	long	quot;
	long	factor, v;

	fstop = BIG;			/* set default stopping value */
	if (argc >= 2) {		/* get starting no. from arg */
		if (sscanf(argv[1], "%lf", &fstart) != 1
		    || fstart < 0.0 || fstart > BIG) {
			ouch();
			exit(1);
		}
		if (argc > 2) {		/* get stopping value from arg */
			if (sscanf(argv[2], "%lf", &fstop) != 1
			    || fstop < fstart || fstop > BIG) {
				ouch();
				exit(1);
			}
		}
	} else {			/* get starting no. from stdin */
		if ((i = scanf("%lf", &fstart)) != 1
		    || fstart < 0.0 || fstart > BIG) {
			if (i != EOF)
				ouch();
			exit(1);
		}
		if ((i = scanf("%lf", &fstop)) != 1
		    || fstop < fstart || fstop > BIG) {
			if (i != EOF) {
				ouch();
				exit(1);
			}
		}
	}
	start = (long)fstart;
	stop = (long)fstop;

	/*
	 * Quick list of primes < 100
	 */
	if (start <= 97) {
		for (fp = pt; *fp < start; fp++)
			;
		do
			if (*fp <= stop)
				(void) printf("%u\n", *fp);
			else
				exit(0);
		while (++fp < &pt[sizeof(pt) / sizeof(*pt)]);
		start = 100;
	}
	quot = start/2;
	start = quot * 2 + 1;	/* make sure that start is odd */

/*
 * Loop until done:
 */
    for (;;) {
	/*
	 * Generate primes via sieve of Eratosthenes
	 */
	for (p = table; p < &table[TABSIZE]; p++)	/* clear sieve */
		*p = '\0';
	/*
	 *  v = highest useful factor
	 */
	v = (long)sqrt((double)start + (double)tabbits); /* watch overflow */
	sieve(3);
	sieve(5);
	sieve(7);
	factor = 11;
	fp = &factab[1];
	do {
		sieve(factor);
		factor += *fp;
		if (++fp >= &factab[sizeof(factab) / sizeof(*factab)])
			fp = factab;
	} while (factor <= v);
	/*
	 * Print generated primes
	 */
	for (i = 0; i < 8*TABSIZE; i += 2) {
		if ((table[i>>3] & bittab[i&07]) == 0)
			if (start <= stop )
				(void) printf("%u\n", start);
		if (start < stop)	/* watch overflow */
			start += 2;
		else
			exit(0);
	}
    }
}

/*
 * Insert all multiples of given factor into the sieve
 */
sieve(factor)
long factor;
{
	register int	i;
	long off;
	long quot;

	quot = start / factor;
	off = (quot * factor) - start;
	if ((int)off < 0)
		off += factor;
	while (off < tabbits ) {
		i = (int)off;
		table[i>>3] |= bittab[i&07];
		off += factor;
	}
}

/*
 * Error message
 */
ouch()
{
	(void) fprintf(stderr, "Ouch.\n");
}
