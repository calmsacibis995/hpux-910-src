#ifdef __cplusplus
   #include <stdio.h>
   extern "C" {
     extern void yyerror(char *);
     extern int yylex();
   }
#endif	/* __cplusplus */ 

# line 2 "parser.y"
/*
 *  $Header: parser.y,v 1.1.109.5 92/02/28 15:59:32 ash Exp $
 */

/*%Copyright%*/
/************************************************************************
*									*
*	GateD, Release 2						*
*									*
*	Copyright (c) 1990,1991,1992 by Cornell University		*
*	    All rights reserved.					*
*									*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY		*
*	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT		*
*	LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY		*
*	AND FITNESS FOR A PARTICULAR PURPOSE.				*
*									*
*	Royalty-free licenses to redistribute GateD Release		*
*	2 in whole or in part may be obtained by writing to:		*
*									*
*	    GateDaemon Project						*
*	    Information Technologies/Network Resources			*
*	    143 Caldwell Hall						*
*	    Cornell University						*
*	    Ithaca, NY 14853-2602					*
*									*
*	GateD is based on Kirton's EGP, UC Berkeley's routing		*
*	daemon	 (routed), and DCN's HELLO routing Protocol.		*
*	Development of Release 2 has been supported by the		*
*	National Science Foundation.					*
*									*
*	Please forward bug fixes, enhancements and questions to the	*
*	gated mailing list: gated-people@gated.cornell.edu.		*
*									*
*	Authors:							*
*									*
*		Jeffrey C Honig <jch@gated.cornell.edu>			*
*		Scott W Brim <swb@gated.cornell.edu>			*
*									*
*************************************************************************
*									*
*      Portions of this software may fall under the following		*
*      copyrights:							*
*									*
*	Copyright (c) 1988 Regents of the University of California.	*
*	All rights reserved.						*
*									*
*	Redistribution and use in source and binary forms are		*
*	permitted provided that the above copyright notice and		*
*	this paragraph are duplicated in all such forms and that	*
*	any documentation, advertising materials, and other		*
*	materials related to such distribution and use			*
*	acknowledge that the software was developed by the		*
*	University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote		*
*	products derived from this software without specific		*
*	prior written permission.  THIS SOFTWARE IS PROVIDED		*
*	``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,	*
*	INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF	*
*	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.		*
*									*
************************************************************************/


#include	"include.h"
#include	<sys/stat.h>
#include	"parse.h"
#include	"rip.h"
#include	"hello.h"
#include	"egp.h"
#include	"bgp.h"
#include	"snmp.h"

char parse_error[BUFSIZ];
char *parse_filename;

static	int	parse_group_index;
static	proto_t	parse_proto;
static	proto_t	parse_prop_proto;
static	gw_entry	**parse_gwlist;
static	gw_entry	*parse_gwp;
static  char *parse_serv_proto;
#ifdef	PROTO_EGP
static	struct egpngh *ngp, egp_group, *gr_ngp;
#endif	/* PROTO_EGP */
#ifdef	PROTO_BGP
static	bgpPeer *bnp;
#endif	/* PROTO_BGP */

char *parse_directory;
int parse_state;
proto_t protos_seen;
metric_t parse_metric;
flag_t	parse_flags;
pref_t	parse_preference;
sockaddr_un parse_addr;

static void yyerror();			/* Log a parsing error */

#define	PARSE_ERROR	yyerror(parse_error);	YYERROR;
#define	PROTO_SEEN	if (protos_seen & parse_proto) {\
    sprintf(parse_error, "parse_proto_seen: duplicate %s clause",\
		gd_lower(trace_bits(rt_proto_bits, parse_proto)));\
	PARSE_ERROR; } else protos_seen |= parse_proto


# line 109 "parser.y"
typedef union  {
	int	num;
	char	*ptr;
	flag_t	flag;
	time_t	time;
	as_t	as;
	proto_t	proto;
	metric_t metric;
	pref_t	pref;
	if_entry	*ifp;
	adv_entry	*adv;
	gw_entry	*gwp;
	sockaddr_un	sockaddr;
	dest_mask	dm;
} YYSTYPE;
# define EOS 257
# define UNKNOWN 258
# define NUMBER 259
# define STRING 260
# define HNAME 261
# define T_DIRECT 262
# define T_INTERFACE 263
# define T_PROTO 264
# define T_METRIC 265
# define T_LEX 266
# define T_PARSE 267
# define T_CONFIG 268
# define T_DEFAULT 269
# define T_YYDEBUG 270
# define T_YYSTATE 271
# define T_YYQUIT 272
# define T_INCLUDE 273
# define T_DIRECTORY 274
# define T_ON 275
# define T_OFF 276
# define T_QUIET 277
# define T_POINTOPOINT 278
# define T_SUPPLIER 279
# define T_GATEWAY 280
# define T_PREFERENCE 281
# define T_DEFAULTMETRIC 282
# define T_ASIN 283
# define T_ASOUT 284
# define T_NEIGHBOR 285
# define T_METRICOUT 286
# define T_GENDEFAULT 287
# define T_NOGENDEFAULT 288
# define T_DEFAULTIN 289
# define T_DEFAULTOUT 290
# define T_EGP 291
# define T_GROUP 292
# define T_VERSION 293
# define T_MAXUP 294
# define T_SOURCENET 295
# define T_P1 296
# define T_P2 297
# define T_PKTSIZE 298
# define T_BGP 299
# define T_HOLDTIME 300
# define T_LINKTYPE 301
# define T_INTERNAL 302
# define T_HORIZONTAL 303
# define T_TRUSTEDGATEWAYS 304
# define T_SOURCEGATEWAYS 305
# define T_RIP 306
# define T_NORIPOUT 307
# define T_NORIPIN 308
# define T_HELLO 309
# define T_NOHELLOOUT 310
# define T_NOHELLOIN 311
# define T_REDIRECT 312
# define T_NOICMPIN 313
# define T_ICMP 314
# define T_SNMP 315
# define T_PORT 316
# define T_PASSWD 317
# define T_PASSIVE 318
# define T_STATIC 319
# define T_ANNOUNCE 320
# define T_NOANNOUNCE 321
# define T_LISTEN 322
# define T_NOLISTEN 323
# define T_MARTIANS 324
# define T_PROPAGATE 325
# define T_ACCEPT 326
# define T_RESTRICT 327
# define T_NORESTRICT 328
# define T_MASK 329
# define T_AS 330
# define T_OPTIONS 331
# define T_NOINSTALL 332
# define T_TRACEOPTIONS 333
# define T_TRACEFILE 334
# define T_REPLACE 335
# define T_ALL 336
# define T_NONE 337
# define T_GENERAL 338
# define T_EXTERNAL 339
# define T_ROUTE 340
# define T_UPDATE 341
# define T_KERNEL 342
# define T_TASK 343
# define T_TIMER 344
# define T_NOSTAMP 345
# define T_MARK 346
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif

/* __YYSCLASS defines the scoping/storage class for global objects
 * that are NOT renamed by the -p option.  By default these names
 * are going to be 'static' so that multi-definition errors
 * will not occur with multiple parsers.
 * If you want (unsupported) access to internal names you need
 * to define this to be null so it implies 'extern' scope.
 * This should not be used in conjunction with -p.
 */
#ifndef __YYSCLASS
# define __YYSCLASS static
#endif
YYSTYPE yylval;
__YYSCLASS YYSTYPE yyval;
typedef int yytabelem;
# define YYERRCODE 256

# line 2642 "parser.y"


/*
 *	Log any parsing errors
 */
static void
yyerror(s)
char *s;
{
	const char *cp;

	trace(TR_ALL, 0, NULL);
	tracef("parse: %s %s ",
		parse_where(),
		s);

	switch (yychar) {
	case STRING:
	case UNKNOWN:
	case HNAME:
	    tracef("at '%s'",
		   yylval.ptr);
	    break;
	case NUMBER:
	    tracef("at '%d'",
		   yylval.num);
	    break;
	default:
	    cp = parse_keyword_lookup(yychar);
	    if (cp) {
		tracef("at '%s'",
		   cp);
	    }
	}
	trace(TR_ALL, LOG_ERR, NULL);
	trace(TR_ALL, 0, NULL);
}
__YYSCLASS yytabelem yyexca[] ={
-1, 0,
	0, 1,
	263, 47,
	291, 72,
	299, 72,
	306, 72,
	309, 72,
	312, 72,
	315, 72,
	319, 185,
	324, 47,
	325, 194,
	326, 194,
	330, 47,
	331, 47,
	-2, 0,
-1, 1,
	0, -1,
	-2, 0,
-1, 2,
	0, 2,
	263, 47,
	291, 72,
	299, 72,
	306, 72,
	309, 72,
	312, 72,
	315, 72,
	319, 185,
	324, 47,
	325, 194,
	326, 194,
	330, 47,
	331, 47,
	-2, 0,
	};
# define YYNPROD 288
# define YYLAST 637
__YYSCLASS yytabelem yyact[]={

   228,   226,   227,   382,   108,   407,   187,   399,   428,   377,
   200,   198,   373,   233,   234,   120,   287,   121,   288,   201,
   104,   114,   203,   282,    86,   105,   117,    79,   115,   204,
    83,   153,   117,    79,   115,   365,   112,   186,   181,   157,
    44,    43,    41,   400,   185,   161,    24,   118,   379,   378,
   139,   210,   375,   374,    71,    83,    75,    76,    77,   209,
   182,   162,   190,   463,   464,   141,   208,   180,    79,   207,
    85,   289,   205,   142,   468,    82,   452,    15,   273,   206,
   148,    62,   404,   149,   179,   113,   124,   111,   123,    73,
    47,    48,    59,    51,    52,   113,    64,   111,   163,    65,
    82,   183,   211,   109,    66,    74,   403,    23,   113,   119,
   111,    96,   154,    25,    22,    95,   401,   196,   184,   185,
   326,   124,   412,   123,   487,   161,    57,    55,    58,    60,
    61,    63,    72,    67,    68,    69,    70,   113,   164,   111,
   478,   162,   117,    79,   115,   480,   429,   146,    87,   199,
    71,   419,    75,    76,    77,    35,   418,   367,   113,   190,
   111,   117,   155,    36,   185,   117,    79,   115,   313,   294,
    33,   293,    49,    34,   368,   112,    37,    62,   163,    38,
   195,   124,   479,   123,   124,    73,   123,   476,    59,   124,
   475,   123,    64,   451,   225,    65,   235,   450,   229,   232,
    66,    74,   240,   239,   238,   470,   230,   454,   408,   275,
   346,   236,   445,   328,   345,   137,   277,   444,   443,   442,
   260,   188,    57,   190,    58,    60,    61,    63,    72,    67,
    68,    69,    70,   124,   276,   123,   455,   328,   189,   124,
   364,   123,   109,   286,   363,   290,   291,   328,   285,   362,
   164,   328,   333,   336,   285,   356,   164,   278,   297,   355,
   298,   289,   354,   292,   300,   301,   304,   141,   305,   471,
   472,   337,   307,   308,   311,   142,   312,   321,   320,   329,
   330,   317,   143,   318,   411,   144,   279,   322,   145,   316,
   280,   281,   324,   283,   284,   315,    10,    11,   310,   309,
   124,   303,   123,   329,   330,   302,   335,   124,   405,   123,
    12,    13,    14,   329,   330,   285,   285,   329,   330,   296,
   299,   351,   285,   285,   124,   334,   123,   295,   306,   267,
   113,   113,   111,   111,   357,   358,   224,   223,   380,   285,
   338,    47,    48,   128,   130,   129,   366,   348,   323,   353,
   361,   262,   176,   220,   349,   359,   360,   117,    79,   115,
   218,   216,   369,   370,   117,    79,   115,   426,   124,   371,
   123,   352,   214,    16,    17,   252,   263,   264,   212,   193,
   335,   192,   410,   409,   244,   413,   414,   266,   335,   335,
   406,   191,   178,   265,   124,   427,   123,   425,   420,   175,
   421,   156,   152,   393,   151,   430,   441,   422,   416,   417,
   124,   423,   123,   446,   150,   336,   448,   449,   402,   160,
   392,   135,   125,   289,   342,   285,   388,   164,   389,   390,
   391,   344,   117,    79,   115,   394,   395,   396,   190,   336,
   124,   101,   123,    98,   457,    97,    50,   289,   461,   466,
    45,   460,   467,   141,   473,   474,   336,   465,   458,   459,
   269,   142,   110,   332,   289,   331,   325,   237,   340,   231,
   174,   341,   113,   113,   111,   111,   481,   482,   393,   483,
   172,   343,   117,    79,   115,   270,   271,   170,   485,   272,
   488,   168,   112,   166,    94,   392,   387,    84,   383,   384,
   439,   388,   126,   389,   390,   391,   254,    81,   386,   385,
   394,   395,   396,   257,   127,   246,   484,   436,   437,   456,
   433,   434,   249,   432,   116,   435,    56,    46,   347,   241,
   177,   255,   256,   117,    79,   115,   158,   440,   438,   222,
   247,   248,    78,   112,   242,     3,   243,   136,    18,    40,
   398,   274,   221,    93,   258,   259,   173,   138,    92,   431,
   397,   319,   268,   250,   251,   219,   171,    91,   381,   486,
   477,   453,   415,   350,   314,   261,    99,   100,   217,   197,
   169,   102,    90,   376,   253,   215,   167,    89,   106,   372,
   245,   213,   131,   165,    88,    32,    31,    30,    29,   138,
    28,    27,   159,    80,    21,   132,   133,   134,    20,    42,
     9,    39,     8,    26,     7,    19,     6,     5,     4,     2,
   194,     1,    -1,   447,   469,   327,   107,   122,   424,   202,
   339,   140,   147,   103,    53,    54,   462 };
__YYSCLASS yytabelem yypact[]={

    40, -3000,    40, -3000, -3000, -3000,  -217,  -136,  -277,  -285,
   193, -3000,  -185,   -87,   189,  -180,  -210,  -192, -3000, -3000,
 -3000, -3000,  -257,   374, -3000,  -111, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,  -185, -3000,
   371, -3000, -3000,  -149,  -153, -3000,   188, -3000, -3000,   186,
 -3000,  -192,  -192,   184,  -114, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,  -315, -3000,
  -232, -3000, -3000, -3000,  -233,  -227,   165, -3000,    66,    66,
  -185,  -185,  -185,   164,   223,   -24,  -226, -3000, -3000,   157,
   147, -3000, -3000,   145, -3000, -3000, -3000,   -94,   144, -3000,
  -290, -3000, -3000, -3000,   490, -3000, -3000, -3000,  -220, -3000,
   105, -3000, -3000, -3000, -3000, -3000,   370, -3000, -3000, -3000,
 -3000,   368,   364,   357,   347, -3000,   274,   135,  -196,  -292,
  -162, -3000, -3000, -3000, -3000, -3000,  -293,   -42, -3000, -3000,
 -3000, -3000, -3000,   134,   124,   122, -3000,   223,   -98,  -140,
 -3000,  -110,  -240, -3000, -3000,   121, -3000,   115, -3000,   104,
 -3000,   103, -3000,    96, -3000, -3000,    80,    79, -3000,   105,
   105,  -111,   346,   105,   105,  -240,  -111,   344,   105,   105,
  -110, -3000, -3000, -3000, -3000,   483, -3000, -3000, -3000, -3000,
 -3000, -3000,   501, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000,   259, -3000,   250, -3000,    95, -3000,   204,
 -3000,   -47, -3000, -3000, -3000,  -237, -3000, -3000, -3000,  -237,
  -237, -3000,  -117,  -117, -3000, -3000,  -203,  -193,   173,   173,
 -3000,   -98,   -88,   -90, -3000,    70,    62,  -240,  -110, -3000,
   105,   105, -3000,    48,    44,  -240,  -110, -3000,   105,   105,
 -3000,    42,    41,  -240,  -110,   -91, -3000, -3000,    38,    32,
  -240,  -110, -3000, -3000,    21,    20,  -240, -3000,   105, -3000,
 -3000,   343,    -5,   342,   340, -3000,   129,   200,    14,   162,
    91,    87,   482, -3000, -3000, -3000, -3000, -3000, -3000,  -227,
   105,   105, -3000, -3000, -3000, -3000,  -227,   105,   105, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,   105,
 -3000, -3000, -3000,  -227,   105, -3000,     5,     2,    -2,  -233,
  -233, -3000, -3000,  -193,    -8,   -13,   -17, -3000,  -295,  -106,
 -3000, -3000, -3000, -3000, -3000,  -193,  -193,   -98,  -255,  -262,
   215, -3000,  -270,    -9, -3000, -3000, -3000,  -237, -3000,   -19,
   -43,   183, -3000, -3000, -3000,  -111,    85,   105,   105,   159,
    -3, -3000,  -255, -3000, -3000, -3000,  -262, -3000, -3000, -3000,
 -3000, -3000, -3000,  -111,  -111,  -103,  -108,  -240,  -110, -3000,
 -3000, -3000,   105,   105,    98,  -113,  -113,   237,  -270, -3000,
 -3000,   -38, -3000,   -39,   -40,   -45,  -203, -3000, -3000,   173,
   173,   -60,   -64, -3000, -3000,   -49, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,   461,
 -3000, -3000,  -110,  -111,  -111, -3000,   105,  -240,  -239,   105,
  -113, -3000, -3000, -3000, -3000, -3000,    85,   -51,    85,    85,
 -3000, -3000, -3000,   -67,   -70, -3000,  -119, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,   -75,
  -112,  -233,  -233, -3000, -3000, -3000, -3000,   105,   458, -3000,
 -3000,  -203, -3000, -3000,  -135, -3000,   140, -3000, -3000 };
__YYSCLASS yytabelem yypgo[]={

     0,    21,    11,     6,     8,   636,   514,   502,   526,   635,
   634,   633,   632,    50,   631,   630,    24,    10,    60,   629,
    17,     2,   462,     1,   628,   627,    14,     4,    15,    47,
    13,   626,    23,   625,    18,    16,   624,   623,     5,   524,
     0,   622,   621,   619,   545,   618,   617,   616,   615,   614,
   613,   612,   611,   610,   609,   608,   604,   603,   507,    70,
   602,   419,   601,   600,   598,   597,   596,   595,   594,   593,
   591,   590,   589,    12,   587,   586,   585,   584,   583,     9,
   582,   580,   578,   575,   574,   573,   572,   571,   570,   569,
   568,     3,   567,   566,   565,   562,   561,   560,   559,   558,
   556,   552,   551,   550,     7,   549,   547,   539,   215 };
__YYSCLASS yytabelem yyr1[]={

     0,    42,    42,    43,    43,    44,    44,    44,    44,    44,
    44,    44,    44,    45,    45,    45,    45,    45,    46,    46,
    11,    11,    10,    10,     9,     9,     8,     8,     8,     8,
     8,     8,     8,     8,     8,     8,     8,     8,     8,     8,
     8,     8,     8,     8,     8,     8,     8,    47,    48,    48,
    48,    48,    57,    57,    58,    58,    55,    59,    60,    60,
    61,    61,    61,    29,    29,    28,    28,    20,    25,    25,
    56,    16,    49,    50,    50,    50,    50,    50,    50,    62,
    68,    69,    69,    70,    70,    70,    71,    71,    71,    71,
    71,    72,    72,    73,    73,    63,    74,    75,    75,    76,
    76,    76,    77,    77,    77,    77,    77,    78,    78,    79,
    79,    64,    80,    81,    81,    82,    82,    82,    83,    83,
    83,    83,    84,    86,    86,    86,    87,    88,    85,    85,
    85,    90,    90,    90,    90,    90,    89,    89,    91,    91,
    91,    91,    91,    91,    91,    91,    91,    65,    92,    93,
    93,    94,    94,    94,    95,    95,    95,    96,    97,    97,
    98,    98,    98,    98,    98,    98,    98,    98,    98,     5,
     5,    66,    99,   100,   100,   101,   101,   101,   102,   102,
   102,   103,   103,   104,    67,    51,    52,   106,   106,   106,
   105,   107,   108,   108,    53,    54,    54,    54,    54,    54,
    54,    54,    54,    14,    14,    14,    32,    32,    32,    33,
    33,    35,    35,    35,    34,    34,    34,    34,    15,    15,
    15,    15,    15,    38,    38,    37,    37,    37,    36,    36,
    31,    31,    31,    27,    27,    27,    22,    22,    22,    30,
    30,    26,    23,    23,    24,    24,    24,    21,    21,    21,
    21,    40,    40,    12,    12,    13,    13,     6,     6,     7,
     7,     7,     7,     2,     3,     3,    18,    18,    17,    17,
    17,    17,    19,    19,    19,    19,    19,    19,    19,    19,
    19,    39,     1,     4,     4,     4,    41,    41 };
__YYSCLASS yytabelem yyr2[]={

     0,     0,     2,     2,     4,     2,     2,     4,     4,     4,
     4,     5,     2,     7,     7,     5,     9,     9,     7,     9,
     1,     3,     3,     3,     3,     5,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     1,     2,     2,
     6,    11,     2,     4,     3,     3,    11,     1,     2,     4,
     5,     5,     3,     3,     3,     3,     5,     3,     3,     3,
     7,     3,     1,     2,     2,     2,     2,     2,     2,    11,
     1,     0,     6,     0,     6,     7,     5,     5,     9,     5,
     5,     2,     4,     3,     3,    11,     1,     0,     6,     0,
     6,     7,     5,     5,     9,     5,     5,     2,     4,     3,
     3,    11,     1,     0,     6,     0,     6,     7,     5,     5,
     5,    13,     1,     0,     6,     7,     9,     1,     0,     4,
     4,     5,     5,     5,     5,     5,     0,     4,     5,     3,
     3,     3,     5,     5,     5,     5,     5,    11,     1,     0,
     6,     0,     6,     7,     5,     5,     9,     1,     0,     4,
     5,     5,     5,     3,     5,     5,     5,     5,     5,     3,
     3,    11,     1,     0,     6,     0,     6,     7,     5,     9,
     5,     2,     4,     3,     7,     1,    10,     4,     6,     7,
     3,     1,     9,     9,     1,    21,    17,    21,    21,    21,
    17,    21,    21,     3,     3,     3,     1,     7,     7,     7,
     5,     5,     7,     7,    13,     9,    13,    13,     3,     3,
     3,     3,     3,     1,     7,     1,     7,     7,     7,     5,
     5,     7,     7,     3,     3,     7,     3,     3,     3,     3,
     5,     3,     3,     3,     3,     3,     3,     3,     7,    11,
    15,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     1,     5,     1,     5,     3,     3,
     7,     7,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     7,    11,     3,     3 };
__YYSCLASS yytabelem yychk[]={

 -3000,   -42,   -43,   -44,   -45,   -46,   -47,   -49,   -51,   -53,
   256,   257,   270,   271,   272,    37,   333,   334,   -44,   -48,
   -55,   -56,   331,   324,   263,   330,   -50,   -62,   -63,   -64,
   -65,   -66,   -67,   306,   309,   291,   299,   312,   315,   -52,
  -105,   319,   -54,   326,   325,   257,    -6,   275,   276,   259,
   257,   273,   274,   -10,    -9,   337,    -8,   336,   338,   302,
   339,   340,   291,   341,   306,   309,   314,   343,   344,   345,
   346,   264,   342,   299,   315,   266,   267,   268,   -39,   260,
   -57,   -58,   332,   287,   123,   -59,   -16,   259,   -68,   -74,
   -80,   -92,   -99,    -6,   123,   264,   264,   257,   257,   -39,
   -39,   257,    -8,   -11,   335,   257,   -58,   -31,   -27,   336,
   -22,   -21,   269,   -40,    -1,   261,   -39,   259,   -29,   336,
   -28,   -20,   -25,   -21,   -40,   257,    -7,    -6,   277,   279,
   278,    -7,    -6,    -6,    -6,   257,  -106,  -108,   -22,   -13,
   -14,   291,   299,   306,   309,   312,   -13,   -12,   306,   309,
   257,   257,   257,   125,   -27,   256,   257,   329,    46,   -60,
   -61,   265,   281,   318,   -20,   -69,   123,   -75,   123,   -81,
   123,   -93,   123,  -100,   123,   125,  -108,   256,   257,   280,
   263,   330,   -18,   263,   280,   281,   330,    -3,   263,   280,
   265,   257,   257,   257,   -22,    -1,   257,   -61,    -2,   259,
   -17,   259,   -19,   262,   269,   312,   319,   309,   306,   299,
   291,   342,   257,   -70,   257,   -76,   257,   -82,   257,   -94,
   257,  -101,  -107,   257,   257,   -26,   -23,   -21,   -40,   -20,
   -16,   123,   -28,   -30,   -26,   -17,   -16,   123,   -28,   -30,
    -2,    46,    43,    45,   125,   -71,   256,   281,   282,   263,
   304,   305,   125,   -77,   256,   281,   282,   263,   304,   305,
   125,   -83,   256,   281,   282,   298,   292,   125,   -95,   256,
   281,   282,   285,   125,  -102,   256,   281,   263,   304,   -18,
   -18,   -18,   -32,   -18,   -18,   -26,    -3,   -35,   -34,   264,
    -3,    -3,    -1,   259,   259,   257,   257,   -17,    -2,   -59,
   -30,   -30,   257,   257,   -17,    -2,   -59,   -30,   -30,   257,
   257,   -17,    -2,   259,   -84,   257,   257,   -17,    -2,   -96,
   257,   257,   -17,   -59,   -30,   123,   125,   -33,   256,   322,
   323,   123,   123,   123,   125,   -34,   256,   257,   -13,   -15,
   306,   309,   262,   319,   269,   123,   123,    46,   -29,   -29,
   -85,   -23,   -29,   -32,   257,   257,   257,   -27,   -27,   -32,
   -32,   -35,   257,   257,   257,   330,    -3,   263,   280,   -35,
   -35,    -1,   -72,   -73,   308,   307,   -78,   -79,   311,   310,
   123,   -90,   -91,   283,   284,   294,   293,   281,   286,   288,
   289,   290,   280,   263,   295,   296,   297,   -97,  -103,  -104,
   313,   125,   -18,   125,   125,   125,   -16,   -38,   123,   -28,
   -30,   125,   125,   -73,   -79,   -86,   -16,   -16,   259,   259,
   -17,    -2,   -26,   -20,   -24,   -21,   269,   -40,    -4,   259,
    -4,   -98,   286,   283,   284,   288,   280,   281,   301,   263,
   300,  -104,   257,   257,   257,   257,    -3,   -37,    -3,    -3,
   257,   257,   125,   -87,   256,   285,    58,    -2,   -16,   -16,
   -26,   -17,    -5,   302,   303,   -20,    -4,   -38,   125,   -36,
   256,   320,   321,   -38,   -38,   257,   257,   -88,   259,   257,
   257,   -27,   -27,   -23,    58,    -3,   -89,   259,   -91 };
__YYSCLASS yytabelem yydef[]={

    -2,    -2,    -2,     3,     5,     6,     0,     0,     0,     0,
     0,    12,     0,     0,     0,     0,     0,     0,     4,     7,
    48,    49,     0,     0,    57,     0,     8,    73,    74,    75,
    76,    77,    78,    80,    96,   112,   148,   172,     0,     9,
     0,   190,    10,     0,     0,    11,     0,   257,   258,     0,
    15,     0,     0,     0,    22,    23,    24,    26,    27,    28,
    29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
    39,    40,    41,    42,    43,    44,    45,    46,    20,   281,
     0,    52,    54,    55,     0,     0,     0,    71,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    13,    14,     0,
     0,    18,    25,     0,    21,    50,    53,     0,     0,   233,
   234,   236,   237,   238,   247,   251,   252,   282,     0,    63,
    64,    65,    67,    68,    69,    70,    81,   259,   260,   261,
   262,    97,   113,   149,   173,   184,     0,     0,     0,     0,
   266,   255,   256,   203,   204,   205,     0,   264,   253,   254,
    16,    17,    19,     0,     0,     0,   230,     0,     0,     0,
    58,     0,     0,    62,    66,     0,    83,     0,    99,     0,
   115,     0,   151,     0,   175,   191,     0,     0,   187,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    51,   231,   232,   235,   248,    56,    59,    60,   263,
    61,   268,   269,   272,   273,   274,   275,   276,   277,   278,
   279,   280,    79,     0,    95,     0,   111,     0,   147,     0,
   171,     0,   186,   188,   189,   266,   241,   242,   243,   266,
   266,   206,   266,   266,   239,   267,   264,     0,   264,   264,
   265,     0,     0,     0,    82,     0,     0,     0,     0,    57,
     0,     0,    98,     0,     0,     0,     0,    57,     0,     0,
   114,     0,     0,     0,     0,     0,   122,   150,     0,     0,
     0,     0,   157,   174,     0,     0,     0,    57,     0,   192,
   193,     0,     0,     0,     0,   240,     0,     0,     0,     0,
     0,     0,   249,   270,   271,    84,    85,    86,    87,     0,
    89,    90,   100,   101,   102,   103,     0,   105,   106,   116,
   117,   118,   119,   120,   128,   152,   153,   154,   155,     0,
   176,   177,   178,     0,   180,   206,     0,     0,     0,     0,
     0,   206,   206,     0,     0,     0,     0,   211,     0,   264,
   218,   219,   220,   221,   222,     0,     0,     0,     0,     0,
     0,   158,     0,     0,   196,   207,   208,   266,   210,     0,
     0,     0,   200,   212,   213,     0,   223,     0,     0,     0,
     0,   250,    88,    91,    93,    94,   104,   107,   109,   110,
   123,   129,   130,     0,     0,     0,     0,     0,     0,   139,
   140,   141,     0,     0,     0,     0,     0,   156,   179,   181,
   183,     0,   209,     0,     0,     0,   264,   215,   225,   264,
   264,     0,     0,    92,   108,     0,   131,   132,   133,   134,
   135,   138,   142,   143,   144,   244,   245,   246,   145,   283,
   146,   159,     0,     0,     0,   163,     0,     0,     0,     0,
     0,   182,   195,   197,   198,   199,   223,     0,   223,   223,
   201,   202,   121,     0,     0,   127,     0,   160,   161,   162,
   164,   165,   166,   169,   170,   167,   168,   214,   224,     0,
     0,     0,     0,   216,   217,   124,   125,     0,   284,   226,
   227,   264,   229,   136,     0,   228,   126,   285,   137 };
typedef struct { char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

__YYSCLASS yytoktype yytoks[] =
{
	"EOS",	257,
	"UNKNOWN",	258,
	"NUMBER",	259,
	"STRING",	260,
	"HNAME",	261,
	"T_DIRECT",	262,
	"T_INTERFACE",	263,
	"T_PROTO",	264,
	"T_METRIC",	265,
	"T_LEX",	266,
	"T_PARSE",	267,
	"T_CONFIG",	268,
	"T_DEFAULT",	269,
	"T_YYDEBUG",	270,
	"T_YYSTATE",	271,
	"T_YYQUIT",	272,
	"T_INCLUDE",	273,
	"T_DIRECTORY",	274,
	"T_ON",	275,
	"T_OFF",	276,
	"T_QUIET",	277,
	"T_POINTOPOINT",	278,
	"T_SUPPLIER",	279,
	"T_GATEWAY",	280,
	"T_PREFERENCE",	281,
	"T_DEFAULTMETRIC",	282,
	"T_ASIN",	283,
	"T_ASOUT",	284,
	"T_NEIGHBOR",	285,
	"T_METRICOUT",	286,
	"T_GENDEFAULT",	287,
	"T_NOGENDEFAULT",	288,
	"T_DEFAULTIN",	289,
	"T_DEFAULTOUT",	290,
	"T_EGP",	291,
	"T_GROUP",	292,
	"T_VERSION",	293,
	"T_MAXUP",	294,
	"T_SOURCENET",	295,
	"T_P1",	296,
	"T_P2",	297,
	"T_PKTSIZE",	298,
	"T_BGP",	299,
	"T_HOLDTIME",	300,
	"T_LINKTYPE",	301,
	"T_INTERNAL",	302,
	"T_HORIZONTAL",	303,
	"T_TRUSTEDGATEWAYS",	304,
	"T_SOURCEGATEWAYS",	305,
	"T_RIP",	306,
	"T_NORIPOUT",	307,
	"T_NORIPIN",	308,
	"T_HELLO",	309,
	"T_NOHELLOOUT",	310,
	"T_NOHELLOIN",	311,
	"T_REDIRECT",	312,
	"T_NOICMPIN",	313,
	"T_ICMP",	314,
	"T_SNMP",	315,
	"T_PORT",	316,
	"T_PASSWD",	317,
	"T_PASSIVE",	318,
	"T_STATIC",	319,
	"T_ANNOUNCE",	320,
	"T_NOANNOUNCE",	321,
	"T_LISTEN",	322,
	"T_NOLISTEN",	323,
	"T_MARTIANS",	324,
	"T_PROPAGATE",	325,
	"T_ACCEPT",	326,
	"T_RESTRICT",	327,
	"T_NORESTRICT",	328,
	"T_MASK",	329,
	"T_AS",	330,
	"T_OPTIONS",	331,
	"T_NOINSTALL",	332,
	"T_TRACEOPTIONS",	333,
	"T_TRACEFILE",	334,
	"T_REPLACE",	335,
	"T_ALL",	336,
	"T_NONE",	337,
	"T_GENERAL",	338,
	"T_EXTERNAL",	339,
	"T_ROUTE",	340,
	"T_UPDATE",	341,
	"T_KERNEL",	342,
	"T_TASK",	343,
	"T_TIMER",	344,
	"T_NOSTAMP",	345,
	"T_MARK",	346,
	"-unknown-",	-1	/* ends search */
};

__YYSCLASS char * yyreds[] =
{
	"-no such reduction-",
	"config : /* empty */",
	"config : statements",
	"statements : statement",
	"statements : statements statement",
	"statement : parse_statement",
	"statement : trace_statement",
	"statement : define_order define_statement",
	"statement : proto_order proto_statement",
	"statement : route_order route_statement",
	"statement : control_order control_statement",
	"statement : error EOS",
	"statement : EOS",
	"parse_statement : T_YYDEBUG onoff_option EOS",
	"parse_statement : T_YYSTATE NUMBER EOS",
	"parse_statement : T_YYQUIT EOS",
	"parse_statement : '%' T_INCLUDE string EOS",
	"parse_statement : '%' T_DIRECTORY string EOS",
	"trace_statement : T_TRACEOPTIONS trace_options_none EOS",
	"trace_statement : T_TRACEFILE string trace_replace EOS",
	"trace_replace : /* empty */",
	"trace_replace : T_REPLACE",
	"trace_options_none : trace_options",
	"trace_options_none : T_NONE",
	"trace_options : trace_option",
	"trace_options : trace_options trace_option",
	"trace_option : T_ALL",
	"trace_option : T_GENERAL",
	"trace_option : T_INTERNAL",
	"trace_option : T_EXTERNAL",
	"trace_option : T_ROUTE",
	"trace_option : T_EGP",
	"trace_option : T_UPDATE",
	"trace_option : T_RIP",
	"trace_option : T_HELLO",
	"trace_option : T_ICMP",
	"trace_option : T_TASK",
	"trace_option : T_TIMER",
	"trace_option : T_NOSTAMP",
	"trace_option : T_MARK",
	"trace_option : T_PROTO",
	"trace_option : T_KERNEL",
	"trace_option : T_BGP",
	"trace_option : T_SNMP",
	"trace_option : T_LEX",
	"trace_option : T_PARSE",
	"trace_option : T_CONFIG",
	"define_order : /* empty */",
	"define_statement : interface_statement",
	"define_statement : as_statement",
	"define_statement : T_OPTIONS option_list EOS",
	"define_statement : T_MARTIANS '{' dest_mask_list '}' EOS",
	"option_list : option",
	"option_list : option_list option",
	"option : T_NOINSTALL",
	"option : T_GENDEFAULT",
	"interface_statement : T_INTERFACE interface_init interface_list_all interface_option_list EOS",
	"interface_init : /* empty */",
	"interface_option_list : interface_option",
	"interface_option_list : interface_option_list interface_option",
	"interface_option : T_METRIC metric",
	"interface_option : T_PREFERENCE preference",
	"interface_option : T_PASSIVE",
	"interface_list_all : T_ALL",
	"interface_list_all : interface_list",
	"interface_list : interface",
	"interface_list : interface_list interface",
	"interface : interface_addr",
	"interface_addr : ip_addr",
	"interface_addr : host_name",
	"as_statement : T_AS autonomous_system EOS",
	"autonomous_system : NUMBER",
	"proto_order : /* empty */",
	"proto_statement : rip_statement",
	"proto_statement : hello_statement",
	"proto_statement : egp_statement",
	"proto_statement : bgp_statement",
	"proto_statement : redirect_statement",
	"proto_statement : snmp_statement",
	"rip_statement : T_RIP rip_init interior_option rip_group EOS",
	"rip_init : /* empty */",
	"rip_group : /* empty */",
	"rip_group : '{' rip_group_stmts '}'",
	"rip_group_stmts : /* empty */",
	"rip_group_stmts : rip_group_stmts rip_group_stmt EOS",
	"rip_group_stmts : rip_group_stmts error EOS",
	"rip_group_stmt : T_PREFERENCE preference",
	"rip_group_stmt : T_DEFAULTMETRIC metric",
	"rip_group_stmt : T_INTERFACE interface_init interface_list_all rip_interface_options",
	"rip_group_stmt : T_TRUSTEDGATEWAYS gateway_list",
	"rip_group_stmt : T_SOURCEGATEWAYS gateway_list",
	"rip_interface_options : rip_interface_option",
	"rip_interface_options : rip_interface_options rip_interface_option",
	"rip_interface_option : T_NORIPIN",
	"rip_interface_option : T_NORIPOUT",
	"hello_statement : T_HELLO hello_init interior_option hello_group EOS",
	"hello_init : /* empty */",
	"hello_group : /* empty */",
	"hello_group : '{' hello_group_stmts '}'",
	"hello_group_stmts : /* empty */",
	"hello_group_stmts : hello_group_stmts hello_group_stmt EOS",
	"hello_group_stmts : hello_group_stmts error EOS",
	"hello_group_stmt : T_PREFERENCE preference",
	"hello_group_stmt : T_DEFAULTMETRIC metric",
	"hello_group_stmt : T_INTERFACE interface_init interface_list_all hello_interface_options",
	"hello_group_stmt : T_TRUSTEDGATEWAYS gateway_list",
	"hello_group_stmt : T_SOURCEGATEWAYS gateway_list",
	"hello_interface_options : hello_interface_option",
	"hello_interface_options : hello_interface_options hello_interface_option",
	"hello_interface_option : T_NOHELLOIN",
	"hello_interface_option : T_NOHELLOOUT",
	"egp_statement : T_EGP egp_init onoff_option egp_group EOS",
	"egp_init : /* empty */",
	"egp_group : /* empty */",
	"egp_group : '{' egp_group_stmts '}'",
	"egp_group_stmts : /* empty */",
	"egp_group_stmts : egp_group_stmts egp_group_stmt EOS",
	"egp_group_stmts : egp_group_stmts error EOS",
	"egp_group_stmt : T_PREFERENCE preference",
	"egp_group_stmt : T_DEFAULTMETRIC metric",
	"egp_group_stmt : T_PKTSIZE NUMBER",
	"egp_group_stmt : T_GROUP egp_group_init egp_group_options '{' egp_peer_stmts '}'",
	"egp_group_init : /* empty */",
	"egp_peer_stmts : /* empty */",
	"egp_peer_stmts : egp_peer_stmts egp_peer_stmt EOS",
	"egp_peer_stmts : egp_peer_stmts error EOS",
	"egp_peer_stmt : T_NEIGHBOR egp_peer_init host egp_peer_options",
	"egp_peer_init : /* empty */",
	"egp_group_options : /* empty */",
	"egp_group_options : egp_group_options egp_group_option",
	"egp_group_options : egp_group_options egp_peer_option",
	"egp_group_option : T_ASIN autonomous_system",
	"egp_group_option : T_ASOUT autonomous_system",
	"egp_group_option : T_MAXUP NUMBER",
	"egp_group_option : T_VERSION NUMBER",
	"egp_group_option : T_PREFERENCE preference",
	"egp_peer_options : /* empty */",
	"egp_peer_options : egp_peer_options egp_peer_option",
	"egp_peer_option : T_METRICOUT metric",
	"egp_peer_option : T_NOGENDEFAULT",
	"egp_peer_option : T_DEFAULTIN",
	"egp_peer_option : T_DEFAULTOUT",
	"egp_peer_option : T_GATEWAY gateway",
	"egp_peer_option : T_INTERFACE interface",
	"egp_peer_option : T_SOURCENET network",
	"egp_peer_option : T_P1 time",
	"egp_peer_option : T_P2 time",
	"bgp_statement : T_BGP bgp_init onoff_option bgp_group EOS",
	"bgp_init : /* empty */",
	"bgp_group : /* empty */",
	"bgp_group : '{' bgp_group_stmts '}'",
	"bgp_group_stmts : /* empty */",
	"bgp_group_stmts : bgp_group_stmts bgp_group_stmt EOS",
	"bgp_group_stmts : bgp_group_stmts error EOS",
	"bgp_group_stmt : T_PREFERENCE preference",
	"bgp_group_stmt : T_DEFAULTMETRIC metric",
	"bgp_group_stmt : T_NEIGHBOR bgp_peer_init host bgp_peer_options",
	"bgp_peer_init : /* empty */",
	"bgp_peer_options : /* empty */",
	"bgp_peer_options : bgp_peer_options bgp_peer_option",
	"bgp_peer_option : T_METRICOUT metric",
	"bgp_peer_option : T_ASIN autonomous_system",
	"bgp_peer_option : T_ASOUT autonomous_system",
	"bgp_peer_option : T_NOGENDEFAULT",
	"bgp_peer_option : T_GATEWAY gateway",
	"bgp_peer_option : T_PREFERENCE preference",
	"bgp_peer_option : T_LINKTYPE bgp_linktype",
	"bgp_peer_option : T_INTERFACE interface",
	"bgp_peer_option : T_HOLDTIME time",
	"bgp_linktype : T_INTERNAL",
	"bgp_linktype : T_HORIZONTAL",
	"redirect_statement : T_REDIRECT redirect_init onoff_option redirect_group EOS",
	"redirect_init : /* empty */",
	"redirect_group : /* empty */",
	"redirect_group : '{' redirect_group_stmts '}'",
	"redirect_group_stmts : /* empty */",
	"redirect_group_stmts : redirect_group_stmts redirect_group_stmt EOS",
	"redirect_group_stmts : redirect_group_stmts error EOS",
	"redirect_group_stmt : T_PREFERENCE preference",
	"redirect_group_stmt : T_INTERFACE interface_init interface_list_all redirect_interface_options",
	"redirect_group_stmt : T_TRUSTEDGATEWAYS gateway_list",
	"redirect_interface_options : redirect_interface_option",
	"redirect_interface_options : redirect_interface_options redirect_interface_option",
	"redirect_interface_option : T_NOICMPIN",
	"snmp_statement : T_SNMP onoff_option EOS",
	"route_order : /* empty */",
	"route_statement : static_init '{' route_stmts '}' static_finit",
	"route_stmts : route_stmt EOS",
	"route_stmts : route_stmts route_stmt EOS",
	"route_stmts : route_stmts error EOS",
	"static_init : T_STATIC",
	"static_finit : /* empty */",
	"route_stmt : dest T_GATEWAY gateway preference_option",
	"route_stmt : dest T_INTERFACE interface preference_option",
	"control_order : /* empty */",
	"control_statement : T_ACCEPT T_PROTO proto_exterior T_AS autonomous_system preference_option '{' accept_list '}' EOS",
	"control_statement : T_ACCEPT T_PROTO accept_interior preference_option '{' accept_list '}' EOS",
	"control_statement : T_ACCEPT T_PROTO accept_interior T_INTERFACE interface_list preference_option '{' accept_list '}' EOS",
	"control_statement : T_ACCEPT T_PROTO accept_interior T_GATEWAY gateway_list preference_option '{' accept_list '}' EOS",
	"control_statement : T_PROPAGATE T_PROTO proto_exterior T_AS autonomous_system metric_option '{' prop_source_list '}' EOS",
	"control_statement : T_PROPAGATE T_PROTO proto_interior metric_option '{' prop_source_list '}' EOS",
	"control_statement : T_PROPAGATE T_PROTO proto_interior T_INTERFACE interface_list metric_option '{' prop_source_list '}' EOS",
	"control_statement : T_PROPAGATE T_PROTO proto_interior T_GATEWAY gateway_list metric_option '{' prop_source_list '}' EOS",
	"accept_interior : T_RIP",
	"accept_interior : T_HELLO",
	"accept_interior : T_REDIRECT",
	"accept_list : /* empty */",
	"accept_list : accept_list accept_listen EOS",
	"accept_list : accept_list error EOS",
	"accept_listen : T_LISTEN dest_mask preference_option",
	"accept_listen : T_NOLISTEN dest_mask",
	"prop_source_list : prop_source EOS",
	"prop_source_list : prop_source_list prop_source EOS",
	"prop_source_list : prop_source_list error EOS",
	"prop_source : T_PROTO proto_exterior T_AS autonomous_system metric_option prop_restrict_option",
	"prop_source : T_PROTO prop_interior metric_option prop_restrict_option",
	"prop_source : T_PROTO prop_interior T_INTERFACE interface_list metric_option prop_restrict_option",
	"prop_source : T_PROTO prop_interior T_GATEWAY gateway_list metric_option prop_restrict_option",
	"prop_interior : T_RIP",
	"prop_interior : T_HELLO",
	"prop_interior : T_DIRECT",
	"prop_interior : T_STATIC",
	"prop_interior : T_DEFAULT",
	"prop_restrict_option : /* empty */",
	"prop_restrict_option : '{' prop_restrict_list '}'",
	"prop_restrict_list : /* empty */",
	"prop_restrict_list : prop_restrict_list prop_restrict EOS",
	"prop_restrict_list : prop_restrict_list error EOS",
	"prop_restrict : T_ANNOUNCE dest_mask metric_option",
	"prop_restrict : T_NOANNOUNCE dest_mask",
	"dest_mask_list : dest_mask EOS",
	"dest_mask_list : dest_mask_list dest_mask EOS",
	"dest_mask_list : dest_mask_list error EOS",
	"dest_mask : T_ALL",
	"dest_mask : dest",
	"dest_mask : dest T_MASK dest",
	"dest : ip_addr",
	"dest : T_DEFAULT",
	"dest : host_name",
	"gateway_list : gateway",
	"gateway_list : gateway_list gateway",
	"gateway : host",
	"host : ip_addr",
	"host : host_name",
	"network : ip_addr",
	"network : T_DEFAULT",
	"network : host_name",
	"ip_addr : octet",
	"ip_addr : octet '.' octet",
	"ip_addr : octet '.' octet '.' octet",
	"ip_addr : octet '.' octet '.' octet '.' octet",
	"host_name : HNAME",
	"host_name : string",
	"proto_interior : T_RIP",
	"proto_interior : T_HELLO",
	"proto_exterior : T_EGP",
	"proto_exterior : T_BGP",
	"onoff_option : T_ON",
	"onoff_option : T_OFF",
	"interior_option : onoff_option",
	"interior_option : T_QUIET",
	"interior_option : T_SUPPLIER",
	"interior_option : T_POINTOPOINT",
	"metric : NUMBER",
	"metric_option : /* empty */",
	"metric_option : T_METRIC metric",
	"preference_option : /* empty */",
	"preference_option : T_PREFERENCE preference",
	"preference : NUMBER",
	"preference : pref_option",
	"preference : pref_option '+' NUMBER",
	"preference : pref_option '-' NUMBER",
	"pref_option : T_DIRECT",
	"pref_option : T_DEFAULT",
	"pref_option : T_REDIRECT",
	"pref_option : T_STATIC",
	"pref_option : T_HELLO",
	"pref_option : T_RIP",
	"pref_option : T_BGP",
	"pref_option : T_EGP",
	"pref_option : T_KERNEL",
	"string : STRING",
	"octet : NUMBER",
	"time : NUMBER",
	"time : NUMBER ':' NUMBER",
	"time : NUMBER ':' NUMBER ':' NUMBER",
	"port : NUMBER",
	"port : HNAME",
};
#endif /* YYDEBUG */
#define YYFLAG  (-3000)
/* @(#) $Revision: 66.3 $ */    

/*
** Skeleton parser driver for yacc output
*/

#if defined(NLS) && !defined(NL_SETN)
#include <msgbuf.h>
#endif

#ifndef nl_msg
#define nl_msg(i,s) (s)
#endif

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab

#ifndef __RUNTIME_YYMAXDEPTH
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#else
#define YYACCEPT	{free_stacks(); return(0);}
#define YYABORT		{free_stacks(); return(1);}
#endif

#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( (nl_msg(30001,"syntax error - cannot backup")) );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
/* define for YYFLAG now generated by yacc program. */
/*#define YYFLAG		(FLAGVAL)*/

/*
** global variables used by the parser
*/
# ifndef __RUNTIME_YYMAXDEPTH
__YYSCLASS YYSTYPE yyv[ YYMAXDEPTH ];	/* value stack */
__YYSCLASS int yys[ YYMAXDEPTH ];		/* state stack */
# else
__YYSCLASS YYSTYPE *yyv;			/* pointer to malloc'ed value stack */
__YYSCLASS int *yys;			/* pointer to malloc'ed stack stack */
#ifdef __cplusplus
	extern char *malloc(int);
	extern char *realloc(char *, int);
	extern void free();
# else
	extern char *malloc();
	extern char *realloc();
	extern void free();
# endif /* __cplusplus */
static int allocate_stacks(); 
static void free_stacks();
# ifndef YYINCREMENT
# define YYINCREMENT (YYMAXDEPTH/2) + 10
# endif
# endif	/* __RUNTIME_YYMAXDEPTH */
long  yymaxdepth = YYMAXDEPTH;

__YYSCLASS YYSTYPE *yypv;			/* top of value stack */
__YYSCLASS int *yyps;			/* top of state stack */

__YYSCLASS int yystate;			/* current state */
__YYSCLASS int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
__YYSCLASS int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */



/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
int
yyparse()
{
	register YYSTYPE *yypvt;	/* top of value stack for $vars */

	/*
	** Initialize externals - yyparse may be called more than once
	*/
# ifdef __RUNTIME_YYMAXDEPTH
	if (allocate_stacks()) YYABORT;
# endif
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

	goto yystack;
	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
# ifndef __RUNTIME_YYMAXDEPTH
			yyerror( (nl_msg(30002,"yacc stack overflow")) );
			YYABORT;
# else
			/* save old stack bases to recalculate pointers */
			YYSTYPE * yyv_old = yyv;
			int * yys_old = yys;
			yymaxdepth += YYINCREMENT;
			yys = (int *) realloc(yys, yymaxdepth * sizeof(int));
			yyv = (YYSTYPE *) realloc(yyv, yymaxdepth * sizeof(YYSTYPE));
			if (yys==0 || yyv==0) {
			    yyerror( (nl_msg(30002,"yacc stack overflow")) );
			    YYABORT;
			    }
			/* Reset pointers into stack */
			yy_ps = (yy_ps - yys_old) + yys;
			yyps = (yyps - yys_old) + yys;
			yy_pv = (yy_pv - yyv_old) + yyv;
			yypv = (yypv - yyv_old) + yyv;
# endif

		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( (nl_msg(30003,"syntax error")) );
				yynerrs++;
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
				yynerrs++;
			skip_init:
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 11:
# line 212 "parser.y"
{
				yyerrok;
			} break;
case 13:
# line 221 "parser.y"
{
#if	YYDEBUG != 0
				if (yypvt[-1].num == T_OFF) {
					yydebug = 0;
				} else {
					yydebug = 1;
				}
				trace(TR_CONFIG, 0, "parse: %s yydebug %s ;",
					parse_where(),
					yydebug ? "on" : "off");
#endif	/* YYDEBUG */
			} break;
case 14:
# line 234 "parser.y"
{
#if	YYDEBUG != 0
				if (yypvt[-1].num < 0 || yypvt[-1].num > PS_MAX) {
					(void) sprintf(parse_error, "invalid yystate value: %d",
						yypvt[-1].num);
					PARSE_ERROR;
				}
				parse_state = yypvt[-1].num;
				trace(TR_CONFIG, 0, "parse: %s yystate %d ;",
					parse_where(),
					parse_state);
#endif	/* YYDEBUG */
			} break;
case 15:
# line 248 "parser.y"
{
#if	YYDEBUG != 0
				trace(TR_CONFIG, 0, "parse: %s yyquit ;",
					parse_where());
				quit(0);
#endif	/* YYDEBUG */
			} break;
case 16:
# line 256 "parser.y"
{
			    char *name = yypvt[-1].ptr;

			    switch (*name) {
			    case '/':
				break;

			    default:
				name = (char *) malloc(strlen(name) + strlen(parse_directory) + 2);

				strcpy(name, parse_directory);
				strcat(name, "/");
				strcat(name, yypvt[-1].ptr);
			    }
				
			    if (parse_include(name)) {
				PARSE_ERROR;
			    }
			} break;
case 17:
# line 276 "parser.y"
{
#ifndef	vax11c
			    struct stat stbuf;

			    if (stat(yypvt[-1].ptr, &stbuf) < 0) {
				sprintf(parse_error, "stat(%s): %m",
					yypvt[-1].ptr);
				PARSE_ERROR;
			    }

			    switch (stbuf.st_mode & S_IFMT) {
			    case S_IFDIR:
				break;

			    default:
				sprintf(parse_error, "%s is not a directory",
					yypvt[-1].ptr);
				PARSE_ERROR;
			    }
#endif	/* vax11c */
			    if (parse_directory) {
				free(parse_directory);
			    }
			    if (yypvt[-1].ptr[strlen(yypvt[-1].ptr)-1] == '/') {
				yypvt[-1].ptr[strlen(yypvt[-1].ptr)-1] = (char) 0;
			    }
			    parse_directory = yypvt[-1].ptr;
			    trace(TR_PARSE, 0, "parse: %s included file prefeix now %s",
				  parse_where(),
				  parse_directory);
			} break;
case 18:
# line 312 "parser.y"
{
			    if (yypvt[-1].flag) {
				if (trace_flags) {
				    trace_flags = trace_flags_save = yypvt[-1].flag;
				    trace_display(trace_flags);
				} else {
				    trace_flags_save = yypvt[-1].flag;
				    if (trace_file) {
					trace_on(trace_file, TRUE);
				    }
				}
			    } else {
				if (trace_flags) {
				    trace_display(0);
				    trace_off();
				}
				trace_flags = trace_flags_save = yypvt[-1].flag;
			    }
			} break;
case 19:
# line 332 "parser.y"
{
			    if (trace_flags) {
				trace_flags_save = trace_flags;
				trace_off();
			    }
			    trace_on(trace_file = yypvt[-2].ptr, yypvt[-1].num);
			} break;
case 20:
# line 341 "parser.y"
{ yyval.num = TRUE; } break;
case 21:
# line 342 "parser.y"
{ yyval.num = FALSE; } break;
case 22:
# line 345 "parser.y"
{ yyval.flag = yypvt[-0].flag; } break;
case 23:
# line 346 "parser.y"
{ yyval.flag = (flag_t) 0; } break;
case 24:
# line 349 "parser.y"
{ yyval.flag = yypvt[-0].flag; } break;
case 25:
# line 350 "parser.y"
{ yyval.flag = yypvt[-1].flag | yypvt[-0].flag; } break;
case 26:
# line 353 "parser.y"
{ yyval.flag = TR_ALL; } break;
case 27:
# line 354 "parser.y"
{ yyval.flag = TR_INT|TR_EXT|TR_RT; } break;
case 28:
# line 355 "parser.y"
{ yyval.flag = TR_INT; } break;
case 29:
# line 356 "parser.y"
{ yyval.flag = TR_EXT; } break;
case 30:
# line 357 "parser.y"
{ yyval.flag = TR_RT; } break;
case 31:
# line 358 "parser.y"
{ yyval.flag = TR_EGP; } break;
case 32:
# line 359 "parser.y"
{ yyval.flag = TR_UPDATE; } break;
case 33:
# line 360 "parser.y"
{ yyval.flag = TR_RIP; } break;
case 34:
# line 361 "parser.y"
{ yyval.flag = TR_HELLO; } break;
case 35:
# line 362 "parser.y"
{ yyval.flag = TR_ICMP; } break;
case 36:
# line 363 "parser.y"
{ yyval.flag = TR_TASK; } break;
case 37:
# line 364 "parser.y"
{ yyval.flag = TR_TIMER; } break;
case 38:
# line 365 "parser.y"
{ yyval.flag = TR_NOSTAMP; } break;
case 39:
# line 366 "parser.y"
{ yyval.flag = TR_MARK; } break;
case 40:
# line 367 "parser.y"
{ yyval.flag = TR_PROTOCOL; } break;
case 41:
# line 368 "parser.y"
{ yyval.flag = TR_KRT; } break;
case 42:
# line 369 "parser.y"
{ yyval.flag = TR_BGP; } break;
case 43:
# line 370 "parser.y"
{ yyval.flag = TR_SNMP; } break;
case 44:
# line 371 "parser.y"
{ yyval.flag = TR_LEX; } break;
case 45:
# line 372 "parser.y"
{ yyval.flag = TR_PARSE; } break;
case 46:
# line 373 "parser.y"
{ yyval.flag = TR_CONFIG; } break;
case 47:
# line 379 "parser.y"
{
				if (parse_new_state(PS_DEFINE)) {
					PARSE_ERROR;
				}
			} break;
case 51:
# line 391 "parser.y"
{
				martian_list = parse_adv_append(martian_list, yypvt[-2].adv, TRUE);
				if (!martian_list) {
					PARSE_ERROR;
				}
				parse_adv_list("martians", (char *)0, (adv_entry *)0, yypvt[-2].adv);
			} break;
case 54:
# line 407 "parser.y"
{
				install = FALSE;
			} break;
case 55:
# line 411 "parser.y"
{
			    rt_default_needed = TRUE;
			} break;
case 56:
# line 421 "parser.y"
{
				parse_interface(yypvt[-2].adv);
			} break;
case 57:
# line 427 "parser.y"
{
				parse_metric = -1;
				parse_preference = 0;
				parse_flags = 0;
			} break;
case 60:
# line 441 "parser.y"
{
				if (parse_metric_check(RTPROTO_DIRECT, yypvt[-0].metric)) {
					PARSE_ERROR;
				}
				parse_metric = yypvt[-0].metric;
				parse_flags |= IFS_METRICSET;
			} break;
case 61:
# line 449 "parser.y"
{
				parse_preference = yypvt[-0].pref;
			} break;
case 62:
# line 453 "parser.y"
{
				parse_flags |= IFS_NOAGE;
			} break;
case 63:
# line 460 "parser.y"
{
				/* Return a null pointer to indicate all interfaces */
				yyval.adv = (adv_entry *) 0;
			} break;
case 64:
# line 465 "parser.y"
{
				yyval.adv = yypvt[-0].adv;
			} break;
case 65:
# line 472 "parser.y"
{
				yyval.adv = adv_alloc(ADVF_TINTF | ADVF_FIRST, (proto_t) 0);
				yyval.adv->adv_ifp = yypvt[-0].ifp;
			} break;
case 66:
# line 477 "parser.y"
{
				yyval.adv = adv_alloc(ADVF_TINTF, (proto_t) 0);
				yyval.adv->adv_ifp = yypvt[-0].ifp;
				yyval.adv = parse_adv_append(yypvt[-1].adv, yyval.adv, TRUE);
				if (!yyval.adv) {
					PARSE_ERROR;
				}
			} break;
case 67:
# line 488 "parser.y"
{
				yyval.ifp = if_withaddr(&yypvt[-0].sockaddr);
				if (!yyval.ifp) {
					(void) sprintf(parse_error, "Invalid interface at '%A'",
						&yypvt[-0].sockaddr);
					PARSE_ERROR;
				}
				trace(TR_PARSE, 0, "parse: %s INTERFACE: %A (%s)",
					parse_where(),
					&yyval.ifp->int_addr,
					yyval.ifp->int_name);
			} break;
case 68:
# line 503 "parser.y"
{
				yyval.sockaddr = yypvt[-0].sockaddr;
			} break;
case 69:
# line 507 "parser.y"
{ 
				if(parse_addr_hname(&yyval.sockaddr, yypvt[-0].ptr, TRUE, FALSE)) {
					if_entry *ifp;

					ifp = if_withname(yypvt[-0].ptr);
					if (!ifp) {
						(void) sprintf(parse_error, "unknown interface name at %s",
							yypvt[-0].ptr);
						PARSE_ERROR;
					}
					if (ifp->int_state & IFS_POINTOPOINT) {
					    yyval.sockaddr = ifp->int_dstaddr;
					} else {
					    yyval.sockaddr = ifp->int_addr;
					}
				}
			} break;
case 70:
# line 529 "parser.y"
{
				trace(TR_CONFIG, 0, "parse: %s autonomoussystem %d ;",
					parse_where(),
					yypvt[-1].as);
				my_system = yypvt[-1].as;
			} break;
case 71:
# line 539 "parser.y"
{
				if (parse_limit_check("autonomous system", yypvt[-0].num, LIMIT_AS_LOW, LIMIT_AS_HIGH)) {
					PARSE_ERROR;
				}
				yyval.as = yypvt[-0].num;
			} break;
case 72:
# line 550 "parser.y"
{
				if (parse_new_state(PS_PROTO)) {
					PARSE_ERROR;
				}
			} break;
case 79:
# line 568 "parser.y"
{
#ifdef	PROTO_RIP
 			        PROTO_SEEN;

				doing_rip = TRUE;
				rip_pointopoint = FALSE;
				rip_supplier = -1;
				switch (yypvt[-2].num) {
					case T_OFF:
						doing_rip = FALSE;
						rip_supplier = FALSE;
						break;
					case T_ON:
						break;
					case T_QUIET:
						rip_supplier = FALSE;
						break;
					case T_SUPPLIER:
						rip_supplier = TRUE;
						break;
					case T_POINTOPOINT:
						rip_pointopoint = TRUE;
						rip_supplier = TRUE;
						break;
				}
#endif	/* PROTO_RIP */
			} break;
case 80:
# line 598 "parser.y"
{
#ifdef	PROTO_RIP
				parse_proto = RTPROTO_RIP;
				parse_gwlist = &rip_gw_list;

				rip_default_metric = RIPHOPCNT_INFINITY;
				rip_preference = RTPREF_RIP;

#endif	/* PROTO_RIP */
			} break;
case 85:
# line 617 "parser.y"
{
				yyerrok;
			} break;
case 86:
# line 623 "parser.y"
{
#ifdef	PROTO_RIP
				rip_preference = yypvt[-0].pref;
#endif	/* PROTO_RIP */
			} break;
case 87:
# line 629 "parser.y"
{
#ifdef	PROTO_RIP
				if (parse_metric_check(RTPROTO_RIP, yypvt[-0].metric)) {
					PARSE_ERROR;
				}
				rip_default_metric = yypvt[-0].metric;
#endif	/* PROTO_RIP */
			} break;
case 88:
# line 638 "parser.y"
{
#ifdef	PROTO_RIP
				parse_interface(yypvt[-1].adv);
#endif	/* PROTO_RIP */
			} break;
case 89:
# line 644 "parser.y"
{
#ifdef	PROTO_RIP
				rip_n_trusted += parse_gw_flag(yypvt[-0].adv, RTPROTO_RIP, GWF_TRUSTED);
				if (!rip_n_trusted) {
					PARSE_ERROR;
				}
#endif	/* PROTO_RIP */
			} break;
case 90:
# line 653 "parser.y"
{
#ifdef	PROTO_RIP
				rip_n_source += parse_gw_flag(yypvt[-0].adv, RTPROTO_RIP, GWF_SOURCE);
				if (!rip_n_source) {
					PARSE_ERROR;
				}
#endif	/* PROTO_RIP */
			} break;
case 93:
# line 670 "parser.y"
{
#ifdef	PROTO_RIP
				parse_flags |= IFS_NORIPIN;
#endif	/* PROTO_RIP */
			} break;
case 94:
# line 676 "parser.y"
{
#ifdef	PROTO_RIP
				parse_flags |= IFS_NORIPOUT;
#endif	/* PROTO_RIP */
			} break;
case 95:
# line 686 "parser.y"
{
#ifdef	PROTO_HELLO
 			        PROTO_SEEN;

				doing_hello = TRUE;
				hello_pointopoint = FALSE;
				hello_supplier = -1;
				switch (yypvt[-2].num) {
					case T_OFF:
						doing_hello = FALSE;
						hello_supplier = FALSE;
						break;
					case T_ON:
						break;
					case T_QUIET:
						hello_supplier = FALSE;
						break;
					case T_SUPPLIER:
						hello_supplier = TRUE;
						break;
					case T_POINTOPOINT:
						hello_pointopoint = TRUE;
						hello_supplier = TRUE;
						break;
				}
#endif	/* PROTO_HELLO */
			} break;
case 96:
# line 716 "parser.y"
{
#ifdef	PROTO_HELLO
				parse_proto = RTPROTO_HELLO;
				parse_gwlist = &hello_gw_list;
				
				hello_default_metric = DELAY_INFINITY;
				hello_preference = RTPREF_HELLO;
#endif	/* PROTO_HELLO */
			} break;
case 101:
# line 735 "parser.y"
{
				yyerrok;
			} break;
case 102:
# line 742 "parser.y"
{
#ifdef	PROTO_HELLO
				hello_preference = yypvt[-0].pref;
#endif	/* PROTO_HELLO */
			} break;
case 103:
# line 748 "parser.y"
{
#ifdef	PROTO_HELLO
				if (parse_metric_check(RTPROTO_HELLO, yypvt[-0].metric)) {
					PARSE_ERROR;
				}
				hello_default_metric = yypvt[-0].metric;
#endif	/* PROTO_HELLO */
			} break;
case 104:
# line 757 "parser.y"
{
#ifdef	PROTO_HELLO
				parse_interface(yypvt[-1].adv);
#endif	/* PROTO_HELLO */
			} break;
case 105:
# line 763 "parser.y"
{
#ifdef	PROTO_HELLO
				hello_n_trusted += parse_gw_flag(yypvt[-0].adv, RTPROTO_HELLO, GWF_TRUSTED);
				if (!hello_n_trusted) {
					PARSE_ERROR;
				}
#endif	/* PROTO_HELLO */
			} break;
case 106:
# line 772 "parser.y"
{
#ifdef	PROTO_HELLO
				hello_n_source += parse_gw_flag(yypvt[-0].adv, RTPROTO_HELLO, GWF_SOURCE);
				if (!hello_n_source) {
					PARSE_ERROR;
				}
#endif	/* PROTO_HELLO */
			} break;
case 109:
# line 789 "parser.y"
{
#ifdef	PROTO_HELLO
				parse_flags |= IFS_NOHELLOIN;
#endif	/* PROTO_HELLO */
			} break;
case 110:
# line 795 "parser.y"
{
#ifdef	PROTO_HELLO
				parse_flags |= IFS_NOHELLOOUT;
#endif	/* PROTO_HELLO */
			} break;
case 111:
# line 805 "parser.y"
{
#ifdef	PROTO_EGP
 			        PROTO_SEEN;

				if (yypvt[-2].num == T_OFF) {
					doing_egp = FALSE;
					trace(TR_CONFIG, 0, "parse: %s egp off ;",
					      parse_where());
				} else {
					doing_egp = TRUE;
					if (!my_system) {
						(void) sprintf(parse_error, "parse: %s autonomous-system not specified",
							parse_where());
						PARSE_ERROR;
					}
					if (!egp_neighbors) {
						(void) sprintf(parse_error, "parse: %s no EGP neighbors specified",
							parse_where());
						PARSE_ERROR;
					}

#if	defined(AGENT_SNMP)
					egp_sort_neighbors();
#endif	/* defined */(AGENT_SNMP)

					if (trace_flags & TR_CONFIG) {
					    trace(TR_CONFIG, 0, "parse: %s egp on {",
						  parse_where());
					    trace(TR_CONFIG, 0, "parse: %s   preference %d ;",
						  parse_where(),
						  egp_preference);
					    trace(TR_CONFIG, 0, "parse: %s   defaultmetric %d ;",
						  parse_where(),
						    egp_default_metric);

					    gr_ngp = (struct egpngh *) 0;
					    EGP_LIST(ngp) {
						if (gr_ngp != ngp->ng_gr_head) {
						    if (gr_ngp) {
							trace(TR_CONFIG, 0, "parse: %s   } ;",
							      parse_where());
						    }
						    gr_ngp = ngp->ng_gr_head;
						    tracef("parse: %s   group",
							   parse_where());
						    if (ngp->ng_options & NGO_VERSION) {
							tracef(" version %d", ngp->ng_version);
						    }
						    if (ngp->ng_options & NGO_MAXACQUIRE) {
							tracef(" maxup %d", ngp->ng_gr_acquire);
						    }
						    if (ngp->ng_options & NGO_ASIN) {
							tracef(" asin %d", ngp->ng_asin);
						    }
						    if (ngp->ng_options & NGO_ASOUT) {
							tracef(" asout %d", ngp->ng_asout);
						    }
						    if (ngp->ng_options & NGO_PREFERENCE) {
							tracef(" preference %d", ngp->ng_preference);
						    }
						    trace(TR_CONFIG, 0, " {");
						}
						tracef("parse: %s     neighbor %s",
						       parse_where(),
						       ngp->ng_name);
						if (ngp->ng_options & NGO_INTERFACE) {
						    tracef(" intf %A",
							   &ngp->ng_interface->int_addr);
						}
						if (ngp->ng_options & NGO_SADDR) {
						    tracef(" sourcenet %A",
							   &ngp->ng_saddr);
						}
						if (ngp->ng_options & NGO_GATEWAY) {
						    tracef(" gateway %A",
							   &ngp->ng_gateway);
						}
						if (ngp->ng_options & NGO_METRICOUT) {
						    tracef(" egpmetricout %d", ngp->ng_metricout);
						}
						if (ngp->ng_options & NGO_NOGENDEFAULT) {
						    tracef(" nogendefault");
						}
						if (ngp->ng_options & NGO_DEFAULTIN) {
						    tracef(" acceptdefault");
						}
						if (ngp->ng_options & NGO_DEFAULTOUT) {
						    tracef(" propagatedefault");
						}
						if (ngp->ng_options & NGO_P1) {
						    tracef(" p1 %#T",
							   ngp->ng_P1);
						}
						if (ngp->ng_options & NGO_P2) {
						    tracef(" p2 %#T",
							   ngp->ng_P2);
						}
						trace(TR_CONFIG, 0, " ;");
					    } EGP_LISTEND ;
 					    trace(TR_CONFIG, 0, "parse: %s   } ;",
						  parse_where());
 					    trace(TR_CONFIG, 0, "parse: %s } ;",
						  parse_where());
					}
				}
#endif	/* PROTO_EGP */
			} break;
case 112:
# line 915 "parser.y"
{
			    parse_proto = RTPROTO_EGP;

			    egp_default_metric = EGP_INFINITY;
			    egp_preference = RTPREF_EGP;
			    egp_pktsize = EGPMAXPACKETSIZE;
			} break;
case 117:
# line 931 "parser.y"
{
				yyerrok;
			} break;
case 118:
# line 937 "parser.y"
{
#ifdef	PROTO_EGP
				egp_preference = yypvt[-0].pref;
#endif	/* PROTO_EGP */
			} break;
case 119:
# line 943 "parser.y"
{
#ifdef	PROTO_EGP
				if (parse_metric_check(RTPROTO_EGP, yypvt[-0].metric)) {
					PARSE_ERROR;
				}
				egp_default_metric = yypvt[-0].metric;
#endif	/* PROTO_EGP */
			} break;
case 120:
# line 952 "parser.y"
{
#ifdef	PROTO_EGP
			    if (parse_limit_check("packetsize", yypvt[-0].num, EGP_LIMIT_PKTSIZE)) {
				PARSE_ERROR;
			    }
			    egp_pktsize = yypvt[-0].num;
#endif	/* PROTO_EGP */
			} break;
case 121:
# line 961 "parser.y"
{
#ifdef	PROTO_EGP
				if (gr_ngp->ng_gr_acquire > gr_ngp->ng_gr_number) {
					(void) sprintf(parse_error,
						       "maxacquire %u is greater than number of neighbors %u in group %d",
						       gr_ngp->ng_gr_acquire,
						       gr_ngp->ng_gr_number,
						       parse_group_index);
				} else if (!gr_ngp->ng_gr_acquire) {
					gr_ngp->ng_gr_acquire = gr_ngp->ng_gr_number;
				}

#endif	/* PROTO_EGP */
			} break;
case 122:
# line 978 "parser.y"
{
#ifdef	PROTO_EGP
				/* Clear group structure and set fill pointer */
				memset((caddr_t) &egp_group, (char) 0, sizeof(egp_group));
				ngp = &egp_group;
				sockclear_in(&ngp->ng_addr);
				sockclear_in(&ngp->ng_gateway);
				sockclear_in(&ngp->ng_paddr);
				sockclear_in(&ngp->ng_saddr);
				/* First neighbor in group is head of group */
				gr_ngp = (struct egpngh *) 0;
				parse_group_index++;
#endif	/* PROTO_EGP */
			} break;
case 125:
# line 997 "parser.y"
{
				yyerrok;
			} break;
case 126:
# line 1003 "parser.y"
{
#ifdef	PROTO_EGP
			    struct egpngh *ngp2, *last = (struct egpngh *) 0;

			    /* Set neighbor's address */
			    ngp->ng_addr = yypvt[-1].sockaddr.in;	/* struct copy */
			    ngp->ng_gw.gw_proto = RTPROTO_EGP;
			    strcpy(ngp->ng_name, inet_ntoa(ngp->ng_addr.sin_addr));

			    /* Set group pointer and count this neighbor */
			    ngp->ng_gr_head = gr_ngp;
			    ngp->ng_gr_index = parse_group_index;
			    gr_ngp->ng_gr_number++;

			    if (!egp_neighbor_head) {
				egp_neighbor_head = ngp;	/* first neighbor */
				egp_neighbors++;
			    } else {
				EGP_LIST(ngp2) {
				    if (equal(&ngp->ng_addr, &ngp2->ng_addr)) {
					if (ngp2->ng_flags & NGF_DELETE) {
					    if (!egp_neighbor_changed(ngp2, ngp)) {
						ngp2->ng_flags &= ~NGF_DELETE;
						(void) free((caddr_t) ngp);
						break;
					    } else {
						ngp->ng_flags = NGF_WAIT;
					    }
					} else {
					    (void) sprintf(parse_error, "duplicate EGP neighbor at %A",
						    &ngp->ng_addr);
					    PARSE_ERROR;
					}
				    }
				    if (!ngp2->ng_next) {
					last = ngp2;
				    }
				} EGP_LISTEND ;
			    }
			    
			    /* Add this neighbor to end of the list */
			    if (last) {
				last->ng_next = ngp;
				egp_neighbors++;
			    }
#endif	/* PROTO_EGP */
			} break;
case 127:
# line 1053 "parser.y"
{
#ifdef	PROTO_EGP
				ngp = (struct egpngh *) calloc(1, sizeof(struct egpngh));
				if (!ngp) {
					trace(TR_ALL, LOG_ERR, "parse: %s calloc: %m",
						parse_where()),
					quit(errno);
				}
				/* Initialize neighbor structure with group structure */
				memcpy((caddr_t) ngp, (caddr_t) &egp_group, sizeof(*ngp));
				/* This neighbor is head of the group */
				if (!gr_ngp) {
					gr_ngp = ngp;
				}
				parse_gwlist = &parse_gwp;
#endif	/* PROTO_EGP */
			} break;
case 131:
# line 1080 "parser.y"
{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_ASIN;
				ngp->ng_asin = yypvt[-0].as;
#endif	/* PROTO_EGP */
			} break;
case 132:
# line 1087 "parser.y"
{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_ASOUT;
				ngp->ng_asout = yypvt[-0].as;
#endif	/* PROTO_EGP */
			} break;
case 133:
# line 1094 "parser.y"
{
#ifdef	PROTO_EGP
				/* XXX - Limit check maxup value */
				ngp->ng_options |= NGO_MAXACQUIRE;
				ngp->ng_gr_acquire = yypvt[-0].num;
#endif	/* PROTO_EGP */
			} break;
case 134:
# line 1102 "parser.y"
{
#ifdef	PROTO_EGP
				if ( !(EGPVMASK & (1 << (yypvt[-0].num - 2))) ) {
					(void) sprintf(parse_error, "unsupported EGP version: %d",
						yypvt[-0].num);
					PARSE_ERROR;
				}
				ngp->ng_options |= NGO_VERSION;
				ngp->ng_version = yypvt[-0].num;
#endif	/* PROTO_EGP */
			} break;
case 135:
# line 1114 "parser.y"
{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_PREFERENCE;
				ngp->ng_preference = yypvt[-0].pref;
#endif	/* PROTO_EGP */
			} break;
case 138:
# line 1128 "parser.y"
{
#ifdef	PROTO_EGP
				if (parse_metric_check(RTPROTO_EGP, yypvt[-0].metric)) {
					PARSE_ERROR;
				}
				ngp->ng_options |= NGO_METRICOUT;
				ngp->ng_metricout = yypvt[-0].metric;
#endif	/* PROTO_EGP */
			} break;
case 139:
# line 1138 "parser.y"
{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_NOGENDEFAULT;
#endif	/* PROTO_EGP */
			} break;
case 140:
# line 1144 "parser.y"
{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_DEFAULTIN;
#endif	/* PROTO_EGP */
			} break;
case 141:
# line 1150 "parser.y"
{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_DEFAULTOUT;
#endif	/* PROTO_EGP */
			} break;
case 142:
# line 1156 "parser.y"
{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_GATEWAY;
				sockcopy(&yypvt[-0].gwp->gw_addr, &ngp->ng_gateway);
				(void) free((caddr_t)yypvt[-0].gwp);
				parse_gwlist = (gw_entry **) 0;
				parse_gwp = (gw_entry *) 0;
#endif	/* PROTO_EGP */
			} break;
case 143:
# line 1166 "parser.y"
{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_INTERFACE;
				ngp->ng_interface = yypvt[-0].ifp;
#endif	/* PROTO_EGP */
			} break;
case 144:
# line 1173 "parser.y"
{
#ifdef	PROTO_EGP
				ngp->ng_options |= NGO_SADDR;
				sockcopy(&yypvt[-0].sockaddr, &ngp->ng_saddr);
#endif	/* PROTO_EGP */
			} break;
case 145:
# line 1180 "parser.y"
{
#ifdef	PROTO_EGP
			    if (parse_limit_check("P1", yypvt[-0].time, EGP_P1, MAXHELLOINT)) {
				PARSE_ERROR;
			    }
			    ngp->ng_options |= NGO_P1;
			    ngp->ng_P1 = yypvt[-0].time;
#endif	/* PROTO_EGP */
			} break;
case 146:
# line 1190 "parser.y"
{
#ifdef	PROTO_EGP
			    if (parse_limit_check("P2", yypvt[-0].time, EGP_P2, MAXPOLLINT)) {
				PARSE_ERROR;
			    }
			    ngp->ng_options |= NGO_P2;
			    ngp->ng_P2 = yypvt[-0].time;
#endif	/* PROTO_EGP */
			} break;
case 147:
# line 1204 "parser.y"
{
#ifdef	PROTO_BGP
 			        PROTO_SEEN;

				if (yypvt[-2].num == T_OFF) {
					doing_bgp = FALSE;
					trace(TR_CONFIG, 0, "parse: %s bgp off ;",
					      parse_where());
				} else {
					doing_bgp = TRUE;
					if (!my_system) {
						(void) sprintf(parse_error, "parse: %s autonomous-system not specified",
							parse_where());
						PARSE_ERROR;
					}
					if (!bgp_n_peers) {
						(void) sprintf(parse_error, "parse: %s no BGP peers specified",
							parse_where());
						PARSE_ERROR;
					}
					if (trace_flags & TR_CONFIG) {
					    trace(TR_CONFIG, 0, "parse: %s bgp on {",
						  parse_where());
					    trace(TR_CONFIG, 0, "parse: %s   preference %d ;",
						  parse_where(),
						  bgp_preference);
					    trace(TR_CONFIG, 0, "parse: %s   defaultmetric %d ;",
						  parse_where(),
						    bgp_default_metric);
					    BGP_LIST(bnp) {
					        tracef("parse: %s     neighbor %s",
						       parse_where(),
						       bnp->bgp_name);
						if (bnp->bgp_options & BGPO_HOLDTIME) {
						    tracef(" holdtime %#T",
							   bnp->bgp_holdtime_out);
						}
						if (bnp->bgp_options & BGPO_LINKTYPE) {
						    tracef(" linktype %s",
							   trace_state(bgpOpenType, bnp->bgp_linktype));
						}
						if (bnp->bgp_options & BGPO_GATEWAY) {
						    tracef(" gateway %A",
							   &bnp->bgp_gateway);
						}
						if (bnp->bgp_options & BGPO_INTERFACE) {
						    tracef(" intf %A",
							   &bnp->bgp_interface->int_addr);
						}
						if (bnp->bgp_options & BGPO_METRICOUT) {
						    tracef(" metricout %d",
							   bnp->bgp_metricout);
						}
						if (bnp->bgp_options & BGPO_ASIN) {
						    tracef(" asin %d",
							   bnp->bgp_asin);
						}
						if (bnp->bgp_options & BGPO_ASOUT) {
						    tracef(" asout %d",
							   bnp->bgp_asout);
						}
						if (bnp->bgp_options & BGPO_NOGENDEFAULT) {
					            tracef(" nogendefault");
						}
						if (bnp->bgp_options & BGPO_PREFERENCE) {
						    tracef(" preference %d",
							   bnp->bgp_preference);
						}
						trace(TR_CONFIG, 0, " ;");
					    } BGP_LISTEND ;
					    trace(TR_CONFIG, 0, "parse: %s } ;",
						  parse_where());
					}
				    }
#endif	/* PROTO_BGP */
			} break;
case 148:
# line 1282 "parser.y"
{ parse_proto = RTPROTO_BGP; } break;
case 153:
# line 1292 "parser.y"
{
				yyerrok;
			} break;
case 154:
# line 1298 "parser.y"
{
#ifdef	PROTO_BGP
				bgp_preference = yypvt[-0].pref;
#endif	/* PROTO_BGP */
			} break;
case 155:
# line 1304 "parser.y"
{
#ifdef	PROTO_BGP
				if (parse_metric_check(RTPROTO_BGP, yypvt[-0].metric)) {
					PARSE_ERROR;
				}
				bgp_default_metric = yypvt[-0].metric;
#endif	/* PROTO_BGP */
			} break;
case 156:
# line 1313 "parser.y"
{
#ifdef	PROTO_BGP
			    bgpPeer *bnp2, *last = (bgpPeer *) 0;

			    /* Set peer address */
			    bnp->bgp_addr = yypvt[-1].sockaddr.in;	/* struct copy */
			    bnp->bgp_gw.gw_proto = RTPROTO_BGP;
			    strcpy(bnp->bgp_name, inet_ntoa(bnp->bgp_addr.sin_addr));

			    /* Add to end of peer list */
			    if (bgp_peers == NULL) {
				bgp_peers = bnp;	/* first peer */
				bgp_n_peers++;
			    } else {
				BGP_LIST(bnp2) {
				    if (equal_in(bnp->bgp_addr.sin_addr, bnp2->bgp_addr.sin_addr)) {
					if (bnp2->bgp_flags & BGPF_DELETE) {
					    if (!bgp_peer_changed(bnp2, bnp)) {
						bnp2->bgp_flags &= ~BGPF_DELETE;
						(void) free((caddr_t) bnp);
						break;
					    } else {
						/* XXX - BGP doesn't have to wait, does it? */
						bnp->bgp_flags = BGPF_WAIT;
					    }
					} else {
					    (void) sprintf(parse_error, "duplicate BGP peer at %A",
						    &bnp->bgp_addr);
					    PARSE_ERROR;
					}
				    }
				    if (!bnp2->bgp_next) {
					last = bnp2;
				    }
				} BGP_LISTEND ;
			    }
			    if (last) {
				last->bgp_next = bnp;
				bgp_n_peers++;
			    }

#endif	/* PROTO_BGP */
			} break;
case 157:
# line 1359 "parser.y"
{
#ifdef	PROTO_BGP
				bnp = (bgpPeer *) calloc(1, sizeof(bgpPeer));
				if (!bnp) {
					trace(TR_ALL, LOG_ERR, "parse: %s calloc: %m",
						parse_where());
					quit(errno);
				}
				sockclear_in(&bnp->bgp_addr);
				sockclear_in(&bnp->bgp_gateway);
				parse_gwlist = &parse_gwp;
#endif	/* PROTO_BGP */
			} break;
case 160:
# line 1380 "parser.y"
{
#ifdef	PROTO_BGP
				if (parse_metric_check(RTPROTO_BGP, yypvt[-0].metric)) {
					PARSE_ERROR;
				}
				bnp->bgp_options |= BGPO_METRICOUT;
				bnp->bgp_metricout = yypvt[-0].metric;
#endif	/* PROTO_BGP */
			} break;
case 161:
# line 1390 "parser.y"
{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_ASIN;
				bnp->bgp_asin = yypvt[-0].as;
#endif	/* PROTO_BGP */
			} break;
case 162:
# line 1397 "parser.y"
{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_ASOUT;
				bnp->bgp_asout = yypvt[-0].as;
#endif	/* PROTO_BGP */
			} break;
case 163:
# line 1404 "parser.y"
{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_NOGENDEFAULT;
#endif	/* PROTO_BGP */
			} break;
case 164:
# line 1410 "parser.y"
{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_GATEWAY;
				sockcopy(&yypvt[-0].gwp->gw_addr, &bnp->bgp_gateway);
				(void) free((caddr_t)yypvt[-0].gwp);
				parse_gwlist = (gw_entry **) 0;
				parse_gwp = (gw_entry *) 0;
#endif	/* PROTO_BGP */
			} break;
case 165:
# line 1420 "parser.y"
{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_PREFERENCE;
				bnp->bgp_preference = yypvt[-0].pref;
#endif	/* PROTO_BGP */
			} break;
case 166:
# line 1427 "parser.y"
{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_LINKTYPE;
				bnp->bgp_linktype = yypvt[-0].num;
#endif	/* PROTO_BGP */
			} break;
case 167:
# line 1434 "parser.y"
{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_INTERFACE;
				bnp->bgp_interface = yypvt[-0].ifp;
#endif	/* PROTO_BGP */
			} break;
case 168:
# line 1441 "parser.y"
{
#ifdef	PROTO_BGP
				bnp->bgp_options |= BGPO_HOLDTIME;
				bnp->bgp_holdtime_out = yypvt[-0].time;
#endif	/* PROTO_BGP */
			} break;
case 169:
# line 1450 "parser.y"
{
#ifdef	PROTO_BGP
				yyval.num = openLinkInternal;
#endif	/* PROTO_BGP */
			} break;
case 170:
# line 1456 "parser.y"
{
#ifdef	PROTO_BGP
				yyval.num = openLinkHorizontal;
#endif	/* PROTO_BGP */
			} break;
case 171:
# line 1466 "parser.y"
{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
 			        PROTO_SEEN;

				ignore_redirects = (yypvt[-2].num == T_OFF) ? TRUE : FALSE;
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			} break;
case 172:
# line 1476 "parser.y"
{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
			    redirect_preference = RTPREF_REDIRECT;
			    parse_proto = RTPROTO_REDIRECT;
			    parse_gwlist = &redirect_gw_list;
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			} break;
case 177:
# line 1493 "parser.y"
{
				yyerrok;
			} break;
case 178:
# line 1499 "parser.y"
{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
				redirect_preference = yypvt[-0].pref;
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			} break;
case 179:
# line 1505 "parser.y"
{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
				parse_interface(yypvt[-1].adv);
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			} break;
case 180:
# line 1511 "parser.y"
{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
				redirect_n_trusted += parse_gw_flag(yypvt[-0].adv, RTPROTO_REDIRECT, GWF_TRUSTED);
				if (!redirect_n_trusted) {
					PARSE_ERROR;
				}
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			} break;
case 183:
# line 1529 "parser.y"
{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
				parse_flags |= IFS_NOICMPIN;
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			} break;
case 184:
# line 1538 "parser.y"
{
#ifdef	AGENT_SNMP
			        parse_proto = RTPROTO_SNMP;
 			        PROTO_SEEN;

				doing_snmp = (yypvt[-1].num == T_OFF) ? FALSE : TRUE;
				trace(TR_CONFIG, 0, "parse: %s snmp %s ;",
					parse_where(),
					doing_snmp ? "on" : "off");
#endif	/* AGENT_SNMP */
			} break;
case 185:
# line 1554 "parser.y"
{
				if (parse_new_state(PS_ROUTE)) {
					PARSE_ERROR;
				}
			} break;
case 189:
# line 1567 "parser.y"
{
				yyerrok;
			} break;
case 190:
# line 1573 "parser.y"
{
				/* Need to set this for static routes, not used for interface routes */
				parse_proto = RTPROTO_STATIC;
				parse_gwlist = &rt_gw_list;
				rt_open(rt_task);
				trace(TR_CONFIG, 0, "parse: %s static {",
				      parse_where());
			} break;
case 191:
# line 1584 "parser.y"
{
				(void) rt_close(rt_task, (gw_entry *) 0, 0);
				trace(TR_CONFIG, 0, "parse: %s } ;",
				      parse_where());
			} break;
case 192:
# line 1592 "parser.y"
{
				flag_t table;
				rt_entry *rt;
				sockaddr_un gateway;

				gateway = yypvt[-1].gwp->gw_addr;	/* struct copy */

				table = gd_inet_ishost(&yypvt[-3].sockaddr) ? RTS_HOSTROUTE : RTS_INTERIOR;

				if (rt = rt_locate(table, &yypvt[-3].sockaddr, RTPROTO_STATIC)) {
				    if (rt->rt_state & RTS_NOAGE) {
					(void) sprintf(parse_error, "duplicate static route to %A",
						&yypvt[-3].sockaddr);
					PARSE_ERROR;
				    }
				    rt->rt_state |= RTS_NOAGE;
				    if (!rt_change(rt,
						   &gateway,
						   0,
						   (time_t) 0,
						   yypvt[-0].pref ? yypvt[-0].pref : RTPREF_STATIC)) {
					rt = (rt_entry *) 0;
				    }
				} else {
				    rt = rt_add(&yypvt[-3].sockaddr,
						(sockaddr_un *) 0,
						&gateway,
						(gw_entry *) 0,
						0,
						table | RTS_NOAGE,
						RTPROTO_STATIC,
						my_system,
						(time_t) 0,
						yypvt[-0].pref ? yypvt[-0].pref : RTPREF_STATIC);
				}
				if (!rt) {
					(void) sprintf(parse_error, "error adding static route to %A",
						&yypvt[-3].sockaddr);
					PARSE_ERROR;
				}
				trace(TR_CONFIG, 0, "parse: %s   %A gateway %A preference %d ;",
					parse_where(),
					&yypvt[-3].sockaddr,
					&gateway,
					yypvt[-0].pref ? yypvt[-0].pref : RTPREF_STATIC);
			} break;
case 193:
# line 1639 "parser.y"
{
				flag_t table;
				rt_entry *rt;

				table = gd_inet_ishost(&yypvt[-3].sockaddr) ? RTS_HOSTROUTE : RTS_INTERIOR;

				if (rt = rt_locate(table, &yypvt[-3].sockaddr, RTPROTO_DIRECT)) {
				    if (rt->rt_state & RTS_NOAGE) {
					(void) sprintf(parse_error, "duplicate interface route to %A",
						&yypvt[-3].sockaddr);
					PARSE_ERROR;
				    }
				    rt->rt_state |= RTS_NOAGE;
				    if (!rt_change(rt,
						   &yypvt[-1].ifp->int_addr,
						   0,
						   (time_t) 0,
						   yypvt[-0].pref ? yypvt[-0].pref : RTPREF_STATIC)) {
					rt = (rt_entry *) 0;
				    }
				} else {
				    rt = rt_add(&yypvt[-3].sockaddr,
						(sockaddr_un *) 0,
						&yypvt[-1].ifp->int_addr,
						(gw_entry *) 0,
						yypvt[-1].ifp->int_metric,
						table | RTS_NOAGE,
						RTPROTO_DIRECT,
						my_system,
						(time_t) 0,
						yypvt[-0].pref ? yypvt[-0].pref : RTPREF_STATIC);
				}
				if (!rt) {
				    (void) sprintf(parse_error, "error adding interface route to %A",
					    &yypvt[-3].sockaddr);
				    PARSE_ERROR;
				}
				trace(TR_CONFIG, 0, "parse: %s   %A interface %A preference %d",
					parse_where(),
					&yypvt[-3].sockaddr,
					&yypvt[-1].ifp->int_addr,
					yypvt[-0].pref ? yypvt[-0].pref : RTPREF_STATIC);
			} break;
case 194:
# line 1687 "parser.y"
{
				if (parse_new_state(PS_CONTROL)) {
					PARSE_ERROR;
				}
			} break;
case 195:
# line 1696 "parser.y"
{
				adv_entry *adv;

				/*
				 *	Tack the list of destinations onto the end of the list
				 *	for neighbors with the specified AS.
				 */
				adv = adv_alloc(ADVF_TAS, yypvt[-7].proto);
				adv->adv_as = yypvt[-5].as;
				adv->adv_list = yypvt[-2].adv;
				if (yypvt[-4].pref > 0) {
					adv->adv_flag |= ADVF_OTPREFERENCE;
					adv->adv_preference = yypvt[-4].pref;
				}

				switch (yypvt[-7].proto) {
#ifdef	PROTO_BGP
					case RTPROTO_BGP:
						if (!parse_adv_ext(&bgp_accept_list, adv)) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_BGP */
#ifdef	PROTO_EGP
					case RTPROTO_EGP:
						if (!parse_adv_ext(&egp_accept_list, adv)) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_EGP */
				}

				parse_adv_list("accept", "listen", adv, adv->adv_list);
			} break;
case 196:
# line 1731 "parser.y"
{
				adv_entry *adv;

				/*
				 *	Append the dest_mask list to the end of the accept list
				 *	for the specified protocol.
				 */
				adv = adv_alloc((flag_t) 0, yypvt[-5].proto);
				adv->adv_list = yypvt[-2].adv;
				if (yypvt[-4].pref > 0) {
					adv->adv_flag |= ADVF_OTPREFERENCE;
					adv->adv_preference = yypvt[-4].pref;
				}

				switch (yypvt[-5].proto) {
#ifdef	PROTO_HELLO
					case RTPROTO_HELLO:
						hello_accept_list = parse_adv_append(hello_accept_list, adv, TRUE);
						if (!hello_accept_list) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_HELLO */
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
					case RTPROTO_REDIRECT:
						redirect_accept_list = parse_adv_append(redirect_accept_list, adv, TRUE);
						if (!redirect_accept_list) {
							PARSE_ERROR;
						}
						break;
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
#ifdef	PROTO_RIP
					case RTPROTO_RIP:
						rip_accept_list = parse_adv_append(rip_accept_list, adv, TRUE);
						if (!rip_accept_list) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_RIP */
				}
				parse_adv_list("accept", "listen", adv, adv->adv_list);
			} break;
case 197:
# line 1774 "parser.y"
{
				adv_entry *adv, *advn, **int_adv = (adv_entry **) 0;

				switch (yypvt[-7].proto) {
#ifdef	PROTO_HELLO
					case RTPROTO_HELLO:
						int_adv = parse_adv_interface(&hello_int_accept);
						break;
#endif	/* PROTO_HELLO */
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
					case RTPROTO_REDIRECT:
						int_adv = parse_adv_interface(&redirect_int_accept);
						break;
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
#ifdef	PROTO_RIP
					case RTPROTO_RIP:
						int_adv = parse_adv_interface(&rip_int_accept);
						break;
#endif	/* PROTO_RIP */
				}

				for (adv = yypvt[-5].adv; adv; adv = adv->adv_next) {
				    adv->adv_proto = yypvt[-7].proto;
				    if (yypvt[-4].pref > 0) {
					adv->adv_flag |= ADVF_OTPREFERENCE;
					adv->adv_preference = yypvt[-4].pref;
				    }
				    adv->adv_list = parse_adv_append(adv->adv_list, yypvt[-2].adv, FALSE);
				}
				adv = yypvt[-5].adv;
				adv_free_list(yypvt[-2].adv);
				parse_adv_list("accept", "listen", adv, adv->adv_list);
				do {
					advn = adv->adv_next;
					adv->adv_next = NULL;
					if (!(int_adv[adv->adv_ifp->int_index - 1] =
					    parse_adv_append(INT_CONTROL(int_adv, adv->adv_ifp), adv, TRUE))) {
						PARSE_ERROR;
					}
				} while (adv = advn);
			} break;
case 198:
# line 1816 "parser.y"
{
				/*
				 * A side effect is that accept_interior sets parse_gwlist for gateway_list
				 */
				adv_entry *adv, *advn;

				for (adv = yypvt[-5].adv; adv; adv = adv->adv_next) {
				    adv->adv_proto = yypvt[-7].proto;
				    if (yypvt[-4].pref > 0) {
					adv->adv_flag |= ADVF_OTPREFERENCE;
					adv->adv_preference = yypvt[-4].pref;
				    }
				    adv->adv_list = parse_adv_append(adv->adv_list, yypvt[-2].adv, FALSE);
				}
				adv = yypvt[-5].adv;
				adv_free_list(yypvt[-2].adv);
				parse_adv_list("accept", "listen", adv, adv->adv_list);
				do {
					advn = adv->adv_next;
					adv->adv_next = NULL;
					adv->adv_gwp->gw_accept = parse_adv_append(adv->adv_gwp->gw_accept, adv, TRUE);
					if (!adv->adv_gwp->gw_accept) {
						PARSE_ERROR;
					}
				} while (adv = advn);
			} break;
case 199:
# line 1843 "parser.y"
{
				adv_entry *adv;

				/*
				 *	Tack the list of destinations onto the end of the list
				 *	for neighbors with the specified AS.
				 */
				adv = adv_alloc(ADVF_TAS, yypvt[-7].proto);
				adv->adv_as = yypvt[-5].as;
				adv->adv_list = yypvt[-2].adv;
				if (yypvt[-4].metric > 0) {
					adv->adv_flag |= ADVF_OTMETRIC;
					adv->adv_metric = yypvt[-4].metric;
				}

				switch (yypvt[-7].proto) {
#ifdef	PROTO_BGP
					case RTPROTO_BGP:
						if (!parse_adv_ext(&bgp_propagate_list, adv)) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_BGP */
#ifdef	PROTO_EGP
					case RTPROTO_EGP:
						if (!parse_adv_ext(&egp_propagate_list, adv)) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_EGP */
				}

				parse_adv_prop_list(adv);
			} break;
case 200:
# line 1878 "parser.y"
{
				adv_entry *adv;

				/*
				 *	Append the dest_mask list to the end of the propagate list
				 *	for the specified protocol.
				 */
				adv = adv_alloc((flag_t) 0, yypvt[-5].proto);
				adv->adv_list = yypvt[-2].adv;
				if (yypvt[-4].metric > 0) {
					adv->adv_flag |= ADVF_OTMETRIC;
					adv->adv_metric = yypvt[-4].metric;
				}

				switch (yypvt[-5].proto) {
#ifdef	PROTO_HELLO
					case RTPROTO_HELLO:
						hello_propagate_list = parse_adv_append(hello_propagate_list, adv, TRUE);
						if (!hello_propagate_list) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_HELLO */
#ifdef	PROTO_RIP
					case RTPROTO_RIP:
						rip_propagate_list = parse_adv_append(rip_propagate_list, adv, TRUE);
						if (!rip_propagate_list) {
							PARSE_ERROR;
						}
						break;
#endif	/* PROTO_RIP */
				}
				parse_adv_prop_list(adv);
			} break;
case 201:
# line 1913 "parser.y"
{
				adv_entry *adv, *advn, **int_adv = (adv_entry **) 0;

				switch (yypvt[-7].proto) {
#ifdef	PROTO_HELLO
					case RTPROTO_HELLO:
						int_adv = parse_adv_interface(&hello_int_propagate);
						break;
#endif	/* PROTO_HELLO */
#ifdef	PROTO_RIP
					case RTPROTO_RIP:
						int_adv = parse_adv_interface(&rip_int_propagate);
						break;
#endif	/* PROTO_RIP */
				}

				for (adv = yypvt[-5].adv; adv; adv = adv->adv_next) {
				    adv->adv_proto = yypvt[-7].proto;
				    if (yypvt[-4].metric > 0) {
					adv->adv_flag |= ADVF_OTMETRIC;
					adv->adv_metric = yypvt[-4].metric;
				    }
				    adv->adv_list = parse_adv_append(adv->adv_list, yypvt[-2].adv, FALSE);
				}
				adv = yypvt[-5].adv;
				adv_free_list(yypvt[-2].adv);
				parse_adv_prop_list(adv);
				do {
					advn = adv->adv_next;
					adv->adv_next = NULL;
					if (!(int_adv[adv->adv_ifp->int_index - 1] =
					    parse_adv_append(INT_CONTROL(int_adv, adv->adv_ifp), adv, TRUE))) {
						PARSE_ERROR;
					}
				} while (adv = advn);
			} break;
case 202:
# line 1950 "parser.y"
{
			    /*
			     * A side effect is that prop_interior sets parse_gwlist for gateway_list
			     */
			    adv_entry *adv, *advn;

			    for (adv = yypvt[-5].adv; adv; adv = adv->adv_next) {
				adv->adv_proto = yypvt[-7].proto;
				if (yypvt[-4].metric > 0) {
				    adv->adv_flag |= ADVF_OTMETRIC;
				    adv->adv_metric = yypvt[-4].metric;
				}
				adv->adv_list = parse_adv_append(adv->adv_list, yypvt[-2].adv, FALSE);
			    }
			    adv = yypvt[-5].adv;
			    adv_free_list(yypvt[-2].adv);
			    parse_adv_prop_list(adv);
			    do {
				advn = adv->adv_next;
				adv->adv_next = NULL;
				adv->adv_gwp->gw_propagate = parse_adv_append(adv->adv_gwp->gw_propagate, adv, TRUE);
				if (!adv->adv_gwp->gw_propagate) {
				    PARSE_ERROR;
				}
			    } while (adv = advn);
			} break;
case 203:
# line 1983 "parser.y"
{
#ifdef	PROTO_RIP
				yyval.proto = parse_proto = RTPROTO_RIP;
				parse_gwlist = &rip_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, yyval.proto)));
#endif	/* PROTO_RIP */
			} break;
case 204:
# line 1993 "parser.y"
{
#ifdef	PROTO_HELLO
				yyval.proto = parse_proto = RTPROTO_HELLO;
				parse_gwlist = &hello_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, yyval.proto)));
#endif	/* PROTO_HELLO */
			} break;
case 205:
# line 2003 "parser.y"
{
#if	defined(PROTO_ICMP) || defined(RTM_ADD)
				yyval.proto = parse_proto = RTPROTO_REDIRECT;
				parse_gwlist = &redirect_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, yyval.proto)));
#endif	/* defined */(PROTO_ICMP) || defined(RTM_ADD)
			} break;
case 206:
# line 2015 "parser.y"
{
				yyval.adv = (adv_entry *) 0;
			} break;
case 207:
# line 2019 "parser.y"
{
			    if (yypvt[-2].adv) {
				yyval.adv = parse_adv_append(yypvt[-2].adv, yypvt[-1].adv, TRUE);
				if (!yyval.adv) {
					PARSE_ERROR;
				}
			    } else {
				yyval.adv = yypvt[-1].adv;
			    }
			} break;
case 208:
# line 2030 "parser.y"
{
				yyerrok;
			} break;
case 209:
# line 2036 "parser.y"
{
				yyval.adv = adv_alloc(ADVF_TDM, parse_proto);
				yyval.adv->adv_dm = yypvt[-1].dm;
				if (yypvt[-0].pref > 0) {
					yyval.adv->adv_preference = yypvt[-0].pref;
					yyval.adv->adv_flag |= ADVF_OTPREFERENCE;
				}
			} break;
case 210:
# line 2045 "parser.y"
{
				yyval.adv = adv_alloc(ADVF_TDM | ADVF_NO, parse_proto);
				yyval.adv->adv_dm = yypvt[-0].dm;
			} break;
case 211:
# line 2057 "parser.y"
{
				yyval.adv = yypvt[-1].adv;
			} break;
case 212:
# line 2061 "parser.y"
{
				yyval.adv = parse_adv_append(yypvt[-2].adv, yypvt[-1].adv, TRUE);
				if (!yyval.adv) {
					PARSE_ERROR;
				}
			} break;
case 213:
# line 2068 "parser.y"
{
				yyerrok;
			} break;
case 214:
# line 2074 "parser.y"
{
				yyval.adv = adv_alloc(ADVF_TAS | ADVF_FIRST, (proto_t) 0);
				yyval.adv->adv_as = yypvt[-2].as;
				yyval.adv = parse_adv_propagate(yyval.adv, yypvt[-4].proto, yypvt[-1].metric, yypvt[-0].adv);
			} break;
case 215:
# line 2080 "parser.y"
{
				yyval.adv = adv_alloc(ADVF_TANY | ADVF_FIRST, (proto_t) 0);
				yyval.adv = parse_adv_propagate(yyval.adv, yypvt[-2].proto, yypvt[-1].metric, yypvt[-0].adv);
			} break;
case 216:
# line 2085 "parser.y"
{
				yyval.adv = parse_adv_propagate(yypvt[-2].adv, yypvt[-4].proto, yypvt[-1].metric, yypvt[-0].adv);
			} break;
case 217:
# line 2089 "parser.y"
{
				yyval.adv = parse_adv_propagate(yypvt[-2].adv, yypvt[-4].proto, yypvt[-1].metric, yypvt[-0].adv);
			} break;
case 218:
# line 2095 "parser.y"
{
#ifdef	PROTO_RIP
				yyval.proto = parse_proto = RTPROTO_RIP;
				parse_gwlist = &rip_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, yyval.proto)));
#endif	/* PROTO_RIP */
			} break;
case 219:
# line 2105 "parser.y"
{
#ifdef	PROTO_HELLO
				yyval.proto = parse_proto = RTPROTO_HELLO;
				parse_gwlist = &hello_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, yyval.proto)));
#endif	/* PROTO_HELLO */
			} break;
case 220:
# line 2115 "parser.y"
{
				yyval.proto = parse_proto = RTPROTO_DIRECT;
				parse_gwlist = (gw_entry **)0;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, yyval.proto)));
			} break;
case 221:
# line 2123 "parser.y"
{
				yyval.proto = parse_proto = RTPROTO_STATIC;
				parse_gwlist = &rt_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, yyval.proto)));
			} break;
case 222:
# line 2131 "parser.y"
{
				yyval.proto = parse_proto = RTPROTO_DEFAULT;
				parse_gwlist = (gw_entry **)0;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, yyval.proto)));
			} break;
case 223:
# line 2143 "parser.y"
{
				yyval.adv = (adv_entry *) 0;
			} break;
case 224:
# line 2147 "parser.y"
{
				yyval.adv = yypvt[-1].adv;
			} break;
case 225:
# line 2155 "parser.y"
{
			    yyval.adv = (adv_entry *) 0;
			} break;
case 226:
# line 2159 "parser.y"
{
			    if (yypvt[-2].adv) {
				yyval.adv = parse_adv_append(yypvt[-2].adv, yypvt[-1].adv, TRUE);
				if (!yyval.adv) {
				    PARSE_ERROR;
				}
			    } else {
				yyval.adv = yypvt[-1].adv;
			    }
			} break;
case 227:
# line 2170 "parser.y"
{
			    yyerrok;
			} break;
case 228:
# line 2176 "parser.y"
{
			    yyval.adv = adv_alloc(ADVF_TDM, (proto_t) 0);
			    yyval.adv->adv_dm = yypvt[-1].dm;
			    if (yypvt[-0].metric >= 0) {
				yyval.adv->adv_metric = yypvt[-0].metric;
				yyval.adv->adv_flag |= ADVF_OTMETRIC;
			    }
			} break;
case 229:
# line 2185 "parser.y"
{
			    yyval.adv = adv_alloc(ADVF_TDM | ADVF_NO, (proto_t) 0);
			    yyval.adv->adv_dm = yypvt[-0].dm;
			} break;
case 230:
# line 2195 "parser.y"
{
				yyval.adv = adv_alloc(ADVF_TDM, (proto_t) 0);
				yyval.adv->adv_dm = yypvt[-1].dm;
			} break;
case 231:
# line 2200 "parser.y"
{
				yyval.adv = adv_alloc(ADVF_TDM, (proto_t) 0);
				yyval.adv->adv_dm = yypvt[-1].dm;
				yyval.adv = parse_adv_append(yypvt[-2].adv, yyval.adv, TRUE);
				if (!yyval.adv) {
					PARSE_ERROR;
				}
			} break;
case 232:
# line 2209 "parser.y"
{
				yyerrok;
			} break;
case 233:
# line 2217 "parser.y"
{
			    /* XXX - need to specify a generic protocol */
			    sockclear_in(&yyval.dm.dm_dest.in);
			    sockclear_in(&yyval.dm.dm_mask.in);
			    yyval.dm.dm_dest.in.sin_addr.s_addr = INADDR_ANY;
			    yyval.dm.dm_mask.in.sin_addr.s_addr = INADDR_ANY;
			    trace(TR_PARSE, 0, "parse: %s DEST: %A MASK: %A",
				  parse_where(),
				  &yyval.dm.dm_dest,
				  &yyval.dm.dm_mask);
			} break;
case 234:
# line 2229 "parser.y"
{
			    /* XXX - need to match protocols */
			    yyval.dm.dm_dest = yypvt[-0].sockaddr;		/* struct copy */
			    sockclear_in(&yyval.dm.dm_mask.in);
			    yyval.dm.dm_mask.in.sin_addr.s_addr = INADDR_BROADCAST;
			    trace(TR_PARSE, 0, "parse: %s DEST: %A MASK: %A",
				  parse_where(),
				  &yyval.dm.dm_dest,
				  &yyval.dm.dm_mask);
			} break;
case 235:
# line 2240 "parser.y"
{
			    yyval.dm.dm_dest = yypvt[-2].sockaddr;		/* struct copy */
			    yyval.dm.dm_mask = yypvt[-0].sockaddr;		/* struct copy */
			    trace(TR_PARSE, 0, "parse: %s DEST: %A MASK: %A",
				  parse_where(),
				  &yyval.dm.dm_dest,
				  &yyval.dm.dm_mask);
			} break;
case 236:
# line 2252 "parser.y"
{
				yyval.sockaddr = yypvt[-0].sockaddr;
				trace(TR_PARSE, 0, "parse: %s DEST: %A",
					parse_where(),
					&yyval.sockaddr);
			} break;
case 237:
# line 2259 "parser.y"
{
			    sockclear_in(&yyval.sockaddr);
			    socktype_in(&yyval.sockaddr)->sin_addr.s_addr = INADDR_ANY;
				trace(TR_PARSE, 0, "parse: %s DEST: %A",
					parse_where(),
					&yyval.sockaddr);
			} break;
case 238:
# line 2267 "parser.y"
{ 
				if(parse_addr_hname(&yyval.sockaddr, yypvt[-0].ptr, TRUE, TRUE)) {
					PARSE_ERROR;
				}
				trace(TR_PARSE, 0, "parse: %s DEST: %A",
					parse_where(),
					&yyval.sockaddr);
			} break;
case 239:
# line 2279 "parser.y"
{
				yyval.adv = adv_alloc(ADVF_TGW | ADVF_FIRST, (proto_t) 0);
				yyval.adv->adv_gwp = yypvt[-0].gwp;
			} break;
case 240:
# line 2284 "parser.y"
{
				yyval.adv = adv_alloc(ADVF_TGW, (proto_t) 0);
				yyval.adv->adv_gwp = yypvt[-0].gwp;
				yyval.adv = parse_adv_append(yypvt[-1].adv, yyval.adv, TRUE);
				if (!yyval.adv) {
					PARSE_ERROR;
				}
			} break;
case 241:
# line 2296 "parser.y"
{
				/*
				 *	Make sure host is on a locally attached network then
				 *	find or create a gw structure for it.  Requires that
				 *	parse_proto and parse_gwlist are previously set
				 */
				if (!if_withdst(&yypvt[-0].sockaddr)) {
					(void) sprintf(parse_error, "gateway not a host address on an attached network: '%A'",
						&yypvt[-0].sockaddr);
					PARSE_ERROR;
				}
				if (!parse_gwlist) {
					(void) sprintf(parse_error, "gateway specification not valid for %s",
						trace_bits(rt_proto_bits, parse_proto));
					PARSE_ERROR;
				}
				yyval.gwp = gw_locate(parse_gwlist, parse_proto, &yypvt[-0].sockaddr);
				trace(TR_PARSE, 0, "parse: %s GATEWAY: %A  PROTO: %s",
					parse_where(),
					&yyval.gwp->gw_addr,
					gd_lower(trace_bits(rt_proto_bits, yyval.gwp->gw_proto)));
			} break;
case 242:
# line 2322 "parser.y"
{
				if (!gd_inet_ishost(&yypvt[-0].sockaddr)) {
					(void) sprintf(parse_error, "not a host address: '%A'",
						&yypvt[-0].sockaddr);
					PARSE_ERROR;
				}
				yyval.sockaddr = yypvt[-0].sockaddr;
				trace(TR_PARSE, 0, "parse: %s HOST: %A",
					parse_where(),
					&yyval.sockaddr);
			} break;
case 243:
# line 2334 "parser.y"
{ 
				if(parse_addr_hname(&yyval.sockaddr, yypvt[-0].ptr, TRUE, FALSE)) {
					PARSE_ERROR;
				}
				trace(TR_PARSE, 0, "parse: %s HOST: %A",
					parse_where(),
					&yyval.sockaddr);
			} break;
case 244:
# line 2346 "parser.y"
{
				if (gd_inet_ishost(&yypvt[-0].sockaddr)) {
					(void) sprintf(parse_error, "not a network address: '%A'",
						&yypvt[-0].sockaddr);
					PARSE_ERROR;
				}
				yyval.sockaddr = yypvt[-0].sockaddr;
				trace(TR_PARSE, 0, "parse: %s NETWORK: %A",
					parse_where(),
					&yyval.sockaddr);
			} break;
case 245:
# line 2358 "parser.y"
{
			    sockclear_in(&yyval.sockaddr);
			    socktype_in(&yyval.sockaddr)->sin_addr.s_addr = INADDR_ANY;
			    trace(TR_PARSE, 0, "parse: %s NETWORK: %A",
				  parse_where(),
				  &yyval.sockaddr);
			} break;
case 246:
# line 2366 "parser.y"
{ 
				if(parse_addr_hname(&yyval.sockaddr, yypvt[-0].ptr, FALSE, TRUE)) {
					PARSE_ERROR;
				}
				trace(TR_PARSE, 0, "parse: %s NETWORK: %A",
					parse_where(),
					&yyval.sockaddr);
			} break;
case 247:
# line 2378 "parser.y"
{
				u_long addr;

				addr = yypvt[-0].num << 24;
				parse_addr_long(&yyval.sockaddr, htonl(addr));
			} break;
case 248:
# line 2385 "parser.y"
{
				u_long addr;

				addr = 	(yypvt[-2].num << 24) + (yypvt[-0].num << 16);
				parse_addr_long(&yyval.sockaddr, htonl(addr));
			} break;
case 249:
# line 2392 "parser.y"
{
				u_long addr;

				addr = (yypvt[-4].num << 24) + (yypvt[-2].num << 16) + (yypvt[-0].num << 8);
				parse_addr_long(&yyval.sockaddr, htonl(addr));
			} break;
case 250:
# line 2399 "parser.y"
{
				u_long addr;

				addr = (yypvt[-6].num << 24) + (yypvt[-4].num << 16) + (yypvt[-2].num << 8) + yypvt[-0].num;
				parse_addr_long(&yyval.sockaddr, htonl(addr));
			} break;
case 251:
# line 2409 "parser.y"
{
			    yyval.ptr = yypvt[-0].ptr;
			} break;
case 252:
# line 2413 "parser.y"
{
			    yyval.ptr = yypvt[-0].ptr;
			} break;
case 253:
# line 2420 "parser.y"
{
#ifdef	PROTO_RIP
				yyval.proto = parse_prop_proto = RTPROTO_RIP;
				parse_gwlist = &rip_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, yyval.proto)));
#endif	/* PROTO_RIP */
			} break;
case 254:
# line 2430 "parser.y"
{
#ifdef	PROTO_HELLO
				yyval.proto = parse_prop_proto = RTPROTO_HELLO;
				parse_gwlist = &hello_gw_list;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, yyval.proto)));
#endif	/* PROTO_HELLO */
			} break;
case 255:
# line 2442 "parser.y"
{
#ifdef	PROTO_EGP
				yyval.proto = parse_prop_proto = RTPROTO_EGP;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, yyval.proto)));
#endif	/* PROTO_EGP */
			} break;
case 256:
# line 2451 "parser.y"
{
#ifdef	PROTO_BGP
				yyval.proto = parse_prop_proto = RTPROTO_BGP;
				trace(TR_PARSE, 0, "parse: %s PROTO: %s",
					parse_where(),
					gd_lower(trace_bits(rt_proto_bits, yyval.proto)));
#endif	/* PROTO_BGP */
			} break;
case 257:
# line 2461 "parser.y"
{ yyval.num = T_ON; } break;
case 258:
# line 2462 "parser.y"
{ yyval.num = T_OFF; } break;
case 259:
# line 2465 "parser.y"
{ yyval.num = yypvt[-0].num; } break;
case 260:
# line 2466 "parser.y"
{ yyval.num = T_QUIET; } break;
case 261:
# line 2467 "parser.y"
{ yyval.num = T_SUPPLIER; } break;
case 262:
# line 2468 "parser.y"
{ yyval.num = T_POINTOPOINT; } break;
case 263:
# line 2474 "parser.y"
{
				yyval.metric = yypvt[-0].num;
				trace(TR_PARSE, 0, "parse: %s METRIC: %d",
					parse_where(),
					yyval.metric);
			} break;
case 264:
# line 2483 "parser.y"
{
				yyval.metric = -1;
			} break;
case 265:
# line 2487 "parser.y"
{
				yyval.metric = yypvt[-0].metric;
				if (parse_metric_check(parse_prop_proto, yypvt[-0].metric)) {
				    PARSE_ERROR;
				}
			} break;
case 266:
# line 2498 "parser.y"
{
				/* Take advantage of the fact that only interfaces can have a preference of zero */
				yyval.pref = 0;
			} break;
case 267:
# line 2503 "parser.y"
{
				yyval.pref = yypvt[-0].pref;
			} break;
case 268:
# line 2509 "parser.y"
{
				if (parse_limit_check("preference", yypvt[-0].num, LIMIT_PREFERENCE_LOW, LIMIT_PREFERENCE_HIGH)) {
					PARSE_ERROR;
				}
				yyval.pref = yypvt[-0].num;
			} break;
case 269:
# line 2516 "parser.y"
{
				if (parse_limit_check("preference", yypvt[-0].num, LIMIT_PREFERENCE_LOW, LIMIT_PREFERENCE_HIGH)) {
					PARSE_ERROR;
				}
				yyval.pref = yypvt[-0].num;
			} break;
case 270:
# line 2523 "parser.y"
{
				yyval.pref = yypvt[-2].num + yypvt[-0].num;
				if (parse_limit_check("preference", yyval.pref, LIMIT_PREFERENCE_LOW, LIMIT_PREFERENCE_HIGH)) {
					PARSE_ERROR;
				}
			} break;
case 271:
# line 2530 "parser.y"
{
				yyval.pref = yypvt[-2].num - yypvt[-0].num;
				if (parse_limit_check("preference", yyval.pref, LIMIT_PREFERENCE_LOW, LIMIT_PREFERENCE_HIGH)) {
					PARSE_ERROR;
				}
			} break;
case 272:
# line 2539 "parser.y"
{ yyval.num = RTPREF_DIRECT; } break;
case 273:
# line 2540 "parser.y"
{ yyval.num = RTPREF_DEFAULT; } break;
case 274:
# line 2541 "parser.y"
{ yyval.num = RTPREF_REDIRECT; } break;
case 275:
# line 2542 "parser.y"
{ yyval.num = RTPREF_STATIC; } break;
case 276:
# line 2545 "parser.y"
{ yyval.num = RTPREF_HELLO; } break;
case 277:
# line 2546 "parser.y"
{ yyval.num = RTPREF_RIP; } break;
case 278:
# line 2547 "parser.y"
{ yyval.num = RTPREF_BGP; } break;
case 279:
# line 2548 "parser.y"
{ yyval.num = RTPREF_EGP; } break;
case 280:
# line 2549 "parser.y"
{ yyval.num = RTPREF_KERNEL; } break;
case 281:
# line 2554 "parser.y"
{
				/* Remove quotes from the string if present */
				char *cp;

				cp = yypvt[-0].ptr;
				if (*cp == '"' || *cp == '<') {
					cp++;
				}
				yyval.ptr = parse_strdup(cp);
				cp = yyval.ptr;
				cp += strlen(cp) - 1;
				if (*cp == '"' || *cp == '>') {
					*cp = (char) 0;
				}
				trace(TR_PARSE, 0, "parse: %s STRING: \"%s\"",
					parse_where(),
					yyval.ptr);
			} break;
case 282:
# line 2575 "parser.y"
{
				if (parse_limit_check("octet", yypvt[-0].num, LIMIT_OCTET_LOW, LIMIT_OCTET_HIGH)) {
					PARSE_ERROR;
				}
				yyval.num = yypvt[-0].num;
			} break;
case 283:
# line 2585 "parser.y"
{
			  	if (parse_limit_check("seconds", yypvt[-0].num, 0, -1)) {
				  	PARSE_ERROR;
				};
				yyval.time = yypvt[-0].num;
			} break;
case 284:
# line 2592 "parser.y"
{
			  	if (parse_limit_check("minutes", yypvt[-2].num, 0, -1)) {
				  	PARSE_ERROR;
				}
			  	if (parse_limit_check("seconds", yypvt[-0].num, 0, 59)) {
				  	PARSE_ERROR;
				}
				yyval.time = (yypvt[-2].num * 60) + yypvt[-0].num;
			} break;
case 285:
# line 2602 "parser.y"
{
			  	if (parse_limit_check("hours", yypvt[-4].num, 0, -1)) {
				  	PARSE_ERROR;
				}
			  	if (parse_limit_check("minutes", yypvt[-2].num, 0, 59)) {
				  	PARSE_ERROR;
				}
			  	if (parse_limit_check("seconds", yypvt[-0].num, 0, 59)) {
				  	PARSE_ERROR;
				}
				yyval.time = ((yypvt[-4].num * 60) + yypvt[-2].num) * 60 + yypvt[-0].num;
			} break;
case 286:
# line 2618 "parser.y"
{
			    if (parse_limit_check("port", yypvt[-0].num, LIMIT_PORT_LOW, LIMIT_PORT_HIGH)) {
				PARSE_ERROR;
			    }
			    yyval.num = yypvt[-0].num;
			} break;
case 287:
# line 2625 "parser.y"
{
			    struct servent *sp;

			    if (!(sp = getservbyname(yypvt[-0].ptr, parse_serv_proto))) {
				(void) sprintf(parse_error, "unknown protocol %s/%s",
					yypvt[-0].ptr, parse_serv_proto);
				PARSE_ERROR;
			    }

			    yyval.num = sp->s_port;

			    trace(TR_PARSE, 0, "parse: %s PORT %s (%d)",
				  parse_where(),
				  yypvt[-0].ptr,
				  ntohs(yyval.num));
			} break;
	}
	goto yystack;		/* reset registers in driver code */
}

# ifdef __RUNTIME_YYMAXDEPTH

static int allocate_stacks() {
	/* allocate the yys and yyv stacks */
	yys = (int *) malloc(yymaxdepth * sizeof(int));
	yyv = (YYSTYPE *) malloc(yymaxdepth * sizeof(YYSTYPE));

	if (yys==0 || yyv==0) {
	   yyerror( (nl_msg(30004,"unable to allocate space for yacc stacks")) );
	   return(1);
	   }
	else return(0);

}


static void free_stacks() {
	if (yys!=0) free((char *) yys);
	if (yyv!=0) free((char *) yyv);
}

# endif  /* defined(__RUNTIME_YYMAXDEPTH) */

