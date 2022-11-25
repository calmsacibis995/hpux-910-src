/* @(#) $Revision: 37.1 $ */      
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                 bit.h                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define	BITSPERWORD	32		/* number of bits in HP-UX ints */
#define	ino4blk(x)	(FMAP_INUM + ((x) / 1024))

#define	_bit(i)		(1 << (BITSPERWORD - 1 - (i)))
#define	bitval(word, i)	(word & _bit(i))
#define setbit(word, i)	(word |= (_bit(i)))
#define	clrbit(word, i)	(word &= ~(_bit(i)))
