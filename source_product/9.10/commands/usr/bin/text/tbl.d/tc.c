/* @(#) $Revision: 32.1 $ */      
#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
#endif NLS
 /* tc.c: find character not in table to delimit fields */
# include "t..c"
#ifdef NLS16
#include <nl_ctype.h>	/* Must be included to define FIRSTof2() macro */
#endif
choochar()
{
/* choose funny characters to delimit fields */
int had[128], ilin,icol, k;
char *s;
for(icol=0; icol<128; icol++)
	had[icol]=0;
F1 = F2 = 0;
for(ilin=0;ilin<nlin;ilin++)
	{
	if (instead[ilin]) continue;
	if (fullbot[ilin]) continue;
	for(icol=0; icol<ncol; icol++)
		{
		k = ctype(ilin, icol);
		if (k==0 || k == '-' || k == '=')
			continue;
		s = table[ilin][icol].col;
		if (point(s))
#ifdef NLS16
		while (*s) {
			if ( FIRSTof2((unsigned char)(*s)) ) {
				s++;	/* Skip HP15 or illegal code */
				if (!(*s)) break;
				s++;	/* Skip HP15 or illegal code */
			} else if ( (*s)&0200 ) {
				s++;	/* Skip 8-bit code */
			} else {
				had[*s++]=1;
			}
		}
#else
		while (*s)
			had[*s++]=1;
#endif
		s=table[ilin][icol].rcol;
		if (point(s))
#ifdef NLS16
		while (*s) {
			if ( FIRSTof2((unsigned char)(*s)) ) {
				s+=2;	/* Skip HP15 or illegal code */
			} else if ( (*s)&0200 ) {
				s++;	/* Skip 8-bit code */
			} else {
				had[*s++]=1;
			}
		}
#else
		while (*s)
			had[*s++]=1;
#endif
		}
	}
/* choose first funny character */
for(
	s="\002\003\005\006\007!%&#/?,:;<=>@`^~_{}+-*ABCDEFGHIJKMNOPQRSTUVWXYZabcdefgjkoqrstwxyz";
		*s; s++)
	{
	if (had[*s]==0)
		{
		F1= *s;
		had[F1]=1;
		break;
		}
	}
/* choose second funny character */
for(
	s="\002\003\005\006\007:_~^`@;,<=>#%&!/?{}+-*ABCDEFGHIJKMNOPQRSTUVWXZabcdefgjkoqrstuwxyz";
		*s; s++)
	{
	if (had[*s]==0)
		{
		F2= *s;
		break;
		}
	}
if (F1==0 || F2==0)
	error((nl_msg(30, "couldn't find characters to use for delimiters")));
return;
}
point(s)
{
return(s>= 128 || s<0);
}
