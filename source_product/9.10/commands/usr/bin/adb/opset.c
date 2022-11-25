/* @(#) $Revision: 66.2 $ */   
/****************************************************************************

	DEBUGGER - 68020 instruction printout

****************************************************************************/
#include "defs.h"

int	dotinc;
POS	space;
extern	long	var[];
extern	int	fpa_addr;
extern	long	dot;
extern	MAP	txtmap;

char *badop = "\t???";
char *IMDF;				/* immediate data format */

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
int odivl(),o881(),tosrccr(),fromsrccr(),omove16();

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
  0xFFF8, 0x4840, oreg, "\tswap\t%%d%D",
  0xFFF8, 0x4848, oreg, "\tbkpt\t&%d",
  0xFFC0, 0x4840, oneop, "pea",
  0xFFF8, 0x4880, oreg, "\text.w\t%%d%D",
  0xFFF8, 0x48C0, oreg, "\text.l\t%%d%D",
  0xFFF8, 0x49C0, oreg, "\textb.l\t%%d%D",
  0xFB80, 0x4880, omovem, 0,
  0xFFFF, 0x4AFC, oprint, "illegal",
  0xFFC0, 0x4AC0, oneop, "tas",
  0xFF00, 0x4A00, soneop, "tst",
  0xFFC0, 0x4C00, odivl, "mul",
  0xFFC0, 0x4C40, odivl, "div",
  0xFFF0, 0x4E40, otrap, 0,
  0xFFF8, 0x4E50, olink, "w",
  0xFFF8, 0x4E58, oreg, "\tunlk\t%%a%D",
  0xFFF8, 0x4E60, oreg, "\tmov.l\t%%a%D,%%usp",
  0xFFF8, 0x4E68, oreg, "\tmov.l\t%%usp,%%a%D",
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
  0xFF00, 0xF600, omove16, "move16",	/* SMT */
  0xF000, 0xF000, o881, 0,		/* mc68881 instruction */
  0, 0, 0, 0
};

printins(f, idsp, inst)
{
	register struct opdesc *p;
	register int (*fun)();

	space = idsp; dotinc = 2;
	if (f) IMDF = "&%d"; else IMDF = "&%x";
	for (p = opdecode; p->mask; p++)
		if ((inst & p->mask) == p->match) break;
	if (p->mask != 0) (*p->opfun)(inst, p->farg);
	else printf(badop);
}

long
instfetch(size)
int size;
{
	register long l1, l2;

	if (size==4)
	{
		l1 = leng(chkget(inkdot(dotinc), space));
		l1 <<= 16;
		l2 = leng(chkget(inkdot(dotinc += 2), space));
		l1 = (l1 | l2);
	}
	else
	{
		l1 = (long)(chkget(inkdot(dotinc), space)) & 0xFFFF;
		if (l1 & 0x8000) l1 |= 0xFFFF0000;
	}
	dotinc += 2;
	return(l1);
}

printea(mode,reg,size)
long mode, reg;
int size;
{
	long index;
	char imf[6];

	switch ((int)(mode)) {
	  case 0:	printf("%%d%D",reg);   /* DATA REG DIRECT MODE */
			break;

	  case 1:	printf("%%a%D",reg);   /* ADDRESS REG DIRECT */
			break;

	  case 2:	printf("(%%a%D)",reg);  /* ADDR REG INDIRECT */
			break;

	  case 3:	printf("(%%a%D)+",reg); /* ADDR IND W/ POSTINC */
			break;

	  case 4:	printf("-(%%a%D)",reg);  /* ADDR IND W/ PREDEC */
			break;

	  case 5:	printf("%h(%%a%D)",instfetch(2),reg); /* W/ DISPL */
			break;

	  case 6:	/* ADDR REG & MEM IND w/ DISPl */
			mode6(reg);
			break;

	  /* the special addressing modes */
	  case 7:	switch ((int)(reg))
			{
			  /* absolute short address */
			  case 0:	index = instfetch(2);
					printf("%x.w",index);
					break;

			  /* absolute long address */
			  case 1:	index = instfetch(4);
					psymoff(index, ISYM, "");
					break;

			  /* pc with displacement */
			  case 2:	printf("%h(%%pc)",instfetch(2));
					break;

			  /* pc with index */
			  case 3:	mode6(-1);
					break;

			  /* immediate data */
			  case 4:	
			         	index = instfetch(size>=4?4:2);
					strcpy(imf,IMDF);
					if (size >= 4) imf[2] -= 'a'-'A';
					printf(imf, index);
					while (size > 4)
					{
			         	  index = instfetch(4);
					  printc(' ');
					  printf(imf, index);
					  size -= 4;
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
	long ext1;

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

	disp = (char)(ext1&0377);
	scale = mapscale(ext1);
	if (reg == -1)
		printf("%h(%%pc",disp);
	else
		printf("%h(%%a%d",disp,reg);
	printf(",%%%c%D.%c",
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
	long i_is, disp;

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
		if (disp) printf(",%h", instfetch(disp) );
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
		if (disp) printf(",%h", instfetch(disp) );
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

	bs = (ext1>>7)&01;
	is = (ext1>>6)&01;
	bd_size = (ext1>>4)&03;
	bd_size = ((bd_size==1) ? 0 : (bd_size==2) ? 2 : (bd_size==3) ? 4 :  -1);

	if (bd_size) { printf("%h",instfetch(bd_size)); flag++; }
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
	   printf("%%%c%D.%c",
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
register long ext;
{
	ext >>= 9;
	ext &= 03;
	return((ext==0) ? 1 : (ext==1) ? 2 : (ext==2) ? 4 : (ext==3) ? 8 : -1);
}

mapsize(inst,start,byte)
register long inst;
int start, byte;
{
	inst >>= start;
	inst &= 03;
	if (byte) return((inst==1) ? 1 : (inst==2) ? 2 : (inst==3) ? 4 : -1);
	else return((inst==0) ? 1 : (inst==1) ? 2 : (inst==2) ? 4 : -1);
}

char *suffix(size)
register int size;
{
	return((size==1) ? ".b" : (size==2) ? ".w" : (size==4) ? ".l" : ".?");
}

omove(inst, s)
register long inst;
char *s;

{
	int size;

	if (!Dragon_move(inst,s,0)) {
		printf("\tmov.%c\t",*s);
		size = ((*s == 'b') ? 1 : (*s == 'w') ? 2 : 4);
		printea((inst>>3)&07,inst&07,size);
		printc(',');
		printea((inst>>6)&07,(inst>>9)&07,size);
	};
}

omovep(inst,s)
register long inst;
char *s;
{
	printf("\tmovp.%s\t",s);
	if (inst & 0x0080)
	{	printf("%%d%D,",inst>>9);
		printea(5,inst & 0x7,0);
	}
	else
	{	printea(5,inst & 0x7,0);
		printf(",%%d%D",inst>>9);
	}
}

omovec(inst)
register long inst;
{
	register long r;

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
register long inst;
{
	register long r;
	register long t;
	register long c;

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
{
	register long disp = inst & 0377;
	char *s; 

	s = ".b";
	if (disp == 0) { s = ".w"; disp = instfetch(2); }
	else if (disp == 0xff) { s = ".l"; disp = instfetch(4); }
	else if (disp > 127) disp |= ~0377;
	printf("\tb%s%s\t",bname[(int)((inst>>8)&017)],s);
	psymoff(disp+inkdot(2), ISYM, "");
}

odbcc(inst,dummy)
long inst;
{
	printf("\tdb%s\t",ccname[(int)((inst>>8)&017)]);
	printea(0,inst&07,2);
	printc(',');
	psymoff(instfetch(2)+inkdot(2), ISYM, "");
}

oscc(inst,dummy)
long inst;
{
	printf("\ts%s\t",ccname[(int)((inst>>8)&017)]);
	printea((inst>>3)&07,inst&07,1);
}

biti(inst, dummy)
register long inst;
{
	printf("\t%s\t", bit[(int)((inst>>6)&03)]);
	if (inst&0x0100) printf("%%d%D,", inst>>9);
	else { printf(IMDF, instfetch(2)); printc(','); }
	printEA(inst,2);
}

opmode(inst,opcode)
long inst;
char *opcode;
{
	register int opmode = (int)((inst>>6) & 07);
	register int reg = (int)((inst>>9) & 07);
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
register long inst;
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
			printf(IMDF, (rx ? rx : 8));
			printf(",%%d%d", ry);
		}
	}
}		

oimmed(inst,opcode) 
long inst;
register char *opcode;
{
	register int size = mapsize(inst,6,0);
	long constant;
	char imf[6];

	if (size > 0)
	{
		constant = instfetch(size==4?4:2);
		printf("\t%s%s\t", opcode, suffix(size));
		strcpy(imf,IMDF);
		if (size == 4) imf[2] -= 'a'-'A';
		if (*opcode == 'c')
		{	/* cmpi */
			printEA(inst,size); printc(',');
			printf(imf, constant);
			return;
		}
		printf(imf, constant); printc(',');
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
register long	inst;
char	*opcode;
{
	register int size = mapsize(inst,6,0);
	int ry = (inst&07), rx = ((inst>>9)&07);
	char *c;

	c = ((inst & 0x1000) ? suffix(size) : " ");
	printf("\t%s%s\t", opcode, c);
	if (*opcode == 'e')
	{
		if (inst & 0x0080) printf("%%d%D,%%a%D", rx, ry);
		else if (inst & 0x0008) printf("%%a%D,%%a%D", rx, ry);
		else printf("%%d%D,%%d%D", rx, ry);
	}
	else if ((inst & 0xF000) == 0xB000) printf("(%%a%D)+,(%%a%D)+", rx, ry);
	else if (inst & 0x8) printf("-(%%a%D),-(%%a%D)", ry, rx);
	else printf("%%d%D,%%d%D", ry, rx);
}

olink(inst,s)
long inst;
char *s;
{
	char imf[6];
	int size = (*s == 'l')? 4:2;
	printf("\tlink.%c\t%%a%D,", *s, inst&07);
	strcpy(imf, IMDF);
	if (size == 4) imf[2] -= 'a'-'A';
	printf(imf, instfetch(size));
}

otrap(inst,dummy)
long inst;
{
	printf("\ttrap\t");
	printf(IMDF, inst&017);
}

tosrccr(inst,reg)
long inst;
char *reg;
{
	printf("\tmov\t");
	printEA(inst,2);
	printf(",%s",reg);
}

fromsrccr(inst,reg)
long inst;
char *reg;
{
	printf("\tmov\t%s,",reg);
	printEA(inst,2);
}

oneop(inst,opcode)
long inst;
char *opcode;
{
	printf("\t%s\t",opcode);
	printEA(inst,0);
}


pregmask(mask)
register int mask;
{
        register short i, j, flag = 0;
	char c = 'd';

	if (mask)
	    for (;;) {
	        j = 0;
	        do {
	            for (i = j; i < 8; i++) if (mask & (1 << i)) break;
		    if (i == 8) break;
	            for (j = i + 1; j < 8; j++) if (!(mask & (1 << j))) break;
		    if (flag++) printf("/");
		    if (j == (i + 1)) printf("%%%c%d", c, i);
		    else printf("%%%c%d-%%%c%d", c, i, c, j - 1);
                } while (j < 8);
		if (c == 'a') return;
		c = 'a';
		mask >>= 8;
	    }
	else printf("&0");
}

omovem(inst,dummy)
long inst;
{
	register short i;
	register int list = 0, mask = 0100000;
	register int reglist = (int)(instfetch(2));

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
	if (*opcode == 'c' && !(inst&0200)) printEA(inst, 4);
	else printEA(inst,2);
	printf(",%%%c%D",(*opcode=='l')?'a':'d',(inst>>9)&07);
}

soneop(inst,opcode)
long inst;
char *opcode;
{
	register int size;

	if (!Dragon_tst(inst)) {
		size = mapsize(inst,6,0);
		if (size > 0)
		{
			printf("\t%s%s\t",opcode,suffix(size));
			printEA(inst,0);
		}
		else printf(badop);
	};
}

oquick(inst,opcode)
long inst;
char *opcode;
{
	register int size, data;
	int Dragon_subq();

	if (!Dragon_subq(inst)) {
		size = mapsize(inst,6,0);
		data = (int)((inst>>9) & 07);
		if (data == 0) data = 8;
		if (size > 0)
		{
			printf("\t%s%s\t", opcode, suffix(size));
			printf(IMDF, data); printc(',');
			printEA(inst,0);
		}
		else printf(badop);
	};
}

omoveq(inst,dummy)
long inst;
{
	register int data = (int)(inst & 0377);

	if (data > 127 && IMDF[2]=='d') data |= ~0377;
	printf("\tmovq\t"); printf(IMDF, data);
	printf(",%%d%D", (inst>>9)&07);
}

oprint(inst,opcode)
long inst;
char *opcode;
{
	printf("\t%s",opcode);
}

ortd(inst,opcode)	/* rtd, stop */
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
	register int size = mapsize(inst,9,1);
	register int ext = (int) instfetch(2);

	printf("\tcas%s\t%%d%d,%%d%d,", suffix(size), ext&07, (ext>>6)&07);
	printEA(inst,0);
}

ocas2(inst)
{
	register int size = mapsize(inst,9,1);
	register int ext1 = (int) instfetch(2);
	register int ext2 = (int) instfetch(2);

	printf("\tcas2%s\t%%d%d:%%d%d,%%d%d:%%d%d,%%%c%d:%%%c%d", 
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
	register int ext = instfetch(2);
	register int size = mapsize(inst, 9, 0);

	if (ext & 04000)
	{
		printf("\tchk2%s\t", suffix(size));
		printEA(inst,0);
		printf(",%%%c%D", (ext&0100000)?'a':'d', (ext>>12)&07 );
	}
	else
	{
		printf("\tcmp2%s\t", suffix(size));
		printf("%%%c%D,", (ext&0100000)?'a':'d', (ext>>12)&07 );
		printEA(inst,0);
	}
}

/* long multiply and divide */
odivl(inst, opcode)
char *opcode;
{
	register int ext = instfetch(2);
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
	register int ext = instfetch(2);

	printf("\t%s\t", opcode);
	if (inst & 010)
		printf("-(%%a%d),-(%%a%d),", inst&07, (inst>>9)&07 );
	else
		printf("%%d%d,%%d%d,", inst&07, (inst>>9)&07 );
	printf(IMDF, ext);
}

otrapcc(inst)
{
	register opmode = inst&07;

	if (opmode == 4)
	{
		printf("\tt%s", ccname[(int) ((inst>>8)&017)]);
		return;
	}
	if (opmode == 2)
	{
		printf("\ttp%s.w\t", ccname[(int) ((inst>>8)&017)]);
		printf(IMDF, instfetch(2));
	}
	else if (opmode == 3)
	{
		char imf[6];

		printf("\ttp%s.l\t", ccname[(int) ((inst>>8)&017)]);
		strcpy(imf, IMDF);
		imf[2] -= 'a'-'A';
		printf(imf, instfetch(4));
	}
	else oscc(inst, 0);

}

bitfld(inst)
register long inst;
{
	register int ext = instfetch(2);
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


omove16(inst,pneum)
register long inst;
char *pneum;
{	register int ext;
	printf("\t%s\t",pneum);
	if (inst&0x20)				/* reg. to reg. */
	{	printea(3,inst&0x7);
		printf(",");
		ext = instfetch(2);
		printea(3,(ext>>12)&0x7);
	}
	else if (inst&0x8)			/* addr to reg. */
	{	printea(7,1);
		printf(",");
		printea((inst&0x10)?2:3,inst&0x7);
	}
	else					/* reg. to addr */
	{	printea((inst&0x10)?2:3,inst&0x7);
		printf(",");
		printea(7,1);
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

char *cpred[] = { "f", "eq", "ogt", "oge", "olt", "ole",
		  "ogl", "or", "un", "ueq", "ugt", "uge",
		  "ult", "ule", "neq", "t",
                  "sf", "seq", "gt", "ge", "lt", "le",
		  "gl", "gle", "ngle", "ngl", "nle", "nlt",
		  "nge", "ngt", "sneq", "st" };

o881(inst)
register long inst;
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

	  default:	printf("\t???\t(68881)");
	}

}

/* general type co-processor instruction format */
fgen(inst)
register long inst;
{
	register cword = instfetch(2);
	register opclas = (cword>>13)&07;
	int rx = (cword>>10)&07;
	int ry = (cword>>7)&07;
	int ext = cword&0177;
	register short flag;

	switch (opclas)
	{
	  /* fp data reg to fp data reg */
	  case 0:
		printf("\tf");
		if (ext <= 0x28) printf("%s.x", fop[ext]);
		else if (ext == 0x38) 
			{
			printf("cmp.x\t%%fp%d,%%fp%d", ry, rx);
			break;
			}
		else if (ext == 0x3A) { printf("test.x"); ry = rx; }
		else if (ext >= 0x30 && ext <= 0x37) 
		     { 
		     printf("sincos.x\t%%fp%d,%%fp%d:%%fp%d", rx, ext&07, ry); 
		     break;
		     }
		else if ((ext & 0x40) == 0x40)	/* 68040 - SMT */
		     { if (ext & 0x4) printf("d");
		       else printf("s");
		       if (ext == 0x41 || ext == 0x45) printf("%s.x", fop[4]);
		       else printf("%s.x", fop[ext & 0x3B]);
		     }				/* end 68040 */
		else printf("???"); 
		/* the tests on ext in the following were added to exclude  */
		/* opcodes for which the assembler requires two arguments   */
		/* from the general merging of identical registers to one   */
		/* argument.  It will need changes if additional floating   */
		/* opcodes besides ftest come into existance with bit 5 set */
		/* but for which merging is appropriate.  SmT - 01/02/90    */
		if ( (rx == ry) 
		     &&
		     !( (ext == 0) || (ext == 0x40) || (ext == 0x44) ||
			( ((ext & 0x20) == 0x20) && (ext != 0x3A))
		      )
		   ) printf("\t%%fp%d", rx);
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
			else if ((ext & 0x40) == 0x40)	/* 68040 - SMT */
		             { if (ext & 0x4) printf("d");
		       	       else printf("s");
		               if (ext == 0x41 || ext == 0x45)
				  printf("%s", fop[4]);
		               else printf("%s", fop[ext & 0x3B]);
		     	     }				/* end 68040 */
			else printf("???");
			printf("%s\t", srcfmt[rx]);
			if(ext == 0x38) printf("%%fp%d,", ry);
			if ((inst & 077) == 074) printIMM(rx);
			else printEA(inst, srcsize[rx]);
			if (ext >= 0x30 && ext <= 0x37) 
			{ 
			  printf(",%%fp%d:%%fp%d", ext&07, ry); 
			  break;
			}
			if(ext != 0x3A && ext != 0x38) printf(",%%fp%d", ry);
		}
		else
		{
			/* move constant to fp data reg */
			printf("\tfmovcr\t&%x,%%fp%d", ext, ry);
		}
		break;

	  case 3:
	  	/* move fp data reg to ext dest */
		printf("\tfmov%s\t%%fp%d,", srcfmt[rx], ry);
		printEA(inst,0);
		if (rx == 3) 
		{
			if (ext & 0x040) ext |= 0xffffff80 ;
			printf("{&%d}", ext);
		}
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
		if (rx&4) { printf("%%fpcr"); flag++; }
		if (rx&2)
		{
		  if (flag) printc('/'); else flag++;
		  printf("%%fpsr");
		}
		if (rx&1)
		{
		  if (flag) printc('/'); 
		  printf("%%fpiar");
		}
		if (opclas==5) { printc(','); printEA(inst,4); }
		break;

	  case 6:
	  case 7:
		/* move multiple fp data regs */
		printf("\tfmovm.x\t");
		if (opclas==6) { printEA(inst,0); printf(","); }
		if (rx & 2) printf("%%d%d", (cword>>4)&07 );
		else
		{
		    register int mask, i, j;

		    if (rx & 4) {
		        i = 1;
		        mask = 0;
		        do {
			    mask += mask;
			    if (i & cword) mask++;
		        } while ((i += i) <= 0x80);
		    } else mask = cword & 0377;

		    if (mask) {
			flag = 0;
			j = 0;
			do {
			    for (i = j; i < 8; i++) 
				if (mask & (1 << i)) break;
			    if (i == 8) break;
			    for (j = i + 1; j < 8; j++) 
				if (!(mask & (1 << j))) break;
			    if (flag++) printf("/");
			    if (j == (i + 1))
				printf("%%fp%d", i);
			    else
				printf("%%fp%d-%%fp%d", i, j - 1);
			} while (j < 8);
		    } else printf("&0");
		};
		if (opclas==7) { printf(","); printEA(inst,0); }
		break;
	  
	  default:	printf("\t???\t(68881)");

	}
}

/* type 1 instructions */
ftype1(inst)
{
	register cword = instfetch(2);

	if ((cword&077) > 31 ) {printf("\t???\t(68881)"); return; }

	if (((inst>>3)&07) == 1)
	{
		/* decrement & branch on condition */
		printf("\tfdb%s\t", cpred[cword&077]);
		printf("%%d%d,", inst&07 );
		psymoff(instfetch(2)+inkdot(4), ISYM, "");
		return;
	}
	if (((inst>>3)&07) == 7)
	{
		/* trap on condition */
		if((inst&07) == 4)
		{
		  printf("\tft%s", cpred[cword&077]);
		  return;
		}
		/* trap on condition with word parameter */
		if((inst&07) == 2)
		{
		  printf("\tftp%s.w\t", cpred[cword&077]);
		  printf("%x", instfetch(2));
		  return;
		}
		/* trap on condition with long parameter */
		if((inst&07) == 3)
		{
		  printf("\tftp%s.l\t", cpred[cword&077]);
		  printf("%x", instfetch(4));
		  return;
		}
	}
	/* set on condition */
	printf("\tfs%s\t", cpred[cword&077]);
	printEA(inst,0);
}

/* fbcc instructions */
fbranch(inst, size)
{
	int disp = instfetch(size);

	if ((inst&077) == 0 && disp == 0) {printf("\tfnop"); return; }
	if ((inst&077) > 31 ) {printf("\t???\t(68881)"); return; }
	printf("\tfb%s.%c\t", cpred[inst&077], (size == 2)? 'w':'l' );
	psymoff(disp+inkdot(2), ISYM, "");
}

/***************     Dragon disassembler     ****************/

#define DABORT { dotinc = dotincsave; return 0; }

static char *D_branch[32] = { "sne", "nlt", "nle", "ngt", "nge", "ngl", 
	"ngle", "sf", "ne", "uge", "ugt", "ule", "ult", "ueq", "un", "f", 
	"seq", "lt", "le", "gt", "ge", "gl", "gle", "st", "eq", "olt", 
	"ole", "ogt", "oge", "ogl", "or", "t" 
};

static char *D_opcode[37] = {
	"2add", "2sub", "2mul", "2div", "2rsub", "2rdiv", "add", "sub",
	"mul", "div", "rdiv", "rsub", "2cmp", "abs", "acos", "asin",
	"atan", "cmp", "cos", "cvb", "cvd", "cvl", "cvs", "cvw", "etox",
	"intrz", "log10", "log2", "logn", "mov", "neg", "ov", "movcr",
	"sin", "sqrt", "tan", "test"
};

static unsigned short fastop[32] = {
	 0, 29, 36, 29,  6,  7,  8,  9,
	 0,  0,  0,  0,  0,  0,  0,  0,
	 0,  1,  2,  3,  4,  5,  0,  0,
	17, 13, 22, 20, 20, 21, 11,  0
};

static unsigned short slowop[32] = {
	 0,  0, 25, 30, 10, 34,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,
	 5,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,
};

/*
	Dragon_tst : A Dragon branch or fpwait has possibly been
	found. Disassemble if it is a Dragon instruction and return
	1.  Otherwise, return 0 and leave everything in the same
	state as at invocation.

	inst - instruction
*/

static int Dragon_tst(inst)
long inst;
{
	short mode, branch;
	long offset, disp, dotincsave = dotinc;
	unsigned long count, length;
	unsigned short wait;
	char suffix;

	dotinc -= 2;
	if (!Dragon_count(&count,&length,&wait)) DABORT;
	dotinc = dotincsave;

	mode = (inst >> 3) & 7;

	if (mode == 2) offset = 0;
	else if (mode == 5) offset = instfetch(2);
	else offset = instfetch(4);

	if (offset) {
	    offset >>= 4;
	    branch = instfetch(2);
	    if (branch & 0x100) offset += 16;  /* bmi or bpl? */

	    /* Determine size of branch */
	    branch &= 0xFF;
	    if (branch) {
	        if (branch == 0xFF) {
		    disp = instfetch(4);
		    suffix = 'l';
		} else {
		    disp = branch;
		    if (disp & 0x80) disp |= 0xFFFFFF00;
		    suffix = 'b';
	        };
	    } else {
	        disp = instfetch(2);
	        suffix = 'w';
	    };

	    /* Print Dragon instruction */
	    printf("\tfpb%s.%c\t",D_branch[offset],suffix);

	    /* Print branch target */
	    psymoff(disp+inkdot(6), ISYM, "");

	} else printf("\tfpwait");

	dotinc = dotincsave + length - 2;
	return 1;
}

/*
	Dragon_move : A general Dragon instruction has possibly been
	found. Disassemble if it is a Dragon instruction and return
	1.  Otherwise, return 0 and leave everything in the same
	state as at invocation.

	inst - instruction
	s - suffix
	pred - if this instruction has been preceded by a "subq.w &8,%an",
		this is the instruction count comprising the Dragon in-
		struction (minus one for the "subq"); otherwise, it is 0
*/

static int Dragon_move(inst,s,pred)
long inst, pred;
char *s;
{
	char *gcvt(), buffer[30], suffix;
	long offset, dotincsave = dotinc;
	short Dreg1, Dreg2, dstflg, dstmode, dstreg, eflag, extend[2][6];
	short next[2][6], nextinst, opcode, opindex, srcmode, srcreg, swap;
	union { double d; float f; unsigned long l[2]; } number;
	unsigned long count, length;
	unsigned short ustemp, wait;

	if (pred) count = pred;
	else {
	   dotinc -= 2;
	   if (!Dragon_count(&count,&length,&wait)) DABORT;
	   dotinc = dotincsave;
	};

	/* Isolate instruction fields */
	srcmode = (inst >> 3) & 7;
	dstmode = (inst >> 6) & 7;
	srcreg = inst & 7;
	dstreg = (inst >> 9) & 7;

	/* Get information for addressing modes */
	(void) D_modes(inst,s,extend);
	
	/* Moving to or from Dragon card? */
	if ((dstmode == 5) && (dstreg == var[VARR])) dstflg = 1;
	else if ((inst & 0x0FC0) == 0x03C0) {
	    ustemp = extend[1][2];
	    offset = ((extend[1][1] << 16) | ustemp) - fpa_addr;
	    dstflg = (offset >= 0xffff8000) && (offset <= 0x00018000);
	} else dstflg = 0;

	if (dstflg) {

	    /* Get Dragon card offset */
	    if (dstmode == 5) offset = extend[1][1]; 

	    opcode = (offset >> 12) & 0x1F;
	    Dreg1 = (offset >> 8) & 0xF;
	    Dreg2 = (offset >> 4) & 0xF;

	    if ((offset & 3) == 1) { /* Register-to-register instruction */

		/* Determine opcode mnemonic */
		opindex = (offset & 8) ? ((opcode == 0x1C) && (offset & 4)
		    ? 22 : fastop[opcode]) : slowop[opcode];

		/* Determine mnemonic suffix */
		if ((offset & 0x1E008) == 0x1A008) suffix = 'l';
		else suffix = offset & 4 ? 'd' : 's';

		/* Swap operands if it's a "fpcmp" */
		if ((offset & 0x1F008) == 0x18008)
		    { swap = Dreg1; Dreg1 = Dreg2; Dreg2 = swap; }

		/* Print mnemonic and first operand */
		printf("\tfp%s.%c\t%%fpa%d",D_opcode[opindex],suffix,Dreg1);

		/* Print second operand if ... */
		if ((Dreg1 != Dreg2) || ((1 << opcode) & (offset & 8 ?
		    0x411F00F8 : 0x00010010))) printf(",%%fpa%d",Dreg2);

		/* If this is a slow instruction, check for wait loop */
		if ((count == 1) && !(offset & 8) && 
		    ((1 << opcode) & 0x00010030)) printf(",.nowait");

	    } else { /* move from external to Dragon */

		/* Check for special registers */
		if ((offset == 4) || (offset == 8)) {
		    printf("\tfpmov.l\t");
		    dotinc = dotincsave;
		    printea(srcmode, srcreg, 4);
		    printf(",%%fpa%cr",offset == 4 ? 's' : 'c');
		} else {
		    /* Determine mnemonic suffix */
		    if (pred) suffix = 'd';
		    else if ((offset & 0x1E008) == 0x1A008) suffix = 'l';
		    else suffix = count & 1 ? 's' : 'd';

		    if (suffix == 'd') {
			nextinst = instfetch(2);
			D_modes(nextinst,"l",next);
			if ((nextinst & 0x01C0) == 0x0140) 
			    offset = next[1][1];
			else {
			    ustemp = next[1][2];
			    offset = ((next[1][1] << 16) | ustemp) - fpa_addr;
			};
			Dreg1 = (offset >> 8) & 0xF;
			Dreg2 = (offset >> 4) & 0xF;
			opcode = (offset >> 12) & 0x1F;
		    };

		    if ((offset & 0x1F008) == 0x01008) { /* fpmov */
			Dreg2 = Dreg1;
			opindex = 31;
		    } else if ((offset & 0x1F008) == 0x18008) { /* fpm2cmp */
			{ swap = Dreg1; Dreg1 = Dreg2; Dreg2 = swap; }
			opindex = 12;
		    } else opindex = (offset & 8) ? ((opcode == 0x1C) && 
		        (offset & 4) ? 22 : fastop[opcode]) : slowop[opcode];

		    dotinc = dotincsave;
		    printf("\tfpm%s.%c\t",D_opcode[opindex],suffix);

		    /* Special handling for immediates */
		    if ((srcmode == 7) && (srcreg == 4)) {
			if ((suffix == 's') || (suffix == 'd')) {
			    ustemp = extend[0][2];
			    number.l[0] = (extend[0][1] << 16) | ustemp;
			    if (suffix == 's') number.d = (double) number.f;
			    else {
			        ustemp = next[0][2];
		                number.l[1] = (next[0][1] << 16) |
		                    ustemp;
			    };
			    printf("&0f%s",gcvt(number.d,
				suffix == 's' ? 7 : 17, buffer));
			} else printea(srcmode,srcreg,4);
		    } else {
			if (pred) printf("-");
			printea(srcmode,srcreg,0);
			if ((suffix == 'd') && !srcmode) 
			    printf(":%%d%d",nextinst & 7);
		    };

		    printf(",%%fpa%d",Dreg1);

		    if ((Dreg1 != Dreg2) || (opindex <= 12))
			printf(",%%fpa%d",Dreg2);

		    /* If this is a slow instruction, check for wait loop */
		    if ((count <= 2) && !(offset & 8) && 
			((1 << opcode) & 0x00010030)) printf(",.nowait");

		};
	    };

	/* Move from Dragon to external location */
	} else {

	    dotinc = dotincsave + (srcmode == 5 ? 2 : 4);

	    /* Get Dragon card offset */
	    if (srcmode == 5) offset = extend[0][1];
	    else {
		ustemp = extend[0][2];
		offset = ((extend[0][1] << 16) | ustemp) - fpa_addr;
	    };

	    /* Check for special register */
	    if ((offset == 4) || (offset == 8)) {
		printf("\tfpmov.l\t%%fpa%cr,",offset == 4 ? 's' : 'c');
		printea(dstmode, dstreg, 4);

	    /* General move */
	    } else {

		Dreg1 = (offset >> 8) & 0xF;

		/* Byte or word move? */
		if (offset & 3) {
		    suffix = offset & 1 ? 'b' : 'w';
		    printf("\tfpmov.%c\t%%fpa%d,", suffix, Dreg1);
		    printea(dstmode, dstreg, 0);
		} else {
		    suffix = count == 1 ? 's' : 'd';
		    printf("\tfpmov.%c\t%%fpa%d,", suffix, Dreg1);
		    printea(dstmode, dstreg, 0);
		    if ((suffix == 'd') && !dstmode) {
			nextinst = instfetch(2);
			printf(":%%d%d",(nextinst >> 9) & 7);
		    };
		};
	    };
	};

	dotinc = dotincsave + length - 2;
	return 1;
}

/*
	Dfetch : Return additional instruction words, checking for end.

	size - number of bytes to fetch
	eflag - return flag: 0 = no error; 1 = error
*/

static long Dfetch(size,eflag)
int size;
unsigned short *eflag;
{
	if (inkdot(dotinc) + size > txtmap.e1) {
		*eflag = 1;
		return 0;
	} else {
		*eflag = 0;
		return instfetch(size);
	};
}

/*
	D_modes : collect additional addressing mode information
	for a particular move instruction. All info is put into
	the array extend. Returns 1 for an error; 0 for no error.

	inst - the move instruction
	s - move instruction suffix
	extend[0][1 thru 5] - source extension words
	extend[1][1 thru 5] - destination extension words
	extend[0][0] - number of source extension words
	extend[1][0] - number of destination extension words
*/

static int D_modes(inst,s,extend)
long inst;
char *s;
unsigned short extend[2][6];
{

	short mode[2], reg[2], temp, eflag = 0, i, j;
	long Dfetch();

	/* Isolate instruction fields */
	mode[0] = (inst >> 3) & 7;
	mode[1] = (inst >> 6) & 7;
	reg[0] = inst & 7;
	reg[1] = (inst >> 9) & 7;

	for (i = 0; i < 2; i++) {
		if (mode[i] == 1) return 1;
		else if ((mode[i] == 5) || ((mode[i] == 7) && (reg[i] == 2))) {
			if (i && (mode[i] == 7)) return 1;
			extend[i][0] = 1;  /* one extension word */
			extend[i][1] = Dfetch(2,&eflag);
			if (eflag) break;
		} else if ((mode[i] == 6) || 
		   ((mode[i] == 7) && (reg[i] == 3))) {
			if (i && (mode[i] == 7)) return 1;
			extend[i][1] = temp = Dfetch(2,&eflag);
			if (eflag) break;
			j = 1;
			if (temp & 0x100) { /* long format? */

				/* reserved mode? */
				if (((temp & 0x44) == 0x44) ||
				    ((temp & 7) == 4) ||
				    !(temp & 0x30)) return 1;

				if (temp & 0x20) { /* base displacement? */
					extend[i][++j] = Dfetch(2,&eflag);
					if (eflag) break;
					if (temp & 0x10) { /* long? */
						extend[i][++j] = 
							Dfetch(2,&eflag);
						if (eflag) break;
					};
				};
				if (temp & 0x2) { /* outer displacement? */
					extend[i][++j] = Dfetch(2,&eflag);
					if (eflag) break;
					if (temp & 0x1) { /* long? */
						extend[i][++j] = 
							Dfetch(2,&eflag);
						if (eflag) break;
					};
				};
			};
			extend[i][0] = j;
		} else if (mode[i] == 7) {
			if (i && (reg[i] & 6)) return 1;
			else if (reg[i] < 5) {
				j = 0;
				extend[i][++j] = Dfetch(2,&eflag);
				if (eflag) break;
				if ((reg[i] == 1) || 
				    ((reg[i] == 4) && (*s == 'l'))) {
					extend[i][++j] = Dfetch(2,&eflag);
					if (eflag) break;
				};
				extend[i][0] = j;
			} else return 1;
		} else extend[i][0] = 0;
	};
	return eflag;
}

/* 
	D_compare : compare the addresses associated with two move.l's
	to determine if the second is the second half of a double
	instruction. Returns a 1 if it the second is compatible with
	the first; returns 0 otherwise.

	mode1 - addressing mode for first move
	reg1  - register for first move
	mode2 - addressing mode for second move
	reg2  - register for second move
	arg1, arg2 - arrays containing addressing info
	index - 0 for source; 1 for destination
*/

static int D_compare(mode1, reg1, mode2, reg2, arg1, arg2, index)
short mode1, reg1, mode2, reg2, arg1[2][6], arg2[2][6], index;
{
	short ext1, ext2, iis1, iis2, i;
	long offset1, offset2;
	unsigned short ustemp;

	switch (mode1) {

	/* Data register direct */
	case 0:
		/* Second must also be data register direct */
		return mode2 == 0;

	/* Address register indirect */
	case 2:
		/* Correct for pair:   (%a1) and 4(%a1) */
		return (reg1 == reg2) && (mode2 == 5) && (arg2[index][1] == 4);

	/* Address register indirect w/ postincrement */
	case 3:

	/* Address register indirect w/ predecrement */
	case 4:
		/* Correct for pair:   (%a1)+ and (%a1)+ */
		/*		    or -(%a1) and -(%a1) */
		return (mode1 == mode2) && (reg1 == reg2);

	/* Address register indirect w/ displacement */
	case 5:
		if (reg1 != reg2) return 0;
		offset1 = arg1[index][1] + 4;

		/* Correct for pair:   -4(%a1) and (%a1) */
		if (mode2 == 2) return !offset1;

		/* Correct for pair:   12(%a1) and 16(%a1) */
		if (mode2 == 5) return offset1 == arg2[index][1];

		/* Correct for pair:   0x7FFE(%a1) and (0x8002,%a1) */
		if ((mode2 == 6) && (arg2[index][1] == 0x0170)) {
			ustemp = arg2[index][3];
			return offset1 == (arg2[index][2] << 16) | ustemp;
		};

		return 0;
	
	/* Various modes */
	case 7:
		if (mode2 != 7) return 0;

		/* Absolute addressing */
		if (reg1 <= 1) {
			if (reg1) {
				ustemp = arg1[index][2];
				offset1 = (arg1[index][1] << 16) | ustemp;
			} else offset1 = arg1[index][1];
			if (!reg2) return (offset1 + 4) == arg2[index][1];
			else if (reg2 == 1) {
				ustemp = arg2[index][2];
				return (offset1 + 4) ==
					(arg2[index][1] << 16) | ustemp;
			} else return 0;
		};
		
		/* Immediate addressing */
		if (reg1 == 4) return reg2 == 4;

		/* Program counter indirect with displacement */
		if (reg1 == 2) {
			offset1 = arg1[index][1] + 4 
				  - 2 * (arg1[0][0] + arg1[1][0] + 1);

			/* Correct for pair:   12(%pc) and 16(%pc) */
			if (reg2 == 2) return offset1 == arg2[index][1];

			/* Correct for pair:   0x7FFE(%pc) and (0x8002,%pc) */
			if ((reg2 == 3) && (arg2[index][1] == 0x0170)) {
				ustemp = arg2[index][3];
				return offset1 == (arg2[index][2] << 16) | 
				       ustemp;
			};

			return 0;
		};

		if (reg2 == 2) {
			if (arg1[index][1] != 0x0170) return 0;
			ustemp = arg1[index][3];
			return (((arg1[index][2] << 16) | ustemp) + 4
				 - 2 * (arg1[0][0] + arg1[0][1] + 1)) ==
				 arg2[index][1];
		};

		if (reg2 != 3) return 0;
		break;

	/* Address register and memory indirect with index */
	case 6:
		if (mode2 == 5) {
			if (arg1[index][1] != 0x0170) return 0;
			ustemp = arg1[index][3];
			return (((arg1[index][2] << 16) | ustemp) + 4) ==
				arg2[index][1];
		};

		if (mode2 != 6) return 0;
		break;
	};

	ext1 = arg1[index][1];
	ext2 = arg2[index][1];

	/* Check for compatible addressing modes */
	if (ext1 & 0x0100) { /* Full format extension word? */
		if (ext2 & 0x0100) { /* second also full format? */
		    if (((ext1 & 0x00C0) != (ext2 & 0x00C0)) ||
					/* BS and IS the same? */
		    (!(ext1 & 0x0080) && (reg1 != reg2)) ||
					/* if !BS, is base reg same? */
		    (!(ext1 & 0x0040) && ((ext1 & 0xFE00) != 
		    (ext2 & 0xFE00))) || /* if !IS, is inx reg same? */
		    (!(ext1 & 7) != !(ext2 & 7)) ||
					/* compatible modes? */
		    ((ext1 & 4) != (ext2 & 4))) return 0;
		} else { /* brief format */
		    if ((reg1 != reg2) || /* compare base registers */
			((ext1 & 0xF7) != 0x20) ||
			/* !BS, !IS, word displacement, mem direct */
			((ext1 &0xFE00) != (ext2 & 0xFE00))) return 0;
					/* compare index registers */
		};
	} else { /* brief format */
		if ((reg1 != reg2) || /* compare base registers */
		   ((ext1 &0xFE00) != (ext2 & 0xFE00))) return 0;
					/* compare index registers */
		if (ext2 & 0x0100) { /* full format */
		    if ((ext2 & 0xF7) != 0x20) return 0;
			/* !BS, !IS, word displacement, mem direct */
		};
	};

	/* Determine offsets */
	if (ext1 & 0x0100) { /* Full format extension word? */
		offset1 = 0;
		i = arg1[index][0];
		if (iis1 = ext1 & 7) {
			if (iis1 & 2) {
				offset1 = arg1[index][i--];
				if (iis1 & 1) {
					offset1 &= 0xFFFF;
					offset1 |= arg1[index][i] << 16;
				};
			};
		} else {
			if (ext1 & 0x20) {
				offset1 = arg1[index][i--];
				if (ext1 & 0x10) {
					offset1 &= 0xFFFF;
					offset1 |= arg1[index][i] << 16;
				};
			};
		};
	} else { /* brief format */
		offset1 = ext1 & 0xFF;
		if (offset1 & 0x80) offset1 |= 0xFFFFFF00;
	};

	if (ext2 & 0x0100) { /* Full format extension word? */
		offset2 = 0;
		i = arg2[index][0];
		if (iis2 = ext2 & 7) {
			if (iis2 & 2) {
				offset2 = arg2[index][i--];
				if (iis2 & 1) {
					offset2 &= 0xFFFF;
					offset2 |= arg2[index][i] << 16;
				};
			};
		} else {
			if (ext2 & 0x20) {
				offset2 = arg2[index][i--];
				if (ext2 & 0x10) {
					offset2 &= 0xFFFF;
					offset2 |= arg2[index][i] << 16;
				};
			};
		};
	} else { /* brief format */
		offset2 = ext2 & 0xFF;
		if (offset2 & 0x80) offset2 |= 0xFFFFFF00;
	};

	if ((mode1 == 7) && !((ext1 & 0x0100) && (ext1 & 0x0087)))
	    offset1 -= 2 * (arg1[0][0] + arg1[1][0] + 1);

	return (offset1 + 4) == offset2;
}


static int Dragon_subq(inst)
long inst;
{
	unsigned long count, length;
	unsigned short wait;
	long dotincsave = dotinc;

	dotinc -= 2;
	if (!Dragon_count(&count,&length,&wait)) DABORT;
	dotinc = dotincsave;
	if (!Dragon_move(instfetch(2),"l",count - 1)) DABORT;
	dotinc = dotincsave + length - 2;
	return 1;
}

int Dragon_count(count, length, wait)
unsigned long *count, *length; 
unsigned short *wait;
{
	char *s;
	long dotincsave, inst, newdotinc, nextoffset, offset;
	short Dreg1, Dreg2, Dreg, cnt, dstflg, dstmode, dstreg, eflag;
	short extend[2][6], len, ndstmode, ndstreg, next[2][6];
	short nextinst, nsrcmode, nsrcreg, opcode, size, snglflg, srcflg;
	short srcmode, srcreg;
	unsigned short nextopcode, ustemp;

	dotincsave = dotinc;

	/* Does the user want dragon disassembly ? */
	if (!var[VARF]) goto abort;

	*wait = 0;

	/* get instruction */
	dotinc = 0;
	inst = Dfetch(2,&eflag);		
	if (eflag) goto abort;

	/* Check for tst instruction */
	if ((inst & 0xFFC0) == 0x4A00) {
	    len = 2;

	    /* address register indirect addressing? */
	    if ((inst & 0x0038) == 0x0010) {
		if ((inst & 0x0007) != var[VARR]) goto abort;
		offset = 0;

	    /* relative addressing? */
	    } else if ((inst & 0x0038) == 0x0028) {
		if ((inst & 0x0007) != var[VARR]) goto abort;
		offset = Dfetch(2,&eflag);
	    	if (!offset) goto abort;
		len += 2;

	    } else goto abort;

	    /* check for fpwait instruction */
	    if (!offset) {
		if (Dfetch(2,&eflag) != 0x6BFC) goto abort;
		*count = 2;
		*length = len + 2;
		*wait = 1;
		dotinc = dotincsave;
		return 1;
	    };

	    /* is this a branch instruction? */
	    if ((offset & 0xFFFFFF0F) != 0x0000000C) goto abort;

	    /* check for bmi or bpl following */
	    inst = Dfetch(2,&eflag);
	    if ((inst & 0xFE00) != 0x6A00) goto abort;
	    len += 2;
	    if (inst & 0xFF) {
		if ((inst & 0xFF) == 0xFF) {
		    if (inkdot(dotinc) + 4 > txtmap.e1) goto abort;
		    len += 4;
		};
	    } else {
		if (inkdot(dotinc) + 2 > txtmap.e1) goto abort;
		len += 2;
	    };

	    /* we have a Dragon branch instruction */
	    *count = 2;
	    *length = len;
	    dotinc = dotincsave;
	    return 1;
	};

	/* Check for subq instruction */
	if ((inst & 0xFFF8) == 0x5148) {
	    len = 2;
	    srcreg = inst & 0x7;
	    inst = Dfetch(2,&eflag);

	    /* check for "mov.l (%an),..." */
	    if ((inst & 0xF03F) != (0x2010 | srcreg)) goto abort;
	    len += 2;

	    /* Get Dragon offset */
	    dstmode = (inst >> 6) & 7;
	    dstreg = (inst >> 9) & 7;
	    if ((dstmode == 5) && (dstreg == var[VARR])) {
		offset = Dfetch(2,&eflag);
		len += 2;
	    } else goto abort;
	    if (eflag) goto abort;
	    
	    /* First instruction must be an fpmov */
	    if ((offset & 0xFFFFF0FF) != 0x00001008) goto abort;
	    Dreg = (offset >> 8) & 0xF;

	    /* Get next instruction and check for "mov.l 4(%an),..." */
	    inst = Dfetch(2,&eflag);
	    if ((inst & 0xF03F) != (0x2028 | srcreg)) goto abort;
	    if (Dfetch(2,&eflag) != 4) goto abort;
	    len += 4;

	    /* Get Dragon offset */
	    dstmode = (inst >> 6) & 7;
	    dstreg = (inst >> 9) & 7;
	    if ((dstmode == 5) && (dstreg == var[VARR])) {
		offset = Dfetch(2,&eflag);
		len += 2;
	    } else if ((dstmode == 7) && (dstreg == 1)) {
		offset = Dfetch(4,&eflag) - fpa_addr;
		len += 4;
		if ((offset & 0xFFFF0000) != 0x00010000) goto abort;
	    } else goto abort;
	    if (eflag) goto abort;
	    
	    if ((offset & 0x7) != 4) goto abort;  /* must be double move */
	    if ((offset < 0xFFFF8000) || (offset > 0x00018000)) goto abort;
	    opcode = (offset >> 12) &0x1F;
	    if ((1 << opcode) & (offset & 8 ? 0x8CE0FF03 : 0xFFFEFFE3))
		goto abort;   /* valid opcode? */

	    /* check for register inconsistencies */
	    if ((offset & 0x1F008) == 0x02008) {  /* fpmtest? */
		if ((offset & 0xFF0) != ((Dreg << 8) | (Dreg << 4))) goto abort;
	    } else {
		if (Dreg != ((offset >> (offset >= 0x00010000 ? 4 : 8)) & 0xF))
		    goto abort;
	    };

	    cnt = 3;

	    /* Check for wait loop */
	    if (!(offset & 8) && ((1 << opcode) & 0x00010030)) {
	        inst = Dfetch(2,&eflag);
		if (((inst & 0xFFF8) == 0x4A10) &&
		    ((inst & 7) == var[VARR]) &&
		    (Dfetch(2,&eflag) == 0x6BFC)) {
			cnt += 2;
			len += 4;
			*wait = 1;
		};
	    };

	    *count = cnt;
	    *length = len;
	    dotinc = dotincsave;
	    return 1;
	};

	/* Check for move instruction */
	if ((inst & 0xF000) && !(inst & 0xC000)) {

	    /* Determine size */
	    if (inst & 0x2000) {
	        if (inst & 0x1000) goto abort;
	        else { size = 4; s = "l"; };
	    } else { size = 1; s = "b"; };
    
	    /* Isolate instruction fields */
	    srcmode = (inst >> 3) & 7;
	    dstmode = (inst >> 6) & 7;
	    srcreg = inst & 7;
	    dstreg = (inst >> 9) & 7;
    
	    /* Get information for addressing modes */
	    if (D_modes(inst,s,extend)) goto abort;
    	
	    /* Moving to or from Dragon card? */
	    if ((srcmode == 5) && (srcreg == var[VARR])) srcflg = 1;
	    else srcflg = 0;

	    if ((dstmode == 5) && (dstreg == var[VARR])) dstflg = 1;
	    else if ((dstmode == 7) && (dstreg == 1)) {
	        ustemp = extend[1][2];
	        offset = ((extend[1][1] << 16) | ustemp) - fpa_addr;
		if ((offset & 0xFFFF0000) != 0x00010000) dstflg = 0;
	    } else dstflg = 0;
    
	    if (srcflg == dstflg) goto abort; /* abort if neither or both */
    
	    if (dstflg) {
    
	        /* Get Dragon card offset */
	        if (dstmode == 5) offset = extend[1][1]; 
	        else { /* the offset has already been calculated */ };
    
	        /* Isolate fields */
	        opcode = (offset >> 12) & 0x1F;
	        Dreg1 = (offset >> 8) & 0xF;
	        Dreg2 = (offset >> 4) & 0xF;
    
	        if ((offset & 3) == 1) { /* Register-to-register instruction */
    
		    /* Check for valid opcode */
		    if ((inst & 0xF03F) != 0x1000) /* must be "mov.b %d0,..." */
		        goto abort; 
		    if ((offset & 3) != 1) goto abort;
		    if (offset & 8) { /* fast instruction? */
		        if ((1 << opcode) & 0x8CFFFF03) { /* maybe invalid? */
			    if ((opcode & 0x1E) == 0x1A) {
			        if (offset & 4) goto abort;	/* cv[sd].l? */
			    } else goto abort;
		        };
		    } else {
		        if ((1 << opcode) & 0xFFFFFFE3) goto abort;
		    };
    
		    /* Error if "fptest" has second operand */
		    if (((offset & 0x1F008) == 0x02008) && (Dreg1 != Dreg2))
			goto abort;
    
		    cnt = 1;
		    len = 4;
    
	            /* Check for wait loop */
	            if (!(offset & 0x02008)) {
	                inst = Dfetch(2,&eflag);
		        if (((inst & 0xFFF8) == 0x4A10) &&
			    ((inst & 7) == var[VARR]) &&
			    (Dfetch(2,&eflag) == 0x6BFC)) {
			        cnt += 2;
			        len += 4;
				*wait = 1;
			    };
	            };
    
		    *count = cnt;
		    *length = len;
		    dotinc = dotincsave;
		    return 1;

		} else {

		    /* Check for special registers */
		    if ((offset == 4) || (offset == 8)) {
			if (size != 4) goto abort;
			*count = 1;
			*length = 2 * (extend[0][0] + extend[1][0]) + 2;
			dotinc = dotincsave;
			return 1;
		    };

		    /* Check for valid opcode */
		    if (offset & 8) {
			if ((1 << opcode) & 0x80E0FF01) goto abort;
			if ((offset & 0x1E004) == 0x1A004) goto abort;
		    } else {
			if ((1 << opcode) & 0xFFFEFFE3) goto abort;
		    };

		    /* Verify sizes */
		    if ((size != 4) || (offset & 3)) goto abort;

		    /* fpmov can't have second register */
		    if (Dreg2 && ((offset & 0x1F008) == 0x01008)) goto abort;

		    snglflg = 1;
		    cnt = 1;
		    len = 2 * (extend[0][0] + extend[1][0]) + 2;

		    newdotinc = dotinc;

		    /* Check for double */
		    if ((offset & 0xFFFFF0FB) != 0x00001008) goto single;

		    /* Is there more code? */
		    nextinst = Dfetch(2,&eflag);
		    if (eflag) goto single;

		    /* Is this a mov.l instruction? */
		    if ((nextinst & 0xF000) != 0x2000) goto single;

		    /* get address info */
		    if (D_modes(nextinst,s,next)) goto single;

		    nsrcmode = (nextinst >> 3) & 7;
		    ndstmode = (nextinst >> 6) & 7;
		    nsrcreg = nextinst & 7;
		    ndstreg = (nextinst >> 9) & 7;

		    /* verify it's an operation on the Dragon */
		    if (ndstmode == 5) {
			if (ndstreg != var[VARR]) goto single;
			nextoffset = next[1][1];
		    } else if ((ndstmode == 7) && (ndstreg == 1)) {
			ustemp = next[1][2];
			nextoffset = ((next[1][1] << 16) | ustemp) - fpa_addr;
			if ((nextoffset & 0xFFFF0000) != 0x00010000) 
			    goto single;
		    } else goto single;

		    /* Load bits must be zero */
		    if (nextoffset & 3) goto single;

		    /* only double moves can go in reverse order */
		    if ((offset & 4) && !((srcmode == 4) && 
		        (nextoffset & 0x1F0FF) == 0x01008))
			goto single;

		    /* must be other half of double */
		    if ((offset & 4) == (nextoffset & 4)) goto single;

		    /* valid instruction? */
		    nextopcode = (nextoffset >> 12) & 0x1F;
		    if ((1 << nextopcode) & (nextoffset & 8 ? 0x8CE0FF01 : 
			0xFFFEFFE3)) goto single;

		    /* Registers must match */
		    if (Dreg1 != ((nextoffset >> (nextoffset >= 0x00010000 ?
			4 : 8)) & 0xF)) goto single;
		
		    /* "fptest" and "fpmov" can't have 2nd reg */
		    if (nextoffset & 8) {
			if ((nextopcode == 1) && (nextoffset & 0xF0))
			    goto single;
			else if ((nextopcode == 2) && (Dreg1 !=
			    ((nextoffset >> 4) & 0xF))) goto single;
		    };

		    /* Source addressing modes compatible? */
		    if (!D_compare(srcmode, srcreg, nsrcmode,
			nsrcreg, extend, next, 0)) goto single;

		    snglflg = 0;
		    cnt++;
		    len += 2 * (next[0][0] + next[1][0]) + 2;
		    opcode = nextopcode;
		    offset = nextoffset;

		    single: if (snglflg) {
			if (offset & 4) goto abort;
			if (((offset & 0x1F008) == 0x02008) && (Dreg1 !=
			    Dreg2)) goto abort;
			dotinc = newdotinc;
		    };

	            /* Check for wait loop */
		    if (!(offset & 8) && ((1 << opcode) & 0x00010030)) {
	                inst = Dfetch(2,&eflag);
			if (((inst & 0xFFF8) == 0x4A10) &&
			    ((inst & 7) == var[VARR]) &&
			    (Dfetch(2,&eflag) == 0x6BFC)) {
			        cnt += 2;
			        len += 4;
				*wait = 1;
			    };
	            };

		    *count = cnt;
		    *length = len;
		    dotinc = dotincsave;
		    return 1;
		};

	    /* Move from Dragon */
	    } else {

	        /* Get Dragon card offset */
		offset = extend[0][1];

	        /* Check for special register */
	        if ((offset == 4) || (offset == 8)) {
		    if (size != 4) goto abort;
		    cnt = 1;
		    len = 2 * (extend[0][0] + extend[1][0]) + 2;

	        /* Move to external location */
	        } else {
		    /* Check that this is a move */
		    if ((offset & 0xFFFFF0FB) != 0x00001008) goto abort;
    
		    cnt = 1;
		    len = 2 * (extend[0][0] + extend[1][0]) + 2;
    
		    /* Byte or word move? */
		    if (size != 4) goto abort;
    
		    /* Is this a double move? */
		    snglflg = 1;
		    nextinst = Dfetch(2,&eflag);   /* get next instr */
		    if (eflag) goto mvsngl;  /* code left? */
		    if ((nextinst & 0xF000) != 0x2000) 
			goto mvsngl; /* mov.l? */
		    if (D_modes(nextinst,s,next)) goto mvsngl;
    
		    /* Get Dragon offset */
		    if (((nextinst & 0x38) == 0x28) &&
			((nextinst & 7) == var[VARR])) 
			nextoffset = next[0][1];
		    else goto mvsngl;
    
		    /* Check for other half of same Dragon instruction */
		    if ((offset & 0xFFFFFFFB) != (nextoffset & 0xFFFFFFFB))
			goto mvsngl;
		    if ((offset & 4) == (nextoffset & 4)) goto mvsngl;
    
		    /* compatible addr mode? */
		    if (!D_compare(dstmode, dstreg, (nextinst >> 6) & 7, 
			(nextinst >> 9) & 7, extend, next, 1)) goto mvsngl;
    
		    /* reverse order only for predecrement */
		    if ((dstmode != 4) && (offset & 4)) goto mvsngl;
    
		    snglflg = 0;
		    cnt++;
		    len += 2 * (next[0][0] + next[1][0]) + 2;
    
		    mvsngl: if (snglflg && (offset & 4)) goto abort;
	        };
		*count = cnt;
		*length = len;
		dotinc = dotincsave;
		return 1;
	    };
	};

abort:	dotinc = dotincsave;
	return 0;
};

printIMM(type)
int type;
{
    union { short s; float f; double d; long x[3]; } num, val;
    int i, exp2, exp;
    double exp10, z, p, r;
    char c, buffer[30];

    printf("&");
    switch (type) {
    case 0: printf("%d", instfetch(4)); return;
    case 4:
    case 6: printf("%d", instfetch(2)); return;
    case 1: num.x[0] = instfetch(4);
            printf("0f%s",gcvt(num.f, 6, buffer));
            return;
    case 5: num.x[0] = instfetch(4);
            num.x[1] = instfetch(4);
            printf("0f%s",gcvt(num.d, 13, buffer));
            return;
    default:num.x[0] = instfetch(4);
            num.x[1] = instfetch(4);
            num.x[2] = instfetch(4);
    };

    printf("0f");
    if (num.s < 0) {
        printf("-");
        num.s &= 0x7FFF;
    };

    exp2 = num.s;
    if (exp2 == 0x7FFF) {
        if (num.x[1] || num.x[2]) {
            printf("NaN(");
	    num.x[1] <<= 1;
	    if (num.x[2] < 0) num.x[1]++;
	    num.x[2] <<= 1;
            printBCD(&num.x[1]);
            printf(")");
        } else printf("INF");
        return;
    };

    if (type == 2) {
        if (!exp2) {
            if (!num.x[1]) {
                if (!(num.x[1] = num.x[2])) { printf("0.0"); return; };
                num.x[2] = 0;
                exp2 -= 32;
            };
            while (num.x[1] > 0) {
                num.x[1] <<= 1;
                if (num.x[2] < 0) num.x[1]++;
                num.x[2] <<= 1;
                exp2--;
            };
        };
        exp2 -= 16383;
        val.x[0] = 0x3FF00000 | ((num.x[1] >> 11) & 0xFFFFF);
        val.x[1] = (num.x[1] << 21) | ((num.x[2] >> 11) & 0xFFFFF);
        exp10 = (double) exp2 * 0.3010299956639812;
        exp = (int) exp10;
        if (exp10 < 0) exp--;
        exp10 -= (double) exp;
        exp10 *= 2.3025850929940459;
	if (exp10 > 5.55111512312578270e-17) {
	    i = 1;
	    z = 0.69314718055994531;
	    while (exp10 > z) { i++; exp10 -= z; };
	    val.s += i << 4;
	    z = exp10 * exp10;
	    p = ((0.31555192765684646E-4 * z + 0.75753180159422777E-2) * z 
		+ 0.25) * exp10;
	    r = 0.5 + p / ((((0.75104028399870046E-6 * z + 
		0.63121894374398504E-3) * z + 0.56817302698551222E-1) * z + 
		0.5) - p);
	    r *= val.d;
	} else r = val.d;
	if (r >= 10.0) while (r >= 10.0) { r /= 10.0; exp++; }
	else if (r < 1.0) while (r < 1.0) { r *= 10.0; exp--; };
	printf("%sE%d",gcvt(r,8,buffer),exp);
    } else {
	exp2 = 100 * ((exp2 >> 8) & 0xF) + 10 * ((exp2 >> 4) & 0xF) + 
	       (exp2 & 0xF);
	if (!(c = num.x[0] & 0xF)) {
	    if (!num.x[1]) {
		if (!(num.x[1] = num.x[2])) { printf("0.0"); return; };
		num.x[2] = 0;
		exp2 -= 8;
            };
	    do {
		c = (num.x[1] >> 28) & 0xF;
		num.x[1] <<= 4;
		num.x[1] |= (num.x[2] >> 28) & 0xF;
		num.x[2] <<= 4;
		exp2--;
	    } while (!c);
        };
	printf("%c.",c + '0');
        printBCD(&num.x[1]);
	printf("E");
	if (num.x[0] & 0x40000000) printf("-");
	printf("%d",exp2);
    };
}

printBCD(x) long x[];
{
    long i, j, val;
    char c, buffer[18], *ptr;

    val = x[0];
    ptr = buffer;
    *ptr++ = ' ';
    for (j = 0; j < 2; j++) {
        for (i = 28; i >= 0; i -= 4) {
            c = ((val >> i) & 0xF) + '0';
            if (c > '9') c += 'A' - '9' - 1;
	    *ptr++ = c;
        };
        val = x[1];
    };
    while (*--ptr == '0');
    if (*ptr == ' ') ptr++;
    *++ptr = '\0';
    printf("%s",&buffer[1]);
}
