/* @(#) $Revision: 70.1 $ */      

/* Constant and data definitions to support span-dependent optimization
 * of branch instructions.  The instructions handled are bCC, bsr, bra.
 * We work down from the top: assume a worst case (largest size), and
 * then shorten where possible.
 */

/* Enumeration/bitflag definitions that will be used in the span-dependent
 * structures.
 */
/* Type of span-dependent node: high bit of char is fixed-flag.  Lower seven bits serve as
 * an enumeration type.
 */
# define SDSPANFXD	0x80	/* modifier flag: span-node but can't shrink anymore */
# define SDNDTYPE	0x7f	/* to mask off modifier bits, and leave just the enumeration. */
# define SDSPAN		1
# define SDLABEL	2
# define SDLALIGN	3

/* Span-dependent opcode type */
# define SDBCC		0
# define SDBRA		1
# define SDBSR		2
# define SDCPBCC	3	/* Co-processor branches */

/* Span-dependent sizes */
# define SDZERO		0	/* ??? not sure */
# define SDBYTE		1
# define SDWORD		2
# define SDLONG		3
# define SDUNKN		4
# define SDEXTN		5


/* The following structure will be used to build a chain of information
 * about span dependent instructions, and their targets (labels).
 */

struct sdi {
	struct sdi  * sdi_next;		/* next sdi in list. This field 
					 * MUST COME FIRST -- because of
					 * the way sdi_listhead and sdi_listtail
					 * are used when the lists are empty.
					 */
	long	sdi_location0;		/* Original location of the span
					 * or label node.  */
	long	sdi_location;		/* Current location of the span
					 * or label node.  */
	rexpr  sdi_exp;			/* Expression for the target */
					/* *** really should use something
					 * smaller, this has room for floats
					 *  ******/
	struct sdi * sdi_target;	/* For a span node, a ptr to the
					 * corresponding label node
					 * for its target */
	long	sdi_offset;		/* Offset from target label */
	unsigned char 	sdi_nodetype;	/* mark whether we have a "span"
					 * node or a "label" node.  */
	unsigned char	sdi_optype;	/* opcode type */
	unsigned char	sdi_size;	/* Current size of the instruction
					 * SDBYTE, etc. */
	char	sdi_length;		/* Current length of the sdi in bytes */
	long	sdi_span;		/* Current span of the sdi (?? don't
					 * really need, would be for integrity
					 * checks only) */
	char	sdi_tdelta;		/* Total change made to this sdi */
	char	sdi_idelta;		/* Incremental delta, for last loop */
	long	sdi_base;		/* Base origin of the span, current */
	long	sdi_line;		/* src lineno, in case of error message */

	};

/* NOTE that the sdi structure is also used for SDALIGN nodes, which
 * contain info about lalign nodes.  Here the fields are not all used,
 * and are interpreted differently:
 * The relavent fields in the sdi struct are:
 *	sdi_nodtype:	SDLALIGN
 *	sdi_span :	the lalignment value.
 *	sdi_offset:	final number of filler bytes
 *	sdi_tdelta:	the change in number of filler
 *			bytes, from the initial worst
 *			case assignment of (lalignval-2).
 *	sdi_location0:	original location of lalign.
 *	sdi_location:	final location after span-dependent.
 *
 ***********************************************************************
 */

extern short sdi_length[][6];
extern short sdi_loc_to_base[];
extern short sdopt_flag;
extern struct sdi * makesdi_spannode();
extern struct sdi * makesdi_labelnode();
extern int  map_sdisize();
extern struct sdi * sdi_listhead;
extern struct sdi * sdi_listtail;
extern struct sdi * makesdi_lalignnode();
extern long sdi_dotadjust();
