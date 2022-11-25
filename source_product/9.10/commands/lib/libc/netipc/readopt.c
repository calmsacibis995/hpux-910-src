/* @(#) $Revision: 66.1 $ */

#include "nipc.h"

#ifdef _NAMESPACE_CLEAN
#define legal_option _legal_option
#define memcpy _memcpy
#endif /* _NAMESPACE_CLEAN */

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _readopt readopt
#define readopt _readopt
#endif /* _NAMESPACE_CLEAN */

/* read the Nth option */
void
readopt( opt, index, code, length, buffer, result )
	caddr_t	opt;
	int	index;
	short	*code;
	short	*length;
	caddr_t	buffer;
	short	*result;

{			/***** readopt *****/

	struct opthead		*head;
	struct optentry		*entryp;


	/* check for bad option pointer */
	if (opt == 0)
		ERR_RETURN(NSR_ADDR_OPT );

	/* cast the head of the buffer */
	head = (struct opthead *)opt;

	/* check of a bad index */
	if (index < 0  ||  index >= head->opt_entries)
		ERR_RETURN(NSR_OPT_ENTRY_NUM);

	/* point to the option entry specified by index */
	entryp = (struct optentry *)
		((unsigned int)head + sizeof(struct opthead) +
		(index*sizeof(struct optentry)));

	/* do sanity check on the option */
	if (!legal_option(entryp->opte_code )	  ||
	     entryp->opte_length < 0		 	  ||
	     (entryp->opte_offset + entryp->opte_length)  >
	     (head->opt_length + sizeof(struct opthead)))
		ERR_RETURN(NSR_OPT_CANTREAD);

	/* CHECK THE SUPPLIED BUFFER IS LARGE ENOUGH */
	if (*length < entryp->opte_length)
		ERR_RETURN(NSR_OPT_DATA_LEN);

	/* SET THE OUTPUT PARM. TO SIZE OF THE OPTION */
	*length = entryp->opte_length;		/* amount of data to move */

	/* SET THE CODE OF THE INDEXED OPTION */
	*code	= entryp->opte_code;

	/* move the data to the user buffer */
	memcpy( buffer, (caddr_t)head+entryp->opte_offset, (unsigned)*length );

	ERR_RETURN(NSR_NO_ERROR);

}			/***** readopt *****/
