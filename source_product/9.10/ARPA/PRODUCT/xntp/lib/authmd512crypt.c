/* $Header: authmd512crypt.c,v 1.2.109.2 94/10/28 17:03:31 mike Exp $
 *  md5crypt - MD5 based authentication routines
 */

# include "ntp_stdlib.h"
# include "md5.h"

extern U_LONG   cache_keyid;
extern char    *cache_key;
extern int  cache_keylen;

/*
 * Stat counters, imported from data base module
 */
extern U_LONG   authencryptions;
extern U_LONG   authdecryptions;
extern U_LONG   authkeyuncached;
extern U_LONG   authdecryptok;
extern U_LONG   authnokey;

/*
 * For our purposes an NTP packet looks like:
 *
 *	a variable amount of encrypted data, multiple of 8 bytes, followed by:
 *	NOCRYPT_OCTETS worth of unencrypted data, followed by:
 *	BLOCK_OCTETS worth of ciphered checksum.
 */
# define	NOCRYPT_OCTETS	4
# define	BLOCK_OCTETS	16

# define	NOCRYPT_LONGS	((NOCRYPT_OCTETS)/sizeof(U_LONG))
# define	BLOCK_LONGS	((BLOCK_OCTETS)/sizeof(U_LONG))

static MD5_CTX  ctx;

/*
 *  Do first stage of a two stage authenticator generation.
 */

void
MD5auth1crypt (keyno, pkt, length)
U_LONG  keyno;
U_LONG *pkt;
int     length;			/* length of all encrypted
				   data */
{

    authencryptions++;

    if (keyno != cache_keyid)
        {
	authkeyuncached++;
	if (!authhavekey (keyno))
	    {
	    authnokey++;
	    return;
	    }
        }

    MD5Init (&ctx);
    MD5Update (&ctx, cache_key, cache_keylen);
    MD5Update (&ctx, (char *)pkt, length - 8);
    /* just leave the partially computed value in the
       static MD5_CTX */
}

/*
 *  Do second state of a two stage authenticator generation.
 */
int
MD5auth2crypt (keyno, pkt, length)
U_LONG  keyno;
U_LONG *pkt;
int     length;			/* total length of
				   encrypted area */
{
    /* 
     *  Don't bother checking the keys.  The first stage would have
     *  handled that.  Finish up the generation by also including the
     *  last 8 bytes of the data area.
     */

    MD5Update (&ctx, (char *)(pkt) + length - 8, 8);
    MD5Final (&ctx);

    bcopy ((char   *)ctx.digest, (char *)&pkt[NOCRYPT_LONGS + length / sizeof (U_LONG)],
	    BLOCK_OCTETS);
    return 4 + BLOCK_OCTETS;
}
