/* @(#) $Revision: 70.1 $ */      

/* structures for intermediate code */

struct instruction {
   int noperands;
   unsigned short ivalue;
   unsigned short iclass;  /* ??? for generic handling of a class like I_Bcc */
   unsigned short operation_size;
   unsigned char  cpid;		/* for co-processor instructions */
   struct sdi * sdi_info;
   operand_addr opaddr[MAXOPERAND];
};

#define	FILL	0L	/* value for fill bytes within data */
#define	TXTFILL	0L	/* fill value in text */
#define TXTPAD	0x4e71	/* pad value at end of text -- use NOP's */

struct filler {
	long fillcount;
	short fillsize;
	long fillval;
};

