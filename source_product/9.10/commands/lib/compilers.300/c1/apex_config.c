/* SCCS apex_config.c    REV(64.1);       DATE(91/11/30        15:39:56) */
/* KLEENIX_ID @(#)apex_config.c	64.1 91/11/30 */

#include <stdio.h>
#include <string.h>
#include "apex.h"

#define INBUF_LEN	128
#define NUM_STDS	64	
#define POOL_SIZE	256

#define FALSE	0
#define TRUE	1

struct std_defs *get_names(fname)
  char *fname;
{
char buf[INBUF_LEN];
char *start, *end;
struct std_defs *std_p;
int std_size;
char *pool, *p;
int index=0;
int bit_num;
int std_num;	/* This holds the main std num, 
		 *exclusive of the referenced stds */
int str_len;
FILE *fp;
int this_std[NUM_STD_WORDS], this_num[NUM_STD_WORDS];

	fp = fopen(fname, "r");
	if (fp == NULL) {
	    return NULL;
	}

	/* a string pool to hold all standards names (may eventually hold
	 * more names than the 128 bit positions in a stds array because
	 * of synonyms.  Set initial size as a compromise between stds
	 * (128 bits) and origin (32 bits)
	 */
	std_size = NUM_STDS * sizeof(struct std_defs);
	if ((std_p = (struct std_defs *)malloc(std_size) ) == NULL)
	    return NULL;
	if ( (pool = (char *)malloc(POOL_SIZE) ) == NULL)
	    return NULL;
	p = pool;
	
	while ( fgets(buf, INBUF_LEN, fp) != NULL ) {
	    start = buf;
	    if (*start != '\n' && *start != '#') {
		/* non-blank, non-comment */
		while (*start == ' ' || *start == '\t')
		    start++;
		if (*start != '\n') {	/* non-blank */
		    /* look for standard number */
		    if (!isdigit(*start))
			return NULL;
		    bit_num = atoi(start);
		    std_num = bit_num;
		    std_zero(this_std);
		    set_bit(this_std, bit_num);
		    while (isdigit(*start))
			start++;
		    while (*start == ' ' || *start == '\t')
			start++;
		    if (*start == ':') {
			/* include some referenced standards */
			start++;
			while (*start == ' ' || *start == '\t')
			    start++;
			while (isdigit(*start)) {
			    bit_num = atoi(start);
			    std_zero(this_num);
			    set_bit(this_num, bit_num);
			    std_or(this_std, this_num, this_std);
			    while (isdigit(*start))
				start++;
			    while (*start == ' ' || *start == '\t')
				start++;
			    if (*start == ',')
				start++;
			    while (*start == ' ' || *start == '\t')
				start++;
			}
		    }
		    
		    while (*start != '\n' && *start != '\0' ) {
			end = start;
			while (isalnum(*end) || *end=='_' || *end=='.' )
			    end++;
			str_len = end - start;
			if ( p-pool+str_len+1 > POOL_SIZE ) {
			    if ( (pool = (char *)malloc(POOL_SIZE) ) == NULL)
				return NULL;
			    p = pool;
			}
			strncpy(p, start, str_len);
			if (index > std_size) {
			    std_size *= 2;
			    if ((std_p = (struct std_defs *)
					    realloc(std_p, std_size) ) == NULL)
				return NULL;
			}
			std_p[index].name = p;
			std_cpy(std_p[index].bits, this_std);
			std_p[index].std_num = std_num;
			index++;
			p += str_len;
			*p++ = '\0';
			start = end;
			if (*start != '\n' && *start != '\0' )
			    start++;	/* skip over separator */
		    }
		}
	    }
	}
	fclose(fp);
	std_p[index].name = NULL;
	std_zero(std_p[index].bits);
	return std_p;
}

int get_bit(std, name, list)
  long std[NUM_STD_WORDS];
  char *name;
  struct std_defs *list;
{
	while (list && list->name) {
	    if (strcmp(name, list->name) == 0) {
		std_cpy(std, list->bits);
		return 0;
	    } else
		list++;
	}
	return -1;
}


/* Set the bit in std corresponding to the "std_num" of "name" in "list".
 * This std setting (without the referenced standards) will be used for 
 * the encoding of the libraries from the APEX directives.  Don't encode
 * all referenced standards in the definition, or there will be standards
 * matches whenever you target any of the subset referenced standards.
 * E.g., a POSIX routine must set only bit 2 or else a POSIX, but non-ANSI
 * routine will look like it's compliant to ANSI.
 */
int get_std_num(std, name, list)
  long std[NUM_STD_WORDS];
  char *name;
  struct std_defs *list;
{
	std_zero(std);
	while (list && list->name) {
	    if (strcmp(name, list->name) == 0) {
		set_bit(std, list->std_num);
		return 0;
	    } else
		list++;
	}
	return -1;
}

void set_bit(stds, bit)
  long stds[NUM_STD_WORDS];
  int bit;
{
  int pos, i;

	if (bit==ALL_STD_BITS) {
	    for (i=0; i<NUM_STD_WORDS; i++)
		stds[i] = ~0;
	} else if ( bit<=0 || bit>(NUM_STD_WORDS*32) )
	    return;
	else {
	    pos = bit - 1;		/* 1-based bit position */
	    stds[pos/32] |= ( 1 << (pos%32) );
	}
}


void std_print(this_std, stdp)
long this_std[NUM_STD_WORDS];
struct std_defs *stdp;
{
    int comma;
    int i;
    int alias;
    long match[NUM_STD_WORDS];

	comma = FALSE;
	printf("[");
	for ( ; stdp->name; stdp++) {
	    std_zero(match);
	    set_bit(match, stdp->std_num);
	    if (std_overlap(match, this_std) ) {
		if (comma)
		    printf(",%s", stdp->name);
		else {
		    printf("%s", stdp->name);
		    comma = TRUE;
		}
		/* print only the first name for a standard (skip aliases) */
		alias = stdp->std_num;
		while ( (stdp+1)->name && (stdp+1)->std_num == alias )
			stdp++;
	    }
	}
	printf("]");
}


print_names(stdp)
struct std_defs stdp[];
{
int index;
int prev_num;

	index = 0;
	prev_num = -1;

	while (stdp[index].name) {
	    if (prev_num == stdp[index].std_num) 
		fprintf(stderr, ", ");
	    else {
		fprintf(stderr, "\n\t");
		prev_num = stdp[index].std_num;
	    }
	    fprintf(stderr, "%s", stdp[index].name);
	    index++;
	}
	fprintf(stderr, "\n");
}



/* standards manipulation routines */

void std_zero(std)
long std[NUM_STD_WORDS];
{
int i;
	for (i=0; i<NUM_STD_WORDS; i++)
	    std[i] = 0;
}

void std_cpy(to, from)
long to[NUM_STD_WORDS], from[NUM_STD_WORDS];
{
int i;
	for (i=0; i<NUM_STD_WORDS; i++)
	    to[i] = from[i];
}


/* compare standards s1 and s2, returning -1, 0, or +1 as s1 is <, =, or > s2 */
int std_cmp(s1, s2)
long s1[NUM_STD_WORDS], s2[NUM_STD_WORDS];
{
int i;
	for (i=0; i<NUM_STD_WORDS; i++) {
	    if ((unsigned)s1[i] < (unsigned)s2[i])
		return -1;
	    else if ((unsigned)s1[i] > (unsigned)s2[i])
		return 1;
	}

	return 0;

}


void std_or(in1, in2, out)
long in1[NUM_STD_WORDS];
long in2[NUM_STD_WORDS];
long out[NUM_STD_WORDS];
{
int i;
	for (i=0; i<NUM_STD_WORDS; i++)
	    out[i] = in1[i] | in2[i];
}


int std_overlap(in1, in2)
long in1[NUM_STD_WORDS];
long in2[NUM_STD_WORDS];
{
int i;
	for (i=0; i<NUM_STD_WORDS; i++)
	    if (in1[i] & in2[i])
		return TRUE;

	return FALSE;
}


int std_equal(in1, in2)
long in1[NUM_STD_WORDS];
long in2[NUM_STD_WORDS];
{
int i;
	for (i=0; i<NUM_STD_WORDS; i++)
	    if (in1[i] != in2[i])
		return FALSE;

	return TRUE;
}
