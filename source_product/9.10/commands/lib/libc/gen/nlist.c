/* @(#) $Revision: 66.2 $ */   
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define fopen _fopen
#define fread _fread
#define fseek _fseek
#define nlist _nlist
#define fclose _fclose
#define b_magic _b_magic
#ifdef __lint
#define feof _feof
#define getc _getc
#endif  /* __lint */
#endif  /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <a.out.h>

#ifdef _NAMESPACE_CLEAN
#undef b_magic
#pragma _HP_SECONDARY_DEF _b_magic b_magic
#define b_magic _b_magic
#endif
short b_magic[] = {EXEC_MAGIC, SHARE_MAGIC, DEMAND_MAGIC, RELOC_MAGIC, SHL_MAGIC, DL_MAGIC, 0};/* MFM */

#ifdef _NAMESPACE_CLEAN
#undef nlist
#pragma _HP_SECONDARY_DEF _nlist nlist
#define nlist _nlist
#endif

nlist(name, list)
char *name;					/* file name */
struct nlist *list;				/* ptr to nlist list */
{
	register FILE *f;			/* MFM */
	register struct nlist *p;
	register char *cp;
	register short i, c;			/* MFM */
	register long pos;			/* MFM */
	struct exec filhdr;
	char symbuf[SYMLENGTH];
	struct nlist_ sym;			/* MFM */

	/* the nlist structure no longer contains its own name */
	
	for(p = list; p->n_name && p->n_name[0]; p++) {
		p->n_value = 0;
		p->n_type = 0;
		p->n_length = 0;
		p->n_almod = 0;
		p->n_unused = 0;
	}
	f = fopen(name, "r");
	if(f == NULL) return(-1);
	fread((char *)&filhdr, sizeof filhdr, 1, f);
	if (feof(f)) goto bad;
	for(i=0; b_magic[i]; i++)
		if(b_magic[i] == filhdr.a_magic.file_type) break;
	if(b_magic[i] == 0) goto bad;
	fseek(f, (long)LESYMPOS, 0);

	/* the following loop has been extensively rewritten MFM */
	for (pos = filhdr.a_lesyms; pos > 0; pos -= sizeof(sym)+sym.n_length)
	{
		/* first fill the symbuf */
		fread(&sym, sizeof sym, 1, f);
		if (feof(f)) goto bad;
		for (cp = symbuf, i = sym.n_length; i>0 ; --i)
		{
			if ((c = getc(f)) == EOF) goto bad;
			*cp++ = c;
		}
		if (sym.n_length < SYMLENGTH) *cp = 0;		/* make asciz */
		else *--cp = 0;

		/* now check against the clist */
		for(p = list; p->n_name && p->n_name[0]; p++)
		{
			/* NOTE : WE COULD CHECK p->n_length AGAINST 
			   s->n_length FIRST IF p-> n_length IS REQUIRED TO
			   BE SET BEFORE THE CALL. CONSIDER LATER.
		        */
			cp = p->n_name;
			for(i=0;i<SYMLENGTH;i++)
			{
				c = *cp++;
				if (symbuf[i] != c) goto cont;
				if (c == 0) break;
			}
			p->n_value = sym.n_value;
			p->n_type = sym.n_type;
			p->n_length = sym.n_length;
			p->n_almod = sym.n_almod;
			p->n_unused = sym.n_unused;
			continue;
		cont:		;
		}
	}
	fclose(f);
	return(0);

bad:	fclose(f);
	return(-1);
}
