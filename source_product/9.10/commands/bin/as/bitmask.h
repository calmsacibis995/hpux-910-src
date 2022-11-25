/* @(#) $Revision: 70.1 $ */    

/* BITMASK(b,n) constructs a bitmask of n '1' bits, starting at bit
 * b.  where the bits are numbered 32 31 .. 0 and b will refer to the
 * leftmost '1' in the mask.
 * This macro should only be used for 32-bit masks ( or 16-bit).
 */
# define BITMASK1(n)  ( (1L << (n) ) - 1)
# define BITMASK(b,n) ( BITMASK1(n) << (b-(n)+1) )
