#include <stdio.h>
#include <a.out.h>

/* until a.out.h is 8.0 : define the new relocation regions */
#define RPC	04
#define RDLT    05
#define RPLT	06

#define ISYM		2
#define printc(x)	printf("%c",x)
#define leng(a)		((long)((unsigned)(a)))
#define h_FMT		0
#define	x_FMT		1
#define	X_FMT		2
#define	IMDF		x_FMT

static long dot;
static long label;

long instfetch();

FILE *infile;
FILE *trelocfile;	/* extra file pointer for reading text reloc */

struct reloctab {	/* dynamicly allocate space for text reloc seg */
 int number;		/* number of r_info records */
 struct r_info *tab;	/* array of SIZE r_info records */
} treloctab;		/* the text relocation table */

struct r_info *trelocrec= NULL;	/* current text relocation record */
int isReloc = 0;	/* true if a relocation record applies to an address */

struct stable {
	struct stable *next;	/* pointer to next table entry in value order*/
	struct stable *nextsym; /* pointer to next symbol in sym order */
	long value;		/* value of this symbol */
	unsigned char type;	/* symbol type */
	int number;		/* symbol number */
	char name[1];		/* name of this symbol, asciz form */
}   *symhead = NULL,		/* list sorted by value of symbols in Text */
    *symlist = NULL;		/* list sort by number */

static symNumber = 0;		/* the current symbol number */
#define isInTextSeg(x) ((x & TEXT))


static char *badop = "\t???";

char *bname[16] = { "ra", "sr", "hi", "ls", "cc", "cs", "ne",
		    "eq", "vc", "vs", "pl", "mi", "ge", "lt", "gt", "le" };

char *ccname[16] = { "t", "f", "hi", "ls", "cc", "cs", "ne",
		    "eq", "vc", "vs", "pl", "mi", "ge", "lt", "gt", "le" };

char *shro[4] = { "as", "ls", "rox", "ro" };

char *bit[4] = { "btst", "bchg", "bclr", "bset" };

char *bfld[8] = { "bftst", "bfextu", "bfchg", "bfexts", 
		  "bfclr", "bfffo",  "bfset", "bfins" };

int omove(),obranch(),oimmed(),oprint(),oneop(),soneop(),oreg(),ochk();
int olink(),omovem(),oquick(),omoveq(),otrap(),oscc(),opmode(),shroi();
int extend(),biti(),odbcc(),omovep(), omovec(), omoves(), ortd();
int ortm(),ocallm(),ocas(),ocas2(),ochk2(),opack(),otrapcc(),bitfld();
int odivl(),o881(),tosrccr(),fromsrccr();

char * contreg();

struct opdesc
{
	unsigned short mask, match;
	int (*opfun)();
	char *farg;
} opdecode[] =
{					/* order is important below */
  0xF178, 0x0108, omovep, "w",
  0xF178, 0x0148, omovep, "l",
  0xF000, 0x1000, omove, "b",		/* move instructions */
  0xF000, 0x2000, omove, "l",
  0xF000, 0x3000, omove, "w",
  0xF000, 0x6000, obranch, 0,		/* branches */
  0xFFF0, 0x06C0, ortm, 0,		/* op class 0  */
  0xFFC0, 0x06C0, ocallm, 0,
  0xF9C0, 0x00C0, ochk2, 0,		/* chk2, cmp2 */
  0xFF00, 0x0000, oimmed, "ori",
  0xFF00, 0x0200, oimmed, "andi",
  0xFF00, 0x0400, oimmed, "subi",
  0xFF00, 0x0600, oimmed, "addi",
  0xFF00, 0x0800, biti, 0,		/* static bit */
  0xF9FF, 0x08FC, ocas2, 0,
  0xF9C0, 0x08C0, ocas, 0,
  0xFF00, 0x0A00, oimmed, "eori",
  0xFF00, 0x0C00, oimmed, "cmpi",
  0xFF00, 0x0E00, omoves, 0,
  0xF100, 0x0100, biti, 0,		/* dynamic bit */
  0xFFC0, 0x40C0, fromsrccr, "%sr", /* op class 4 */
  0xFFC0, 0x42C0, fromsrccr, "%cc",
  0xFF00, 0x4000, soneop, "negx",
  0xFF00, 0x4200, soneop, "clr",
  0xFFC0, 0x44C0, tosrccr, "%cc",
  0xFF00, 0x4400, soneop, "neg",
  0xFFC0, 0x46C0, tosrccr, "%sr",
  0xFF00, 0x4600, soneop, "not",
  0xFFF8, 0x4808, olink, "l",
  0xFFC0, 0x4800, oneop, "nbcd",
  0xFFF8, 0x4840, oreg, "\tswap\t%%d%d",
  0xFFF8, 0x4848, oreg, "\tbkpt\t&%d",
  0xFFC0, 0x4840, oneop, "pea",
  0xFFF8, 0x4880, oreg, "\text.w\t%%d%d",
  0xFFF8, 0x48C0, oreg, "\text.l\t%%d%d",
  0xFFF8, 0x49C0, oreg, "\textb.l\t%%d%d",
  0xFB80, 0x4880, omovem, 0,
  0xFFFF, 0x4AFC, oprint, "illegal",
  0xFFC0, 0x4AC0, oneop, "tas",
  0xFF00, 0x4A00, soneop, "tst",
  0xFFC0, 0x4C00, odivl, "mul",
  0xFFC0, 0x4C40, odivl, "div",
  0xFFF0, 0x4E40, otrap, 0,
  0xFFF8, 0x4E50, olink, "w",
  0xFFF8, 0x4E58, oreg, "\tunlk\t%%a%d",
  0xFFF8, 0x4E60, oreg, "\tmov.l\t%%a%d,%%usp",
  0xFFF8, 0x4E68, oreg, "\tmov.l\t%%usp,%%a%d",
  0xFFFF, 0x4E70, oprint, "reset",
  0xFFFF, 0x4E71, oprint, "nop",
  0xFFFF, 0x4E72, ortd, "stop",
  0xFFFF, 0x4E73, oprint, "rte",
  0xFFFF, 0x4E74, ortd, "rtd",
  0xFFFF, 0x4E75, oprint, "rts",
  0xFFFF, 0x4E76, oprint, "trapv",
  0xFFFF, 0x4E77, oprint, "rtr",
  0xFFFE, 0x4E7A, omovec, 0,
  0xFFC0, 0x4E80, oneop, "jsr",
  0xFFC0, 0x4EC0, oneop, "jmp",
  0xF1C0, 0x4100, ochk, "chk.l",
  0xF1C0, 0x4180, ochk, "chk.w",
  0xF1C0, 0x41C0, ochk, "lea",
  0xF0F8, 0x50C8, odbcc, 0,
  0xF0F8, 0x50F8, otrapcc, 0,
  0xF0C0, 0x50C0, oscc, 0,
  0xF100, 0x5000, oquick, "addq",
  0xF100, 0x5100, oquick, "subq",
  0xF000, 0x7000, omoveq, 0,
  0xF1C0, 0x80C0, ochk, "divu.w",
  0xF1C0, 0x81C0, ochk, "divs.w",
  0xF1F0, 0x8100, extend, "sbcd",
  0xF1F0, 0x8140, opack, "pack",
  0xF1F0, 0x8180, opack, "unpk",
  0xF000, 0x8000, opmode, "or",
  0xF1C0, 0x91C0, opmode, "sub",
  0xF130, 0x9100, extend, "subx",
  0xF000, 0x9000, opmode, "sub",
  0xF1C0, 0xB1C0, opmode, "cmp",
  0xF138, 0xB108, extend, "cmpm",
  0xF100, 0xB000, opmode, "cmp",
  0xF100, 0xB100, opmode, "eor",
  0xF1C0, 0xC0C0, ochk, "mulu.w",
  0xF1C0, 0xC1C0, ochk, "muls.w",
  0xF1F8, 0xC188, extend, "exg",
  0xF1F8, 0xC148, extend, "exg",
  0xF1F8, 0xC140, extend, "exg",
  0xF1F0, 0xC100, extend, "abcd",
  0xF000, 0xC000, opmode, "and",
  0xF1C0, 0xD1C0, opmode, "add",
  0xF130, 0xD100, extend, "addx",
  0xF000, 0xD000, opmode, "add",
  0xF8C0, 0xE8C0, bitfld, 0,		/* bit field instructions */
  0xF100, 0xE000, shroi, "r",
  0xF100, 0xE100, shroi, "l",
  0xF000, 0xF000, o881, 0,		/* mc68881 instruction */
  0, 0, 0, 0
};

main(argc,argv)
int argc;
char **argv;
{
	struct exec filhdr;
	struct stable *s;
	unsigned short opcd;
	long lastlabel, limit;

	if (argc != 2) {
		fprintf(stderr,"Usage: das file\n");
		exit(1);
	};

	if ((infile = fopen(argv[1],"r")) == NULL) {
		fprintf(stderr,"can't open %s for input\n",argv[1]);
		exit(1);
	};

	fread((char *)&filhdr, sizeof(filhdr), 1, infile);

	if (N_BADMAG(filhdr)) {
		fprintf(stderr,"file is not in a.out format\n");
		exit(1);
	};

	fseek(infile, (long) LESYMPOS, 0);
	readsym(filhdr.a_lesyms);

	/* read the text relocation table */
	
	if ((trelocfile = fopen(argv[1],"r")) == NULL) {
		fprintf(stderr,"can't open %s for input\n",argv[1]);
		exit(1);
	};

	/* seek to the text reloc segment */
	if (fseek(trelocfile, (long) RTEXTPOS, 0) == -1)
	 {
	     fprintf(stderr, "error seeking to text reloc seg in %s\n",
		     argv[1]);
	     exit(1);
	 }

	/* allocate storage for the text reloc table */
	treloctab.number = filhdr.a_trsize / sizeof (struct r_info);
	if ((treloctab.tab = (struct r_info *)malloc(filhdr.a_trsize)) == NULL)
	 {
	     fprintf(stderr, "out of memory allocating treloc tab from %s\n",
		     argv[1]);
	     exit(1);
	 }

	/* read the text reloc segment */

	if(fread((char *)treloctab.tab, (long) sizeof(struct r_info),
		 treloctab.number,trelocfile) != treloctab.number)
	 {
	     fprintf(stderr, "error reading text reloc seg in %s\n",
		     argv[1]);
	     exit(1);
	 }
	trelocrec = treloctab.tab;	/* point current rec at 1st record */
	fclose(trelocfile);


	 

	fseek(infile, (long) TEXTPOS, 0);
	dot = 0;
	lastlabel = 0;
	printf("\t\t\ttext\n");
	limit = filhdr.a_text;
	for (s = symhead; s != NULL; s = s->next) 
		if ((s->type & 0xF) == TEXT) break;
	label = s == NULL ? limit : s->value;
	while (dot < limit) {
		while (dot == label) {
			printf("0000  %s:\n",s->name);
			lastlabel = dot;
			while ((s = s->next) != NULL)
				if ((s->type & 0xF) == TEXT) break;
			label = s == NULL ? limit : s->value;
		};
		printf("%04X \t\t",dot - lastlabel);
		opcd = instfetch(2);
		printins(opcd);
		printf("\n");
	};

	fseek(infile, (long) DATAPOS, 0);
	dot = filhdr.a_text;
	printf("\t\t\tdata\n");
	lastlabel = dot;
	limit = dot + filhdr.a_data;
	for (s = symhead; s != NULL; s = s->next) 
		if ((s->type & 0xF) == DATA) break;
	label = s == NULL ? limit : s->value;

	while (dot < limit) {
		while (dot == label) {
			printf("0000  %s:\n",s->name);
			lastlabel = dot;
			while ((s = s->next) != NULL)
				if ((s->type & 0xF) == DATA) break;
			label = s == NULL ? limit : s->value;
		};
		while (label - dot >= 4) {
			printf("%04X \t\t\tlong\t",dot - lastlabel);
			printf("0x%08X\n",instfetch(4));
		};
		while (label - dot >= 2) {
			printf("%04X \t\t\tword\t",dot - lastlabel);
			printf("0x%04X\n",instfetch(2));
		};
		if (dot < label) {
			printf("%04X \t\t\tbyte\t",dot - lastlabel);
			printf("0x%02X\n",instfetch(1));
		};
	};

	fclose(infile);
	return 0;
}

/* print symbol name... */
pname(val)
    int val;
{	
	struct stable *s,
	*p = NULL;		/* previous text seg pointer */

	for (s = symhead; s != NULL; s = s->next)
	 {
	     if (val == s->value)
	      {
		  printf("%s",s->name);
		  return;
	      }
	     else  if(val < s->value)
		      break;
	     p = s;
	 }
	if (p != NULL)
	 {
	     printf("%s+",p->name);
	     hexprint(X_FMT,val - p->value,0);
	 }
	else
	    hexprint(X_FMT,val,0);
}

/* print symbol name... */
psymname(val)
     long val;
{	
        int symnumber;
	struct stable *s, *closest;
	int index;

	if (trelocrec->r_segment == REXT
	    || trelocrec->r_segment == RDLT
	    || trelocrec->r_segment == RPLT
	    || trelocrec->r_segment == RPC)
	 {
	     /* scan down list of symbols until symnumber'th symbol found */
	     symnumber = trelocrec->r_symbolnum;
	     for (s = symlist, index = 0; s != NULL && index < symnumber;
		  s = s->nextsym, index ++)
		 ;
	     if (s != NULL)
	      {
		  if (val)
		   {
		       printf("%s+",s->name);
		       hexprint(X_FMT,val,1);
		   }
		  else
		      printf("%s",s->name);
	      }
	     else
		 hexprint(X_FMT,val,1);
	 }
	else 
	 {
	     /* symbol is in BSS look through the symbol table to
		a symbol with the value, val, remember the closest value */
	     closest = symlist;
	     for (s = symlist; s && val != s->value; s = s->nextsym)
		 if (abs(s->value - val) < abs(closest->value - val))
		     closest = s;

	     if (s == NULL)
	      {
		  /* use the closet symbol */
		  printf("%s+",closest->name);
		  hexprint(X_FMT,val - closest->value,1);
	      }
	     else
		 /* found a symbol */
		 printf("%s",s->name);
	 }
	if (treloctab.number)
	 {
	     if (--treloctab.number)
		 trelocrec++;	/* move to next record */
	     isReloc = 0;
	 }
}
	

/*
 * build linked lists of symbols one in  linker symbol table order  the
 * other sorted by value
 */
readsym(size)
    long size;
{	
	struct nlist_ s;
	 char *p;
	 struct stable *st,*prev,*t;
	static struct stable *lastsym ;
	 long i;
	char name[256];

	while (size > 0) {
		fread(&s, (long) sizeof (s), 1, infile);
		if (feof(infile)) break;
		size -= (sizeof s + s.n_length);
		for ( p=name, i = s.n_length; i > 0; i--)
			if (((*p = getc(infile)) == EOF) || (*p == 0))
			{	
				fprintf(stderr,"error in symbol table");
				return;
			}
			else p++;
		*p = '\0';
		st = (struct stable *)malloc(sizeof(*st) + strlen(name));
		strcpy(st->name,name);
		st->value = s.n_value;
		st->type = s.n_type;
		st->number = symNumber++;

		/* install the symbol at the end of symlist */
		if (symlist == NULL)
		 {
		     /* first symbol */
		     lastsym = symlist = st;
		 }
		else
		 {
		     /*install at end of symlist and move lastsym to new end */
		     lastsym->nextsym = st;
		     lastsym = st;
		 }
		
		if (isInTextSeg(st->type))
		 {
		     /* insert in the list sorted by value */
		     for (prev = NULL,t = symhead; t != NULL;
			  prev = t,t = t->next)
			 if (t->value > st->value)
			     break;
		     if (prev == NULL)
			 symhead = st;
		     else
			 prev->next = st;
		     st->next = t;
		 }
	}
}

printins(inst)
    long inst;
{	
	 struct opdesc *p;

	for (p = opdecode; p->mask; p++)
	    if ((inst & p->mask) == p->match)
		break;

	if (p->mask != 0)
	    (*p->opfun)(inst, p->farg);
	else
	 {
	     printf("\tword\t");
	     hexprint(x_FMT,inst,0);
	 }
}

long instfetch(size)
 int size;
{	
	 int ans = 0;

	 /* is there a text relocation record for this address? */
	 
	 if (treloctab.number)
	  {
	      if (dot == trelocrec->r_address)
		  isReloc = 1;
	  }


	while (size--) {
		ans <<= 8;
		if (dot < label) {
			ans |= getc(infile) & 0377;
			dot++;
		};
	}

	return(ans);
}

printea(mode,reg,size)
long mode, reg;
int size;
{
	long index;
	long offset;		/* offset in text seg */
	short shortDisp;
	int imf;
	long location;
	
	switch ((int)(mode)) {
	  case 0:	printf("%%d%d",reg);   /* DATA REG DIRECT MODE */
			break;

	  case 1:	printf("%%a%d",reg);   /* ADDRESS REG DIRECT */
			break;

	  case 2:	printf("(%%a%d)",reg);  /* ADDR REG INDIRECT */
			break;

	  case 3:	printf("(%%a%d)+",reg); /* ADDR IND W/ POSTINC */
			break;

	  case 4:	printf("-(%%a%d)",reg);  /* ADDR IND W/ PREDEC */
			break;

	  case 5:	shortDisp = instfetch(2);	/* ADDR IND W/ DISP */
	                hexprint(h_FMT,shortDisp,0);
	                printf("(%%a%d)",reg);
			break;

	  case 6:	/* ADDR REG & MEM IND w/ DISPl */
			mode6(reg);
			break;

	  /* the special addressing modes */
	  case 7:	switch ((int)(reg))
			{
			  /* absolute short address */
			  case 0:	shortDisp = instfetch(2);
					hexprint(x_FMT,shortDisp,0);
					printf(".w");
					break;

			  /* absolute long address */
			  case 1:
			     offset = instfetch(4);  /* read value in textseg*/
			     psymname(offset);
			     break;

			  /* pc with displacement */
			  case 2:	shortDisp = instfetch(2)+2;
					hexprint(h_FMT,shortDisp,0);
					printf("(%%pc)");
					break;

			  /* pc with index */
			  case 3:	mode6(-1);
					break;

			  /* immediate data */
			  case 4:
			                location = dot;
			     		index = 0;
			         	index = instfetch(size>=4?4:2);
					imf = size >= 4 ? X_FMT : x_FMT;
					printf("&");
			                if (isReloc &&
 					    trelocrec->r_address == location)
					    hexprint(imf,index,0);
			                else
					 {
					    /*
					     * ignore any symbols, they are
					     * really for the next operand
					     */
					    hexprint(imf,index,1);
					    while (size > 4)
					     {
						 index = instfetch(4);
						 printf(" &");
						 printf(imf,index);
						 size -= 4;
					     }
					}
					 
					break;

			  default:	printf("???");
					break;
			}
			break;

	  default:	printf("???");
	}
}

mode6(reg)
long reg;
{
	long ext1 = 0;

	ext1 = instfetch(2);
	if ((ext1&0400) == 0) 	/* brief format */
		brief_format(ext1, reg);
	else 			/* full format */
		full_format(ext1, reg);
}

/* (addr reg or PC) ind with index + 8 bit disp */
brief_format(ext1, reg)
long ext1, reg;
{
	long disp, scale;
	char temp;

	disp = (char)(ext1&0377);
	scale = mapscale(ext1);
	if (reg == -1) {
	 	/* temp = disp+2; <-- I think this is wrong JB 4/2/90 */
	 	temp = disp;
		hexprint(h_FMT,temp,0);
		printf("(%%pc");
	} else {
		temp = disp;
		hexprint(h_FMT,temp,0);
		printf("(%%a%d",reg);
	};
	printf(",%%%c%d.%c",
	 	(ext1&0100000)?'a':'d',(ext1>>12)&07,
	 	(ext1&04000)?'l':'w');
	if (scale == 1)
		printf(")");
	else
		printf("*%d)", scale);
}

full_format(ext1, reg)
long ext1, reg;
{
	long i_is, disp = 0;
	long temp = 0;

	i_is = ext1&07;
	switch (i_is)
	{
	   case 0: 	/*no mem indirection */
		printf("(");
		print_bdax(ext1, reg, 0);
		printf(")");
		break;

			/* pre-indexed */
	   case 1:	/* null displacement */
	   case 2:	/* word displacement */
	   case 3:	/* long displacement */

		disp = (i_is - 1) * 2;
		printf("([");
		print_bdax(ext1, reg, 0);
		printf("]");
		if (disp) {
			printf(",");
			if (disp == 2) {
				temp = instfetch(disp);
				hexprint(h_FMT,temp,0);
			} else hexprint(h_FMT,instfetch(disp),0);
		};
		printf(")");
		break;

	   case 4:	/* reserved */
		printf("???");
		break;

			/* post-indexed */
	   case 5:	/* null displacement */
	   case 6:	/* word displacement */
	   case 7:	/* long displacement */

		disp = (i_is - 5) * 2;
		printf("([");
		print_bdax(ext1, reg, 1);
		if (disp) {
			printf(",");
			if (disp == 2) {
				temp = instfetch(disp);
				hexprint(h_FMT,temp,0);
			} else hexprint(h_FMT,instfetch(disp),0);
		};
		printf(")");
		break;
	}
}

/* print base disp, base reg and index reg for full format */
print_bdax(ext1, reg, post)
long ext1, reg, post;
{
	long bs, is, bd_size, scale;
	short flag = 0;
	short temp = 0;

	bs = (ext1>>7)&01;
	is = (ext1>>6)&01;
	bd_size = (ext1>>4)&03;
	bd_size = ((bd_size==1) ? 0 : (bd_size==2) ? 2 : (bd_size==3) ? 4 : 0);

	if (bd_size) { 
		if (bd_size == 2) {
			temp = instfetch(bd_size);
			hexprint(h_FMT,temp,0);
		} else
		    hexprint(h_FMT,instfetch(bd_size),0);
		flag++; 
	};
	if (!bs || reg==-1)
	{
	   if(flag) printf(","); else flag++;
	   if (reg == -1)
	   	printf("%%%s", (bs) ? "zpc" : "pc");
	   else
	   	printf("%%a%d", reg);
	}
	if (post) { printf("]"); flag++; }
	if (!is)
	{
	   if(flag) printf(",");
	   scale = mapscale(ext1);
	   printf("%%%c%d.%c",
	 	   (ext1&0100000)?'a':'d',(ext1>>12)&07,
	  	   (ext1&04000)?'l':'w');
	   if (scale != 1) printf("*%d", scale);
	}
	else if (post) printf("???"); 
}

printEA(ea,size)
long ea;
int size;
{
	printea((ea>>3)&07,ea&07,size);
}

mapscale(ext)
 long ext;
{
	ext >>= 9;
	ext &= 03;
	return((ext==0) ? 1 : (ext==1) ? 2 : (ext==2) ? 4 : (ext==3) ? 8 : -1);
}

mapsize(inst,start,byte)
 long inst;
int start, byte;
{
	inst >>= start;
	inst &= 03;
	if (byte) return((inst==1) ? 1 : (inst==2) ? 2 : (inst==3) ? 4 : -1);
	else return((inst==0) ? 1 : (inst==1) ? 2 : (inst==2) ? 4 : -1);
}

char *suffix(size)
 int size;
{
	return((size==1) ? ".b" : (size==2) ? ".w" : (size==4) ? ".l" : ".?");
}

omove(inst, s)
 long inst;
char *s;

{
	int size;

	printf("\tmov.%c\t",*s);
	size = ((*s == 'b') ? 1 : (*s == 'w') ? 2 : 4);
	printea((inst>>3)&07,inst&07,size);
	printc(',');
	printea((inst>>6)&07,(inst>>9)&07,size);
}

omovep(inst,s)
 long inst;
char *s;
{
	printf("\tmovp.%s\t",s);
	if (inst & 0x0080)
	{	printf("%%d%d,",inst>>9);
		printea(5,inst & 0x7,0);
	}
	else
	{	printea(5,inst & 0x7,0);
		printf(",%%d%d",inst>>9);
	}
}

omovec(inst)
 long inst;
{
	 long r = 0;

	printf("\tmovc\t");
	r = instfetch(2);
	if (inst & 1)
	{	printea((r >> 15) & 1, (r >> 12) & 0x07, 0);
		printf(",%%%s",contreg(r & 0x0fff));
	}
	else
	{	printf("%%%s,",contreg(r & 0x0fff));
		printea((r >> 15) & 1, (r >> 12) & 0x07, 0);
	}
}

omoves(inst)
 long inst;
{
	 long r;
	 long t;
	 long c;

	switch ((inst>>6)&3) {
	case 0:
		t = 1;
		c = 'b';
		break;
	case 1:
		t = 2;
		c = 'w';
		break;
	case 2:
		t = 4;
		c = 'l';
	}
	printf("movs.%c\t",c);
	r = instfetch(2);
	if (r & 0x0800)
	{	printea((r>>15)&1,(r>>12)&7,0);
		printc(',');
		printea((inst>>3)&7,inst&7,t);
	}
	else
	{	printea((inst>>3)&7,inst&7,t);
		printc(',');
		printea((r>>15)&1,(r>>12)&7,0);
	}
}	


obranch(inst,dummy)
    long inst;
    int dummy;
{
	 long disp = inst & 0377;
	unsigned long loc = dot;
	char *s; 

	s = ".b";
	if (disp == 0) { 
		s = ".w"; 
		disp = instfetch(2); 
		if (disp > 0x7FFF) disp |= 0xFFFF0000;
	} else if (disp == 0xff) { 
		s = ".l"; 
		disp = instfetch(4); 
	} else if (disp > 0x7F) 
		disp |= 0xFFFFFF00;
	if (isReloc)
	 {
	     printf("\tb%s%s\t",bname[(int)((inst>>8)&017)],".l");
	     psymname(disp);
	 }
	else
	 {
	     printf("\tb%s%s\t",bname[(int)((inst>>8)&017)],s);
	     pname(disp+loc);
	 }
}

odbcc(inst,dummy)
    long inst;
    int dummy;
{
	 short disp;

	printf("\tdb%s\t",ccname[(int)((inst>>8)&017)]);
	printea(0,inst&07,2);
	printc(',');
	disp = (short) instfetch(2);
	if (isReloc)
	    psymname(0);
	else
	    pname((long) disp + dot - 2);
}

oscc(inst,dummy)
    long inst;
    int dummy;
{
	printf("\ts%s\t",ccname[(int)((inst>>8)&017)]);
	printea((inst>>3)&07,inst&07,1);
}

biti(inst, dummy)
    long inst;
    int dummy;
{
	printf("\t%s\t", bit[(int)((inst>>6)&03)]);
	if (inst&0x0100) printf("%%d%d,", inst>>9);
	else { 
		printf("&"); 
		hexprint(IMDF, instfetch(2),0); 
		printc(',');
	};
	printEA(inst,2);
}

opmode(inst,opcode)
long inst;
char *opcode;
{
	 int opmode = (int)((inst>>6) & 07);
	 int reg = (int)((inst>>9) & 07);
	int size;

	size = (opmode==0 || opmode==4) ?
		1 : (opmode==1 || opmode==3 || opmode==5) ? 2 : 4;
	printf("\t%s%s\t", opcode, suffix(size));
	if (opmode>=4 && opmode<=6)
	{
		printf("%%d%d,",reg);
		printea((inst>>3)&07,inst&07, size);
	}
	else
	{
	  if (*opcode == 'c')
	  {
		printf("%%%c%d,",(opmode<=2)?'d':'a',reg);
		printea((inst>>3)&07,inst&07, size);
	  }
	  else
	  {
		printea((inst>>3)&07,inst&07, size);
		printf(",%%%c%d",(opmode<=2)?'d':'a',reg);
	  }
	}
}

shroi(inst,ds)
 long inst;
char *ds;
{
	int rx, ry;
	char *opcode;
	if ((inst & 0xC0) == 0xC0)
	{
		opcode = shro[(int)((inst>>9)&03)];
		printf("\t%s%s\t", opcode, ds);
		printEA(inst,0);
	}
	else
	{
		opcode = shro[(int)((inst>>3)&03)];
		printf("\t%s%s%s\t", opcode, ds, suffix(mapsize(inst,6,0)));
		rx = (int)((inst>>9)&07); ry = (int)(inst&07);
		if ((inst>>5)&01) printf("%%d%d,%%d%d", rx, ry);
		else
		{
			printf("&");
			hexprint(IMDF, (rx ? rx : 8),0);
			printf(",%%d%d", ry);
		}
	}
}		

oimmed(inst,opcode) 
long inst;
 char *opcode;
{
	 int size = mapsize(inst,6,0);
	long constant = 0;
	int imf;

	if (size > 0)
	{
		constant = instfetch((size == 4) ? 4 : 2);
		printf("\t%s%s\t", opcode, suffix(size));
		imf = size == 4 ? X_FMT : x_FMT;
		if (*opcode == 'c')
		{	/* cmpi */
			printEA(inst,size); printc(',');
			printf("&");
			/* force hex print */
			hexprint(imf, constant,1);
			return;
		}
		printf("&");
		hexprint(imf, constant,0); printc(',');
		if ((char)(inst) == 0x3c) printf("%%cc");
		else if ((char)(inst) == 0x7c) printf("%%sr");
		else printEA(inst,size);
	}
	else printf(badop);
}

oreg(inst,opcode)
long inst;
char *opcode;
{
	printf(opcode, (inst & 07));
}

extend(inst, opcode)
 long	inst;
char	*opcode;
{
	 int size = mapsize(inst,6,0);
	int ry = (inst&07), rx = ((inst>>9)&07);
	char *c;

	c = ((inst & 0x1000) ? suffix(size) : " ");
	printf("\t%s%s\t", opcode, c);
	if (*opcode == 'e')
	{
		if (inst & 0x0080) printf("%%d%d,%%a%d", rx, ry);
		else if (inst & 0x0008) printf("%%a%d,%%a%d", rx, ry);
		else printf("%%d%d,%%d%d", rx, ry);
	}
	else if ((inst & 0xF000) == 0xB000) printf("(%%a%d)+,(%%a%d)+", rx, ry);
	else if (inst & 0x8) printf("-(%%a%d),-(%%a%d)", ry, rx);
	else printf("%%d%d,%%d%d", ry, rx);
}

olink(inst,s)
long inst;
char *s;
{
	int size = *s == 'l' ? 4 : 2;
	printf("\tlink.%c\t%%a%d,&", *s, inst&07);
	hexprint(size == 4 ? X_FMT : x_FMT, instfetch(size),0);
}

otrap(inst,dummy)
    long inst;
    int dummy;
{
	printf("\ttrap\t&");
	hexprint(IMDF, inst&017,0);
}

tosrccr(inst,reg)
long inst;
char *reg;
{
	printf("\tmov\t");
	printEA(inst,0);
	printf(",%s",reg);
}

fromsrccr(inst,reg)
long inst;
char *reg;
{
	printf("\tmov\t%s,",reg);
	printEA(inst,0);
}

oneop(inst,opcode)
long inst;
char *opcode;
{
	printf("\t%s\t",opcode);
	printEA(inst,0);
}

pregmask(mask)
 int mask;
{
	 short i;
	 short flag = 0;

	printf("&<");
	for (i=0; i<16; i++)
	{
		if (mask&1)
		{
			if (flag) printc(','); else flag++;
			printf("%%%c%d",(i<8)?'d':'a',i&07);
		}
		mask >>= 1;
	}
	printf(">");
}

omovem(inst,dummy)
    long inst;
    int dummy;
{
	 short i;
	 int list = 0, mask = 0100000;
	 int reglist = (int)(instfetch(2));

	if ((inst & 070) == 040)	/* predecrement */
	{
		for(i = 15; i > 0; i -= 2)
		{ list |= ((mask & reglist) >> i); mask >>= 1; }
		for(i = 1; i < 16; i += 2)
		{ list |= ((mask & reglist) << i); mask >>= 1; }
		reglist = list;
	}
	printf("\tmovm.%c\t",(inst&0100)?'l':'w');
	if (inst&02000)
	{
		printEA(inst,0);
		printc(',');
		pregmask(reglist);
	}
	else
	{
		pregmask(reglist);
		printc(',');
		printEA(inst,0);
	}
}

ochk(inst,opcode)
long inst;
char *opcode;
{
	printf("\t%s\t",opcode);
	if (*opcode == 'c' && !(inst&0277)) printEA(inst, 4);
	else printEA(inst,0);
	printf(",%%%c%d",(*opcode=='l')?'a':'d',(inst>>9)&07);
}

soneop(inst,opcode)
long inst;
char *opcode;
{
	 int size = mapsize(inst,6,0);

	if (size > 0)
	{
		printf("\t%s%s\t",opcode,suffix(size));
		printEA(inst,0);
	}
	else printf(badop);
}

oquick(inst,opcode)
long inst;
char *opcode;
{
	 int size = mapsize(inst,6,0);
	 int data = (int)((inst>>9) & 07);

	if (data == 0) data = 8;
	if (size > 0)
	{
		printf("\t%s%s\t&", opcode, suffix(size));
		/* force hex printing */
		hexprint(IMDF, data,1); 
		printc(',');
		printEA(inst,0);
	}
	else printf(badop);
}

omoveq(inst,dummy)
    long inst;
    int dummy;
{
	 int data = (int)(inst & 0377);

	if (data > 127) data |= ~0377;
	printf("\tmovq\t&"); 
	hexprint(IMDF, data,0);
	printf(",%%d%d", (inst>>9)&07);
}

oprint(inst,opcode)
    int inst;
    char *opcode;
{
	printf("\t%s",opcode);
}

ortd(inst,opcode)	/* rtd, stop */
    int inst;
    char *opcode;
{
	printf("\t%s\t&%d",opcode,instfetch(2));
}

ortm(inst)
{
	printf("\trtm\t%%%c%d", (inst & 010)?'a':'d', inst&07);
}

ocallm(inst)
{
	printf("\tcallm\t&%d,", instfetch(2) & 0377);
	printEA(inst,0);
}

ocas(inst)
{
	 int size = mapsize(inst,9,1);
	 int ext = (int) instfetch(2);

	printf("\tcas%s\t%%d%d,%%d%d,", suffix(size), ext&07, (ext>>6)&07);
	printEA(inst,0);
}

ocas2(inst)
{
	 int size = mapsize(inst,9,1);
	 int ext1 = (int) instfetch(2);
	 int ext2 = (int) instfetch(2);

	printf("\tcas2%s\t%%d%d:%%d%d,%%d%d:%%d%d,(%%%c%d):(%%%c%d)", 
		suffix(size), 
		ext1&07,
		ext2&07,
		(ext1>>6)&07,
		(ext2>>6)&07,
		(ext1&0100000)?'a':'d',
		(ext1>>12)&07,
		(ext2&0100000)?'a':'d',
		(ext2>>12)&07
	      );
}

ochk2(inst)
{
	 int ext = (int) instfetch(2);
	 int size = mapsize(inst, 9, 0);

	if (ext & 04000)
		printf("\tchk2%s\t", suffix(size));
	else
		printf("\tcmp2%s\t", suffix(size));
	printf("%%%c%d,", (ext&0100000)?'a':'d', (ext>>12)&07 );
	printEA(inst,0);
}

/* long multiply and divide */
odivl(inst, opcode)
char *opcode;
{
	 int ext = (int) instfetch(2);
	int size = (ext>>10)&1;
	int dr = ext&07;
	int dq = (ext>>12)&07;
	short flag = 0;

	printf("\t%s", opcode);
	printf("%c", (ext&004000) ? 's' : 'u' );
	if (*opcode == 'd' && !size && dr != dq) { printc('l'); flag++;}
	printf(".l\t");
	printEA(inst, 4);
	if (size || flag) printf(",%%d%d:%%d%d", dr, dq);
	else printf(",%%d%d", dq);
}

opack(inst, opcode)
char *opcode;
{
	 int ext = (int) instfetch(2);

	printf("\t%s\t", opcode);
	if (inst & 010)
		printf("-(%%a%d),-(%%a%d),&", inst&07, (inst>>9)&07 );
	else
		printf("%%d%d,%%d%d,&", inst&07, (inst>>9)&07 );
	hexprint(IMDF, ext,0);
}

otrapcc(inst)
    int inst;
{
	int  opmode = inst&07;

	if (opmode == 4)
	{
		printf("\tt%s", ccname[(int) ((inst>>8)&017)]);
		return;
	}
	if (opmode == 2)
	{
		printf("\ttp%s.w\t&", ccname[(int) ((inst>>8)&017)]);
		hexprint(IMDF, instfetch(2),0);
	}
	else if (opmode == 3)
	{
		printf("\ttp%s.l\t&", ccname[(int) ((inst>>8)&017)]);
		hexprint(X_FMT, instfetch(4),0);
	}
	else oscc(inst,0);

}

bitfld(inst)
 long inst;
{
	 int ext = (int) instfetch(2);
	int type;

	type = (inst >> 8) & 07;
	printf("\t%s\t", bfld[type] );
	if (type == 7) 		/* bfins */
		printf("%%d%d,", (ext>>12)&07 );
	printEA(inst,0);
	if (ext & 04000)	/* Do */
		printf("{%%d%d:", (ext>>6)&07 );
	else
		printf("{&%d:", (ext>>6)&037 );
	if (ext & 040)		/* Dw */
		printf("%%d%d}", ext&07 );
	else
		printf("&%d}", (ext&037)? (ext&037): 32 );
	if (type == 1 || type == 3 || type == 5) /* bfextu, bfexts, bfff0 */
		printf(",%%d%d", (ext>>12)&07 );

}

char * contreg(reg)
    int reg;
{
	switch (reg) {
	case 0:
		return "sfc";
	case 1:
		return "dfc";
	case 2:
		return "cacr";
	case 0x800:
		return "usp";
	case 0x801:
		return "vbr";
	case 0x802:
		return "caar";
	case 0x803:
		return "msp";
	case 0x804:
		return "isp";
	default:
		return "???";
	}
}

/***************     MC 68881 dis-assembler     ****************/

char *fop[] = { "mov", "int", "sinh", "intrz", "sqrt",		/* 00-04 */
		"???", "lognp1", "???", "etoxm1", "tanh", 	/* 05-09 */
		"atan", "???", "asin", "atanh", "sin", 		/* 0A-0E */
		"tan", "etox", "twotox", "tentox", "???", 	/* 0F-13 */
		"logn", "log10", "log2", "???", "abs",		/* 14-18 */
		"cosh", "neg", "???", "acos", "cos",		/* 19-1D */
		"getexp", "getman", "div", "mod", "add",	/* 1E-22 */
		"mul", "sgldiv", "rem", "scale", "sglmul",	/* 23-27 */
		"sub" };

char *srcfmt[] = { ".l", ".s", ".x", ".p", ".w", ".d", ".b", ".p" };

int srcsize[] = { 4, 4, 12, 12, 2, 8, 1, 12 };

#define SIZEOFCPRED 32			/* how many strings are in cpred */

char *cpred[] = { "f", "eq", "ogt", "oge", "olt", "ole",
		  "ogl", "or", "un", "ueq", "ugt", "uge",
		  "ult", "ule", "neq", "t",
                  "sf", "seq", "gt", "ge", "lt", "le",
		  "gl", "gle", "ngle", "ngl", "nle", "nlt",
		  "nge", "ngt", "sneq", "st" };

o881(inst)
 long inst;
{
	int type = (inst>>6)&07;

	switch (type)
	{
	  /* general */
	  case 0:
		fgen(inst);
		break;

	  /* fscc, fdbcc, and ftraps */
	  case 1:
		ftype1(inst);
		break;

	  /* branch on condition */
	  case 2:
		fbranch(inst,2);
		break;

	  /* long branch on condition */
	  case 3:
		fbranch(inst,4);
		break;

	  /* save state */
	  case 4:
		oneop(inst, "fsave");
		break;

	  /* restore state */
	  case 5:
		oneop(inst, "frestore");
		break;

	  default:	printf("???\t(68881)");
	}

}

/* general type co-processor instruction format */
fgen(inst)
 long inst;
{
	long  cword = instfetch(2);
	int  opclas = (cword>>13)&07;
	int rx = (cword>>10)&07;
	int ry = (cword>>7)&07;
	int ext = cword&0177;
	 short flag;

	switch (opclas)
	{
	  /* fp data reg to fp data reg */
	  case 0:
		printf("\tf");
		if (ext <= 0x28) printf("%s.x", fop[ext]);
		else if (ext == 0x38) printf("cmp.x");
		else if (ext == 0x3A) { printf("test.x"); ry = rx; }
		else if (ext >= 0x30 && ext <= 0x37) 
		     { 
		     printf("sincos.x\t%%fp%d,%%fp%d:%%fp%d", rx, ext&07, ry); 
		     break;
		     }
		else printf("???"); 
		if (rx == ry) printf("\t%%fp%d", rx);
		else printf("\t%%fp%d,%%fp%d", rx, ry);
		break;

	  case 2:
		if(rx != 7)
		{
	  		/* external operand to fp data reg */
			printf("\tf");
			if (ext <= 0x28) printf("%s", fop[ext]);
			else if (ext == 0x38) printf("cmp");
			else if (ext == 0x3A) printf("test");
			else if (ext >= 0x30 && ext <= 0x37) printf("sincos");
			else printf("???");
			printf("%s\t", srcfmt[rx]);
			printEA(inst, srcsize[rx]);
			if (ext >= 0x30 && ext <= 0x37) 
			{ 
			  printf(",%%fp%d:%%fp%d", ext&07, ry); 
			  break;
			}
			if(ext != 0x3A) printf(",%%fp%d", ry);
		}
		else
		{
			/* move constant to fp data reg */
			printf("\tfmovcr\t&");
			hexprint(x_FMT, ext,0);
			printf(",%%fp%d", ry);
		};
		break;

	  case 3:
	  	/* move fp data reg to ext dest */
		printf("\tfmov%s\t%%fp%d,", srcfmt[rx], ry);
		printEA(inst,0);
		if (rx == 3) {
			printf("{&");
			hexprint(x_FMT, ext,0);
			printf("}");
		};
		if (rx == 7) printf("{%%d%d}", (ext>>4)&07);
		break;

	  case 4:
	  case 5:
		/* move system registers */
		if (rx == 1 || rx == 2 || rx == 4)
			printf("\tfmov.l\t");
		else
			printf("\tfmovm.l\t");
		if (opclas==4) { printEA(inst,4); printc(','); }
		flag = 0;
		if (rx&4) { printf("%%fpcontrol"); flag++; }
		if (rx&2)
		{
		  if (flag) printc('/'); else flag++;
		  printf("%%fpstatus");
		}
		if (rx&1)
		{
		  if (flag) printc('/'); 
		  printf("%%fpiaddr");
		}
		if (opclas==5) { printc(','); printEA(inst,4); }
		break;

	  case 6:
	  case 7:
		/* move multiple fp data regs */
		printf("\tfmovm.x\t");
		if (opclas==6) { printEA(inst,0); printc(','); }
		if (rx & 2) printf("%%d%d", (cword>>4)&07 );
		else
		{
			 int mask = cword & 0377;
			 int i;
			flag = 0;
			for (i = 0; i < 8; i++)
			{
			  if (mask&1)
			  {
				if (flag) printc('/'); else flag++;
				printf("%%fp%d", (rx&4) ? (7-i):i);
			  }
			  mask >>= 1;
			}
		}
		if (opclas==7) { printc(','); printEA(inst,0); }
		break;
	  
	  default:	printf("???\t(68881)");

	}
}

/* type 1 instructions */
ftype1(inst)
{
	long disp = 0;
	long cword = instfetch(2);

	if (((inst>>3)&07) == 1)
	{
		/* decrement & branch on condition */
		printf("\tfdb%s\t", ((cword&077) < SIZEOFCPRED) ?
				     cpred[cword&077] : "---" );
		printf("%%d%d,", inst&07);
		disp = instfetch(2);
		if (disp > 0x7FFF) disp |= 0xFFFF0000;
		if (isReloc)
		    psymname(0);
		else
		    pname(disp + dot - 2);
		return;
	}
	if (((inst>>3)&07) == 7)
	{
		/* trap on condition */
		if((inst&07) == 4)
		{
		  printf("\tft%s",((cword&077) < SIZEOFCPRED) ?
			 cpred[cword&077] : "---" );
		  return;
		}
		/* trap on condition with word parameter */
		if((inst&07) == 2)
		{
		  printf("\tftp%s.w\t&", ((cword&077) < SIZEOFCPRED) ?
			 cpred[cword&077] : "---");
		  hexprint(x_FMT,instfetch(2),0);
		  return;
		}
		/* trap on condition with long parameter */
		if((inst&07) == 3)
		{
		  printf("\tftp%s.l\t&",((cword&077) < SIZEOFCPRED) ?
			 cpred[cword&077] : "---" );
		  hexprint(X_FMT,instfetch(4),0);
		  return;
		}
	}
	/* set on condition */
	if ((cword&077) < SIZEOFCPRED )
	    printf("\tfs%s\t", ((cword&077) < SIZEOFCPRED) ?
		   cpred[cword&077] : "---");
	printEA(inst,0);
}

/* fbcc instructions */
fbranch(inst, size)
{
	long loc;
	int disp = 0;

	loc = dot;
	disp = instfetch(size);
	if ((inst&077) == 0 && disp == 0) {printf("\tfnop"); return; }
	printf("\tfb%s.%c\t", (((inst&077) < SIZEOFCPRED) ?
			       cpred[inst&077] : "---"),
	       (size == 2)? 'w':'l' );
	if (size == 2 && disp > 0x7FFF) disp |= 0xFFFF0000;
	if (isReloc)
	    psymname(0);
	else
	    pname(disp+loc);
}

hexprint(fmt, value,printHex)
    int fmt;
    int value;
    int printHex;
{
	short count;
	char buffer[10];
	char c,*cptr;

	if (isReloc && !printHex)
	    psymname(value);
	else
	 {
	     if (fmt == h_FMT && value < 0)
	      {
		  printf("-");
		  value = -value;
	      }
	     printf("0x");

	     count = fmt == x_FMT ? 4 : 8;
	     cptr = &buffer[9];
	     *cptr = 0;
	     while (count-- > 0)
	      {
		  c = (value & 0xF) + '0';
		  if (c > '9') c += 'A' - '9' - 1;
		  *--cptr = c;
		  value >>= 4;
	      }
	     while (*cptr == '0')
		 cptr++;
	     if (!*cptr) cptr--;
	     printf("%s",cptr);
	 }
}
/*
 * Local Variables:
 * compile-command: "cc -go das das.c"
 * End:
 */
