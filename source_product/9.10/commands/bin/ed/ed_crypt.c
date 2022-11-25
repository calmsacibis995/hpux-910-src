/*
 * @(#) $Revision: 64.1 $
 */

/* ========================================================================= */
/*
** CRYPT block 14
*/
#ifdef CRYPT

#ifdef NLS
#include <locale.h>
#include <nl_types.h>
nl_catd catd;
extern char *msgbuf;
#define NL_SETN 1       /* set number */
#else /* NLS */
#define catgets(i, j, k, s)     (s)
#endif /* NLS */

#include <stdio.h>
#include <signal.h>
#include <termio.h>

#define KSIZE 9

extern char key[];
extern int  yflag;

crblock(permp, buf, nchar, startn)
char	*permp;
char	*buf;
long	startn;
{
	register char	*p1;
	register int n1, n2;
	register char	*t1, *t2, *t3;

	t1 = permp;
	t2 = &permp[256];
	t3 = &permp[512];

	n1 = (int)(startn&0377);
	n2 = (int)((startn>>8)&0377);
	p1 = buf;
	while(nchar--) {
		*p1 = t2[(t3[(t1[(*p1+n1)&0377]+n2)&0377]-n2)&0377]-n1;
		n1++;
		if(n1==256){
			n1 = 0;
			n2++;
			if(n2==256) n2 = 0;
		}
		p1++;
	}
}

getkey()
{
	struct termio b;
	int save;
	void (*signal())();
	void (*sig)();
	char *p;
	int c;

	sig = signal(SIGINT, SIG_IGN);
	ioctl(0, TCGETA, &b);
	save = b.c_lflag;
	b.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
	ioctl(0, TCSETA, &b);

#ifndef NLS
	write(1, "Enter file encryption key: ", 27);
#else /* NLS */
	msgbuf = catgets(catd, NL_SETN, 74, "Enter file encryption key: ");
	write(1, msgbuf, strlen(msgbuf));
#endif /* NLS */

	p = key;
	while(((c=getchr()) != EOF) && (c!='\n')) {
		if(p < &key[KSIZE])
			*p++ = c;
	}
	*p = 0;
	write(1, "\n", 1);
	b.c_lflag = save;
	ioctl(0, TCSETA, &b);
	signal(SIGINT, sig);
	/* this line used to say "return(key[0] != 0);" */
}

/*
 * Besides initializing the encryption machine, this routine
 * returns 0 if the key is null, and 1 if it is non-null.
 */
crinit(keyp, permp)
char	*keyp, *permp;
{
	register char *t1, *t2, *t3;
	int ic, i, j, k, temp, pf[2];
	unsigned random;
	char buf[13];
	long seed;

	if (yflag)
		error(59);
	t1 = permp;
	t2 = &permp[256];
	t3 = &permp[512];

	if (*keyp == 0)
		return(0);

	strncpy(buf, keyp, 8);
	while (*keyp)
		*keyp++ = '\0';
	buf[8] = buf[0];
	buf[9] = buf[1];
	if(pipe(pf) < 0)
		error(0);
	i = fork();
	if(i == -1)
		error(23);
	if(i == 0) {
		close(0);
		close(1);
		dup(pf[0]);
		dup(pf[1]);
		execl("/usr/lib/makekey", "-", (char *) 0, (char *) 0, (char *) 0);
		execl("/lib/makekey", "-", (char *) 0, (char *) 0, (char *) 0);
		exit(2);
	}
	write(pf[1], buf, 10);
	if (wait((int *) 0)== -1 || read(pf[0], buf, 13)!=13) {
		puts("crypt: cannot generate key");
		exit(2);
	}
	close(pf[0]);
	close(pf[1]);
	seed = 123;
	for (i=0; i<13; i++)
		seed = seed*buf[i] + i;
	for(i=0;i<256;i++) {
		t1[i] = i;
		t3[i] = 0;
	}
	for(i=0;i<256;i++) {
		seed = 5*seed + buf[i%13];
		random = (int)(seed % 65521);
		k = 256-1 -i;
		ic = (random&0377)%(k+1);
		random >>= 8;
		temp = t1[k];
		t1[k] = t1[ic];
		t1[ic] = temp;
		if(t3[k]!=0) continue;
		ic = (random&0377) % k;
		while(t3[ic]!=0) ic = (ic+1) % k;
		t3[k] = ic;
		t3[ic] = k;
	}
	for(i=0;i<256;i++)
		t2[t1[i]&0377] = i;
	return(1);
}

makekey(a, b)
char *a, *b;
{
	register int i;
	long gorp;
	char temp[KSIZE + 1];

	for(i = 0; i < KSIZE; i++)
		temp[i] = *a++;
	time(&gorp);
	gorp += getpid();

	for(i = 0; i < 4; i++)
		temp[i] ^= (char)((gorp>>(8*i))&0377);

	i = crinit(temp, b);
}
#endif /* CRYPT */
/* ========================================================================= */
