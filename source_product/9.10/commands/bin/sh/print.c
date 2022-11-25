/* @(#) $Revision: 66.2 $ */     
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	<sys/param.h>


#define		BUFLEN		256

static char	buffer[BUFLEN];	
static int	index = 0;
tchar		numbuf[12];
#ifdef SYSNETUNAM && NLS 
char		cnumbuf[12];	/* char version of numbuf for lan code */
#endif SYSNETUNAM && NLS 

#ifdef NLS
extern void	prct_buff();
extern void	prst_buff();
extern void	prst_cntl();
#endif NLS

extern void	prc_buff();
extern void	prs_buff();
extern void	prn_buff();
extern void	prs_cntl();
extern void	prn_buff();

/*
 * printing and io conversion
 */
prp()
{
	if ((flags & prompt) == 0) {
            if(cmdadr) {
		prst_cntl(cmdadr);
		prs(colon);
            }
          /*else {
                if(flags & rshflg)
                    prs("rsh: ");
                else
                    prs("sh: ");
            }*/
	}
}

prs(as)
char	*as;	
{
	register char	*s;

	if (s = as)
 		write(output, s, length(s) - 1);  
}

#ifdef NLS
/*
 * Can be called with a pointer to a very long string, we may need to write 
 * in several pieces.  So now use to_charm the multiple version of to_char.
 */
prst(as)	
tchar	*as; 			/* tchar version of prs */
{
	extern char *to_charm(); 
	char	*c_as;		/* local varible for char copy NLS */
	tchar	*leftoff = 0;
	int	startlen = -1;

	while (as && startlen < 0) {
		c_as = to_charm(as, &leftoff, &startlen);
		write(output, c_as, abs(startlen));
		if (leftoff != 0)
			as = leftoff;
	}


}
#endif NLS

prc(c)
char	c;
{
	if (c)
 		write(output, &c, 1);
}

#ifdef NLS
prct(tc)	
tchar	tc; 			/* tchar version of prc */
{
	char c;

	if (tc)
		if ((tc&0xff00) == 0 ){  /* check not 16-bit char or quoted */
			c = tc;
 			write(output, &c, 1);
		}
		else {
			tc |= QUOTE; /* output as 2 chars */
			write(output, &tc, 2);
		}
}
#endif NLS

prt(t)
long	t;
{
	register int hr, min, sec;

	t += HZ / 2;
	t /= HZ;
	sec = t % HZ;
	t /= HZ;
	min = t % HZ;

	if (hr = t / HZ)
	{
		prn_buff(hr);
		prc_buff('h');
	}

	prn_buff(min);
	prc_buff('m');
	prn_buff(sec);
	prc_buff('s');
}

prn(n)
	int	n;
{
	itos(n);

	prst(numbuf);
}

itos(n)
{
	register tchar *abuf;
	register unsigned a, i;
	int pr, d;

	abuf = numbuf;

	pr = FALSE;
	a = n;
	for (i = 10000; i != 1; i /= 10)
	{
		if ((pr |= (d = a / i)))
			*abuf++ = d + '0';
		a %= i;
	}
	*abuf++ = a + '0';
	*abuf++ = 0;
}
#ifdef SYSNETUNAM && NLS
itosc(n)			/* char version of itos routine for lan code */
{
	register char *abuf;
	register unsigned a, i;
	int pr, d;

	abuf = cnumbuf;

	pr = FALSE;
	a = n;
	for (i = 10000; i != 1; i /= 10)
	{
		if ((pr |= (d = a / i)))
			*abuf++ = d + '0';
		a %= i;
	}
	*abuf++ = a + '0';
	*abuf++ = 0;
}
#endif SYSNETUNAM && NLS

stoi(icp)
tchar	*icp; 	
{
	register tchar	*cp = icp; 
	register int	r = 0;
	register tchar	c; 

	while ((c = *cp, digit(c)) && c && r >= 0)
	{
		c = c & STRIP;  	/* turn off quote bit */
		r = r * 10 + c - '0';
		cp++;
	}
	if (r < 0 || cp == icp)
		tfailed(icp, nl_msg(607,badnum));
	else
		return(r);
}

/* This routine is identical to stoi but is specifically to 
   check to MAILCHECK value.  When checking this value, a
   integer value should always be returned.  If the value
   is invalid (negative or not a digit), -1 will be returned
   (indicating error).  stoi will cause the sh to exit if
   the MAILCHECK variable is invalid, therefore it can not
   be used in this cause. */

mstoi(icp)
tchar	*icp; 	
{
	register tchar	*cp = icp; 
	register int	r = 0;
	register tchar	c; 

	while ((c = *cp, digit(c)) && c && r >= 0)
	{
		c = c & STRIP;  	/* turn off quote bit */
		r = r * 10 + c - '0';
		cp++;
	}
	if (r < 0 || cp == icp)
		return(-1);
	else
		return(r);
}

prl(n)
long n;
{
	int i;

	i = 11;
	while (n > 0 && --i >= 0)
	{
		numbuf[i] = n % 10 + '0';
		n /= 10;
	}
	numbuf[11] = '\0';
	prst_buff(&numbuf[i]);
}

void
flushb()
{
	if (index)
	{
		buffer[index] = '\0';	
 		write(1, buffer, length(buffer) - 1);
		index = 0;
	}
}


void
prc_buff(c)
	char c;
{
	if (c)
	{
		if (index + 1 >= BUFLEN)
			flushb();

		buffer[index++] = c;
	}
	else
	{
		flushb();
 		write(1, &c, 1); 
	}
}

#ifdef NLS
void
prct_buff(c)		/* tchar version of prc_buff */
tchar c;
{
	tchar ourc[2];
	char *l_c;
	int numchars;

	ourc[0] = c; ourc[1] = 0;
	l_c = to_char(ourc);
	numchars = length(l_c)-1; 	/* # of outgoing char - null */
	if (c)
	{
		if (index + numchars >= BUFLEN)	
			flushb();	

		movstr(l_c, &buffer[index]);
		index += numchars;
	}
	else
	{
		flushb();
 		write(1, l_c, numchars);
	}
}
#endif NLS

void
prs_buff(s)
	char *s;	
{
	register int len = length(s) - 1;

	if (index + len >= BUFLEN)
		flushb();

	if (len >= BUFLEN)
 		write(1, &s, len); 
	else
	{
		movstr(s, &buffer[index]);
		index += len;
	}
}

#ifdef NLS
void
prst_buff(s)
tchar *s;			/* tchar version of prs_buff */
{
	char l_s[1024];
	register int clen;

	(void)sto_char(s, l_s);
	clen = length(l_s) - 1;

	if (index + clen >= BUFLEN)
		flushb();

	if (clen >= BUFLEN)
 		write(1, l_s, clen);
	else
	{
		movstr(l_s, &buffer[index]);
		index += clen;
	}
}
#endif NLS


clear_buff()
{
	index = 0;
}

#ifdef NLS
void
prst_cntl(s)
	tchar *s;			/* tchar version of prs_cntl */
{
	register char *c_ptr = buffer;
	register unsigned char c;
	register char *end_ptr = buffer + BUFLEN - 2; /* Check for overflow */


	while (*s != '\0' && c_ptr < end_ptr)
	{
		if ((*s & CHECK16) == 0){ 	/* if not 16-bit */
			c = (*s & STRIP) ;
		
		/* translate a control character into a printable sequence */

			if (c < '\040') 
			{	/* assumes ASCII char */
				*c_ptr++ = '^';	
				*c_ptr++ = (c + 0100);
			}
			else if (c == 0177) 
			{	/* '\0177' does not work */
				*c_ptr++ = '^';		/* del char */ 
				*c_ptr++ = '?';	
			}
			else 
			{	/* printable character */
				*c_ptr++ = c;
			}

			++s;
		}
		else {			/* else is a 16-bit char. */
			unsigned char c[3]; /* extra spot for null */
			tchar s_plus_1 = s[1];
			s[1] = 0;
			sto_char(s,c);	
			*c_ptr++ = c[0];
			*c_ptr++ = c[1];
			*++s = s_plus_1;
		}
	}
	*c_ptr = '\0';
	prs(buffer);
}
#endif NLS

void
prs_cntl(s)
	char *s;
{
	register char *ptr = buffer;
	register unsigned char c;
	register char *end_ptr = buffer + BUFLEN - 2; /* Check for overflow */


	while (*s != '\0' && ptr < end_ptr)
	{
		c = *s;			/* no longer strip 8th bit, mn */
		
		/* translate a control character into a printable sequence */

		if (c < '\040') 
		{	/* assumes ASCII char */
			*ptr++ = '^';
			*ptr++ = (c + 0100);	/* assumes ASCII char */
		}
		else if (c == 0177) 
		{	/* '\0177' does not work */
			*ptr++ = '^';
			*ptr++ = '?';
		}
		else 
		{	/* printable character */
			*ptr++ = c;
		}

		++s;
	}

	*ptr = '\0';
	prs(buffer);
}


void
prn_buff(n)
	int	n;
{
	itos(n);

	prst_buff(numbuf);
}
