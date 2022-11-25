/* @(#) $Revision: 70.2 $ */       

#ifndef COLLATE_INCLUDED
#define COLLATE_INCLUDED


#define MASK077		077
#define ENDTABLE	0377	/* end mark of 2 to 1 character		*/

struct col_21tab {
	unsigned char	ch1;	/* first char of 2 to 1			*/
	unsigned char	ch2;	/* second char of 2 to 1		*/
	short	seqnum;	/* sequence number			*/
	unsigned char	priority;/* priority				*/
};

struct col_12tab {
	short 	seqnum;	/* seqnum of second char of 1 to 2	*/
	unsigned char	priority;/* priority of 1 to 2 char		*/
};

#endif /* COLLATE_INCLUDED */
#define FORWD  01
#define BACKWD 02
#define POS    04

#define NAMELEN 25 
