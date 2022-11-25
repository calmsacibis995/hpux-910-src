/* @(#) $Revision: 70.1 $ */   
/*
 * Instruction Times for the 68030/68882
 */

#include "sched.h"

#define  isDnorAn(X) ((X == DDIR) || (X == ADIR) || (X == DPAIR))


#define  isIMMED(X)  (X == IMMEDIATE)

#define A 	0
#define B 	1
#define C 	2
#define D 	3
#define E 	4
#define F 	5

#define An 	0
#define Pc 	1

#define fea  	0
#define fiea 	1

#define dAnXn 	0
#define dPcXn 	1

/*
 *			Table to map C2's address mode to the 
 *			correct row in the "eac_sea" table
 */
byte mode_to_sea[] = {	-1,  /* unknown */
		    	-1,  /* other */
		    	 0,  /* %dn */
		    	 1,  /* %an */
		    	 2,  /* (%an) */
		    	 3,  /* (%an)+ */
		    	 4,  /* -(%an) */
		    	 5,  /* d(%an) */
		    	-1,  /* d(%an,%dn) */
		    	-1,  /* (bd,%an,%dn.size*scale) */
		    	 6,  /* d(%pc) */
		    	-1,  /* d(%pc,%dn) */
		    	-1,  /* (bd,%pc,%dn.size*scale) */
		    	 7,  /* xxx.W */
		    	 7,  /* xxx.L */
		    	 8,  /* &xxx */
		    	-1,  /* %dn:%dm */
		    	-1 };/* %fpn */

/*
 *			Table of calculate effective address times
 *			for single effective address instructions
 *
 *		 	    A    B    C    D    E    F    */
byte eac_sea[][6] =  {  {   1,   1,   1,   1,   1,   1 }, /* Dn     */
			{   1,   1,   1,   1,   1,   1 }, /* An     */
			{   1,   1,   1,   1,   1,   1 }, /* (An)   */
			{   1,   2,   2,   1,   2,   2 }, /* (An)+  */
			{   1,   2,   2,   1,   2,   2 }, /* -(An)  */
			{   1,   2,   2,   2,   2,   2 }, /* d(An)  */
			{   3,   4,   1,   3,   3,   3 }, /* d(Pc)  */
			{   1,   2,   1,   2,   1,   2 }, /* (xxx)  */
			{   1,   1,   1,   1,   1,   1 }  /* #data  */
		      };


/*
 *			Table of calculate effective address times
 *			for brief format extension word instructions
 *
 *			      A    B    C    */
byte eac_bfew[][3] =	{ {   3,   4,   3 }, /* (d,An,Xn) */
			  {   5,   0,   5 }, /* (d,Pc,Xn) */
			};


/*
 *			Table of calculate effective address needs the xu
 *			times for brief format extension word instructions
 *
 *			      A    B    C    */
byte eac_bfew_xu[][3] = { {   3,   3,   3 }, /* (d,An,Xn) */
			  {   4,   0,   4 }, /* (d,Pc,Xn) */
			};


/*
 *			Table of calculate effective address times
 *			for full format extension word instructions
 *
 *			     An   Pc    */
byte eac_ffew[][2] =	{ {   7,   8 }, /* fea  */
			  {   8,   9 }, /* fiea */
				};


/*
 *			Table of calculate effective address needs the xu
 *			times for full format extension word instructions
 *
 *			      An   Pc    */
byte eac_ffew_xu[][2] = { {   4,   5 }, /* fea  */
			  {   5,   6 }, /* fiea */
			};

/*
 *			Table to map C2's address mode to the 
 *			correct row in the "eac_mov" table
 */
byte mode_to_mov_src[] = {	-1,  /* unknown */
		    		-1,  /* other */
		    		 0,  /* %dn */
		    		 1,  /* %an */
		    		 2,  /* (%an) */
		    		 3,  /* (%an)+ */
		    		 4,  /* -(%an) */
		    		 5,  /* d(%an) */
		    		 9,  /* d(%an,%dn) */
		    		-1,  /* (bd,%an,%dn.size*scale) */
		    		 7,  /* d(%pc) */
		    		10,  /* d(%pc,%dn) */
		    		-1,  /* (bd,%pc,%dn.size*scale) */
		    		 6,  /* xxx.W */
		    		 6,  /* xxx.L */
		    		 8,  /* &xxx */
		    		-1,  /* %dn:%dm */
		    		 0 };/* %fpn */

/*
 *			Table to map C2's address mode to the 
 *			correct column in the "eac_mov" table
 */
byte mode_to_mov_dst[] = {	-1,  /* unknown */
		    		-1,  /* other */
		    		 0,  /* %dn */
		    		 0,  /* %an */
		    		 1,  /* (%an) */
		    		 2,  /* (%an)+ */
		    		 3,  /* -(%an) */
		    		 4,  /* d(%an) */
		    		 6,  /* d(%an,%dn) */
		    		-1,  /* (bd,%an,%dn.size*scale) */
		    		-1,  /* d(%pc) */
		    		-1,  /* d(%pc,%dn) */
		    		-1,  /* (bd,%pc,%dn.size*scale) */
		    		 5,  /* xxx.W */
		    		 5,  /* xxx.L */
		    		-1,  /* &xxx */
		    		-1,  /* %dn:%dm */
		    		 0 };/* %fpn */


/*
 *			Table of calculate effective address times for
 *			move instructions with a single or brief format
 *			source and destination effective address 
 *
		      /*  Dn  (An) (An)+ -(An) d(An) (xxx) (d,An,Xn) */
byte eac_mov[][8] =  {  {  1,   1,   1,    1,    1,    1,    3 }, /* Dn     */
		  	{  1,   1,   2,    2,    2,    1,    3 }, /* An     */
		  	{  1,   1,   2,    2,    2,    1,    4 }, /* (An)   */
		  	{  1,   2,   2,    2,    2,    2,    4 }, /* (An)+  */
		  	{  1,   2,   2,    2,    2,    2,    4 }, /* -(An)  */
		  	{  1,   2,   2,    2,    2,    2,    4 }, /* d(An)  */
		  	{  1,   1,   2,    2,    2,    2,    4 }, /* (xxx)  */
		  	{  3,   3,   3,    3,    4,    4,    8 }, /* d(Pc)  */
		  	{  1,   1,   2,    2,    2,    2,    3 }, /* #data  */
		  	{  4,   4,   5,    5,    5,    5,    8 }, /*(d,An,Xn)*/
		  	{  5,   5,   6,    6,    6,    6,    9 }  /*(d,Pc,Xn)*/
	              };


/*
 *			Table of calculate effective address needs the xu
 *			times for move instructions with a single or brief 
 *			format source and destination effective address 
 *
/*  			   Dn  (An) (An)+ -(An) d(An) (xxx) (d,An,Xn) */
byte eac_mov_xu[][8] = { {  0,   0,   0,    0,    0,    0,    3 }, /* Dn     */
		     	 {  0,   0,   0,    0,    0,    0,    3 }, /* An     */
		     	 {  0,   0,   0,    0,    0,    0,    3 }, /* (An)   */
		     	 {  0,   0,   0,    0,    0,    0,    3 }, /* (An)+  */
		     	 {  0,   0,   0,    0,    0,    0,    3 }, /* -(An)  */
		     	 {  0,   0,   0,    0,    0,    0,    3 }, /* d(An)  */
		     	 {  0,   0,   0,    0,    0,    0,    3 }, /* (xxx)  */
		     	 {  0,   0,   0,    0,    0,    0,    7 }, /* d(Pc)  */
		     	 {  0,   1,   0,    0,    0,    0,    3 }, /* #data  */
		     	 {  3,   3,   3,    3,    3,    3,    3 }, /*(d,An,Xn)*/
		     	 {  4,   4,   4,    4,    4,    4,    4 }  /*(d,Pc,Xn)*/
	               };

/*
 *			Table of calculate effective address times for
 *			the source of a move instruction with a full 
 *			format extension source or destination 
 *			effective address 
 */
byte eac_ffew_mov_src[] = {	-1,  /* unknown */
		    		-1,  /* other */
			    	 1,  /* %dn */
			    	 1,  /* %an */
			    	 1,  /* (%an) */
			    	 1,  /* (%an)+ */
			    	 1,  /* -(%an) */
			    	 1,  /* d(%an) */
			    	 3,  /* d(%an,%dn) */
			    	 6,  /* (bd,%an,%dn.size*scale) */
			    	 4,  /* d(%pc) */
			    	 4,  /* d(%pc,%dn) */
			    	 6,  /* (bd,%pc,%dn.size*scale) */
			    	 1,  /* xxx.W */
			    	 1,  /* xxx.L */
			    	 1,  /* &xxx */
			    	-1,  /* %dn:%dm */
			    	-1 };/* %fpn */


/*
 *			Table of calculate effective address times for
 *			the destination of a move instruction with a full 
 *			format extension source or destination 
 *			effective address 
 */
byte eac_ffew_mov_dst[] = {	-1,  /* unknown */
		    		-1,  /* other */
		    		 1,  /* %dn */
		    		 1,  /* %an */
		    		 1,  /* (%an) */
		    		 2,  /* (%an)+ */
		    		 2,  /* -(%an) */
		    		 2,  /* d(%an) */
		    		 5,  /* d(%an,%dn) */
		    		 6,  /* (bd,%an,%dn.size*scale) */
		    		-1,  /* d(%pc) */
		    		-1,  /* d(%pc,%dn) */
		    		-1,  /* (bd,%pc,%dn.size*scale) */
		    		 2,  /* xxx.W */
		    		 2,  /* xxx.L */
		    		-1,  /* &xxx */
		    		-1,  /* %dn:%dm */
		    		-1 };/* %fpn */


/*
 *			Table of calculate effective address needs the xu
 *			times for the source of a move instruction with a 
 *			full format extension source or destination 
 *			effective address. To get the destination time you
 *			subtract 1 for all modes except (d,An,Xn) where you
 *			add one.
 */
byte eac_ffew_mov_xu[] = {	0,  /* unknown */
		    		0,  /* other */
		    		0,  /* %dn */
		    		0,  /* %an */
		    		0,  /* (%an) */
		    		0,  /* (%an)+ */
		    		0,  /* -(%an) */
		    		0,  /* d(%an) */
		    		3,  /* d(%an,%dn) */
		    		4,  /* (bd,%an,%dn.size*scale) */
		    		0,  /* d(%pc) */
		    		4,  /* d(%pc,%dn) */
		    		4,  /* (bd,%pc,%dn.size*scale) */
		    		0,  /* xxx.W */
		    		2,  /* xxx.L */
		    		0,  /* &xxx */
		    		0,  /* %dn:%dm */
		    		0 };/* %fpn */


itime40(d)
dag *d;
{
	node *i;
	int m1, m2;
	int alop;
	int almode;

	i = d->inst;

	m1 = i->mode1;
	m2 = i->mode2;

	switch (d->insttype) {

	case iFLOP:
	case iFLOAD:
	case iFSTORE:
		if (m1 == FREG) {
			if (m2 == FREG) {
				d->time.t40.eac = 1;
				d->time.t40.eac_gets_xu = 0;

				d->time.t40.cvrt = NoCvrt;
			}
			else {
				eactime(d, m2, A, A, fea);

				if (i->subop < SINGLE) {
					d->time.t40.cvrt = FltInt;
				}
			}
		}
		else {
			eactime(d, m1, A, A, fea);

			if (i->subop < SINGLE) {
				d->time.t40.cvrt = IntFlt;
			}
		}

		if ((m1 == FREG) || (i->subop <= SINGLE)) {
			d->time.t40.eaf = 1;
		}
		else {
			d->time.t40.eaf = 2;
		}

		if (i->subop == EXTENDED && !d->time.t40.cvrt) {
			d->time.t40.fcu = 4;
		}
		else {
			d->time.t40.fcu = 3;
		}

		d->time.t40.fnu = 3;

		switch (i->op) {

		case FADD:
		case FSUB:
			d->time.t40.fxu = 3;
			break;

		case FMUL:
		case FSGLMUL:
			d->time.t40.fxu = 5;
			break;

		case FDIV:
		case FSGLDIV:
			d->time.t40.fxu = 38;
			break;

		case FSQRT:
			d->time.t40.fxu = 103;
			break;

		case FMOV:
		case FABS:
		case FNEG:
			d->time.t40.fxu = 0;
			d->time.t40.fnu = 0;
			break;

		case FTEST:
		case FCMP:
			d->time.t40.fxu = 3;
			break;

		default:
			internal_error(
			    "Unknown inst type in itime40: %d\n", i->op
			);
			break;

		}
		break;


	case iALOAD:
	case iIOP:
		d->time.t40.cvrt = NoCvrt;

		switch (i->op) {

		default:
			internal_error(
			    "Unknown inst type in itime40: %d\n", i->op
			);
			break;
			

		case ADD:
		case ADDA:
		case AND:
		case EOR:
		case OR:
		case SUB:
		case SUBA:
			if (m2 == ADIR) {
			    jail:
				switch (m1) {

				default:
					internal_error(
			    		   "Bad mode 1 in ADDA: %d\n", m1
					);
					break;
				
				case DDIR:
					d->time.t40.eac = 1;
					d->time.t40.eaf = 1;
					d->time.t40.ixu = 1;
					break;

				case ADIR:
				case IMMEDIATE:
					d->time.t40.eac = 1;
					d->time.t40.eaf = 1;
					if ((i->op == SUB) || (i->op == SUBA)) {
						d->time.t40.ixu = 2;
					}
					else {
						d->time.t40.ixu = 1;
					}
					break;

				case IND:
					d->time.t40.eac = 1;
					d->time.t40.eaf = 1;
					d->time.t40.ixu = 2;
					break;

				case INC:
				case DEC:
				case ADISP:
					d->time.t40.eac = 2;
					d->time.t40.eaf = 1;
					d->time.t40.ixu = 2;
					break;

				case PDISP:
					d->time.t40.eac = 3;
					d->time.t40.eaf = 1;
					d->time.t40.ixu = 2;
					break;

				case ABS_W:
				case ABS_L:
					d->time.t40.eac = 1;
					d->time.t40.eaf = 1;
					d->time.t40.ixu = 2;
					break;

				case AINDEX:
				case AINDBASE:
				case PINDEX:
				case PINDBASE:
					eactime(d, m1, A, A, fea);
					d->time.t40.eaf = 1;
					d->time.t40.ixu = 2;
					break;
				}
			}
			else {
				if (m1 == IMMEDIATE) {
					if (m2 == DDIR) {
						d->time.t40.eac = 1;
						d->time.t40.eac_gets_xu = 0;
					}
					else {
						eactime(d, m2, A, C, fiea);
					}
				}
				else if (m1 == DDIR) {
					eactime(d, m2, A, A, fea);
				}
				else {
					eactime(d, m1, A, A, fea);
				}
				d->time.t40.eaf = 1;
				d->time.t40.ixu  = 1;
			}
			break;

		case ADDQ:
		case SUBQ:
			if ((m2 == DDIR) || (m2 == ADIR)) {
				d->time.t40.eac = 1;
				d->time.t40.eac_gets_xu = 0;
				d->time.t40.eaf = 1;
			}
			else {
				eactime(d, m2, C, B, fea);
				d->time.t40.eaf = 1;
			}

			if ((i->op == SUBQ) && (m2 == ADIR)) {
				d->time.t40.ixu = 2;
			}
			else {
				d->time.t40.ixu = 1;
			}
			break;

		case ASL:
		case ASR:
		case LSL:
		case LSR:
			if (m2 == DDIR) {
				d->time.t40.eac = 1;
				d->time.t40.eac_gets_xu = 0;
				d->time.t40.eaf = 1;
				if (m1 == DDIR) {
					if ((i->op == ASL) || (i->op == ASR)) {
						d->time.t40.ixu = 4;
					}
					else {
						d->time.t40.ixu = 3;
					}
				}
				else {
					if ((i->op == ASL) || (i->op == ASR)) {
						d->time.t40.ixu = 3;
					}
					else {
						d->time.t40.ixu = 2;
					}
				}
			}
			else {
				eactime(d, m2, A, A, fea);
				d->time.t40.eaf = 1;
				if (i->op == ASL) {
					d->time.t40.ixu = 3;
				}
				else {
					d->time.t40.ixu = 2;
				}
			}
			break;

		case BCHG:
		case BCLR:
		case BSET:
		case BTST:
			if (m1 == DDIR) {
				if (m2 == DDIR) {
					d->time.t40.eac = 1;
					d->time.t40.eac_gets_xu = 0;
				}
				else {
					eactime(d, m2, A, A, fea);
				}

				if (i->op == BTST) {
					d->time.t40.ixu = 2;
				}
				else {
					d->time.t40.ixu = 4;
				}
			}
			else {
				if (m2 == DDIR) {
					d->time.t40.eac = 1;
					d->time.t40.eac_gets_xu = 0;
				}
				else {
					eactime(d, m2, D, A, fiea);
				}

				if (i->op == BTST) {
					d->time.t40.ixu = 1;
				}
				else {
					d->time.t40.ixu = 3;
				}
			}
			d->time.t40.eaf = 1;
			break;

		case BFCHG:
		case BFCLR:
		case BFSET:
			if (m1 == DDIR) {
				d->time.t40.eac = 1;
				d->time.t40.eaf = 1;
				d->time.t40.ixu = 6;
			}
			else {
				d->time.t40.eac = 7;
				d->time.t40.eac_gets_xu = 3;
				d->time.t40.eaf = 1;
				d->time.t40.ixu = 3;
			}
			break;

		case BFINS:
			if (m2 == DDIR) {
				d->time.t40.eac = 1;
				d->time.t40.eaf = 1;
				d->time.t40.ixu = 5;
			}
			else {
				d->time.t40.eac = 7;
				d->time.t40.eac_gets_xu = 3;
				d->time.t40.eaf = 1;
				d->time.t40.ixu = 3;
			}
			break;

		case BFEXTS:
		case BFEXTU:
		case BFFFO:
		case BFTST:
			if (m1 == DDIR) {
				d->time.t40.eac = 1;
				d->time.t40.eaf = 1;

				switch (i->op) {

				case BFEXTS:
				case BFEXTU:
					d->time.t40.ixu = 4;
					break;

				case BFFFO:
					d->time.t40.ixu = 6;
					break;

				case BFTST:
					d->time.t40.ixu = 3;
					break;

				}
				d->time.t40.ixu = 5;
			}
			else {
				d->time.t40.eac = 7;
				d->time.t40.eac_gets_xu = 3;
				d->time.t40.eaf = 1;
				d->time.t40.ixu = 2;
			}
			break;

		case SWAP:
			d->time.t40.eac = 1;
			d->time.t40.eac_gets_xu = 0;
			d->time.t40.eaf = 1;
			d->time.t40.ixu  = 2;
			break;

		case EXG:
			if ((m1 == ADIR) && (m2 == ADIR)) {
				d->time.t40.eac = 2;
			}
			else {
				d->time.t40.eac = 1;
			}
			d->time.t40.eac_gets_xu = 0;
			d->time.t40.eaf = 1;
			d->time.t40.ixu  = 1;
			break;

		case EXT:
		case EXTB:
			d->time.t40.eac = 1;
			d->time.t40.eac_gets_xu = 0;
			d->time.t40.eaf = 1;
			if (i->subop == WORD) {
				d->time.t40.ixu  = 2;
			}
			else {
				d->time.t40.ixu  = 1;
			}
			break;

		case LEA:
			if ((m1 == IND) || (m1 == ABS_W) || (m1 == ABS_L)) {
				d->time.t40.eac = 1;
				d->time.t40.eac_gets_xu = 0;
			}
			else if (m1 == ADISP) {
				d->time.t40.eac = 2;
				d->time.t40.eac_gets_xu = 0;
			}
			else if (m1 == PDISP) {
				d->time.t40.eac = 4;
				d->time.t40.eac_gets_xu = 0;
			}
			else {
				eactime(d, m1, A, A, fea);
			}
			d->time.t40.eaf = 1;
			d->time.t40.ixu = 1;
			break;
		
		case PEA:
			eactime(d, m1, A, B, fea);
			d->time.t40.eac++;
			d->time.t40.eaf = 1;
			d->time.t40.ixu = 1;
			break;

		case ROL:
		case ROR:
			if (m2 == DDIR) {
				d->time.t40.eac = 1;
				d->time.t40.eac_gets_xu = 0;
				d->time.t40.eaf = 1;
				if (m1 == DDIR) {
					d->time.t40.ixu = 4;
				}
				else {
					d->time.t40.ixu = 3;
				}
			}
			else {
				eactime(d, m2, A, A, fea);
				d->time.t40.eaf = 1;
				d->time.t40.ixu = 3;
			}
			break;

		case CLR:
		case NEG:
		case NOT:
		case TST:
			if ((m1 == DDIR) || (m1 == ADIR)) {
				d->time.t40.eac = 1;
				d->time.t40.eac_gets_xu = 0;
			}
			else {
				eactime(d, m1, A, A, fea);
			}
			d->time.t40.eaf = 1;
			d->time.t40.ixu = 1;
			break;

		case MULS:
		case MULU:
			if (i->subop == WORD) {
				eactime(d, m1, A, A, fea);
				if (i->op == MULS) {
					d->time.t40.ixu = 16;
				}
				else {
					d->time.t40.ixu = 14;
				}
			}
			else {
				eactime(d, m1, D, A, fea);
				d->time.t40.ixu = 22;
			}
			d->time.t40.eaf = 1;
			break;

		case DIVS:
		case DIVU:
		case DIVSL:
		case DIVUL:
			eactime(d, m1, A, A, fea);
			d->time.t40.eaf = 1;
			if ((i->op == DIVS) || (i->op == DIVSL)) {
				d->time.t40.ixu = 43;
			}
			else {
				d->time.t40.ixu = 43;
			}
			break;

		case CMP:
			if (m2 == IMMEDIATE) {
				if (m1 == DDIR) {
					d->time.t40.eac = 1;
					d->time.t40.eac_gets_xu = 0;
					d->time.t40.eaf = 1;
				}
				else {
					eactime(d, m1, F, A, fea);
					d->time.t40.eaf = 2;
				}
				d->time.t40.ixu  = 1;
			}
			else {
				if (m1 == DDIR) {
					if ((m2 == DDIR) || (m2 == ADIR)) {
						d->time.t40.eac = 1;
						d->time.t40.eac_gets_xu = 0;
					}
					else {
						eactime(d, m2, A, A, fea);
					}
					d->time.t40.eaf = 1;
					d->time.t40.ixu  = 1;
				}
				else if (m1 == ADIR) {
					if ((m2 == DDIR) || (m2 == ADIR)) {
						d->time.t40.eac = 1;
						d->time.t40.eac_gets_xu = 0;
					}
					else {
						eactime(d, m2, E, A, fea);
					}
					d->time.t40.eaf = 1;
					if (i->subop == WORD) {
						d->time.t40.ixu  = 2;
					}
					else {
						d->time.t40.ixu  = 1;
					}
				}
			}
			break;

		case MOVEQ:
			d->time.t40.eac = 1;
			d->time.t40.eac_gets_xu = 0;
			d->time.t40.eaf = 1;
			d->time.t40.ixu  = 1;
			break;

		case MOVE:
			if (m2 == ADIR) {
				/*
				 * move to A-reg, I have no info from
				 * Motorola, for now just treat it like
				 * an ADDA
				 */
				 goto jail;
				
			}
			else {
				if ((m1 == AINDBASE) || (m1 == PINDBASE) ||
				    (m2 == AINDBASE)) {
					d->time.t40.eac = eac_ffew_mov_src[m1] + 
						 eac_ffew_mov_dst[m2];
					if (eac_ffew_mov_xu[m1] != -1) {
						d->time.t40.eac_gets_xu = 
							eac_ffew_mov_xu[m1];
					}
					else {
						d->time.t40.eac_gets_xu = 
							eac_ffew_mov_src[m1] +
							eac_ffew_mov_xu[m2] +
							(m2 == PINDEX)
							  ?  1
							  : -1;
					}
				}
				else {
					d->time.t40.eac = eac_mov[mode_to_mov_src[m1]]
							[mode_to_mov_dst[m2]];
					d->time.t40.eac_gets_xu = 
						eac_mov_xu[mode_to_mov_src[m1]]
							  [mode_to_mov_dst[m2]];
				}
			}
			d->time.t40.eaf = 1;
			d->time.t40.ixu  = 1;
			break;
		}
	}

	if (d->time.t40.eac <= 0) {
		internal_error("Bad eac time in itime40: %d\n", i->op);
	}
}

eactime(d, m, t1, t2, t3)
dag *d;
int  m;
int  t1;
int  t2;
int  t3;
{
	switch (m) {
	case UNKNOWN:
	case OTHER:
	case DPAIR:
	case FREG:
		internal_error("unknown mode in eactime");
		return(0);
	case DDIR:
	case ADIR:
	case IND:
	case INC:
	case DEC:
	case ADISP:
	case PDISP:
	case ABS_W:
	case ABS_L:
	case IMMEDIATE:
		d->time.t40.eac = eac_sea[mode_to_sea[m]][t1];
		d->time.t40.eac_gets_xu = 0;
		break;

	case AINDEX:
		d->time.t40.eac = eac_bfew[dAnXn][t2];
		d->time.t40.eac_gets_xu = eac_bfew_xu[dAnXn][t2];
		break;

	case PINDEX:
		d->time.t40.eac = eac_bfew[dPcXn][t2];
		d->time.t40.eac_gets_xu = eac_bfew_xu[dPcXn][t2];
		break;

	case AINDBASE:
		d->time.t40.eac = eac_ffew[An][t3];
		d->time.t40.eac_gets_xu = eac_ffew_xu[An][t3];
		break;

	case PINDBASE:
		d->time.t40.eac = eac_ffew[Pc][t3];
		d->time.t40.eac_gets_xu = eac_ffew_xu[Pc][t3];
		break;
	}
}
