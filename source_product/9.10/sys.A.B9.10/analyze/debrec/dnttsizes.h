/*
 * @(#)dnttsizes.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 16:22:21 $
 * $Locker:  $
 */

/*
 * Original version based on:
 * UNISRC_ID: @(#)dnttsizes	4.00	85/10/01
 */

# define DNTTSIZEOF(type)  ( (sizeof(type) + DNTTBLOCKSIZE-1) / DNTTBLOCKSIZE)

/* Array that defines the size of a dntt-entry (# of blocks) for each
   dntt-kind.  Indexed by dntt-kind.
*/

long	DNTTSIZE [] = {
	DNTTSIZEOF (struct DNTT_SRCFILE),

	DNTTSIZEOF (struct DNTT_MODULE),
	DNTTSIZEOF (struct DNTT_FUNC),
	DNTTSIZEOF (struct DNTT_FUNC),
	DNTTSIZEOF (struct DNTT_BEGIN),
	DNTTSIZEOF (struct DNTT_END),
	DNTTSIZEOF (struct DNTT_IMPORT),
	DNTTSIZEOF (struct DNTT_LABEL),

	DNTTSIZEOF (struct DNTT_FPARAM),
	DNTTSIZEOF (struct DNTT_SVAR),
	DNTTSIZEOF (struct DNTT_DVAR),
	DNTTSIZEOF (struct DNTT_DVAR),
	DNTTSIZEOF (struct DNTT_CONST),
 
	DNTTSIZEOF (struct DNTT_TYPE),
	DNTTSIZEOF (struct DNTT_TYPE),
	DNTTSIZEOF (struct DNTT_POINTER),
	DNTTSIZEOF (struct DNTT_ENUM),
	DNTTSIZEOF (struct DNTT_MEMENUM),
	DNTTSIZEOF (struct DNTT_SET),
	DNTTSIZEOF (struct DNTT_SUBRANGE),
	DNTTSIZEOF (struct DNTT_ARRAY),
	DNTTSIZEOF (struct DNTT_STRUCT),
	DNTTSIZEOF (struct DNTT_UNION),
	DNTTSIZEOF (struct DNTT_FIELD),
	DNTTSIZEOF (struct DNTT_VARIANT),
	DNTTSIZEOF (struct DNTT_FILE),
	DNTTSIZEOF (struct DNTT_FUNCTYPE),
	DNTTSIZEOF (struct DNTT_WITH),
	DNTTSIZEOF (struct DNTT_COMMON),
	DNTTSIZEOF (struct DNTT_COBSTRUCT),
};
