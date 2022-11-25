/* @(#) $Revision: 66.1 $ */

#include "nipc.h"

#ifdef _NAMESPACE_CLEAN
#define memcpy _memcpy
#define legal_option _legal_option
#endif /* _NAMESPACE_CLEAN */

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _addopt addopt
#define addopt _addopt
#endif /* _NAMESPACE_CLEAN */

void
addopt(opt, index, code, length, data, result)
	char	*opt;
	short	index;
	short	code;
	short	length;
	char	*data;
	short	*result;
{

	struct opthead		*head;
	struct optentry		*entryp;

	/* check for bad option buffer pointer */
	if (opt == 0)
	 	ERR_RETURN(NSR_ADDR_OPT);

	/* cast the buffer pointer */
	head = (struct opthead *)opt;

	/* check for illegal options */
	if (!legal_option(code))
		ERR_RETURN(NSR_OPT_OPTION);

	/* verify that index is legitimate */
	if (index < 0  ||  index >= head->opt_entries)
		ERR_RETURN(NSR_OPT_ENTRY_NUM);

	/* check for a bad data length */
	if (length < 0)
		ERR_RETURN(NSR_OPT_DATA_LEN);

	/* point to where the options entry belongs */
	entryp = (struct optentry *)((unsigned int)head  +
		(index*sizeof(struct optentry)) + sizeof(struct opthead));

	/* fill in the fields of the options entry */
	entryp->opte_offset	= head->opt_length + sizeof(struct opthead);
	entryp->opte_code	= code;
	entryp->opte_length	= length;

	/* move in the new entry's data */
	memcpy( (caddr_t)head+entryp->opte_offset, data, (unsigned)length );

	/* update the length of the option buffer */
	head->opt_length = head->opt_length + length;

	ERR_RETURN(NSR_NO_ERROR);

}	/***** addopt *****/
