
/*****************************************************************
 * $Source: /misc/source_product/9.10/commands.rcs/usr/include/dnttsizes.h,v $
 * $Revision: 70.1 $
 * $Date: 91/12/16 10:56:33 $
 *****************************************************************
 * Revision 66.4  89/12/15  16:02:45  16:02:45  markm (Mark McDowell)
 * Added K_MEMFUNC.
 * 
 * Revision 66.2  89/09/22  16:36:51  16:36:51  jmn (John Newman)
 * added C++ DNTT information; also added K_BLOCKDATA
 * 
 * Revision 66.1  89/08/29  10:01:19  10:01:19  jmn (John Newman)
 * merged s300/s800 file.  Also added RCS header, some additional comments.
 * 
 *
 ************  NOTE **********************************************
 *
 * Since this file contains a data definitions (will allocate space)
 * don't try to include it from more than one file of a multi-file
 * command.
 *****************************************************************
 */

#define DNTTSIZEOF(type)  ( (sizeof(type) + DNTTBLOCKSIZE-1) / DNTTBLOCKSIZE)

/* DNTTSIZE[] is an array that defines the size of each type of dntt-entry
 * (# of blocks).  Indexed by dntt-kind.
 */

long	DNTTSIZE [] = {
	DNTTSIZEOF (struct DNTT_SRCFILE),	/* 0:K_SRCFILE	*/

	DNTTSIZEOF (struct DNTT_MODULE),	/* 1:K_MODULE	*/
	DNTTSIZEOF (struct DNTT_FUNC),		/* 2:K_FUNCTION	*/
	DNTTSIZEOF (struct DNTT_FUNC),		/* 3:K_ENTRY	*/
	DNTTSIZEOF (struct DNTT_BEGIN),		/* 4:K_BEGIN	*/
	DNTTSIZEOF (struct DNTT_END),		/* 5:K_END	*/
	DNTTSIZEOF (struct DNTT_IMPORT),	/* 6:K_IMPORT	*/
	DNTTSIZEOF (struct DNTT_LABEL),		/* 7:K_LABEL	*/

	DNTTSIZEOF (struct DNTT_FPARAM),	/* 8:K_FPARAM	*/
	DNTTSIZEOF (struct DNTT_SVAR),		/* 9:K_SVAR	*/
	DNTTSIZEOF (struct DNTT_DVAR),		/* 10:K_DVAR	*/
	0,                            		/* 11:<<unused>>*/
	DNTTSIZEOF (struct DNTT_CONST),		/* 12:K_CONST	*/
 
	DNTTSIZEOF (struct DNTT_TYPE),		/* 13:K_TYPEDEF	*/
	DNTTSIZEOF (struct DNTT_TYPE),		/* 14:K_TAGDEF	*/
	DNTTSIZEOF (struct DNTT_POINTER),	/* 15:K_POINTER	*/
	DNTTSIZEOF (struct DNTT_ENUM),		/* 16:K_ENUM	*/
	DNTTSIZEOF (struct DNTT_MEMENUM),	/* 17:K_MEMENUM	*/
	DNTTSIZEOF (struct DNTT_SET),		/* 18:K_SET	*/
	DNTTSIZEOF (struct DNTT_SUBRANGE),	/* 19:K_SUBRANGE*/
	DNTTSIZEOF (struct DNTT_ARRAY),		/* 20:K_ARRAY	*/
	DNTTSIZEOF (struct DNTT_STRUCT),	/* 21:K_STRUCT	*/
	DNTTSIZEOF (struct DNTT_UNION),		/* 22:K_UNION	*/
	DNTTSIZEOF (struct DNTT_FIELD),		/* 23:K_FIELD	*/
	DNTTSIZEOF (struct DNTT_VARIANT),	/* 24:K_VARIANT	*/
	DNTTSIZEOF (struct DNTT_FILE),		/* 25:K_FILE	*/
	DNTTSIZEOF (struct DNTT_FUNCTYPE),	/* 26:K_FUNCTYPE*/
	DNTTSIZEOF (struct DNTT_WITH),		/* 27:K_WITH	*/
	DNTTSIZEOF (struct DNTT_COMMON),	/* 28:K_COMMON	*/
	DNTTSIZEOF (struct DNTT_COBSTRUCT),	/* 29:K_COBSTRUCT*/

	DNTTSIZEOF (struct DNTT_XREF),		/* 30:K_XREF	*/
	DNTTSIZEOF (struct DNTT_SA),		/* 31:K_SA	*/
	DNTTSIZEOF (struct DNTT_SA),		/* 32:K_MACRO <<unused>>*/

	DNTTSIZEOF (struct DNTT_FUNC),		/* 33:K_BLOCKDATA*/
#ifdef CPLUSPLUS
	DNTTSIZEOF (struct DNTT_CLASS_SCOPE),	/* 34:K_CLASS_SCOPE*/
	DNTTSIZEOF (struct DNTT_POINTER),	/* 35:K_REFERENCE*/
	DNTTSIZEOF (struct DNTT_PTRMEM),	/* 36:K_PTRMEM   */
	DNTTSIZEOF (struct DNTT_PTRMEM),	/* 37:K_PTRMEMFUNC*/
	DNTTSIZEOF (struct DNTT_CLASS),		/* 38:K_CLASS   */
	DNTTSIZEOF (struct DNTT_GENFIELD),	/* 39:K_GENFIELD */
	DNTTSIZEOF (struct DNTT_VFUNC),		/* 40:K_VFUNC    */
	DNTTSIZEOF (struct DNTT_MEMACCESS),	/* 41:K_MEMACCESS*/
	DNTTSIZEOF (struct DNTT_INHERITANCE),	/* 42:K_INHERITANCE*/
	DNTTSIZEOF (struct DNTT_FRIEND_CLASS),	/* 43:K_FRIEND_CLASS*/
	DNTTSIZEOF (struct DNTT_FRIEND_FUNC),	/* 44:K_FRIEND_FUNC*/
	DNTTSIZEOF (struct DNTT_MODIFIER),	/* 45:K_MODIFIER */
	DNTTSIZEOF (struct DNTT_OBJECT_ID),	/* 46:K_OBJECT_ID */
	DNTTSIZEOF (struct DNTT_FUNC),		/* 47:K_MEMFUNC */
#ifdef TEMPLATES
	DNTTSIZEOF (struct DNTT_TEMPLATE),	/* 48:K_TEMPLATE */
	DNTTSIZEOF (struct DNTT_TEMPL_ARG),	/* 49:K_TEMPL_ARG */
	DNTTSIZEOF (struct DNTT_FUNC_TEMPLATE),	/* 50:K_FUNC_TEMPL*/
	DNTTSIZEOF (struct DNTT_LINK),		/* 51:K_LINK	  */
#endif /* TEMPLATES */
#endif
			/* last element must be indexable by K_MAX */
};
