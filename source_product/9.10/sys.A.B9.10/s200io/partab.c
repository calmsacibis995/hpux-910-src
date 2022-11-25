/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/partab.c,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:16:05 $
 */
/* HPUX_ID: @(#)partab.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


/*
 * Table giving parity for characters and indicating
 * character classes to tty driver.  In particular,
 * if the low 6 bits are 0, then the character needs
 * no special processing on output.
 */

char partab[] = {
	0001,0201,0201,0001,0201,0001,0001,0201,
	0202,0004,0003,0205,0007,0206,0201,0001,
	0201,0001,0001,0201,0001,0201,0201,0001,
	0001,0201,0201,0001,0201,0001,0001,0201,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0201,

	/*  all characters above 127 that are considered printable */
	0001,0001,0001,0001,0001,0001,0001,0001,
	0001,0001,0001,0001,0001,0001,0001,0001,
	0001,0001,0001,0001,0001,0001,0001,0001,
	0001,0001,0001,0001,0001,0001,0001,0001,
	0001,0000,0000,0000,0000,0000,0000,0000,
	0000,0000,0000,0000,0000,0000,0000,0000,
	0000,0000,0000,0000,0000,0000,0000,0000,
	0000,0000,0000,0000,0000,0000,0000,0000,
	0000,0000,0000,0000,0000,0000,0000,0000,
	0000,0000,0000,0000,0000,0000,0000,0000,
	0000,0000,0000,0000,0000,0000,0000,0000,
	0000,0000,0000,0000,0000,0000,0000,0000,
	0000,0000,0000,0000,0000,0000,0000,0000,
	0000,0000,0000,0000,0000,0000,0000,0000,
	0000,0000,0000,0000,0000,0000,0000,0000,
	0000,0000,0000,0000,0000,0000,0000,0001,
};

/* this table is for quick calculation of odd parity */
/*     used by drivers (ti9914.c)	*/

char odd_partab[] = {
	0200,0001,0002,0203,0004,0205,0206,0007,
	0010,0211,0212,0013,0214,0015,0016,0217,
	0020,0221,0222,0023,0224,0025,0026,0227,
	0230,0031,0032,0233,0034,0235,0236,0037,
	0040,0241,0242,0043,0244,0045,0046,0247,
	0250,0051,0052,0253,0054,0255,0256,0057,
	0260,0061,0062,0263,0064,0265,0266,0067,
	0070,0271,0272,0073,0274,0075,0076,0277,

	0100,0301,0302,0103,0304,0105,0106,0307,
	0310,0111,0112,0313,0114,0315,0316,0117,
	0320,0121,0122,0323,0124,0325,0326,0127,
	0130,0331,0332,0133,0334,0135,0136,0337,
	0340,0141,0142,0343,0144,0345,0346,0147,
	0150,0351,0352,0153,0354,0155,0156,0357,
	0160,0361,0362,0163,0364,0165,0166,0367,
	0370,0171,0172,0373,0174,0375,0376,0177
};
