/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/symbol.c,v $
 * $Revision: 1.15.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:32:03 $
 */


/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/


#include "inc.h"
#include "defs.h"
#include "types.h"
#include "externs.h"

#ifdef hp9000s800
read_symbols()
{
	int loc;
	register struct nlist *sp;
	register struct symbol_dictionary_record *sdp;
	int ssiz, syms;
	char *strtab;


	/* read som headr */
	if (read(fsym, (char *)&somhdr, sizeof(somhdr)) != sizeof(somhdr)) {
		fprintf(stderr," Couldn't read som hdr, errno %d\n",errno);
		exit(1);
	}

	/* read aux headr */
	(void)lseek(fsym, (long)somhdr.aux_header_location, 0);
	if (read(fsym, (char *)&auxhdr, sizeof(auxhdr)) != sizeof(auxhdr)){
		fprintf(stderr, " Couldn't read aux headr errno %d\n",errno);
		exit(1);
	}
	/* store away our mapping into the file */
	txtmap.b1 = auxhdr.exec_tmem;
	txtmap.e1 = auxhdr.exec_tmem + auxhdr.exec_tsize;
	txtmap.f1 = auxhdr.exec_tfile;
	txtmap.b2 = auxhdr.exec_dmem;
	txtmap.e2 = auxhdr.exec_dmem + auxhdr.exec_dsize;
	txtmap.f2 = auxhdr.exec_dfile;
	loc = somhdr.symbol_location;
	syms = somhdr.symbol_total *
			sizeof(struct symbol_dictionary_record);
	/* get space for symbol dictionary */
	symdict = (struct symbol_dictionary_record *) malloc((unsigned)syms);
	esymdict = &symdict[somhdr.symbol_total];

	/* read symbol dictionary */
	(void)lseek(fsym, (long)loc, 0);
	if (read(fsym, (char *)symdict, syms) != syms){
		fprintf(stderr," Couldn't read symbol table errno %d\n",errno);
		exit(1);
	}
	loc = somhdr.symbol_strings_location;
	(void)lseek(fsym, (long)loc, 0);
	ssiz = somhdr.symbol_strings_size;
	/* get space for string table */
	strtab = (char *)malloc((unsigned)ssiz);
	if (strtab == 0){
		fprintf(" Couldn't get space for string table\n");
		exit(1);
	}
	/* read string table */
	if (read(fsym, strtab, ssiz) != ssiz){
		fprintf(" Couldn't read string table\n");
		exit(1);
	}
	/* scan symbol dictionary, modify string table index field
	 * to have the pointer to the actual string in the string table
	 * instead of just the index.
	 */
	for (sdp = symdict; sdp < esymdict; sdp++)
		if (sdp->name.n_strx)
			sdp->name.n_name = strtab + sdp->name.n_strx;
}


/*
 * Lookup a symbol by name.
 */
unsigned
lookup(symstr)
	char *symstr;
{
	register struct symbol_dictionary_record *sdp;

	for (sdp = symdict; sdp < esymdict; sdp++){
		if (issym(sdp) && (strcmp(sdp->name.n_name, symstr) == 0))
			return(sdp->symbol_value);
	}
	return(0);
}


/* similar to adb findsym, Find the closet symbol to the address given */
findsym(val)
	register int val;
{
	register struct symbol_dictionary_record *sdp;
	register int diff;

	if (endbss == 0)
		endbss = lookup("end");

	cursym = "unknown";
	diff = 0xffffffff;

	if (val  > endbss){
		diff = val;
		/* check for a few well known tables */
		if ((val  >= (int)vswbuf) && (val < (int)vinode)){
				cursym = "swbuf";
				diff = val - (int)vswbuf;
		} else  if ((val  >= (int)vinode) && (val < (int)vfile)){
				cursym = "inode";
				diff = val - (int)vinode;
		} else  if ((val  >= (int)vfile) && (val < (int)vproc)){
				cursym = "file";
				diff = val - (int)vfile;
		} else  if ((val  >= (int)vproc) && (val <  (int) vsysmap)){
				cursym = "proc";
				diff = val - (int)vproc;
		} else  if ((val  >= (int)vsysmap) && (val < (int)vprp)){
				cursym = "sysmap";
				diff = val - (int)vsysmap;
		} else  if ((val  >= (int)vprp) && (val < (int)vphash)){
				cursym = "pregion";
				diff = val - (int)vprp;
		} else  if ((val  >= (int)vphash) && (val < (int)vpfdat)){
				cursym = "phash";
				diff = val - (int)vphash;
		} else  if ((val  >= (int)vpfdat) && (val < (int)&vpfdat[physmem])){
				cursym = "pfdat";
				diff = val - (int)vpfdat;
		} else  if ((val  >= (int)ubase) && (val < (int)ubase +
		0x0004000)){
				cursym = "ubase";
				diff = val - (int)ubase;
		}
		return(diff);
	}



	for (sdp = symdict; sdp < esymdict; sdp++){
		/* Check for symbol */
		if (issym(sdp)){
			if((val - sdp->symbol_value < diff) &&
				(val >= sdp->symbol_value)){
					diff =(int)val - (int)sdp->symbol_value;
					cursym = sdp->name.n_name;
					if (diff == 0)
						break;
			}
		}
	}
	return(diff);
}

#endif


#ifdef hp9000s300
#include "symdef.h"

SYMPTR		symbol;
CHAR		*lp;
CHAR		isymbol[NCHNAM];		/* MFM */
SYMPTR symget();
unsigned findsym();


SYMPTR
lookup(symstr)
char	*symstr;
{
	register SYMPTR	symp;		/* MFM */
	char tsym[NCHNAM];

	symset();
	while((symp = symget()) != NULL)
		if (((symp->symf & SYMCHK) == symp->symf) &&
		eqsym(symp->symc, symstr, '.')) return(symp);
	strcpy(tsym,"_");
	strcat(tsym,symstr);
	symset();
	while((symp = symget()) != NULL)
		if (((symp->symf & SYMCHK) == symp->symf) &&
		eqsym(symp->symc, tsym, '.')) return((SYMPTR)symp->vals);
	return(NULL);
}

eqsym(s1, s2, c)
register STRING	s1, s2;
CHAR		c;
{
	IF strcmp(s1, s2) == 0
	THEN	return(TRUE);
	ELIF *s1==c
	THEN	CHAR		s3[NCHNAM];
		register short	i;		/* MFM */
		s3[0]=c;
		FOR i=1; i<NCHNAM; i++
		DO s3[i] = *s2++; OD
		return(strcmp(s1,s3)==0);
	ELSE	return(FALSE);
	FI
}


POS		maxoff = 32768;
L_INT		maxstor;
L_INT		mainsval;
short		symnum;
L_INT		symbas;
struct symb	*symtab;
SYMPTR		symbol;
SYMPTR		symnxt;
SYMPTR		symend;
STRING		symnamb;




MAP		txtmap;
MAP		datmap;
int		regoff;
L_INT		maxfile = 1L << 24;
L_INT		txtsiz;
L_INT		datsiz;
L_INT		datbas;
L_INT		stksiz;
short		magic;
L_INT		entrypt;
BHDR 		bhdr;
#ifdef level0
BKPTR		bkpthead;
int		last_txtbas, last_datbas;	/* last relocation bases */
int		def_txtbas,def_datbas;		/* default relocation bases */
#endif

#define BHDRS	sizeof(struct exec)
#define USRSTART	0x0
#define MANX 1

char *malloc();

read_symbols()				/* setup a.out file */
{

	txtmap.ufd = fsym;
	if (read(fsym, &bhdr, BHDRS) == BHDRS)
	{
		magic = bhdr.a_magic.file_type;
		if (N_BADMAG(bhdr) ||
		    bhdr.a_magic.system_id == HP98x6_ID)
		{
			magic = 0;
			fprintf(outf," - warning: not in executable format");
		}
		else
		{
			/* treat relocatable files like unshared */
			if (magic == RELOC_MAGIC)
			{
				fprintf(outf," - warning: relocatable file");
				/* restored later */
				magic = EXEC_MAGIC;
			}

			txtsiz = bhdr.a_text;
			datsiz = bhdr.a_data;
			entrypt = bhdr.a_entry;
			symbas = LESYM_OFFSET(bhdr);
			txtmap.b1 = USRSTART;
			txtmap.e1 = txtmap.b1+txtsiz+(magic==EXEC_MAGIC ? datsiz : 0);
			txtmap.f1 = TEXT_OFFSET(bhdr);
			if (magic == SHARE_MAGIC || magic == DEMAND_MAGIC)
#ifndef	MANX
				datbas = DATA_ADDR(btoc(txtsiz));
#else
				datbas = EXEC_ALIGN(txtsiz);
#endif
			else datbas = txtmap.b1;
			txtmap.b2 = datbas;
			txtmap.e2 = datbas+datsiz+(magic==EXEC_MAGIC ? txtsiz : 0);
			txtmap.f2 = (magic == EXEC_MAGIC ? BHDRS : DATA_OFFSET(bhdr));
#ifdef level0
			def_txtbas = last_txtbas = txtmap.b1;
			def_datbas = last_datbas = txtmap.b2;
#endif

			/* read in symbol table from a.out file */
			setupsym(bhdr.a_lesyms);
			symset();
		}
	}
	if (magic == 0) txtmap.e1 = maxfile;

	if (bhdr.a_magic.file_type == RELOC_MAGIC) magic = RELOC_MAGIC;

/*
	fprintf(outf,"\n");
*/
}





setupsym(ssize)
register long	ssize;
{
	register SYMPTR	symptr;
	register char	*nameptr;
	extern	char	*malloc();
	register short	i;
	register FILE	*bout;
	int 	mns;		/* maximum number of symbols */
	int 	tabsize;	/* size of adb's symbol table structure */

	/*
		adb's symbol table structure

	 --------------------------------------------------------
	| symb structures -->                     <-- name pool  |
	 --------------------------------------------------------
	 ^							^
	 |							|
	symtab						      symnamb

	The table is one data structure, with the struct symb entries
	in the beginning, and the name pool at the end. The symb
	structures have pointers to the symbol name which is stored
	at the other side of this table. The symb structures and name
	pool grow towards each other. The size of the table, tabsize,
	is computed as follow
				a.out symbol table size
	1. Max # of symbols =   ------------------------
				sizeof(struct nlist_) + 1
							^
							|
				min. size of each symbol name is 1 char

	2. Table Size =  size of a.out symbol table +
		 max # of symbols [1 char to make each symbol name asciz] +
		 mns * sizeof (char *) [because symb has an added fld symc] +
		 sizeof symb [for sentinal value at end, ESYM]

	*/

	mns = ssize / ( sizeof (struct nlist_) + 1);
	tabsize = ssize + mns * (1+sizeof(char *)) + sizeof (struct symb);
	symtab = (struct symb *) malloc (tabsize);
	symnamb = (char *)symtab;
	symnamb += tabsize - 1;

	if ((bout = fopen(kernel,"r")) == NULL)
		 printf("\n\tcannot fopen %s", kernel);
	fseek(bout, symbas, 0);			/* seek start of symbols  */
	symnum = 0;
	symptr = symtab;
	if (ssize == 0) printf("\n\tno symbol table - proceeding anyway");
						/* build symbol table	  */
	while (ssize > 0)
	{
		if (fread(symptr, 1, sizeof(struct nlist_), bout) != sizeof(struct nlist_)) break;
		symptr->smtp = SYMTYPE(symptr->symf);
		if ((ssize -= sizeof (struct nlist_)) == 0) break;
		*symnamb = 0;			/* to make asciz */
		nameptr = symnamb - symptr->slength;
		symnamb = nameptr -1;
		symptr->symc = nameptr;

		for (i=0;i<symptr->slength;i++) {
			if (fread(nameptr++, 1,1, bout) != 1) break;
			}
		ssize -= i;					/* MFM */
		symnum++; symptr++;
	}
	symend = symptr;			/* end of symbols */
	symend->smtp = ESYM;
	if ((char *)symptr >= symnamb)
		printf("\n\tbeware: internal error - symbol table");
	else if (ssize) printf("\n\tsymbol table problem - proceeding anyway");
	fclose(bout);
}

longseek(f, adr)
FILE	*f;
long	adr;
{
	return(lseek(f, adr, 0) != -1);
}

psymoff(v, type, s)
long	v;
int	type;
char	*s;
{
	unsigned register w = findsym(v);
	if (w >= maxoff) fprintf(outf,"0x%x", v);
	else
	{
		fprintf(outf,"%s", symbol->symc);
		if (w) fprintf(outf,"+0x%x",w);
	}
	fprintf(outf,s);
}

unsigned
findsym(val)
unsigned long	val;
{
	register long	diff = 0377777L;
	register unsigned long symval;
	register SYMPTR	symptr;
	SYMPTR	symsav = NULL;

	if (endbss == 0)
		endbss = (int)lookup("_end");


	cursym = "unknown";
	diff = 0xffffffff;

	if (val  > endbss){
		diff = val;
		/* check for a few well known tables */
		if ((val  >= (int)vswbuf) && (val < (int)vinode)){
				cursym = "swbuf";
				diff = val - (int)vswbuf;
		} else  if ((val  >= (int)vinode) && (val < (int)vfile)){
				cursym = "inode";
				diff = val - (int)vinode;
		} else  if ((val  >= (int)vfile) && (val < (int)vproc)){
				cursym = "file";
				diff = val - (int)vfile;
		} else  if ((val  >= (int)vproc) && (val <  (int) vsysmap)){
				cursym = "proc";
				diff = val - (int)vproc;
		} else  if ((val  >= (int)vsysmap) && (val < (int)vprp)){
				cursym = "sysmap";
				diff = val - (int)vsysmap;
		} else  if ((val  >= (int)vprp) && (val < (int)vphash)){
				cursym = "pregion";
				diff = val - (int)vprp;
		} else  if ((val  >= (int)vphash) && (val < (int)vpfdat)){
				cursym = "phash";
				diff = val - (int)vphash;
		} else  if ((val  >= (int)vpfdat) && (val < (int)&vpfdat[physmem])){
				cursym = "pfdat";
				diff = val - (int)vpfdat;
		} else  if ((val  >= (int)ubase) && (val < (int)ubase +
		0x0004000)){
				cursym = "ubase";
				diff = val - (int)ubase;
		}
		return(diff);
	}


	if (symptr = symtab)
	{
		while (diff && (symptr->smtp != ESYM))
		{
			symval = symptr->vals;
			if (((val - symval) < diff)
				&& (val >= symval))
			{
				diff = val - symval;
				symsav = symptr;
			}
			symptr++;
		}
		if (symsav){
			symbol = symsav;
			cursym = symsav->symc;
		}
	}
	return(shorten(diff));
}

nextsym()
{
	if (++symbol == symend)
	{
		symbol--;
		return(FALSE);
	}
	else return(TRUE);
}

symset()
{
	symnxt = symtab;
}

SYMPTR
symget()
{
	SYMPTR	ptr;

	if (symnxt >= symend) return(NULL);
	ptr = symnxt; symnxt++;
	return(ptr);
}

#endif



/* check_kernel() , This routine compares the version string of the
 * kernel with the corefile.
 */
check_kernel()
{
	char core_ver[80];
	char kernel_ver[80];


	lseek(fcore,nl[X_VERSION].n_value,0);
	if (read(fcore,(char *)core_ver,80) < 0){
		fprintf(stderr," Could not read core version! errno %d\n",errno);
	}

	/* we know that it is in the data section of the maps */
	lseek(fsym,(nl[X_VERSION].n_value - txtmap.b2 + txtmap.f2),0);
	if (read(fsym,(char *)kernel_ver,80) < 0){
		fprintf(stderr," Could not read kerenel version! errno %d\n",errno);
	}

	if (strncmp(core_ver,kernel_ver,80) != 0){
		fprintf(stderr," WARNING\n WARNING!!! Kernel versions do not match!\n WARNING\n");
	}

}
