/* $Revision: 64.2 $ */
/* Header file for pdf commands */

#include <sys/param.h>

#define FALSE          0
#define TRUE           1
#define MAXLINELEN   (sizeof(struct pdfitem)+10)
#define fragment_size	DEV_BSIZE

/* Predeclarations of the functions used in mkpdf */

char *In_file;                 /* pointer to prototype PDF name    */
char *Out_file;                /* pointer to final PDF name        */
char *regcmp();
char *regex();
char *setgroup();
char *setowner();
int  process_line();
int  setcksum();
void setlink();
void setmode();
void setsize();
void settarget();
void setversion();
void check_links();
int  verbose_check_report();
void bubblesort();
int  parse_line();	/* returns size of file */

/* PDFITEM - this is a structure that is used as storage for one element
   of a PDF. */

struct pdfitem
{
	char pathname[MAXPATHLEN];
	char owner[32];
	char group[32];
	char mode[12];
	char fsize[12];
	char link_count[12];
	char version[50];
	char checksum[32];
	char link_target[MAXPATHLEN];
};
