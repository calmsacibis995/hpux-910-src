#ifndef LD_H_INCLUDED
#define LD_H_INCLUDED

/* Long double typedef - to be added to <stdlib.h> */
typedef struct {
	unsigned int word1, word2, word3, word4;
} long_double;

/* Strtold definition - to be added to <stdlib.h> */
long_double strtold();

/* Language flags - duplicate of those in quad.h */
#define ANSIC	0
#define FORTRAN	1
#define PASCAL	2	/* for future use */
#define ADA	3	/* for future use */

/* Standard Radix */
#define STDRADIX        '.'

#endif LD_H_INCLUDED
