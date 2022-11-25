/* @(#) $Revision: 70.2 $ */     

/****************************************************************************

	DEBUGGER - 68020 instruction printout

****************************************************************************/
#include "Kdb.h"
#define psymoff(a,b,c) print_symbol(a,c)
#define printc kdbputchar

char *badop = "\t???";
char *IMDF = "&%h";				/* immediate data format */
char *IMDF2 = "&%#x";				/* immediate data format */

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
int odivl(),ofline(),tosrccr(),fromsrccr();

char * contreg();

struct opdesc opdecode[] =
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
  0xF000, 0xF000, ofline, 0,		/* f-line instruction */
  0, 0, 0, 0
};

int dotdot;
int instructlen;
#if ! defined(CDBKDB)
extern unsigned int loadpoint;
extern int unsigned kdb_loadpoint;
extern int unsigned TextPgCnt;
extern int mapper_off;
#endif /* not CDBKDB */
extern int kdb_processor;

kdbprntins(addr)
unsigned int addr;
{
	register struct opdesc *p;
	register int (*fun)();
	register int inst;
	int save_addr = addr;
	int adjusted = 0;
#if ! defined(CDBKDB)
	unsigned int endtext;

	endtext = loadpoint + (TextPgCnt * 4096);
	if (!mapper_off)
	if ((addr >= loadpoint) && (addr < endtext)){
		addr -= loadpoint;
		adjusted = 1;
	}
#endif /* not CDBKDB */

	dotdot = addr;
	print_symbol(addr,"");
	inst = fetch(addr,2);
	instructlen = 2;
	for (p = opdecode; p->mask; p++)
		if ((inst & p->mask) == p->match) break;
	if (p->mask != 0) (*p->opfun)(inst, p->farg);
	else printf(badop);
#if ! defined(CDBKDB)
	if (mapper_off)
		printf("\t(MMU off)");
	else {
		if ((addr >= kdb_loadpoint) && (addr < loadpoint))
			printf("\t(debugger space)");
		else if (addr < kdb_loadpoint)
			printf("\t(0x%x, 0x%x)", addr, addr + loadpoint);
		else if (addr >= endtext)
			printf("\t(ram space)");
		else if (adjusted)
                        printf("\t(0x%x) (0x%x)", addr, save_addr);
                else
                        printf("\t(0x%x)", addr);
	}
#endif /* not CDBKDB */
/*
	printf("\n");
*/
	return(instructlen);
}

long
instfetch(size)
int size;
{
	register long l1, l2;

	if (size==4)
	{
		l1 = (long) fetch(INCDOTDOT(instructlen),2);
		l1 <<= 16;
		l2 = (long) fetch(INCDOTDOT(instructlen += 2),2);
		l1 = (l1 | l2);
	}
	else
	{
		l1 = (long) fetch(INCDOTDOT(instructlen),2) & 0xFFFF;
		if (l1 & 0x8000) l1 |= 0xFFFF0000;
	}
	instructlen += 2;
	return(l1);
}

printea(mode,reg,size)
long mode, reg;
int size;
{
	long index;
	char imf[6];


/*
printf("\nprintea: mode is %d, reg is %d\n",mode,reg);
*/

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
					printf("%#x.w",index);
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
					printf(IMDF2, index);
					while (size > 4)
					{
			         	  index = instfetch(4);
					  printc(' ');
					  printf(IMDF2, index);
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

	printf("\tmov.%c\t",*s);
	size = ((*s == 'b') ? 1 : (*s == 'w') ? 2 : 4);
	printea((inst>>3)&07,inst&07,size);
	printc(',');
	printea((inst>>6)&07,(inst>>9)&07,size);
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
		printEA(inst);
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
	long ins_const;
	char imf[6];

	if (size > 0)
	{
		ins_const = instfetch(size==4?4:2);
		printf("\t%s%s\t", opcode, suffix(size));
		strcpy(imf,IMDF2);
		if (*opcode == 'c')
		{	/* cmpi */
			printEA(inst,size); printc(',');
			printf(imf, ins_const);
			return;
		}
		printf(imf, ins_const); printc(',');
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
	printf(IMDF, instfetch(size));
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
	printEA(inst);
}

oneop(inst,opcode)
long inst;
char *opcode;
{
	printf("\t%s\t",opcode);
	printEA(inst);
}

pregmask(mask)
register int mask;
{
	register short i;
	register short flag = 0;

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
		printEA(inst);
		printc(',');
		pregmask(reglist);
	}
	else
	{
		pregmask(reglist);
		printc(',');
		printEA(inst);
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
	register int size = mapsize(inst,6,0);

	if (size > 0)
	{
		printf("\t%s%s\t",opcode,suffix(size));
		printEA(inst);
	}
	else printf(badop);
}

oquick(inst,opcode)
long inst;
char *opcode;
{
	register int size = mapsize(inst,6,0);
	register int data = (int)((inst>>9) & 07);

	if (data == 0) data = 8;
	if (size > 0)
	{
		printf("\t%s%s\t", opcode, suffix(size));
		printf(IMDF, data); printc(',');
		printEA(inst);
	}
	else printf(badop);
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
	printEA(inst);
}

ocas(inst)
{
	register int size = mapsize(inst,9,1);
	register int ext = (int) instfetch(2);

	printf("\tcas%s\t%%d%d,%%d%d,", suffix(size), ext&07, (ext>>6)&07);
	printEA(inst);
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
		printEA(inst);
		printf(",%%%c%D", (ext&0100000)?'a':'d', (ext>>12)&07 );
	}
	else
	{
		printf("\tcmp2%s\t", suffix(size));
		printf("%%%c%D,", (ext&0100000)?'a':'d', (ext>>12)&07 );
		printEA(inst);
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
	printEA(inst);
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
	case 3:
		return "tc";
	case 4:
		return "itt0";
	case 5:
		return "itt1";
	case 6:
		return "dtt0";
	case 7:
		return "dtt1";
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
	case 0x805:
		return "mmusr";
	case 0x806:
		return "urp";
	case 0x807:
		return "srp";
	default:
		return "???";
	}
}

/***************     F-line instruction dis-assembler     ****************/

#define	PMOVE_MASK		0xE0FF
#define	PMOVE0			0x0000
#define	PMOVE2			0x4000

#define	PMOVE_MMUSR_MASK	0xFDFF
#define	PMOVE_MMUSR		0x6000

#define	PLOAD_MASK		0xFDE0
#define	PLOAD			0x2000

#define	PFLUSH_MASK		0xE300
#define	PFLUSH			0x2000

#define	FMOVE_MASK		0xE3FF
#define	FMOVE			0x8000

#define	CPUSH_MASK		0xFF20
#define	CPUSH			0xF420

#define	CINV_MASK		0xFF20
#define	CINV			0xF400

#define	MOV16_PINC_MASK		0xFFF8
#define	MOV16_PINC		0xF620

#define	MOV16_ABS_MASK		0xFFE0
#define	MOV16_ABS		0xF600

#define	MC68040_PFLUSH_MASK	0xFFE0
#define	MC68040_PFLUSH		0xF500

#define	MC68040_PTEST_MASK	0xFFD8
#define	MC68040_PTEST		0xF548


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

ofline(inst)
register long inst;
{
	int type = (inst>>6)&07;

	if (kdb_processor == 3)		/* 68040 */
	{
		if ((inst & MC68040_PTEST_MASK) == MC68040_PTEST) {
			int ax = inst & 0x7;
			int rw = (inst >> 5) & 1;

			printf("\tptest");

			if (rw)
				printf("r");
			else
				printf("w");

			printf("\t(%%a%d)");

			printf("\t(MC68040 only)");
			return;
		}

		if ((inst & MC68040_PFLUSH_MASK) == MC68040_PFLUSH) {
			int ax = inst & 0x7;
			int opmode = (inst >> 3) & 3;

			printf("\tpflush");

			switch (opmode) {
				case 0:
					printf("n");
					/* fall through */
				case 1:
					printf("\t(%%a%d)", ax);
					break;
				case 2:
					printf("an");
					break;
				case 3:
					printf("a");
			}

			printf("\t(MC68040 only)");
			return;
		}

		if (((inst & CINV_MASK) == CINV) || ((inst & CPUSH_MASK) == CPUSH)) {
			int ax = inst & 0x7;
			int scope = (inst >> 3) & 3;
			int cache = (inst >> 6) & 3;

		
			if ((inst & CINV_MASK) == CINV)
				printf("\tcinv");
			else
				printf("\tcpush");
	
			switch (scope) {
				case 0:
					printf("\tIllegal scope");
					break;
				case 1:
					printf("l");
					break;
				case 2:
					printf("p");
					break;
				case 3:
					printf("a");
			}

			switch (cache) {
				case 0:
					printf("\tNo cache specified");
					break;
				case 1:
					printf("\tDC");
					break;
				case 2:
					printf("\tIC");
					break;
				case 3:
					printf("\tIC/DC");
			}

			if ((scope == 1) || (scope == 2))
				printf(",(%%a%d)",ax);

			printf("\t(MC68040 only)");
			return;
		}
	} 	/* end 68040 */

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

	  default:
		if (kdb_processor == 3)		/* 68040 */
	  		printf("???\t(F-line)");
		else
	  		printf("???\t(68881)");
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

	if ((inst & MOV16_PINC_MASK) == MOV16_PINC) {
		int ax = inst & 0x7;
		int ay = (cword >> 12) & 0x7;

		printf("\tmove16\t(%%a%d)+,(%%a%d)+", ax, ay);

		printf("\t(MC68040 only)");
		return;
	}

	if ((inst & MOV16_ABS_MASK) == MOV16_ABS) {
		unsigned int low_addr = instfetch(2) & 0xFFFF;
		unsigned int addr = ((cword << 16) & 0xFFFF0000) | low_addr;
		int opmode = (inst >> 3) & 0x3;
		int ay = inst & 0x7;

		printf("\tmove16\t");

		switch (opmode) {
			case 0:
				printf("(%%%a%d)-,0x%x", ay, addr);
				break;
			case 1:
				printf("0x%x,(%%%a%d)+", addr, ay);
				break;
			case 2:
				printf("(%%%a%d),0x%x", ay, addr);
				break;
			case 3:
				printf("0x%x,(%%%a%d)", addr, ay);
				break;
		}

		printf("\t(MC68040 only)");
		return;
	}

	switch (opclas)
	{
	  case 0:
		if ((cword & PMOVE_MASK) == PMOVE0) {
			int mode = (inst >> 3) & 7;
			int ay = inst & 7;
			int ttreg = (cword >> 10) & 7;
			int rw = (cword >> 9) & 1;
			int fd = (cword >> 8) & 1;

			printf("\tpmove");
			if (fd) 
				printf("fd");

			/* XXX other modes are supported */
			if (mode != 2)
				printf("\t(mode not supported by kdb\t");

			if (rw)
				printf("\t%%tt%d,(%%a%d)", ttreg & 1, ay);
			else
				printf("\t(%%a%d),%%tt%d", ay, ttreg & 1);

			printf("\t(MC68030/MC68851 only)");
			return;
		}
		/* fp data reg to fp data reg */
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

	  case 1:
		if ((cword & PLOAD_MASK) == PLOAD) {
			int ax = inst & 7;
			int mode = (inst >> 3) & 7;
			int rw = (cword >> 9) & 1;
			int fc = cword & 0x1F;

			printf("\tpload");

			if (rw)
				printf("r");
			else
				printf("w");

			switch ((fc >> 3) & 3) {
				case 0:
					if (fc)
						printf("\tdfc,");
					else
						printf("\tsfc,");
					break;
				case 1:
					printf("\t%%d%d,", fc & 7);
					break;
				case 2:
					printf("\t&%d,", fc & 7);
					break;
				case 3:
					printf("\tbad fc field\t");
			}

			/* XXX other modes are supported */
			if (mode != 2)
				printf("\t(mode not supported by kdb\t");

			printf("(%%a%d)", ax);

			printf("\t(MC68030/MC68851 only)");
			return;
		}

		if ((cword & PFLUSH_MASK) == PFLUSH) {
			int ax = inst & 7;
			int mode = (inst >> 3) & 7;
			int fmode = (cword >> 10) & 7;
			int mask = (cword >> 5) & 7;
			int fc = cword & 0x1F;

			printf("\tpflush");

			if (fmode == 1) {
				printf("a");
				printf("\t(MC68030/MC68851 only)");
				return;
			}

			switch ((fc >> 3) & 3) {
				case 0:
					if (fc)
						printf("\tdfc,");
					else
						printf("\tsfc,");
					break;
				case 1:
					printf("\t%%d%d,", fc & 7);
					break;
				case 2:
					printf("\t&%d,", fc & 7);
					break;
				case 3:
					printf("\tbad fc field\t");
			}

			printf("&0x%x", mask);

			if (fmode == 4) {
				printf("\t(MC68030/MC68851 only)");
				return;
			}

			/* XXX other modes are supported */
			if (mode != 2)
				printf("\t(mode not supported by kdb\t");

			printf(",(%%a%d)", ax);

			printf("\t(MC68030/MC68851 only)");
			return;
		}
	  	printf("???\t(F-line)");
		return;
	  case 2:
		if ((cword & PMOVE_MASK) == PMOVE2) {
			int mode = (inst >> 3) & 7;
			int ay = inst & 7;
			int reg = (cword >> 10) & 7;
			int rw = (cword >> 9) & 1;
			int fd = (cword >> 8) & 1;
			char *preg;

			printf("\tpmove");

			if (fd) 
				printf("fd");

			switch (reg) {
				case 0:
					preg = "tc";
					break;
				case 2:
					preg = "srp";
					break;
				case 3:
					preg = "crp";
			}

			/* XXX other modes are supported */
			if (mode != 2)
				printf("\t(mode not supported by kdb\t");

			if (rw)
				printf("\t%%%s,(%%a%d)", preg, ay);
			else
				printf("\t(%%a%d),%%%s", ay, preg);

			printf("\t(MC68030/MC68851 only)");
			return;
		}
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
			printf("\tfmovcr\t&%#x,%%fp%d", ext, ry);
		}
		break;

	  case 3:
		if ((cword & PMOVE_MMUSR_MASK) == PMOVE_MMUSR) {
			int mode = (inst >> 3) & 7;
			int ay = inst & 7;
			int rw = (cword >> 9) & 1;

			printf("\tpmove");

			/* XXX other modes are supported */
			if (mode != 2)
				printf("\t(mode not supported by kdb\t");

			if (rw)
				printf("\t%%mmusr,(%%a%d)", ay);
			else
				printf("\t(%%a%d),%%mmusr", ay);

			printf("\t(MC68030/MC68851 only)");
			return;
		}
	  	/* move fp data reg to ext dest */
		printf("\tfmov%s\t%%fp%d,", srcfmt[rx], ry);
		printEA(inst);
		if (rx == 3) 
		{
			if (ext & 0x040) ext |= 0xffffff80 ;
			printf("{&%d}", ext);
		}
		if (rx == 7) printf("{%%d%d}", (ext>>4)&07);
		break;

	  case 4:
		if ((cword & FMOVE_MASK) != FMOVE) {
			int ax = inst & 7;
			int mode = (inst >> 3) & 7;
			int rw = (cword >> 9) & 1;
			int aopt = (cword >> 8) & 1;
			int areg = (cword >> 5) & 7;
			int level = (cword >> 10) & 7;
			int fc = cword & 0x1F;

			printf("\tptest");

			if (rw)
				printf("r");
			else
				printf("w");

			switch ((fc >> 3) & 3) {
				case 0:
					if (fc)
						printf("\tdfc,");
					else
						printf("\tsfc,");
					break;
				case 1:
					printf("\t%%d%d,", fc & 7);
					break;
				case 2:
					printf("\t&%d,", fc & 7);
					break;
				case 3:
					printf("\tbad fc field\t");
			}

			/* XXX other modes are supported */
			if (mode != 2)
				printf("\t(mode not supported by kdb\t");

			printf("(%%a%d),&%d", ax, level);
			
			if (aopt)
				printf(",%%a%d", areg);

			printf("\t(MC68030/MC68851 only)");
			return;
		}
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
		if (opclas==6) { printEA(inst); printc(','); }
		if (rx & 2) printf("%%d%d", (cword>>4)&07 );
		else
		{
			register int mask = cword & 0377;
			register int i;
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
		if (opclas==7) { printc(','); printEA(inst); }
		break;
	  
	  default:	printf("???\t(F-line)");

	}
}

/* type 1 instructions */
ftype1(inst)
{
	register cword = instfetch(2);

	if (((inst>>3)&07) == 1)
	{
		/* decrement & branch on condition */
		printf("\tfdb%s\t", cpred[cword&077]);
		printf("%%d%d,", inst&07 );
		psymoff(instfetch(2)+inkdot(2), ISYM, "");
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
		  printf("%#x", instfetch(2));
		  return;
		}
		/* trap on condition with long parameter */
		if((inst&07) == 3)
		{
		  printf("\tftp%s.l\t", cpred[cword&077]);
		  printf("%#x", instfetch(4));
		  return;
		}
	}
	/* set on condition */
	printf("\tfs%s\t", cpred[cword&077]);
	printEA(inst);
}

/* fbcc instructions */
fbranch(inst, size)
{
	int disp = instfetch(size);

	if ((inst&077) == 0 && disp == 0) {printf("\tfnop"); return; }
	printf("\tfb%s.%c\t", cpred[inst&077], (size == 2)? 'w':'l' );
	psymoff(disp+inkdot(2), ISYM, "");
}
