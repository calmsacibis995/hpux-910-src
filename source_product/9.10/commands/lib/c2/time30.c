/* @(#) $Revision: 70.1 $ */   
/*
 * Instruction Times for the 68030/68882
 */

#include "sched.h"

#define  RnRm      0
#define  RnEa      1
#define  EaRn      2
#define  EaEa      3


#define  DataRn    0
#define  DataMEM   1


#define  addNULL   0
#define  addFEA    1
#define  addCEA    2
#define  addFIEA   3
#define  addCIEA   4


#define  isDnorAn(X) ((X == DDIR) || (X == ADIR) || (X == DPAIR))


#define  isIMMED(X)  (X == IMMEDIATE)


                        /*   reg   int   sin   dbl   ext */

unsigned char  floph[] = {    17,   21,   30,   36,   42   };


unsigned short flopt[] = {  17,  /* fabs  */
			   607,  /* facos */
			    35,  /* fadd  */
			   563,  /* fasin */
			   385,  /* fatan */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			     0,  /* fbCC  */
			    17,  /* fcmp  */
			   373,  /* fcos  */
			   589,  /* fcosh */
			    87,  /* fdiv  */
			   479,  /* fetox */
			    37,  /* fintrz */
			   563,  /* flog10 */
			   507,  /* flogn */
			    54,  /* fmod  */
			     0,  /* fmov  */
			     0,  /* fmovm */
			    55,  /* fmul  */
			    17,  /* fneg  */
			    43,  /* fsglmul */
			    53,  /* fsgldiv */
			   373,  /* fsin  */
			   669,  /* fsinh */
			    89,  /* fsqrt */
			    35,  /* fsub  */
			   455,  /* ftan  */
			   643,  /* ftanh */
			    15 };/* ftst  */

                        /*   reg   int   sin   dbl   ext */

unsigned char  floadh[] = {   21,   21,   34,   40,   46   };


                        /*   reg   int   sin   dbl   ext */

unsigned char  floadt[] = {    0,    8,    0,    0,    0   };


                        /*   reg   int   sin   dbl   ext */

unsigned char  fstoreh[] = {   0,    0,   38,   44,   50   };


                        /*   reg   int   sin   dbl   ext */

unsigned char  fstoret[] = {   0,    0,    0,    0,    0   };


unsigned char flopea[] = {  0,  /* unknown */
			    0,  /* other */
			    0,  /* %dn */
			    0,  /* %an */
			    2,  /* (%an) */
			    6,  /* (%an)+ */
			    6,  /* -(%an) */
			    2,  /* d(%an) */
			    6,  /* d(%an,%dn) */
			    8,  /* (bd,%an,%dn.size*scale) */
			    2,  /* d(%pc) */
			    6,  /* d(%pc,%dn) */
			    8,  /* (bd,%pc,%dn.size*scale) */
			    2,  /* xxx.W */
			    4,  /* xxx.L */
			    0,  /* &xxx */
			    0,  /* %dn:%dm */
			    0 };/* %fpn */


                       /*   %dn   %an  (%an)  (%an)+  -(%an) */

unsigned char moveh[] = {    2,    2,    0,      0,      0 };
unsigned char moveb[] = {    0,    0,    2,      2,      2 };
unsigned char movet[] = {    0,    0,    1,      1,      2 };


unsigned char moveeah[] = {  0,  /* unknown */
			     0,  /* other */
			     0,  /* %dn */
			     0,  /* %an */
			     2,  /* (%an) */
			     2,  /* (%an)+ */
			     2,  /* -(%an) */
			     2,  /* d(%an) */
			     2,  /* d(%an,%dn) */
			     4,  /* (bd,%an,%dn.size*scale) */
			     2,  /* d(%pc) */
			     2,  /* d(%pc,%dn) */
			     4,  /* (bd,%pc,%dn.size*scale) */
			     2,  /* xxx.W */
			     0,  /* xxx.L */
			     0,  /* &xxx */
			     0,  /* %dn:%dm */
			     0 };/* %fpn */


unsigned char moveeab[] = {  0,  /* unknown */
			     0,  /* other */
			     2,  /* %dn */
			     2,  /* %an */
			     2,  /* (%an) */
			     2,  /* (%an)+ */
			     2,  /* -(%an) */
			     2,  /* d(%an) */
			     6,  /* d(%an,%dn) */
			    10,  /* (bd,%an,%dn.size*scale) */
			     6,  /* d(%pc) */
			     6,  /* d(%pc,%dn) */
			    10,  /* (bd,%pc,%dn.size*scale) */
			     2,  /* xxx.W */
			     6,  /* xxx.L */
			     0,  /* &xxx */
			     0}; /* %dn:%dm */


                        /*  RnRm  RnEa  EaRn */
unsigned char aloph[][3] ={ { 2,    0,    0 },  /* ADD */
			    { 2,    0,    0 },  /* ADDA.L */
			    { 4,    0,    0 },  /* ADDA.W */
			    { 2,    0,    0 },  /* AND */
			    { 2,    0,    0 },  /* EOR */
			    { 2,    0,    0 },  /* OR */
			    { 2,    0,    0 },  /* SUB */
			    { 4,    0,    0 },  /* SUBA.L */
			    { 2,    0,    0 },  /* SUBA.W */
			    { 2,    0,    0 },  /* CMP */
			    { 2,    2,    2 },  /* MULS.L */
			    { 2,    2,    2 },  /* MULS.W */
			    { 2,    2,    2 },  /* MULU.L */
			    { 2,    2,    2 },  /* MULU.W */
			    { 6,    0,    0 },  /* DIVS.L */
			    { 2,    0,    0 },  /* DIVS.W */
			    { 6,    0,    0 },  /* DIVU.L */
			    { 2,    0,    0 }}; /* DIVU.W */


                        /*  RnRm  RnEa  EaRn */
unsigned char alopb[][3] ={ { 0,    2,    2 },  /* ADD */
			    { 0,    0,    2 },  /* ADDA.L */
			    { 0,    0,    0 },  /* ADDA.W */
			    { 0,    2,    2 },  /* AND */
			    { 0,    2,    0 },  /* EOR */
			    { 0,    2,    2 },  /* OR */
			    { 0,    2,    2 },  /* SUB */
			    { 0,    0,    2 },  /* SUBA.L */
			    { 0,    0,    4 },  /* SUBA.W */
			    { 0,    0,    2 },  /* CMP */
			    {42,    0,   42 },  /* MULS.L */
			    {26,    0,   26 },  /* MULS.W */
			    {42,    0,   42 },  /* MULU.L */
			    {26,    0,   26 },  /* MULU.W */
			    {84,    0,   90 },  /* DIVS.L */
			    {54,    0,   56 },  /* DIVS.W */
			    {72,    0,   78 },  /* DIVU.L */
			    {42,    0,   44 }}; /* DIVU.W */


                        /*  RnRm  RnEa  EaRn */
unsigned char alopt[][3] ={ { 0,    1,    0 },  /* ADD */
			    { 0,    0,    0 },  /* ADDA.L */
			    { 0,    0,    0 },  /* ADDA.W */
			    { 0,    1,    0 },  /* AND */
			    { 0,    1,    0 },  /* EOR */
			    { 0,    1,    0 },  /* OR */
			    { 0,    1,    0 },  /* SUB */
			    { 0,    0,    0 },  /* SUBA.L */
			    { 0,    0,    0 },  /* SUBA.W */
			    { 0,    0,    0 },  /* CMP */
			    { 0,    0,    0 },  /* MULS.L */
			    { 0,    0,    0 },  /* MULS.W */
			    { 0,    0,    0 },  /* MULU.L */
			    { 0,    0,    0 },  /* MULU.W */
			    { 0,    0,    0 },  /* DIVS.L */
			    { 0,    0,    0 },  /* DIVS.W */
			    { 0,    0,    0 },  /* DIVU.L */
			    { 0,    0,    0 }}; /* DIVU.W */


                        /*     RnRm      RnEa      EaRn */
unsigned char alopm[][3] ={ { addNULL,  addNULL,  addFEA  },  /* ADD */
			    { addNULL,  addNULL,  addFEA  },  /* ADDA.L */
			    { addNULL,  addNULL,  addFEA  },  /* ADDA.W */
			    { addNULL,  addFEA ,  addFEA  },  /* AND */
			    { addNULL,  addFEA ,  addNULL },  /* EOR */
			    { addNULL,  addFEA ,  addFEA  },  /* OR */
			    { addNULL,  addFEA ,  addFEA  },  /* SUB */
			    { addNULL,  addNULL,  addFEA  },  /* SUBA.L */
			    { addNULL,  addNULL,  addFEA  },  /* SUBA.W */
			    { addNULL,  addNULL,  addFEA  },  /* CMP */
			    { addFIEA,  addNULL,  addFIEA },  /* MULS.L */
			    { addFEA ,  addNULL,  addFEA  },  /* MULS.W */
			    { addFIEA,  addNULL,  addFIEA },  /* MULU.L */
			    { addFEA ,  addNULL,  addFEA  },  /* MULU.W */
			    { addFIEA,  addNULL,  addFIEA },  /* DIVS.L */
			    { addNULL,  addNULL,  addFEA  },  /* DIVS.W */
			    { addFIEA,  addNULL,  addFIEA },  /* DIVU.L */
			    { addNULL,  addNULL,  addFEA  }}; /* DIVU.W */


unsigned char feah[] =    {  0,  /* unknown */
			     0,  /* other */
			     0,  /* %dn */
			     0,  /* %an */
			     1,  /* (%an) */
			     0,  /* (%an)+ */
			     2,  /* -(%an) */
			     2,  /* d(%an) */
			     4,  /* d(%an,%dn) */
			     4,  /* (bd,%an,%dn.size*scale) */
			     2,  /* d(%pc) */
			     4,  /* d(%pc,%dn) */
			     4,  /* (bd,%pc,%dn.size*scale) */
			     2,  /* xxx.W */
			     1,  /* xxx.L */
			     4,  /* &xxx */
			     0,  /* %dn:%dm */
			     0}; /* %fpn */


unsigned char feab[] =    {  0,  /* unknown */
			     0,  /* other */
			     0,  /* %dn */
			     0,  /* %an */
			     1,  /* (%an) */
			     2,  /* (%an)+ */
			     0,  /* -(%an) */
			     0,  /* d(%an) */
			     2,  /* d(%an,%dn) */
			     8,  /* (bd,%an,%dn.size*scale) */
			     0,  /* d(%pc) */
			     2,  /* d(%pc,%dn) */
			     8,  /* (bd,%pc,%dn.size*scale) */
			     0,  /* xxx.W */
			     3,  /* xxx.L */
			     0,  /* &xxx */
			     0,  /* %dn:%dm */
			     0 };/* %fpn */


unsigned char feat[] =    {  0,  /* unknown */
			     0,  /* other */
			     0,  /* %dn */
			     0,  /* %an */
			     1,  /* (%an) */
			     1,  /* (%an)+ */
			     2,  /* -(%an) */
			     2,  /* d(%an) */
			     0,  /* d(%an,%dn) */
			     0,  /* (bd,%an,%dn.size*scale) */
			     2,  /* d(%pc) */
			     0,  /* d(%pc,%dn) */
			     0,  /* (bd,%pc,%dn.size*scale) */
			     2,  /* xxx.W */
			     0,  /* xxx.L */
			     0,  /* &xxx */
			     0,  /* %dn:%dm */
			     0 };/* %fpn */


signed char fieah[] =	  {  0,  /* unknown */
			     0,  /* other */
			    -4,  /* %dn */
			    -4,  /* %an */
			     1,  /* (%an) */
			     4,  /* (%an)+ */
			     2,  /* -(%an) */
			     6,  /* d(%an) */
			     6,  /* d(%an,%dn) */
			    12,  /* (bd,%an,%dn.size*scale) */
			     6,  /* d(%pc) */
			     6,  /* d(%pc,%dn) */
			    12,  /* (bd,%pc,%dn.size*scale) */
			     6,  /* xxx.W */
			     5,  /* xxx.L */
			     0,  /* &xxx */
			     0,  /* %dn:%dm */
			     0}; /* %fpn */


unsigned char fieab[] =   {  0,  /* unknown */
			     0,  /* other */
			     0,  /* %dn */
			     0,  /* %an */
			     3,  /* (%an) */
			     2,  /* (%an)+ */
			     2,  /* -(%an) */
			     2,  /* d(%an) */
			     2,  /* d(%an,%dn) */
			     4,  /* (bd,%an,%dn.size*scale) */
			     4,  /* d(%pc) */
			     2,  /* d(%pc,%dn) */
			     4,  /* (bd,%pc,%dn.size*scale) */
			     0,  /* xxx.W */
			     3,  /* xxx.L */
			     0,  /* &xxx */
			     0,  /* %dn:%dm */
			     0 };/* %fpn */


unsigned char fieat[] =   {  0,  /* unknown */
			     0,  /* other */
			     0,  /* %dn */
			     0,  /* %an */
			     1,  /* (%an) */
			     1,  /* (%an)+ */
			     0,  /* -(%an) */
			     0,  /* d(%an) */
			     0,  /* d(%an,%dn) */
			     0,  /* (bd,%an,%dn.size*scale) */
			     0,  /* d(%pc) */
			     0,  /* d(%pc,%dn) */
			     0,  /* (bd,%pc,%dn.size*scale) */
			     2,  /* xxx.W */
			     0,  /* xxx.L */
			     0,  /* &xxx */
			     0,  /* %dn:%dm */
			     0 };/* %fpn */


signed char ceah[] =      {  0,  /* unknown */
			     0,  /* other */
			     0,  /* %dn */
			     0,  /* %an */
			    -2,  /* (%an) */
			     0,  /* (%an)+ */
			    -2,  /* -(%an) */
			    -2,  /* d(%an) */
			    -6,  /* d(%an,%dn) */
			     4,  /* (bd,%an,%dn.size*scale) */
			     2,  /* d(%pc) */
			     6,  /* d(%pc,%dn) */
			     4,  /* (bd,%pc,%dn.size*scale) */
			    -2,  /* xxx.W */
			    -4,  /* xxx.L */
			     0,  /* &xxx */
			     0,  /* %dn:%dm */
			     0}; /* %fpn */


unsigned char ceab[] =    {  0,  /* unknown */
			     0,  /* other */
			     0,  /* %dn */
			     0,  /* %an */
			     0,  /* (%an) */
			     2,  /* (%an)+ */
			     0,  /* -(%an) */
			     0,  /* d(%an) */
			     0,  /* d(%an,%dn) */
			     8,  /* (bd,%an,%dn.size*scale) */
			     4,  /* d(%pc) */
			     0,  /* d(%pc,%dn) */
			     8,  /* (bd,%pc,%dn.size*scale) */
			     0,  /* xxx.W */
			     0,  /* xxx.L */
			     0,  /* &xxx */
			     0,  /* %dn:%dm */
			     0 };/* %fpn */


signed char cieah[] =	  {  0,  /* unknown */
			     0,  /* other */
			    -4,  /* %dn */
			    -4,  /* %an */
			    -4,  /* (%an) */
			     4,  /* (%an)+ */
			    -4,  /* -(%an) */
			    -6,  /* d(%an) */
			   -10,  /* d(%an,%dn) */
			     8,  /* (bd,%an,%dn.size*scale) */
			     6,  /* d(%pc) */
			   -10,  /* d(%pc,%dn) */
			     8,  /* (bd,%pc,%dn.size*scale) */
			    -6,  /* xxx.W */
			    -8,  /* xxx.L */
			     0,  /* &xxx */
			     0,  /* %dn:%dm */
			     0}; /* %fpn */


unsigned char cieab[] =   {  0,  /* unknown */
			     0,  /* other */
			     0,  /* %dn */
			     0,  /* %an */
			     0,  /* (%an) */
			     2,  /* (%an)+ */
			     0,  /* -(%an) */
			     0,  /* d(%an) */
			     2,  /* d(%an,%dn) */
			     8,  /* (bd,%an,%dn.size*scale) */
			     4,  /* d(%pc) */
			     0,  /* d(%pc,%dn) */
			     8,  /* (bd,%pc,%dn.size*scale) */
			     0,  /* xxx.W */
			     0,  /* xxx.L */
			     0,  /* &xxx */
			     0,  /* %dn:%dm */
			     0 };/* %fpn */



itime30(d)
dag *d;
{
	node	*i;
	int 	m1, m2;
	int 	alop;
	int 	almode;

	i = d->inst;

	m1 = i->mode1;
	m2 = i->mode2;

	switch (d->insttype) {

	case iFLOP:
		if (m1 == FREG) {
			d->time.t30.head = floph[0];
			d->time.t30.body = 4;
			d->time.t30.tail = flopt[i->op - FLOW];
		}
		else {
			if (i->subop < SINGLE) {
				d->time.t30.head = floph[1];
				d->time.t30.body = 19;
				if (isSLOWTAIL(i->op)) {
					d->time.t30.tail = 
						flopt[i->op - FLOW] + 19;
				}
				else {
					d->time.t30.tail = 
						flopt[i->op - FLOW] + 11;
				}
			}
			else {
				d->time.t30.head = 
					floph [i->subop - SINGLE + 2];
				d->time.t30.body = 1;
				d->time.t30.tail = flopt[i->op - FLOW] + 3;
			}
			d->time.t30.head += flopea[m1];
		}
		break;

	case iFLOAD:
	case iFSTORE:
		if (m1 != FREG) {
			if (i->subop < SINGLE) {
				d->time.t30.head = floadh[1];
				d->time.t30.body = 19;
				d->time.t30.tail = floadt[1];
			}
			else {
				d->time.t30.head = floadh[i->subop-SINGLE+2];
				d->time.t30.body = 0;
				d->time.t30.tail = floadt[i->subop-SINGLE+2];
			}
			d->time.t30.head += flopea[m1];
		}
		else if (m2 != FREG) {
			if (i->subop < SINGLE) {
				d->time.t30.head = fstoreh[1];
				d->time.t30.body = 110;
				d->time.t30.tail = fstoret[1];
			}
			else {
				d->time.t30.head = fstoreh[i->subop-SINGLE+2];
				d->time.t30.body = 0;
				d->time.t30.tail = fstoret[i->subop-SINGLE+2];
			}
			d->time.t30.head += flopea[m2];
		}
		else {
			d->time.t30.head = floadh[0];
			d->time.t30.tail = floadt[0];
		}
		break;

	case iALOAD:
	case iIOP:
		switch (i->op) {

		case MOVE:
			if (((m1 == DDIR) || (m1 == ADIR)) &&
			    ((m2 >= DDIR) && (m2 <= DEC))) {
				d->time.t30.head = moveh[m2 - 2];
				d->time.t30.body = moveb[m2 - 2];
				d->time.t30.tail = movet[m2 - 2];
			}
			else {
				d->time.t30.head = feah[m1];
				d->time.t30.body = feab[m1] +
					max(feat[m1],moveeah[m2]) +
					moveeab[m2];
				d->time.t30.tail = 0;
			}
			break;
		
		case SWAP:
		case EXG:
			d->time.t30.head = 4;
			d->time.t30.body = 0;
			d->time.t30.tail = 0;
			break;
		case LEA:
			if (ceah[m1] < 0) {
				d->time.t30.head = 2 - ceah[m1];
				d->time.t30.body = 0;
				d->time.t30.tail = 0;
			}
			else {
				d->time.t30.head = ceah[m1];
				d->time.t30.body = ceab[m1] + 2;
				d->time.t30.tail = 0;
			}
			break;

		case PEA:
			if (ceah[m1] < 0) {
				d->time.t30.head = -ceah[m1];
				d->time.t30.body = 2;
				d->time.t30.tail = 2;
			}
			else {
				d->time.t30.head = ceah[m1];
				d->time.t30.body = ceab[m1] + 2;
				d->time.t30.tail = 2;
			}
			break;

		case ADD:
			alop = 0;
			goto dotalop;
		case ADDA:
			if (i->subop == LONG) {
				alop = 1;
			}
			else {
				alop = 2;
			}
			goto dotalop;
		case AND:
			alop = 3;
			goto dotalop;
		case EOR:
			alop = 4;
			goto dotalop;
		case OR:
			alop = 5;
			goto dotalop;
		case SUB:
			alop = 6;
			goto dotalop;
		case SUBA:
			if (i->subop == LONG) {
				alop = 7;
			}
			else {
				alop = 8;
			}
			goto dotalop;
		case CMP:
			alop = 9;
			goto dotalop;
		case MULS:
			if (i->subop == LONG) {
				alop = 10;
			}
			else {
				alop = 11;
			}
			goto dotalop;
		case MULU:
			if (i->subop == LONG) {
				alop = 12;
			}
			else {
				alop = 13;
			}
			goto dotalop;
		case DIVS:
			if (i->subop == LONG) {
				alop = 14;
			}
			else {
				alop = 15;
			}
			goto dotalop;
		case DIVSL:
			alop = 14;
			goto dotalop;
		case DIVU:
			if (i->subop == LONG) {
				alop = 16;
			}
			else {
				alop = 17;
			}
			goto dotalop;
		case DIVUL:
			alop = 16;

                     dotalop:
			if (!isDnorAn(i->mode1)) {
				almode = EaRn;
			}
			else if (!isDnorAn(i->mode2)) { 
				almode = RnEa;
			}
			else {
				almode = RnRm;
			}

			switch (alopm[alop][almode]) {

			case  addNULL:
				d->time.t30.head = aloph[alop][almode];
				d->time.t30.body = alopb[alop][almode];
				d->time.t30.tail = alopt[alop][almode];
				break;

			case  addFEA:
				d->time.t30.head = feah[m1];
				d->time.t30.body = feab[m1] +
					max(feat[m1],aloph[alop][almode]) +
					alopb[alop][almode];
				d->time.t30.tail = alopt[alop][almode];
				break;

			case  addFIEA:
				if (fieah[m1] < 0) {
					d->time.t30.head = aloph[alop][almode] -
					fieah[m1];
					d->time.t30.body = alopb[alop][almode];
					d->time.t30.tail = alopt[alop][almode];
				}
				else {
					d->time.t30.head = fieah[m1];
					d->time.t30.body = fieab[m1] +
						max(
						    fieat[m1],
						    aloph[alop][almode]
						) +
						alopb[alop][almode];
					d->time.t30.tail = alopt[alop][almode];
				}
				break;

			}
			break;

		case MOVEQ:
		case ADDQ:
		case SUBQ:
			if (isDnorAn(i->mode2)) {
				d->time.t30.head = 2;
				d->time.t30.body = 0;
				d->time.t30.tail = 0;
			}
			else {
				d->time.t30.head = feah[m1];
				d->time.t30.body = feab[m1] + feat[m1] + 2;
				d->time.t30.tail = 1;
			}
			break;

		case CLR:
		case SCC:
		case SCS:
		case SEQ:
		case SF:
		case SGE:
		case SGT:
		case SHI:
		case SLE:
		case SLS:
		case SLT:
		case SMI:
		case SNE:
		case SPL:
		case ST:
			if (isDnorAn(i->mode1)) {
				if (i->op == CLR) {
					d->time.t30.head = 2;
				}
				else {
					d->time.t30.head = 4;
				}
				d->time.t30.body = 0;
				d->time.t30.tail = 0;
			}
			else {
				d->time.t30.head = abs(ceah[m1]);
				if (i->op == CLR) {
					d->time.t30.body = ceab[m1] + 2;
				}
				else {
					d->time.t30.body = ceab[m1] + 4;
				}
				d->time.t30.tail = 1;
			}
			break;

		case NEG:
		case NOT:
			if (isDnorAn(i->mode1)) {
				d->time.t30.head = 2;
				d->time.t30.body = 0;
				d->time.t30.tail = 0;
			}
			else {
				d->time.t30.head = feah[m1];
				d->time.t30.body = feab[m1] + feat[m1] + 2;
				d->time.t30.tail = 1;
			}
			break;

		case EXT:
		case EXTB:
			d->time.t30.head = feah[m1];
			d->time.t30.body = feab[m1] + feat[m1] + 2;
			d->time.t30.tail = 1;
			break;

		case TST:
			if (isDnorAn(i->mode1)) {
				d->time.t30.head = 0;
				d->time.t30.body = 2;
				d->time.t30.tail = 0;
			}
			else {
				d->time.t30.head = feah[m1];
				d->time.t30.body = feab[m1] + feat[m1] + 2;
				d->time.t30.tail = 0;
			}
			break;

		case ASR:
		case LSL:
		case LSR:
			if (isDnorAn(i->mode1)) {
				d->time.t30.head = 6;
				d->time.t30.body = 0;
				d->time.t30.tail = 0;
			}
			else if (isIMMED(i->mode1)) {
				d->time.t30.head = 4;
				d->time.t30.body = 0;
				d->time.t30.tail = 0;
			}
			else {
				d->time.t30.head = feah[m1];
				d->time.t30.body = feab[m1] + feat[m1] + 4;
				d->time.t30.tail = 0;
			}
			break;
		case ASL:
			if (isDnorAn(i->mode1)) {
				d->time.t30.head = 4;
				d->time.t30.body = 4;
				d->time.t30.tail = 0;
			}
			else if (isIMMED(i->mode1)) {
				d->time.t30.head = 2;
				d->time.t30.body = 4;
				d->time.t30.tail = 0;
			}
			else {
				d->time.t30.head = feah[m1];
				d->time.t30.body = feab[m1] + feat[m1] + 6;
				d->time.t30.tail = 0;
			}
			break;
		case ROL:
		case ROR:
			if (isDnorAn(i->mode1)) {
				d->time.t30.head = 6;
				d->time.t30.body = 2;
				d->time.t30.tail = 0;
			}
			else if (isIMMED(i->mode1)) {
				d->time.t30.head = 4;
				d->time.t30.body = 2;
				d->time.t30.tail = 0;
			}
			else {
				d->time.t30.head = feah[m1];
				d->time.t30.body = feab[m1] + feat[m1] + 6;
				d->time.t30.tail = 0;
			}
			break;

		case BTST:
		case BCHG:
		case BCLR:
		case BSET:
			if (isDnorAn(i->mode2)) {
				d->time.t30.head = i->op == BTST ? 4 : 6;
				d->time.t30.body = 0;
				d->time.t30.tail = 0;
			}
			else if (isIMMED(i->mode1)) {
				d->time.t30.head = fieah[m1];
				d->time.t30.body = fieab[m1] + 
					fieat[m1] + 
					i->op == BTST ? 4 : 6;
				d->time.t30.tail = 0;
			}
			else {
				d->time.t30.head = feah[m2];
				d->time.t30.body = feab[m2] + 
					feat[m2] +
					i->op == BTST ? 4 : 6;
				d->time.t30.tail = 0;
			}
			break;

		case BFTST:
		case BFCHG:
		case BFCLR:
		case BFSET:
			if (isDnorAn(i->mode1)) {
				d->time.t30.head = i->op == BFTST ? 8 : 14;
				d->time.t30.body = 0;
				d->time.t30.tail = 0;
			}
			else {
				if (cieah[m1] < 0) {
					d->time.t30.head = 
						(i->op == BFTST ? 8 : 14) -
					        cieah[m1];
					d->time.t30.body = 0;
					d->time.t30.tail = 0;
				}
				else {
					d->time.t30.head = cieah[m1];
					d->time.t30.body = cieab[m1] + 
						i->op == BTST ? 10 : 22;
					d->time.t30.tail = 0;
				}
			}
			break;

		case BFINS:
			if (isDnorAn(i->mode2)) {
				d->time.t30.head = 12;
				d->time.t30.body = 0;
				d->time.t30.tail = 0;
			}
			else {
				if (cieah[m2] < 0) {
					d->time.t30.head = 
						(i->op == BFTST ? 8 : 14) -
					        cieah[m2];
					d->time.t30.body = 0;
					d->time.t30.tail = 0;
				}
				else {
					d->time.t30.head = cieah[m2];
					d->time.t30.body = cieab[m2] + 12;
					d->time.t30.tail = 0;
				}
			}
			break;

		case BFEXTU:
		case BFEXTS:
		case BFFFO:
			if (isDnorAn(i->mode1)) {
				d->time.t30.head = i->op == BFFFO ? 20 : 12;
				d->time.t30.body = 0;
				d->time.t30.tail = 0;
			}
			else {
				if (cieah[m1] < 0) {
					d->time.t30.head = 
						(i->op == BFFFO ? 20 : 12) -
					        cieah[m1];
					d->time.t30.body = 0;
					d->time.t30.tail = 0;
				}
				else {
					d->time.t30.head = cieah[m1];
					d->time.t30.body = cieab[m1] + 
						i->op == BTST ? 22 : 12;
					d->time.t30.tail = 0;
				}
			}
			break;

		default:
			internal_error(
			    "Unknown inst type in itime: %d\n", i->op
			);
			break;

		}
		break;
	}
}
