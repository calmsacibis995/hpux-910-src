/* @(#) $Revision: 64.1 $ */     
#ifndef _PORT_INCLUDED /* allows multiple inclusions */
#define _PORT_INCLUDED
#ifdef __hp9000s300
/* This file encapsulates as many structures (and other information)
   as possible to facilitate porting of code between various machines */

/* These constants really belong in cpp, not here */

#undef vax
#undef focus
#define chipmunk

#ifdef vax
struct short_char {
	char	low_byte;
	char	hi_byte;
};

struct long_short {
	short	low_short;
	short	hi_short;
};

struct long_char {
	char	byte_3;
	char	byte_2;
	char	byte_1;
	char	byte_0;
};
#endif /* vax */

#ifdef chipmunk
struct short_char {
	char	hi_byte;
	char	low_byte;
};

struct long_short {
	short	hi_short;
	short	low_short;
};

struct long_char {
	char	byte_0;
	char	byte_1;
	char	byte_2;
	char	byte_3;
};
#endif /* chipmunk */

#ifdef focus
struct short_char {
	char	hi_byte;
	char	low_byte;
};

struct long_short {
	short	hi_short;
	short	low_short;
};

struct long_char {
	char	byte_0;
	char	byte_1;
	char	byte_2;
	char	byte_3;
};
#endif /* focus */
#endif /* __hp9000s300 */
#endif /* _PORT_INCLUDED */
