/* @(#) $Revision: 64.1 $ */    
#ifndef _NLIST_INCLUDED /* allow multiple inclusions */
#define _NLIST_INCLUDED

/* file nlist.h */

#ifdef __hp9000s300
/* NOTE: this structure resembles, but is not the same as nlist_ defined
	 in a.out.h!
*/
struct nlist {				/* sizeof(struct nlist)=14 */
	char *		n_name;
	long	        n_value;	/* mit = svalue */
	unsigned char	n_type;		/* mit = stype */
	unsigned char	n_length;	/* length of ascii symbol name */
	short	        n_almod;        /* alignment mod */
	short	        n_unused;
};
#endif /* __hp9000s300 */

#ifdef __hp9000s800
struct nlist {
        char		*n_name;
	char		*n_qual;
        unsigned short	n_type;
        unsigned short	n_scope;
        unsigned int	n_info;
        unsigned long	n_value;
};
#endif /* __hp9000s800 */

#endif /* _NLIST_INCLUDED */
