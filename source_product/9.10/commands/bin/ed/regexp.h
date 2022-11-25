/* @(#) $Revision: 27.1 $ */       
#define	CBRA	2
#define	CCHR	4
#define	CDOT	8
#define	CCL	12

#ifdef NLS	/* Multibyte integrity */
#define	NCCL	16
#endif NLS

#define	CDOL	20
#define	CCEOF	22
#define	CKET	24
#define	CBACK	36

#define	STAR	01
#define RNGE	03

#define	NBRA	9

#ifndef NLS
#define STRIP 0177
#define CSETSIZE 128
#define UNSIGNED
#else NLS
#define STRIP 0377
#define CSETSIZE 256
#define UNSIGNED
#endif NLS

#ifdef NLS	/* Multibyte integrity */
#define	FROM	'\220'
#define	TO	'\221'
#endif NLS

#define NBITS 8
#define TABSIZE CSETSIZE/NBITS

#ifndef NLS	/* Multibyte integrity */
#define PLACE(c)	ep[c >> 3] |= bittab[c & 07]
#define ISTHERE(c)	(ep[c >> 3] & bittab[c & 07])
#else NLS
typedef	struct	{
	UNSIGNED char	*base;
	int	offset;
}	STRING;
#endif NLS

UNSIGNED char	*braslist[NBRA];
UNSIGNED char	*braelist[NBRA];
UNSIGNED int	nbra, ebra;
UNSIGNED char	*loc1, *loc2, *locs;
UNSIGNED int	sed;
UNSIGNED int	nodelim;

int	circf;
int	low;
		int	size;

#ifndef NLS	/* Multibyte integrity */
UNSIGNED char	bittab[] = {
	1,
	2,
	4,
	8,
	16,
	32,
	64,
	128
};
#else NLS
int	_braslist[NBRA];
int	_braelist[NBRA];
int	_loc1, _loc2;
#endif NLS

UNSIGNED char *
compile(instring, ep, endbuf, seof)
register UNSIGNED char *ep;
UNSIGNED char *instring, *endbuf;
{
	INIT	/* Dependent declarations and initializations */

#ifndef NLS	/* Multibyte integrity */
	register UNSIGNED c;
#else NLS
	UNSIGNED char	c1, c2;
#endif NLS

	register UNSIGNED eof = seof;
	UNSIGNED char *lastep = instring;
	UNSIGNED int cclcnt;
	UNSIGNED char bracket[NBRA], *bracketp;
	UNSIGNED int closed;

#ifndef NLS	/* Multibyte integrity */
	UNSIGNED char neg;
	UNSIGNED int lc;
#endif NLS

	UNSIGNED int i, cflg;

	lastep = 0;

#ifndef NLS	/* Multibyte integrity */
	if((c = GETC()) == eof || c == '\n') {
		if(c == '\n') {
			UNGETC(c);
#else NLS
	_get16(&c1, &c2);
	if (c1 == '\216' && (c2 == eof || c2 == '\n'))	{
		if(c2 == '\n') {
			UNGETC(c2);
#endif NLS

			nodelim = 1;
		}
		if(*ep == 0 && !sed)
			ERROR(41);
		RETURN(ep);
	}
	bracketp = bracket;
	circf = closed = nbra = ebra = 0;

#ifndef NLS	/* Multibyte integrity */
	if(c == '^')
		circf++;
	else
		UNGETC(c);
	while(1) {
#else NLS
	if (c1 == '\216' && c2 == '^')	{
		circf++;
		_get16(&c1, &c2);
	}
	do	{
#endif NLS

		if(ep >= endbuf)
			ERROR(50);

#ifndef NLS	/* Multibyte integrity */
		c = GETC();
		if(c != '*' && ((c != '\\') || (PEEKC() != '{')))
#else NLS
		if(!(c1 == '\216' && (c2 == '*' ||
			(c2 == '\\' && PEEKC() == '{'))))
#endif NLS

			lastep = ep;

#ifndef NLS	/* Multibyte integrity */
		if(c == eof) {
#else NLS
		if(c1 == '\216' && c2 == eof)	{
#endif NLS

			*ep++ = CCEOF;
			RETURN(ep);
		}

#ifndef NLS	/* Multibyte integrity */
		switch(c) {
#else NLS
		if (c1 == '\216') switch(c2) {
#endif NLS

		case '.':
			*ep++ = CDOT;
			continue;

		case '\n':
			if(!sed) {

#ifndef NLS	/* Multibyte integrity */
				UNGETC(c);
#else NLS
				UNGETC(c2);
#endif NLS

				*ep++ = CCEOF;
				nodelim = 1;
				RETURN(ep);
			}
			else ERROR(36);
		case '*':
			if(lastep==0 || *lastep==CBRA || *lastep==CKET)
				goto defchar;
			*lastep |= STAR;
			continue;

		case '$':
			if(PEEKC() != eof && PEEKC() != '\n')
				goto defchar;
			*ep++ = CDOL;
			continue;

		case '[':
			if(&ep[TABSIZE+1] >= endbuf)
				ERROR(50);

#ifndef NLS	/* Multibyte integrity */
			*ep++ = CCL;
			lc = 0;
			for(i = 0; i < TABSIZE; i++)
				ep[i] = 0;

			neg = 0;
			if((c = GETC()) == '^') {
				neg = 1;
				c = GETC();
			}

			do {
				if(c == '\0' || c == '\n')
					ERROR(49);
				if(c == '-' && lc != 0) {
					if((c = GETC()) == ']') {
						PLACE('-');
						break;
					}
					while(lc < c) {
						PLACE(lc);
						lc++;
					}
				}
				lc = c;
				PLACE(c);
			} while((c = GETC()) != ']');
			if(neg) {
				for(cclcnt = 0; cclcnt < TABSIZE; cclcnt++)
					ep[cclcnt] ^= -1;
				ep[0] &= 0376;
			}
			ep += TABSIZE;

			continue;
#else NLS
			*ep++ = CCL;
			*ep++ = 0;
			cclcnt = 1;
			_get16(&c1, &c2);
			if (c1 == '\216' && c2 == '^')	{
				_get16(&c1, &c2);
				*(ep-2) = NCCL;
			}
			do	{
				if (c1 == '\216' && (c2 == '\0' || c2 == '\n'))
					ERROR(49);
				if (c1 == '\216' && c2 == '-' && *(ep-1) != 0)	{
					_get16(&c1, &c2);
					if (c1 == '\216' && c2==']')	{
						*ep++ = '\216';
						*ep++ = '-';
						cclcnt++;
						if (ep >= endbuf)
							ERROR(50);
						break;
					}
					*ep++ = FROM;
					*ep++ = TO;
					cclcnt++;
					if (ep >= endbuf)
						ERROR(50);
				}
				*ep++ = c1;
				*ep++ = c2;
				cclcnt++;
				if (ep >= endbuf)
					ERROR(50);
				_get16(&c1, &c2);
			}	while (! (c1 == '\216' && c2 == ']'));
			*(lastep+1) = cclcnt;
			continue;

#endif NLS

		case '\\':

#ifndef NLS	/* Multibyte integrity */
			switch(c = GETC()) {
#else NLS
			_get16(&c1, &c2);
			if (c1 == '\216') switch(c2) {
#endif NLS

			case '(':
				if(nbra >= NBRA)
					ERROR(43);
				*bracketp++ = nbra;
				*ep++ = CBRA;
				*ep++ = nbra++;
				continue;

			case ')':
				if(bracketp <= bracket || ++ebra != nbra)
					ERROR(42);
				*ep++ = CKET;
				*ep++ = *--bracketp;
				closed++;
				continue;

			case '{':
				if(lastep == (UNSIGNED char *) (0))
					goto defchar;
				*lastep |= RNGE;
				cflg = 0;
			nlim:

#ifndef NLS	/* Multibyte integrity */
				c = GETC();
#else NLS
				_get16(&c1, &c2);
				if (c1 == '\216')	{
#endif NLS

				i = 0;
				do {

#ifndef NLS	/* Multibyte integrity */
					if('0' <= c && c <= '9')
						i = 10 * i + c - '0';
#else NLS
					if('0' <= c2 && c2 <= '9')
						i = 10 * i + c2 - '0';
#endif NLS
					else
						ERROR(16);

#ifndef NLS	/* Multibyte integrity */
				} while(((c = GETC()) != '\\') && (c != ','));
#else NLS
					_get16(&c1, &c2);
				} while(!(c1=='\216' && (c2=='\\' || c2==',')));
#endif NLS

				if(i > 255)
					ERROR(11);
				*ep++ = i;

#ifndef NLS	/* Multibyte integrity */
				if(c == ',') {
#else NLS
				if(c2 == ',') {
#endif NLS

					if(cflg++)
						ERROR(44);

#ifndef NLS	/* Multibyte integrity */
					if((c = GETC()) == '\\')
#else NLS
					_get16(&c1, &c2);
					if (c1 == '\216' && c2 == '\\')
#endif NLS

						*ep++ = 255;
					else {

#ifndef NLS	/* Multibyte integrity */
						UNGETC(c);
#else NLS
						UNGETC(c2);
#endif NLS

						goto nlim;
						/* get 2'nd number */
					}
				}

#ifndef NLS	/* Multibyte integrity */
				if(GETC() != '}')
#else NLS
				_get16(&c1, &c2);
				if(!(c1 == '\216' && c2 == '}'))
#endif NLS

					ERROR(45);
				if(!cflg)	/* one number */
					*ep++ = i;
				else if((ep[-1] & 0377) < (ep[-2] & 0377))
					ERROR(46);

#ifdef NLS	/* Multibyte integrity */
				}
#endif NLS

				continue;

			case '\n':
				ERROR(36);

			case 'n':

#ifndef NLS	/* Multibyte integrity */
				c = '\n';
#else NLS
				c1 = '\216';
				c2 = '\n';
#endif NLS

				goto defchar;

			default:

#ifndef NLS	/* Multibyte integrity */
				if(c >= '1' && c <= '9') {
					if((c -= '1') >= closed)
						ERROR(25);
					*ep++ = CBACK;
					*ep++ = c;
#else NLS
				if(c1 == '\216' && c2>= '1' && c2<='9') {
					if((c2 -= '1') >= closed)
						ERROR(25);
					*ep++ = CBACK;
					*ep++ = c2;
#endif NLS

					continue;
				}
			}
	/* Drop through to default to use \ to turn off special chars */

#ifndef NLS	/* Multibyte integrity */
		defchar:
		default:
			lastep = ep;
			*ep++ = CCHR;
			*ep++ = c;
		}
	}
#else NLS
		default:
			goto defchar;
		} else	{
		defchar:
			lastep = ep;
			*ep++ = CCHR;
			*ep++ = c1;
			*ep++ = c2;
		}
	}	while(_get16(&c1, &c2), 1);
#endif NLS

}

step(p1, p2)
register UNSIGNED char *p1, *p2;
{

#ifndef NLS	/* Multibyte integrity */
	register c;
#else NLS
	UNSIGNED char	c1, c2, *_getloc();
	STRING	str;

	str.base = p1;
	str.offset = 0;
#endif NLS

	if(circf) {


#ifndef NLS	/* Multibyte integrity */
		loc1 = p1;
		return(advance(p1, p2));
#else NLS
		_loc1 = 0;
		loc1 = str.base;
		return(_advance(str, p2));
#endif NLS

	}
	/* fast check for first character */
	if(*p2==CCHR) {


#ifndef NLS	/* Multibyte integrity */
		c = p2[1];
		do {
			if(*p1 != c)
				continue;
			if(advance(p1, p2)) {
				loc1 = p1;
				return(1);
			}
		} while(*p1++);
#else NLS
		do {
			_getstr(str, &c1, &c2);
			if(c1 == p2[1] && c2 == p2[2] && _advance(str, p2)) {
				_loc1 = str.offset;
				loc1 =_getloc(str, 0);
				return(1);
			}
			str.offset++;
		} while(c1 && c2);
#endif NLS

		return(0);
	}
		/* regular algorithm */
	do {

#ifndef NLS	/* Multibyte integrity */
		if(advance(p1, p2)) {
			loc1 = p1;
			return(1);
		}
	} while(*p1++);
#else NLS
		if(_advance(str, p2)) {
			_loc1 = str.offset;
			loc1 = _getloc(str, 0);
			return(1);
		}
		_getstr(str, &c1, &c2);
		str.offset++;
	} while(c1 && c2);
#endif NLS

	return(0);
}

advance(lp, ep)
register UNSIGNED char *lp, *ep;
{
#ifdef NLS	/* Multibyte integrity */
	STRING	str;

	str.base = lp;
	str.offset = 0;
	return(_advance(str, ep));
}

_advance(lp, ep)
STRING	lp;
register UNSIGNED char	*ep;
{
#endif NLS

#ifndef NLS	/* Multibyte integrity */
	register UNSIGNED char *curlp;
	UNSIGNED char c;
	UNSIGNED char *bbeg;
#else NLS
	register UNSIGNED int curlp;
	UNSIGNED char c1, c2, *_getloc(), *locsdummy;
	UNSIGNED int bbeg;
	STRING	dummy;
#endif NLS

	UNSIGNED int ct;

	while(1)
		switch(*ep++) {

		case CCHR:

#ifndef NLS	/* Multibyte integrity */
			if(*ep++ == *lp++)
#else NLS
			_getstr(lp, &c1, &c2);
			lp.offset++;
			if(*ep++ == c1 && *ep++ == c2)
#endif NLS

				continue;
			return(0);

		case CDOT:


#ifndef NLS	/* Multibyte integrity */
			if(*lp++)
#else NLS
			_getstr(lp, &c1, &c2);
			lp.offset++;
			if(c1 && c2)
#endif NLS

				continue;
			return(0);

		case CDOL:

#ifndef NLS
			if(*lp==0)
#else NLS
			_getstr(lp, &c1, &c2);
			if(!(c1 && c2))
#endif NLS

				continue;
			return(0);

		case CCEOF:

#ifndef NLS	/* Multibyte integrity */ 
			loc2 = lp;
#else NLS 
			_loc2 = lp.offset;
			loc2 = _getloc(lp, 0);
#endif NLS 

			return(1);

		case CCL:

#ifndef NLS	/* Multibyte integrity */ 
			c = *lp++ & STRIP;
			if(ISTHERE(c)) {
				ep += TABSIZE;
				continue;
			}
			return(0);
#else NLS 
			if (_cclass(ep, lp, 1))	{
				lp.offset++;
				ep = ep + 2 * (*ep) - 1;
				continue;
			}
			return(0);

		case NCCL:
			if (_cclass(ep, lp, 0))	{
				lp.offset++;
				ep = ep + 2 * (*ep) - 1;
				continue;
			}
			return(0);
#endif NLS

		case CBRA:

#ifndef NLS	/* Multibyte integrity */
			braslist[*ep++] = lp;
#else NLS 
			_braslist[*ep] = lp.offset;
			braslist[*ep++] = _getloc(lp, 0);
#endif NLS 

			continue;

		case CKET:

#ifndef NLS	/* Multibyte integrity */
			braelist[*ep++] = lp;
#else NLS
			_braelist[*ep] = lp.offset;
			braelist[*ep++] = _getloc(lp, 0);
#endif NLS 
			continue;

		case CCHR|RNGE:

#ifndef NLS	/* Multibyte integrity */
			c = *ep++;
			getrnge(ep);
			while(low--)
				if(*lp++ != c)
					return(0);
			curlp = lp;
			while(size--) 
				if(*lp++ != c)
					break;
			if(size < 0)
				lp++;
#else NLS
			ep += 2;
			getrnge(ep);
			while(low--)	{
				_getstr(lp, &c1, &c2);
				lp.offset++;
				if(!(c1 == *(ep-2) && c2 == *(ep-1)))
					return(0);
			}
			curlp = lp.offset;
			while(size--)	{
				_getstr(lp, &c1, &c2);
				lp.offset++;
				if(!(c1 == *(ep-2) && c2 == *(ep-1)))
					break;
			}
			if(size < 0)
				lp.offset++;
#endif NLS
			ep += 2;
			goto star;

		case CDOT|RNGE:
			getrnge(ep);

#ifndef NLS	/* Multibyte integrity */
			while(low--)
				if(*lp++ == '\0')
					return(0);
			curlp = lp;
			while(size--)
				if(*lp++ == '\0')
					break;
			if(size < 0)
				lp++;
#else NLS
			while(low--)	{
				_getstr(lp, &c1, &c2);
				lp.offset++;
				if(!(c1 && c2))
					return(0);
			}
			curlp = lp.offset;
			while(size--)	{
				_getstr(lp, &c1, &c2);
				lp.offset++;
				if(!(c1 && c2))
					break;
			}
			if(size < 0)
				lp.offset++;
#endif NLS
			ep += 2;
			goto star;

		case CCL|RNGE:

#ifndef NLS	/* Multibyte integrity */
			getrnge(ep + TABSIZE);
			while(low--) {
				c = *lp++ & STRIP;
				if(!ISTHERE(c))
					return(0);
			}
			curlp = lp;
			while(size--) {
				c = *lp++ & STRIP;
				if(!ISTHERE(c))
					break;
			}
			if(size < 0)
				lp++;
			ep += TABSIZE + 2;
#else NLS
		case NCCL|RNGE:
			getrnge(ep + 2 * (*ep) - 1);
			while (low--)	{
				if (! _cclass(ep, lp, *(ep-1) == (CCL|RNGE)))
					return(0);
				lp.offset++;
			}
			curlp = lp.offset;
			while (size--)	{
				if (! _cclass(ep, lp, *(ep-1) == (CCL|RNGE)))
					break;
				lp.offset++;
			}
			lp.offset++;
			ep = ep + 2 * (*ep) - 1;
#endif NLS

			goto star;

		case CBACK:

#ifndef NLS	/* Multibyte integrity */
			bbeg = braslist[*ep];
			ct = braelist[*ep++] - bbeg;
			if(ecmp(bbeg, lp, ct)) {
				lp += ct;
#else NLS
			bbeg = _braslist[*ep];
			ct = _braelist[*ep++] - bbeg;
			if(ecmp(lp, bbeg, ct)) {
				lp.offset += ct;
#endif NLS

				continue;
			}
			return(0);

		case CBACK|STAR:

#ifndef NLS	/* Multibyte integrity */
			bbeg = braslist[*ep];
			ct = braelist[*ep++] - bbeg;
			curlp = lp;
			while(ecmp(bbeg, lp, ct))
				lp += ct;
			while(lp >= curlp) {
				if(advance(lp, ep))	return(1);
				lp -= ct;
			}
#else NLS
			bbeg = _braslist[*ep];
			ct = _braelist[*ep++] - bbeg;
			curlp = lp.offset;
			while(ecmp(lp, bbeg, ct))
				lp.offset += ct;
			while(lp.offset >= curlp) {
				if(_advance(lp, ep))	return(1);
				lp.offset -= ct;
			}
#endif NLS

			return(0);


		case CDOT|STAR:

#ifndef NLS	/* Multibyte integrity */
			curlp = lp;
			while(*lp++);
#else NLS
			curlp = lp.offset;
			do	{
				_getstr(lp, &c1, &c2);
				lp.offset++;
			}	while (c1 && c2);
#endif NLS

			goto star;

		case CCHR|STAR:

#ifndef NLS	/* Multibyte integrity */
			curlp = lp;
			while(*lp++ == *ep);
			ep++;
#else NLS
			curlp = lp.offset;
			do	{
				_getstr(lp, &c1, &c2);
				lp.offset++;
			}	while (c1 == *ep && c2 == *(ep+1));
			ep += 2;
#endif NLS

			goto star;

		case CCL|STAR:

#ifndef NLS	/* Multibyte integrity */
			curlp = lp;
			do {
				c = *lp++ & STRIP;
			} while(ISTHERE(c));
			ep += TABSIZE;
#else NLS
		case NCCL|STAR:
			curlp = lp.offset;
			while (_cclass(ep, lp, *(ep-1) == (CCL|STAR)))
				lp.offset++;
			lp.offset++;
			ep = ep + 2 * (*ep) - 1;
#endif NLS

			goto star;

		star:

#ifndef NLS	/* Multibyte integrity */
			do {
				if(--lp == locs)
					break;
				if(advance(lp, ep))
					return(1);
			} while(lp > curlp);
#else NLS
			dummy.base = locs;
			dummy.offset = 0;
			locsdummy = _getloc(dummy, 0);
			do {
				lp.offset--;
				if (_getloc(lp, 0) == locsdummy)
					break;
				if(_advance(lp, ep))
					return(1);
			} while(lp.offset > curlp);
#endif NLS

			return(0);

		}
}

getrnge(str)
register UNSIGNED char *str;
{
	low = *str++ & 0377;
	size = *str == 255 ? 20000 : (*str &0377) - low;
}

#ifndef NLS	/* Multibyte integrity */
ecmp(a, b, count)
register UNSIGNED char	*a, *b;
register UNSIGNED count;
{
	while(count--)
		if(*a++ != *b++)
			return(0);
	return(1);
}
#else NLS
ecmp(str, off, count)
STRING	str;
int	off, count;
{
	UNSIGNED char	a1, a2, b1, b2;
	STRING	dummy;

	dummy.base = str.base;
	dummy.offset = off;
	while(count--)	{
		_getstr(str, &a1, &a2);
		_getstr(dummy, &b1, &b2);
		str.offset++;
		dummy.offset++;
		if (!(a1 == b1 && a2 == b2))
			return(0);
	}
	return(1);
}

_cclass(set, c, af)
register UNSIGNED char	*set;
STRING	c;
int	af;
{
	register	n;
	UNSIGNED char	c1, c2;

	_getstr(c, &c1, &c2);
	if (!(c1 && c2))
		return(0);
	n = *set++;
	while (--n)	{
		if (*set == c1 && *(set+1) == c2)
			return(af);
		if (*set == FROM && *(set+1) == TO)	{
			if (_conv(*(set-2),*(set-1))<=_conv(c1,c2) &&
			    _conv(c1,c2)<=_conv(*(set+2),*(set+3)))
				return(af);
		}
		set += 2;
	}
	return(! af);
}

_conv(a1, a2)
register	a1, a2;
{
	register	newbase, oldbase;

	if (a1 == '\216')
		return(a2 & STRIP);
	else
		a1 &= STRIP;
	a2 &= STRIP;
	if (a1 & 0200)	{
		oldbase = 0xa1;		/* The 3rd or 4th plane */
		if (a2 & 0200)
			newbase = 0x21;	/* The 4th plane */
		else
			newbase = 0x50;	/* The 3rd plane */
	} else	{
		oldbase = 0x21;		/* The 1st or 2nd plane */
		if (a2 & 0200)
			newbase = 0xa1;	/* The 2nd plane */
		else
			newbase = 0xd0;	/* The 1st plane */
	}
	if (a1 % 2)
		a2 &= 0x7f;		/* a1 is odd */
	else
		a2 |= 0x80;		/* a1 is even */
	a1 = newbase + ((a1 - oldbase) >> 1);
	return((a1 << 8) + a2);
}

_get16(char1, char2)
UNSIGNED char	*char1, *char2;
{
	INIT
	static	kanji = 0;	/* indicates kanji mode */
	UNSIGNED char	dummy;

	*char1 = *char2 = '\0';
	while (dummy = GETC())	{
		if (dummy == '\033')	{
			if (PEEKC() == '$')	{
				dummy = GETC();		/* get '$' */
				if (PEEKC() == '@')	{
					dummy = GETC();	/* get '@' */
					kanji = 1;/* kanji mode on */
					continue;
				}
				UNGETC(dummy);	/* '$' is pushed back */
			} else if (PEEKC() == '(')	{
				dummy = GETC();		/* get '(' */
				if (PEEKC() == '@')	{
					dummy = GETC();	/* get '@' */
					kanji = 0;/* kanji mode off */
					continue;
				}
				UNGETC(dummy);	/* '(' is pushed back */
			}
			*char1 = '\216';
			*char2 = '\033';
		} else if (kanji)	{
			*char1 = dummy;
			*char2 = GETC();
		} else	{
			*char1 = '\216';
			*char2 = dummy;
		}
		break;
	}
}

_getstr(str, c1, c2)
STRING	str;
UNSIGNED char	*c1, *c2;
{
	UNSIGNED char	*loc, *_getloc();
	int	status;

	loc = _getloc(str, &status);
	if (status)	{
		*c1 = *loc;
		*c2 = *(loc + 1);
	} else	{
		*c1 = '\216';
		*c2 = *loc;
	}
}

UNSIGNED char	*_getloc(str, status)
STRING	str;
int	*status;
{
	int	kanji = 0;	/* indicates kanji mode */
	int	inc;

	if (str.base)	{
		str.offset++;
		do	{
			if (! kanji)	{
				if (*(str.base) == '\033' && (!strncmp((str.base)+1, "$@", 2)))	{
					kanji = 1;
					str.base += 3;
				} else	{
					str.base++;
					str.offset--;
					inc = 1;
				}
			} else	{
				if (*(str.base) == '\033' && (!strncmp((str.base)+1, "(@", 2)))	{
					kanji = 0;
					str.base += 3;
				} else	{
					str.base += 2;
					str.offset--;
					inc = 2;
				}
			}
		}	while(str.offset);
		str.base -= inc;
	}
	if (status)
		*status = kanji;
	return(str.base);
}
#endif NLS
