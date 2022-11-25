/* @(#) $Revision: 70.1 $ */      

/* Structure for fixup info between pass1 and pass2 */

struct fixup_info {
	char	f_type;		/* fixup type */
	char	f_size;		/* size of fixup, also size of related rexpr */
	char	f_exptype;
	char	f_pcoffs;
	/* use these in place of dotoffs and dotval, for better packing */
	/*unsigned int f_dotoffs:4;	/* (location - dot) used  to reset value of '.' */
	/*unsigned int f_pcoffs:4;	/* (pc - location) -- how to calculate a PC
				 * relative offset, relative to the value of
				 * location in the fixup record.
				 */
	/*char	f_pad;		/* unused field */
	/*short	f_exptype;	/* type of the related rexpr */
	/*short	f_pad2;		/* unused field */
	long	f_location;	/* location in segment that needs fixing */
	long	f_dotval;	/* current value of '.' */
	long	f_lineno;	/* source lineno for error messages in pass 2 */
	symbol	* f_symptr;	/* symbol pointer for the related rexpr */
	long	f_offset;	/* offset part of the related rexpr */
# ifdef SDOPT
				/* ?? we could eliminate the need for this next
				 * field with a more complex pass2 that 
				 * traverse the sdopt chain at the same time
				 * that it processes the fixup list.
				 */
	struct sdi * f_sdiinfo;	/* pointer to relevant sdi info for a branch */
# endif
	};

/* Types of fixup */
/* The basic types required */
# define F_EXPR		1		/* fix up an expression */
# define F_PCDISP	2		/* PC-relative displacement */
# define F_BCCDISP	3		/* Bcc-type PC-relative displacement */
# define F_ALIGN	4		/* For "align" pseudo-op */
# define F_CPBCCDISP	5		/* CPBcc-type PC-relative displacement */
# define F_LALIGN	6		/* Fix lalign's when doing span-depen-
					 * dent optimization.
					 */

/* The following fixup types anticipate future enhancements/improvements,
 * but are not currently implemented.
 */
# define F_FILLER	10	/* optimization for large "space" op's */
# define F_SETDOT	11	/* rather than keeping "dotval" in fixup_info */
/* The next three fixup types will be needed if we allow immediate subfields
 * (eq, in addq or bkpt) to be pass1 forward (symbolic), resolved to absolute
 * by pass2.
 */
# define F_CHKABS	12	/* check absolute expression in pass2 */
# define F_ISUBFLD	13	/* Fix immediate subfield */
# define F_BFSPEC	14	/* Fix immediate bitfield specifiers */


extern long fixent;
extern long fixindx;
extern struct fixup_info fixbuf[];

# define FIXBUFSZ	10
