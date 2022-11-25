static char *HPUX_ID = "@(#) $Revision: 66.3 $";

/* ar.ranlib : Modified version of the old ranlib.c . This is no longer
 * a seperate command, but is a part of "ar". Since the new ar always
 * maintains an archive symbol table, it forks a call to /usr/lib/ar.ranlib
 * whenever the archive is modified. This version of ranlib creates a file
 * (named by the second argument in the call from ar , previously 
 * called __.SYMDEF) if there are any external symbols defined in any file
 * of the archive. Note, unlike the previous version, it does not call ar
 * to insert the __.SYMDEF file into the archive ( since ar already calls
 * ranlib, that would result in an infinite loop).
 */

#include <sys/types.h>
#include <ar.h>
#include <ranlib.h>
#include <a.out.h>
#include <stdio.h>
#include <globaldefs.h>

#define TABSZ		5000
#define	STRTABSZ	75000
#define STRPTRSZ	128
#define SYMPTRSZ	13

#define TRUE 		1
#define FALSE 		0

struct	ar_hdr	archdr;
long	arsize; /* arsize is the size of the last file element (minus the 
		   header) read by nextel
		*/
struct	exec	filhdr;
FILE	*fi, *fo;
long	off, oldoff;			/* offset counters in the archive file */
long	atol(), ftell();
struct	ranlib *tab;
unsigned short tnum;			/* total number of symbols */
					/* Must be large enough to hold the */
					/* expression TABSZ * SYMPTRSZ */
struct  ranlib *symptr[SYMPTRSZ];       /* array of symbol buffer pointers  */
unsigned short symptrcnt; 		/* symbol buffer pointer index      */
unsigned short symindex;		/* index into current symbol buffer */
unsigned short new;
char	*tstrtab;
int	tssiz;				/* the tstrtab table counter */
					/* Must be large enough to hold the */
					/* expression STRTABSZ * STRPTRSZ */
char    *strptr[STRPTRSZ]; 		/* array of string buffer pointers  */
unsigned short strptrcnt;		/* string buffer pointer index      */	
int     strindex;			/* index into current string buffer */
int	ssiz;
char	*tempnm;
char	symbuf[SYMLENGTH+1];	/* temp buffer for symbol name */

main(argc, argv)
char **argv;
{
	char cmdbuf[BUFSIZ];
	char magbuf[SARMAG+1];
	int  i;

		tempnm = argv[2];
		tssiz = 0;
		fi = fopen(*++argv,"r");
		if (fi == NULL) {
			fprintf(stderr, "ar:(ranlib) cannot open %s\n", *argv);
			exit(1);
		}
		off = SARMAG;
		fread((char *)magbuf, 1, SARMAG, fi);
		if (strncmp(magbuf, ARMAG, SARMAG))
		{	fprintf(stderr, "ar:(ranlib) bad magic number for archive file %s \n", *argv);
			exit(1);
		}
		new = tnum = 0;
		if (nextel(fi) == 0) {
			fclose(fi);
			/* empty archive */
			exit(0);
		}
		strindex = 0;
		strptrcnt = 0;
		tstrtab = (char *)malloc(STRTABSZ);
		strptr[0] = tstrtab;
		symindex = 0;
		symptrcnt = 0;
		tab = (struct ranlib *)malloc(TABSZ * sizeof(struct ranlib));
		symptr[0] = tab;
		do {
			long o;
			register n;
			struct nlist_ sym;

			fread((char *)&filhdr, 1, sizeof(struct exec), fi);
			if (N_BADMAG(filhdr))
				continue;
			if (filhdr.a_lesyms == 0) {
				/* fprintf(stderr, "ar:(ranlib) warning: %s(%s): no symbol table\n", *argv, archdr.ar_name); */
				continue;
			}
			o = LESYMPOS - sizeof (struct exec);
			if (ftell(fi)+o+sizeof(ssiz) >= off) {
				fprintf(stderr, "ar:(ranlib) %s(%s): old format .o file\n", *argv, archdr.ar_name);
				exit(1);
			}
			fseek(fi, o, 1);
			/* we should now be at the beginning of the list */
			n = filhdr.a_lesyms;
			while (n > 0) {
				/* read the nlist_ structure first. Then read
				   the following name into a buffer. 
				*/
				fread((char *)&sym, sizeof(sym), 1, fi);
				fread (symbuf, sym.n_length, 1, fi);
				n -= sizeof(sym) + sym.n_length;
				if ((sym.n_type & EXTERN)==0)
					continue;
				switch (sym.n_type&LO5BITS) {

				case UNDEF : 
					if (sym.n_value) stash(&sym);
					/* Undefined syms with a non-zero 
					   value indicate a .comm symbol.
					   These will be consolidated later. 
					*/
					continue;

				default :
					stash(&sym);
					continue;
				}
			}
		} while(nextel(fi));
		new = fixsize();
		fclose(fi);
		if(tnum == 0) exit(0);
		fo = fopen(argv[1], "w");
		if(fo == NULL) {
			fprintf(stderr, "ar:(ranlib) can't create temporary\n");
			exit(1);
		}
		/* first write out the number of ranlib structures */
		fwrite(&tnum,  sizeof (tnum), 1, fo);
		/* write out the size of the ascii table */
		fwrite(&tssiz, 1, sizeof (tssiz), fo);
		/* write out the ascii table */
		for (i=0; i<strptrcnt; i++) {
			fwrite(strptr[i], STRTABSZ, 1, fo);
		}
		fwrite(strptr[i], tssiz-(strptrcnt*STRTABSZ), 1, fo);
		/* now write out the ranlib structures themselves */
		for (i=0; i<symptrcnt; i++) {
			fwrite((char *)symptr[i], sizeof(struct ranlib), TABSZ, fo);
		}
		fwrite((char *)symptr[i], sizeof(struct ranlib), tnum-(i*TABSZ),fo);
		fclose(fo);
	exit(0);
}

/* nextel reads the archive header and fills certain globals */

nextel(af)
FILE *af;
{
	register char *cp;

	oldoff = off;
	fseek(af, off, 0);
	if (fread((char *)&archdr, 1, sizeof(struct ar_hdr), af) != sizeof(struct ar_hdr))
		return(0);
	for (cp=archdr.ar_name; cp < & archdr.ar_name[sizeof(archdr.ar_name)]; cp++)
		if (*cp == '/')
			*cp = '\0';
	arsize = atol(archdr.ar_size);
	if (arsize & 1)
		arsize++;
	off = ftell(af) + arsize;
	return(1);
}

stash(s)
	struct nlist_ *s;
{
	register unsigned char i = 0;
	
	symbuf[s->n_length] = 0;		/* make symbol asciz */
	tab[symindex].ran_un.ran_strx = tssiz;
	tab[symindex].ran_off = oldoff;

	if(++symindex >= TABSZ) {
		if (++symptrcnt >= SYMPTRSZ) {
			fprintf(stderr, "ar:(ranlib) symbol table overflow\n");
			exit(1);
		} else {
			tab = (struct ranlib *)malloc(TABSZ*sizeof(struct ranlib));
		 	symptr[symptrcnt] = tab;		
			symindex = 0;
                }
	}
	while (TRUE) {
		while ( (tstrtab[strindex++] = symbuf[i++]) && 
			(strindex < STRTABSZ) );
		
		if (strindex == STRTABSZ) {
			if (++strptrcnt >= STRPTRSZ) {
				fprintf(stderr, "ar:(ranlib) string table overflow\n");
				exit(1);
			} else {
				tstrtab = (char *)malloc(STRTABSZ);
				strptr[strptrcnt] = tstrtab;
				strindex = 0;
				if (i > s->n_length) break;
                        }
		} else {
			break;
		}
        }
	tssiz += (s->n_length +1);
	tnum++;
}





/* fixsize -	Now that the asciz table and the ranlib structure table have
		been made, their sizes are known. It is now possible to go
		back thru the ranlib structure table and adjust the ran_off
		fields so that they will show the true offset of the library
		member files relative to the beginning of the archive.
*/


fixsize()
{
	register int i;
	register off_t offdelta;	/* the correction factor taking into
					   account the sizes of the __.SYMDEF
					   structures now that they're known */
        register int blockcount;
        int symtabcnt;
	struct ranlib *symtab;

	if (tssiz&1)
		tssiz++;
	offdelta = sizeof(archdr) + sizeof (tnum) + tnum * sizeof(struct ranlib)
			+ sizeof (tssiz) + tssiz;
	off = SARMAG;

	/* look at the 1st file in the archive. If it's a directory from a prior
	   ranlib run, it will have to be replaced anyway so don't count it.
	*/
	nextel(fi);
	if(archdr.ar_name[0] == 0) offdelta -= sizeof(archdr) + arsize;

	blockcount = 0;
	symtabcnt = 0;
	symtab = symptr[0]; 
	for(i=0; i<tnum; i++) {
		symtab[blockcount++].ran_off += offdelta;
		if (blockcount == TABSZ) {
			symtab = symptr[++symtabcnt];
			blockcount = 0;
		}
        }
	return(new);
}
