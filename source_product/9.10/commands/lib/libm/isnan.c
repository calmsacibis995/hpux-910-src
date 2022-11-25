#define _MANT_MASK 	0x000FFFFF
#define _EXP_MASK 	0x7FF00000

struct doub_bits {
	unsigned long word1, word2;
	};

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _isnan isnan
#define isnan _isnan
#endif /* _NAMESPACE_CLEAN */
int
isnan(dval) 
double dval;
{
struct doub_bits *bts;

	bts = (struct doub_bits *)&dval; 

	/* Check the Exponent (bits 62 - 52) where the MSB (sign bit) is 
	   bit 63 and LSB is bit 0.  If the Exponent is not all 1's (0x7FF)
	   then we have a number.
	 */
	if ((bts->word1 & _EXP_MASK) != _EXP_MASK)
		return (0);
	
	/* See if the mantissa is not all 0's; 
	   If not all 0's, then we have a NaN 
	 */

	/* Check the mantissa bits in the lower word */
	if (bts->word2 != 0)
		return(1);

	/* Check the Mantissa bits in the upper word */
	if ((bts->word1 & _MANT_MASK) != 0)
		return(1);
	else /* Otherwise, it is INF */
		return(0);
}
