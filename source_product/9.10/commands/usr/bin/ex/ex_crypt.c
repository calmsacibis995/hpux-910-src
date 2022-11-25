/*
 * @(#) $Revision: 64.1 $
 *
 * Encryption routines.  These are essentially unmodified from ed.
 */

/* ========================================================================= */
/*
** CRYPT block 1
*/
#ifdef CRYPT

#include "ex.h"
#include "ex_temp.h"
#include "ex_vis.h"
#include "ex_tty.h"

/*
 * crblock: encrypt/decrypt a block of text.
 * buf is the buffer through which the text is both input and
 * output. nchar is the size of the buffer. permp is a work
 * buffer, and startn is the beginning of a sequence.
 */
crblock(permp, buf, nchar, startn)
char *permp;
char *buf;
int nchar;
long startn;
{
	register char *p1;
	int n1;
	int n2;
	register char *t1, *t2, *t3;

	t1 = permp;
	t2 = &permp[256];
	t3 = &permp[512];

	n1 = startn&0377;
	n2 = (startn>>8)&0377;
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

/*
 * makekey: initialize buffers based on user key a.
 */
makekey(a, b)
char *a, *b;
{
       register int i;
	long t;
	char temp[KSIZE + 1];

	for(i = 0; i < KSIZE; i++)
		temp[i] = *a++;
	time(&t);
	t += getpid();
	for(i = 0; i < 4; i++)
		temp[i] ^= (t>>(8*i))&0377;
	crinit(temp, b);
}

/*
 * crinit: besides initializing the encryption machine, this routine
 * returns 0 if the key is null, and 1 if it is non-null.
 */
crinit(keyp, permp)
char    *keyp, *permp;
{
       register char *t1, *t2, *t3;
	register i;
	int ic, k, temp;
	unsigned random;
	char buf[13];
	long seed;

	t1 = permp;
	t2 = &permp[256];
	t3 = &permp[512];
	if(*keyp == 0)
		return(0);
	strncpy(buf, keyp, 8);
	while (*keyp)
		*keyp++ = '\0';

	buf[8] = buf[0];
	buf[9] = buf[1];
	domakekey(buf);

	seed = 123;
	for (i=0; i<13; i++)
		seed = seed*buf[i] + i;
	for(i=0;i<256;i++){
		t1[i] = i;
		t3[i] = 0;
	}
	for(i=0; i<256; i++) {
		seed = 5*seed + buf[i%13];
		random = seed % 65521;
		k = 256-1 - i;
		ic = (random&0377) % (k+1);
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
	for(i=0; i<256; i++)
		t2[t1[i]&0377] = i;
	return(1);
}

/*
 * domakekey: the following is the major nonportable part of the encryption
 * mechanism. A 10 character key is supplied in buffer.
 * This string is fed to makekey (an external program) which
 * responds with a 13 character result. This result is placed
 * in buffer.
 */

 /* domakekey was modified on 8-31-83 so it wouldn't be so machine dependent.
  * (we hope)
  * Instead of using a single pipe to makekey and depending on the execution
  * order after the fork, domakekey now uses two pipes and lets the reads block
  * each other. 	Anny Randel, Hewlett-Packard, FSD
  */
domakekey(buffer)
char *buffer;
{

#ifdef ED1000
	int pf[2];

	if (pipe(pf)<0)
		pf[0] = pf[1] = -1;
	if (fork()==0) {
		close(0);
		close(1);
		dup(pf[0]);
		dup(pf[1]);
		execl("/usr/lib/makekey", "-", 0);
		execl("/lib/makekey", "-", 0);
		exit(1);
	}
	write(pf[1], buffer, 10);
	if (wait((int *)NULL)==-1 || read(pf[0], buffer, 13)!=13)
		error("crypt: cannot generate key");
	close(pf[0]);
	close(pf[1]);
	/* end of nonportable part */

#else
       int tomakekey[2]; 	/* pipe to send key into makekey */
       int frommakekey[2]; 	/* pipe to get key from makekey */
       int forkval;		/* value returned by fork operation */
       int writeval;		/* value returned by write operation */

	if (pipe(tomakekey)<0) {		/* set up to pipe */
		perror ("crypt: input pipe failed");
		tomakekey[0] = tomakekey[1] = -1;
	}
	if (pipe(frommakekey)<0) { 		/* set up from pipe */
		perror ("crypt: output pipe failed");
		frommakekey[0] = frommakekey[1] = -1;
	}


	forkval=fork();		/* fork to makekey */

	if (forkval==-1) {
		perror("crypt: fork failed");
		exit(1);
	}
	else if (forkval==0) {	/* Child executes makekey encryption program */

		close(0);   /* close 0 so stdin available for pipe connection */
		dup(tomakekey[0]); /* set read end of tomakekey pipe to stdin */
		close(tomakekey[1]); /* close write end of tomakekey */
		close(tomakekey[0]); /*close read end of tomakekey(uses stdin)*/

		close(1);  /* close 1 so stdout available for pipe connection */
		dup(frommakekey[1]); /* set write end of frommakekey to stdout*/
		close(frommakekey[1]); /* close write end of frommakekey */
		close(frommakekey[0]);/*close read end of frommakekey(uses stdout*/

		execl("/usr/lib/makekey", "-", 0);
		execl("/lib/makekey", "-", 0);
		perror("crypt: makekey not found");
		exit(1);
	}

	close(tomakekey[0]); /* close read side of tomakekey */
	close(frommakekey[1]); /* close write side of frommkekey */

	write(tomakekey[1], buffer, 10); /*send key to makekey thru stdin pipe*/

	if (read(frommakekey[0], buffer, 13)!=13) {  /* read encryption thru pipe from stdout */
		if (xeflag) {
			merror("crypt: cannot generate key");
			putNFL();
			flush();
			exit(1);
		} else
			error("crypt: cannot generate key");
	}
	close(tomakekey[1]);
	close(frommakekey[0]);
	/* end of (used to be) nonportable part */
#endif /* ED1000 */

}
#endif /* CRYPT */
/* ========================================================================= */
