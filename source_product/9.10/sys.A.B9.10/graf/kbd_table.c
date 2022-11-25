/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/graf/RCS/kbd_table.c,v $
 * $Revision: 1.5.83.7 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/08/10 10:58:54 $
 */

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


#include "../graf/kbd_chars.h"
#include "../graf/kbd.h"

/*	160-201						*/
#define	K_LANG12	167		/* - (90) */
#define	K_LANG13	169		/* = (91) */
#define	K_LANG14	177		/* [ (92) */
#define	K_LANG15	179		/* ] (93) */
#define	K_LANG16	182		/* ; (94) */
#define	K_LANG17	184		/* ' (95) */
#define	K_LANG20	191		/* / (98) */
#define	K_LANG21	197		/* q (104) */
#define	K_LANG22	199		/* w (105) */
#define	K_LANG23	175		/* y (109) */
#define	K_LANG24	201		/* a (112) */
#define	K_LANG26	187		/* z (120) */
#define	K_LANG32	193		/* NP_',' (Shift-62) */
#define	K_LANG33	194		/* NP_'+' (Shift-63) */
#define	K_LANG37	192		/* NP_'-' (Shift-67) */
#define	K_LANG41	185		/* NP_'*' (Shift-71) */
#define	K_LANG45	180		/* NP_'/' (Shift-75) */
#define	K_LANG46	170		/* NP_'`' (Shift-76) */
#define	K_LANG47	171		/* NP_'|' (Shift-77) */
#define	K_LANG48	172		/* NP_'\' (Shift-78) */
#define	K_LANG49	173		/* NP_'~' (Shift-79) */
#define	K_LANG51	160		/* @ (Shift-81) */
#define	K_LANG52	195		/* # (Shift-82) */
#define	K_LANG55	161		/* ^ (Shift-85) */
#define	K_LANG56	162		/* & (Shift-86) */
#define	K_LANG57	163		/* * (Shift-87) */
#define	K_LANG58	164		/* ( (Shift-88) */
#define	K_LANG59	165		/* ) (Shift-89) */
#define	K_LANG60	166		/* _ (Shift-90) */
#define	K_LANG61	168		/* + (Shift-91) */
#define	K_LANG62	176		/* { (Shift-92) */
#define	K_LANG63	178		/* } (Shift-93) */
#define	K_LANG64	181		/* : (Shift-94) */
#define	K_LANG65	183		/* " (Shift-95) */
#define	K_LANG66	188		/* < (Shift-96) */
#define	K_LANG67	189		/* > (Shift-97) */
#define	K_LANG68	190		/* ? (Shift-98) */
#define	K_LANG69	196		/* Q (Shift-104) */
#define	K_LANG70	198		/* W (Shift-105) */
#define	K_LANG71	174		/* Y (Shift-109) */
#define	K_LANG72	200		/* A (Shift-112) */
#define	K_LANG74	186		/* Z (Shift-120) */
#define	K_LANG75	235		/* AT-keyboard */
#define	K_LANG76	236		/* AT-keyboard */
#define	K_LANG77	237		/* AT-keyboard */
#define	K_LANG78	238		/* AT-keyboard */
#define	K_LANG79	239		/* AT-keyboard */


/* Added Support Both NIMITZ and ITF 			*/
/*	202-234						*/
#define	K_LANG0		202		/* ` (1) */
#define	K_LANG1		203		/* \ (2) */
#define	K_LANG2		204		/* 1 (80) */
#define	K_LANG3		205		/* 2 (81) */
#define	K_LANG4		206		/* 3 (82) */
#define	K_LANG5		207		/* 4 (83) */
#define	K_LANG6		208		/* 5 (84) */
#define	K_LANG7		209		/* 6 (85) */
#define	K_LANG8		210		/* 7 (86) */
#define	K_LANG9		211		/* 8 (87) */
#define	K_LANG10	212		/* 9 (88) */
#define	K_LANG11	213		/* 0 (89) */
#define	K_LANG18	214		/* , (96) */
#define	K_LANG19	215		/* . (97) */
#define	K_LANG25	216		/* m (119) */
#define	K_LANG27	217		/* ~ (Shift-1) */
#define	K_LANG28	218		/* | (Shift-2) */
#define	K_LANG29	219		/* DEL/BACKSPACE (Shift-46) */
#define	K_LANG30	220		/* NP_'0' (Shift-60) */
#define	K_LANG31	221		/* NP_'.' (Shift-61) */
#define	K_LANG34	222		/* NP_'1' (Shift-64) */
#define	K_LANG35	223		/* NP_'2' (Shift-65) */
#define	K_LANG36	224		/* NP_'3' (Shift-66) */
#define	K_LANG38	225		/* NP_'4' (Shift-68) */
#define	K_LANG39	226		/* NP_'5' (Shift-69) */
#define	K_LANG40	227		/* NP_'6' (Shift-70) */
#define	K_LANG42	228		/* NP_'7' (Shift-72) */
#define	K_LANG43	229		/* NP_'8' (Shift-73) */
#define	K_LANG44	230		/* NP_'9' (Shift-74) */
#define	K_LANG50	231		/* ! (Shift-80) */
#define	K_LANG53	232		/* $ (Shift-83) */
#define	K_LANG54	233		/* % (Shift-84) */
#define	K_LANG73	234		/* M (Shift-119) */


/* Typewriter keyboard table for KATAKANA */
unsigned char ntz_katab[] = {
	199,204,177,179,180,181,212,213,214,220,
	206,205,209,219,218,185,200,217,210,32,
	215,190,201,216,192,195,178,189,182,221,197,198,
	193,196,188,202,183,184,207,211,194,187,191,203,
	186,208,
	199,204,167,169,170,171,172,173,174,166,
	176,205,222,223,218,185,164,161,165,32,
	215,190,201,216,192,195,168,189,182,221,197,198,
	193,196,188,202,183,184,207,211,175,187,191,203,
	186,208,
};

/* Typewriter keyboard table for EXTEND */
struct {
	unsigned char unshift, shift;
} itf_extend[] = {
	R_i,		R_i,		/* 80 1 */
	'@',		'@',		/* 81 2 */
	'#',		'#',		/* 82 3 */
	R_ONEFOURTH,	R_THREEFOURTH,	/* 83 4 */
	R_ONEHALF,	R_ONEHALF,	/* 84 5 */
	'^',		'^',		/* 85 6 */
	'\\',		'\\',		/* 86 7 */
	'[',		'{',		/* 87 8 */
	']',		'}',		/* 88 9 */
	R_UQUES,	R_UQUES,	/* 89 0 */
	R_MIDLINE,	R_OVERBAR,	/* 90 - */
	R_PLUSMINUS,	R_PLUSMINUS,	/* 91 = */
	R_DEGREE,	R_DEGREE,	/* 92 [ */
	'|',		'|',		/* 93 ] */
	R_DPOUND,	R_DPOUND,	/* 94 ; */
	'`',		'\'',		/* 95 ' */
	'<',		'<',		/* 96 , */
	'>',		'>',		/* 97 . */
	'_',		'_',		/* 98 / */
	K_SPACE,	K_SPACE,	/* 99 space */
	R_zero,		R_ZERO,		/* 100 o */
	R_ip,		R_IP,		/* 100 p */
	R_CENT,		R_CENT,		/* 100 k */
	R_SPOUND,	R_SPOUND,	/* 100 l */
	R_MIDDOT,	R_MIDDOT,	/* 104 q */
	'~',		'~',		/* 105 w */
	R_ae,		R_AE,		/* 106 e */
	R_BQUOTE,	R_BQUOTE,	/* 107 r */
	R_FQUOTE,	R_FQUOTE,	/* 108 t */
	R_HAT,		R_HAT,		/* 109 y */
	R_DDOT,		R_DDOT,		/* 110 u */
	R_TILTA,	R_TILTA,	/* 111 i */
	R_a_DOT,	R_A_DOT,	/* 112 a */
	R_BETA,		R_BETA,		/* 113 s */
	R_d_CROSS,	R_D_CROSS,	/* 114 d */
	R_F,		R_F,		/* 115 f */
	R_OX,		R_OX,		/* 116 g */
	R_YEN,		R_YEN,		/* 117 h */
	'$',		'$',		/* 118 j */
	R_OUSCORE,	R_OUSCORE,	/* 119 m */
	R_PILCROW,	R_PILCROW,	/* 120 z */
	R_s_V,		R_S_V,		/* 121 x */
	R_c_BEARD,	R_C_BEARD,	/* 122 c */
	R_SO,		R_SO,		/* 123 v */
	R_BLACK,	R_BLACK,	/* 124 b */
	R_AUSCORE,	R_AUSCORE,	/* 125 n */
};


/* Language Jumper Conversion Table */
char lang_jump[42] = {
	K_N_USASCII,		/* 0 */	/* NIMITZ keyboards */
	K_N_FRENCHQ,		/* 1 */
	K_N_GERMAN,		/* 2 */
	K_N_SWEDISH,		/* 3 */
	K_N_SPANISH,		/* 4 */
	K_N_KATAKANA,		/* 5 */
	K_N_FRENCHA,		/* 6 */
	K_N_USASCII,		/* 7 */
	K_N_USASCII,		/* 8 */
	K_N_USASCII,		/* 9 */

	K_I_USASCII,		/* 0 */	/* ITF150 keyboards */
	K_I_USASCII,		/* 1 */
	K_I_KANJI,		/* 2 */
	K_I_SWISSFRENCH,	/* 3 */
	K_I_USASCII,		/* 4 */
	K_I_USASCII,		/* 5 */
	K_I_USASCII,		/* 6 */
	K_I_CANENG,		/* 7 */
	K_I_USASCII,		/* 8 */
	K_I_USASCII,		/* 9 */
	K_I_USASCII,		/* 10 */
	K_I_ITALIAN,		/* 11 */
	K_I_USASCII,		/* 12 */
	K_I_DUTCH,		/* 13 */
	K_I_SWEDISH,		/* 14 */
	K_I_GERMAN,		/* 15 */
	K_I_USASCII,		/* 16 */
	K_I_USASCII,		/* 17 */
	K_I_SWISSFRENCH2,	/* 18 */
	K_I_EUROSPANISH,	/* 19 */
	K_I_SWISSGERMAN2,	/* 20 */
	K_I_BELGIAN,		/* 21 */
	K_I_FINNISH,		/* 22 */
	K_I_UNITEDK,		/* 23 */
	K_I_CANFRENCH,		/* 24 */
	K_I_SWISSGERMAN,	/* 25 */
	K_I_NORWEGIAN,		/* 26 */
	K_I_FRENCH,		/* 27 */
	K_I_DANISH,		/* 28 */
	K_I_KATAKANA,		/* 29 */
	K_I_LATSPANISH,		/* 30 */
	K_I_USASCII,		/* 31 */
};

unsigned short mute_enable[28] = {
	K_MUTE,	/* (0) NIMITZ US Standard */
	K_MUTE,	/* (1) NIMITZ French - QWERTY */
	K_MUTE,	/* (2) NIMITZ German */
	K_MUTE,	/* (3) NIMITZ Swedish-Finish */
	K_MUTE,	/* (4) NIMITZ Spanish */
	0,	/* (5) NIMITZ Katanana */
	K_MUTE,	/* (6) NIMITZ French - AZERTY */
	K_MUTE,	/* (7) ITF150 United States */
	K_MUTE,	/* (8) ITF150 Belgian (Mute in 8 bit only) */
	K_MUTE,	/* (9) ITF150 Canadian English (Mute in 8 bit only) */
	K_MUTE,	/* (10) ITF150 Danish (Mute in 8 bit only) */
	K_MUTE,	/* (11) ITF150 Dutch (Mute in 8 bit only) */
	K_MUTE,	/* (12) ITF150 Finnish (Mute Characters - None) */
	K_MUTE,	/* (13) ITF150 French (AZERTY) (Mute in both 7 and 8 bit modes) */
	K_MUTE,	/* (14) ITF150 Canadian French (Mute in 8 bit mode only) */
	K_MUTE,	/* (15) ITF150 Swiss French (Mute in 8 bit mode only) */
	K_MUTE,	/* (16) ITF150 German (Mute Characters - None) */
	K_MUTE,	/* (17) ITF150 Swiss German (Mute in 8 bit mode only) */
	K_MUTE,	/* (18) ITF150 Italian (Mute in 8 bit mode only) */
	K_MUTE,	/* (19) ITF150 Norwegian (Mute in 8 bit mode only) */
	K_MUTE,	/* (20) ITF150 European Spanish (Mute in 8 bit mode only) */
	K_MUTE,	/* (21) ITF150 Latin Spanish (Mute in both 7 and 8 bit modes) */
	K_MUTE,	/* (22) ITF150 Swedish (Mute in 8 bit mode only) */
	K_MUTE,	/* (23) ITF150 United Kingdom (Mute Characters - None) */
	0,	/* (24) ITF150 Katakana (Mute Characters - None) */
	K_MUTE,	/* (25) ITF150 Swiss French II (Mute in 8 bit mode only) */
	K_MUTE,	/* (26) ITF150 Swiss German II (Mute in 8 bit mode only) */
	0,	/* (27) ITF150 Kanji (Mute Characters - None) */
};
/* Look-up table for 8041.
	- 8 bit index consists of 7 lsb keycode and MSB shift bit.
*/
struct {
	unsigned short unshift, shift;
} k_code[] = {
	K_ILLEGAL,	K_ILLEGAL,	/*   0 */
	K_LANG0,	K_LANG27,	/*   1 */	/* ` ~ */
	K_LANG1,	K_LANG28,	/*   2 */	/* \ | */
	K_ESC,		K_DEL,		/*   3 */
	K_ILLEGAL,	K_ILLEGAL,	/*   4 */
	K_BREAK,	K_RESET_KEY,	/*   5 */
	K_STOP,		K_STOP,		/*   6 */
	K_SELECT,	K_SELECT,	/*   7 */
	K_NP_ENTER,	K_NP_ENTER,	/*   8 */
	K_NP_TAB,	K_NP_TAB,	/*   9 */
	K_NP_K0,	K_NP_K0,	/*  10 */
	K_NP_K1,	K_NP_K1,	/*  11 */
	K_NP_K2,	K_NP_K2,	/*  12 */
	K_NP_K3,	K_NP_K3,	/*  13 */
	K_HOME_ARROW,	K_HOME_ARROW,	/*  14 */
	K_PREV,		K_PREV,		/*  15 */
	K_NEXT,		K_NEXT,		/*  16 */
	K_ENTER,	K_PRINT,	/*  17 */
	K_ILLEGAL,	K_ILLEGAL,	/*  18 */	/* left-extend down */
	K_ILLEGAL,	K_ILLEGAL,	/*  19 */	/* right-extend down */
	K_SYSTEM,	K_USER,		/*  20 */
	K_MENU,		K_MENU,		/*  21 */
	K_CLR_LINE,	K_CLR_LINE,	/*  22 */
	K_CLR_DISP,	K_CLR_DISP,	/*  23 */
	K_CAPS_LOCK,	K_CAPS_LOCK,	/*  24 */
	K_TAB,		K_TAB,		/*  25 */
	K_K0,		K_K0,		/*  26 */
	K_K1,		K_K1,		/*  27 */
	K_K2,		K_K2,		/*  28 */
	K_K5,		K_K5,		/*  29 */
	K_K6,		K_K6,		/*  30 */
	K_K7,		K_K7,		/*  31 */
	K_K3,		K_K3,		/*  32 */
	K_K4,		K_K4,		/*  33 */
	K_DOWN_ARROW,	K_DOWN_ARROW,	/*  34 */
	K_UP_ARROW,	K_UP_ARROW,	/*  35 */
	K_K8,		K_K8,		/*  36 */
	K_K9,		K_K9,		/*  37 */
	K_LEFT_ARROW,	K_LEFT_ARROW,	/*  38 */
	K_RIGHT_ARROW,	K_RIGHT_ARROW,	/*  39 */
	K_INSERT_LINE,	K_INSERT_LINE,	/*  40 */
	K_DELETE_LINE,	K_DELETE_LINE,	/*  41 */
	K_RECALL,	K_RECALL,	/*  42 */
	K_INSERT_CHAR,	K_INSERT_CHAR,	/*  43 */
	K_DELETE_CHAR,	K_DELETE_CHAR,	/*  44 */
	K_CLEAR_TO_END,	K_CLEAR_TO_END,	/*  45 */
	K_BACKSPACE,	K_LANG29,	/*  46 */	/* BS DEL */
	K_RUN,		K_RUN,		/*  47 */
	K_EDIT,		K_DISP_FCTNS,	/*  48 */
	K_ALPHA,	K_DUMP_ALPHA,	/*  49 */
	K_GRAPHICS,	K_DUMP_GRAPHICS,/*  50 */
	K_STEP,		K_ANY_CHAR,	/*  51 */
	K_N_CLEAR_LINE,	K_N_CLEAR_SCR,	/*  52 */
	K_RESULT,	K_SET_TAB,	/*  53 */
	K_PRT_ALL,	K_CLR_TAB,	/*  54 */
	K_CLR_IO,	K_N_STOP,	/*  55 */
	K_PAUSE,	K_PAUSE,	/*  56 */
	K_RETURN,	K_RETURN,	/*  57 */	/* Nimitz ENTER */
	K_CONTINUE,	K_CONTINUE,	/*  58 */
	K_EXECUTE,	K_EXECUTE,	/*  59 */
	K_NP_ZERO,	K_LANG30,	/*  60 */	/* NP-^ NP-0 */
	K_LANG79,	K_LANG31,	/*  61 */	/* NP-~ NP-. */
	K_NP_COMMA,	K_LANG32,	/*  62 */	/* NP-, NP-, */
	K_NP_PLUS,	K_LANG33,	/*  63 */	/* NP-+ NP-+ */
	K_NP_ONE,	K_LANG34,	/*  64 */	/* NP-{ NP-1 */
	K_NP_TWO,	K_LANG35,	/*  65 */	/* NP-| NP-2 */
	K_NP_THREE,	K_LANG36,	/*  66 */	/* NP-} NP-3 */
	K_NP_MINUS,	K_LANG37,	/*  67 */	/* NP-- NP-- */
	K_NP_FOUR,	K_LANG38,	/*  68 */	/* NP-[ NP-4 */
	K_NP_FIVE,	K_LANG39,	/*  69 */	/* NP-\ NP-5 */
	K_NP_SIX,	K_LANG40,	/*  70 */	/* NP-] NP-6 */
	K_NP_MULT,	K_LANG41,	/*  71 */	/* NP-* NP-* */
	K_NP_SEVEN,	K_LANG42,	/*  72 */	/* NP-# NP-7 */
	K_NP_EIGHT,	K_LANG43,	/*  73 */	/* NP-` NP-8 */
	K_NP_NINE,	K_LANG44,	/*  74 */	/* NP-@ NP-9 */
	K_NP_DIV,	K_LANG45,	/*  75 */	/* NP-/ NP-/ */
	K_NP_E,		K_LANG46,	/*  76 */	/* NP-E NP-` */
	K_NP_LPAR,	K_LANG47,	/*  77 */	/* NP-( NP-| */
	K_NP_RPAR,	K_LANG48,	/*  78 */	/* NP-) NP-\ */
	K_NP_CARAT,	K_LANG49,	/*  79 */	/* NP-^ NP-~ */
	K_LANG2,	K_LANG50,	/*  80 */	/* 1 ! */
	K_LANG3,	K_LANG51,	/*  81 */	/* 2 @ */
	K_LANG4,	K_LANG52,	/*  82 */	/* 3 # */
	K_LANG5,	K_LANG53,	/*  83 */	/* 4 $ */
	K_LANG6,	K_LANG54,	/*  84 */	/* 5 % */
	K_LANG7,	K_LANG55,	/*  85 */	/* 6 ^ */
	K_LANG8,	K_LANG56,	/*  86 */	/* 7 & */
	K_LANG9,	K_LANG57,	/*  87 */	/* 8 * */
	K_LANG10,	K_LANG58,	/*  88 */	/* 9 ( */
	K_LANG11,	K_LANG59,	/*  89 */	/* 0 ) */
	K_LANG12,	K_LANG60,	/*  90 */	/* - _ */
	K_LANG13,	K_LANG61,	/*  91 */	/* = + */
	K_LANG14,	K_LANG62,	/*  92 */	/* [ { */
	K_LANG15,	K_LANG63,	/*  93 */	/* ] } */
	K_LANG16,	K_LANG64,	/*  94 */	/* ; : */
	K_LANG17,	K_LANG65,	/*  95 */	/* ' " */
	K_LANG18,	K_LANG66,	/*  96 */	/* , < */
	K_LANG19,	K_LANG67,	/*  97 */	/* . > */
	K_LANG20,	K_LANG68,	/*  98 */	/* / ? */
	K_SPACE,	K_SPACE,	/*  99 */
	'o',		'O',		/* 100 */
	'p',		'P',		/* 101 */
	'k',		'K',		/* 102 */
	'l',		'L',		/* 103 */
	K_LANG21,	K_LANG69,	/* 104 */	/* q Q */
	K_LANG22,	K_LANG70,	/* 105 */	/* w W */
	'e',		'E',		/* 106 */
	'r',		'R',		/* 107 */
	't',		'T',		/* 108 */
	K_LANG23,	K_LANG71,	/* 109 */	/* y Y */
	'u',		'U',		/* 110 */
	'i',		'I',		/* 111 */
	K_LANG24,	K_LANG72,	/* 112 */	/* a A */
	's',		'S',		/* 113 */
	'd',		'D',		/* 114 */
	'f',		'F',		/* 115 */
	'g',		'G',		/* 116 */
	'h',		'H',		/* 117 */
	'j',		'J',		/* 118 */
	K_LANG25,	K_LANG73,	/* 119 */	/* m M */
	K_LANG26,	K_LANG74,	/* 120 */	/* z Z */
	'x',		'X',		/* 121 */
	'c',		'C',		/* 122 */
	'v',		'V',		/* 123 */
	'b',		'B',		/* 124 */
	'n',		'N',		/* 125 */
	K_ILLEGAL,	K_ILLEGAL,	/* 126 */
	K_ILLEGAL,	K_ILLEGAL,	/* 127 */
	K_ILLEGAL,	K_ILLEGAL,	/* 128 */
	K_ILLEGAL,	K_ILLEGAL,	/* 129 */
	K_ILLEGAL,	K_ILLEGAL,	/* 130 */
	K_ILLEGAL,	K_ILLEGAL,	/* 131 */
	K_ILLEGAL,	K_ILLEGAL,	/* 132 */
	K_ILLEGAL,	K_ILLEGAL,	/* 133 */
	K_ILLEGAL,	K_ILLEGAL,	/* 134 */
	K_ILLEGAL,	K_ILLEGAL,	/* 135 */
	K_ILLEGAL,	K_ILLEGAL,	/* 136 */
	K_ILLEGAL,	K_ILLEGAL,	/* 137 */
	K_ILLEGAL,	K_ILLEGAL,	/* 138 */
	K_ILLEGAL,	K_ILLEGAL,	/* 139 */
	K_ILLEGAL,	K_ILLEGAL,	/* 140 */
	K_ILLEGAL,	K_ILLEGAL,	/* 141 */
	K_ILLEGAL,	K_ILLEGAL,	/* 142 */
	K_ILLEGAL,	K_ILLEGAL,	/* 143 */
	K_ILLEGAL,	K_ILLEGAL,	/* 144 */
	K_ILLEGAL,	K_ILLEGAL,	/* 145 */
	K_ILLEGAL,	K_ILLEGAL,	/* 146 */
	K_ILLEGAL,	K_ILLEGAL,	/* 147 */
	K_ILLEGAL,	K_ILLEGAL,	/* 148 */
	K_ILLEGAL,	K_ILLEGAL,	/* 149 */
	K_ILLEGAL,	K_ILLEGAL,	/* 150 */
	K_ILLEGAL,	K_ILLEGAL,	/* 151 */
	K_ILLEGAL,	K_ILLEGAL,	/* 152 */
	K_ILLEGAL,	K_ILLEGAL,	/* 153 */
	K_ILLEGAL,	K_ILLEGAL,	/* 154 */
	K_ILLEGAL,	K_ILLEGAL,	/* 155 */
	K_ILLEGAL,	K_ILLEGAL,	/* 156 */
	K_ILLEGAL,	K_ILLEGAL,	/* 157 */
	K_ILLEGAL,	K_ILLEGAL,	/* 158 */
	K_ILLEGAL,	K_ILLEGAL,	/* 159 */
	K_ILLEGAL,	K_ILLEGAL,	/* 160 */
	K_ILLEGAL,	K_ILLEGAL,	/* 161 */
	K_ILLEGAL,	K_ILLEGAL,	/* 162 */
	K_ILLEGAL,	K_ILLEGAL,	/* 163 */
	K_ILLEGAL,	K_ILLEGAL,	/* 164 */
	K_ILLEGAL,	K_ILLEGAL,	/* 165 */
	K_ILLEGAL,	K_ILLEGAL,	/* 166 */
	K_ILLEGAL,	K_ILLEGAL,	/* 167 */
	K_ILLEGAL,	K_ILLEGAL,	/* 168 */
	K_ILLEGAL,	K_ILLEGAL,	/* 169 */
	K_LANG75,	K_LANG76,	/* 170 */
	K_LANG77,	K_LANG78,	/* 171 */
	K_ILLEGAL,	K_ILLEGAL,	/* 172 */
	K_ILLEGAL,	K_ILLEGAL,	/* 173 */
	K_ILLEGAL,	K_ILLEGAL,	/* 174 */
	K_ILLEGAL,	K_ILLEGAL,	/* 175 */
	K_ILLEGAL,	K_ILLEGAL,	/* 176 */
	K_ILLEGAL,	K_ILLEGAL,	/* 177 */
	K_ILLEGAL,	K_ILLEGAL,	/* 178 */
	K_ILLEGAL,	K_ILLEGAL,	/* 179 */
	K_ILLEGAL,	K_ILLEGAL,	/* 180 */
	K_ILLEGAL,	K_ILLEGAL,	/* 181 */
	K_ILLEGAL,	K_ILLEGAL,	/* 182 */
	K_ILLEGAL,	K_ILLEGAL,	/* 183 */
	K_ILLEGAL,	K_ILLEGAL,	/* 184 */
	K_ILLEGAL,	K_ILLEGAL,	/* 185 */
	K_ILLEGAL,	K_ILLEGAL,	/* 186 */
	K_ILLEGAL,	K_ILLEGAL,	/* 187 */
	K_ILLEGAL,	K_ILLEGAL,	/* 188 */
	K_ILLEGAL,	K_ILLEGAL,	/* 189 */
	K_ILLEGAL,	K_ILLEGAL,	/* 190 */
	K_ILLEGAL,	K_ILLEGAL,	/* 191 */
	K_ILLEGAL,	K_ILLEGAL,	/* 192 */
	K_ILLEGAL,	K_ILLEGAL,	/* 193 */
	K_ILLEGAL,	K_ILLEGAL,	/* 194 */
	K_ILLEGAL,	K_ILLEGAL,	/* 195 */
	K_ILLEGAL,	K_ILLEGAL,	/* 196 */
	K_ILLEGAL,	K_ILLEGAL,	/* 197 */
	K_ILLEGAL,	K_ILLEGAL,	/* 198 */
	K_ILLEGAL,	K_ILLEGAL,	/* 199 */
	K_ILLEGAL,	K_ILLEGAL,	/* 200 */
	K_ILLEGAL,	K_ILLEGAL,	/* 201 */
	K_ILLEGAL,	K_ILLEGAL,	/* 202 */
	K_ILLEGAL,	K_ILLEGAL,	/* 203 */
	K_ILLEGAL,	K_ILLEGAL,	/* 204 */
	K_ILLEGAL,	K_ILLEGAL,	/* 205 */
	K_ILLEGAL,	K_ILLEGAL,	/* 206 */
	K_ILLEGAL,	K_ILLEGAL,	/* 207 */
	K_ILLEGAL,	K_ILLEGAL,	/* 208 */
	K_ILLEGAL,	K_ILLEGAL,	/* 209 */
	K_ILLEGAL,	K_ILLEGAL,	/* 210 */
	K_ILLEGAL,	K_ILLEGAL,	/* 211 */
	K_ILLEGAL,	K_ILLEGAL,	/* 212 */
	K_ILLEGAL,	K_ILLEGAL,	/* 213 */
	K_ILLEGAL,	K_ILLEGAL,	/* 214 */
	K_ILLEGAL,	K_ILLEGAL,	/* 215 */
	K_ILLEGAL,	K_ILLEGAL,	/* 216 */
	K_ILLEGAL,	K_ILLEGAL,	/* 217 */
	K_ILLEGAL,	K_ILLEGAL,	/* 218 */
	K_ILLEGAL,	K_ILLEGAL,	/* 219 */
	K_ILLEGAL,	K_ILLEGAL,	/* 220 */
	K_ILLEGAL,	K_ILLEGAL,	/* 221 */
	K_ILLEGAL,	K_ILLEGAL,	/* 222 */
	K_ILLEGAL,	K_ILLEGAL,	/* 223 */
	K_ILLEGAL,	K_ILLEGAL,	/* 224 */
	K_ILLEGAL,	K_ILLEGAL,	/* 225 */
	K_ILLEGAL,	K_ILLEGAL,	/* 226 */
	K_ILLEGAL,	K_ILLEGAL,	/* 227 */
	K_ILLEGAL,	K_ILLEGAL,	/* 228 */
	K_ILLEGAL,	K_ILLEGAL,	/* 229 */
	K_ILLEGAL,	K_ILLEGAL,	/* 230 */
	K_ILLEGAL,	K_ILLEGAL,	/* 231 */
	K_ILLEGAL,	K_ILLEGAL,	/* 232 */
	K_ILLEGAL,	K_ILLEGAL,	/* 233 */
	K_ILLEGAL,	K_ILLEGAL,	/* 234 */
	K_ILLEGAL,	K_ILLEGAL,	/* 235 */
	K_ILLEGAL,	K_ILLEGAL,	/* 236 */
	K_ILLEGAL,	K_ILLEGAL,	/* 237 */
	K_ILLEGAL,	K_ILLEGAL,	/* 238 */
	K_ILLEGAL,	K_ILLEGAL,	/* 239 */
	K_ILLEGAL,	K_ILLEGAL,	/* 240 */
	K_ILLEGAL,	K_ILLEGAL,	/* 241 */
	K_ILLEGAL,	K_ILLEGAL,	/* 242 */
	K_ILLEGAL,	K_ILLEGAL,	/* 243 */
	K_ILLEGAL,	K_ILLEGAL,	/* 244 */
	K_ILLEGAL,	K_ILLEGAL,	/* 245 */
	K_ILLEGAL,	K_ILLEGAL,	/* 246 */
	K_ILLEGAL,	K_ILLEGAL,	/* 247 */
	K_ILLEGAL,	K_ILLEGAL,	/* 248 */
	K_ILLEGAL,	K_ILLEGAL,	/* 249 */
	K_ILLEGAL,	K_ILLEGAL,	/* 250 */
	K_ILLEGAL,	K_ILLEGAL,	/* 251 */
	K_ILLEGAL,	K_ILLEGAL,	/* 252 */
	K_ILLEGAL,	K_ILLEGAL,	/* 253 */
	K_ILLEGAL,	K_ILLEGAL,	/* 254 */
	K_ILLEGAL,	K_ILLEGAL,	/* 255 */
};

/* Nationalized Keyboard Tables */
	/*	160-201	*/
	/* K_LANG51	160	@ (Shift-81) */
	/* K_LANG55	161	^ (Shift-85) */
	/* K_LANG56	162	& (Shift-86) */
	/* K_LANG57	163	* (Shift-87) */
	/* K_LANG58	164	( (Shift-88) */
	/* K_LANG59	165	) (Shift-89) */
	/* K_LANG60	166	_ (Shift-90) */
	/* K_LANG12	167	- (90) */
	/* K_LANG61	168	+ (Shift-91) */
	/* K_LANG13	169	= (91) */
	/* K_LANG46	170	NP_'`' (Shift-76) */
	/* K_LANG47	171	NP_'|' (Shift-77) */
	/* K_LANG48	172	NP_'\' (Shift-78) */
	/* K_LANG49	173	NP_'~' (Shift-79) */
	/* K_LANG71	174	Y (Shift-109) */
	/* K_LANG23	175	y (109) */
	/* K_LANG62	176	{ (Shift-92) */
	/* K_LANG14	177	[ (92) */
	/* K_LANG63	178	} (Shift-93) */
	/* K_LANG15	179	] (93) */
	/* K_LANG45	180	NP_'/' (Shift-75) */
	/* K_LANG64	181	: (Shift-94) */
	/* K_LANG16	182	; (94) */
	/* K_LANG65	183	" (Shift-95) */
	/* K_LANG17	184	' (95) */
	/* K_LANG41	185	NP_'*' (Shift-71) */
	/* K_LANG74	186	Z (Shift-120) */
	/* K_LANG26	187	z (120) */
	/* K_LANG66	188	< (Shift-96) */
	/* K_LANG67	189	> (Shift-97) */
	/* K_LANG68	190	? (Shift-98) */
	/* K_LANG20	191	/ (98) */
	/* K_LANG37	192	NP_'-' (Shift-67) */
	/* K_LANG32	193	NP_',' (Shift-62) */
	/* K_LANG33	194	NP_'+' (Shift-63) */
	/* K_LANG52	195	# (Shift-82) */
	/* K_LANG69	196	Q (Shift-104) */
	/* K_LANG21	197	q (104) */
	/* K_LANG70	198	W (Shift-105) */
	/* K_LANG22	199	w (105) */
	/* K_LANG72	200	A (Shift-112) */
	/* K_LANG24	201	a (112) */
	/* K_LANG0	202	` (1) */
	/* K_LANG1	203	\ (2) */
	/* K_LANG2	204	1 (80) */
	/* K_LANG3	205	2 (81) */
	/* K_LANG4	206	3 (82) */
	/* K_LANG5	207	4 (83) */
	/* K_LANG6	208	5 (84) */
	/* K_LANG7	209	6 (85) */
	/* K_LANG8	210	7 (86) */
	/* K_LANG9	211	8 (87) */
	/* K_LANG10	212	9 (88) */
	/* K_LANG11	213	0 (89) */
	/* K_LANG18	214	, (96) */
	/* K_LANG19	215	. (97) */
	/* K_LANG25	216	m (119) */
	/* K_LANG27	217	~ (Shift-1) */
	/* K_LANG28	218	| (Shift-2) */
	/* K_LANG29	219	DEL/BACKSPACE (Shift-46) */
	/* K_LANG30	220	NP_'0' (Shift-60) */
	/* K_LANG31	221	NP_'.' (Shift-61) */
	/* K_LANG34	222	NP_'1' (Shift-64) */
	/* K_LANG35	223	NP_'2' (Shift-65) */
	/* K_LANG36	224	NP_'3' (Shift-66) */
	/* K_LANG38	225	NP_'4' (Shift-68) */
	/* K_LANG39	226	NP_'5' (Shift-69) */
	/* K_LANG40	227	NP_'6' (Shift-70) */
	/* K_LANG42	228	NP_'7' (Shift-72) */
	/* K_LANG43	229	NP_'8' (Shift-73) */
	/* K_LANG44	230	NP_'9' (Shift-74) */
	/* K_LANG50	231	! (Shift-80) */
	/* K_LANG53	232	$ (Shift-83) */
	/* K_LANG54	233	% (Shift-84) */
	/* K_LANG73	234	M (Shift-119) */

typedef unsigned char langtab[80];

langtab k_n_usascii = {		/* NIMITZ US Standard */
	'@', '^', '&', '*', '(', ')', '_', '-', '+', '=',
	'`', '|', '\\', '~', 'Y', 'y', '{', '[', '}', ']',
	'/', ':', ';', '"', 39, '*', 'Z', 'z', '<', '>',
	'?', '/', '-', ',', '+', '#', 'Q', 'q', 'W', 'w',
	'A', 'a', K_NA, K_NA, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', K_NA, K_NA, K_DEL,
	'^', '~', '{', '|', '}', '[', '\\', ']', '#', '`',
	'@', '!', '$', '%', 'M', K_NA, K_NA, K_NA, K_NA, '.',
};

langtab k_n_frenchq = {		/* NIMITZ French - QWERTY */
	34, 43, 47, 40, 41, 61, 63, 39, 171, 170,
	60, 91, 93, 62, 89, 121, 181, 200, 42, 38,
	92, 201, 197, 179, 203, 64, 90, 122, 59, 58,
	95, 45, 96, 124, 126, '#', 'Q', 'q', 'W', 'w',
	'A', 'a', K_NA, K_NA, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', K_NA, K_NA, K_DEL,
	'^', '~', '{', '|', '}', '[', '\\', ']', '#', '`',
	'@', '!', '$', '%', 'M', K_NA, K_NA, K_NA, K_NA, '.',
};

langtab k_n_german = {		/* NIMITZ German */
	'"', '&', '/', '(', ')', '=', '?', 0xDE, 0x60, 0x27,
	'<', '[', ']', '>', 'Z', 'z', 219, 207, '*', '+',
	'\\', 218, 206, 216, 204, '@', 'Y', 'y', ';', ':',
	'_', '-', 0xB3, '|', '~', '#', 'Q', 'q', 'W', 'w',
	'A', 'a', K_NA, K_NA, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', K_NA, K_NA, K_DEL,
	'^', '~', '{', '|', '}', '[', '\\', ']', '#', '`',
	'@', '!', '$', '%', 'M', K_NA, K_NA, K_NA, K_NA, '.',
};

langtab k_n_swedish = {		/* NIMITZ Swedish-Finish */
	'"', '&', '/', '(', ')', '=', '?', '+' ,0xDC, 0xC5,
	'<', '[', ']', '>', 'Y', 'y', 208, 212, 219, 207,
	'\\', 218, 206, 216, 204, '@', 'Z', 'z', ';', ':',
	'_', '-', 0x27, '|', '~', '#', 'Q', 'q', 'W', 'w',
	'A', 'a', K_NA, K_NA, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', K_NA, K_NA, K_DEL,
	'^', '~', '{', '|', '}', '[', '\\', ']', '#', '`',
	'@', '!', '$', '%', 'M', K_NA, K_NA, K_NA, K_NA, '.',
};

langtab k_n_spanish = {		/* NIMITZ Spanish */
	'"', '&', 0xB8, '(', ')', '=', '?', '+', '/', 0xA8,
	'<', '[', ']', '>', 'Y', 'y', '{', 0xB3, '}', '#',
	'\\', 0xB6, 0xB7, '@', '*', '*', 'Z', 'z', ';', ':',
	'_', '-', 0x27, '|', '+', 0xB9, 'Q', 'q', 'W', 'w',
	'A', 'a', K_NA, K_NA, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', K_NA, K_NA, K_DEL,
	'^', '~', '{', '|', '}', '[', '\\', ']', '#', '`',
	'@', '!', '$', '%', 'M', K_NA, K_NA, K_NA, K_NA, '.',
};

langtab k_n_katakana = {		/* NIMITZ Katanana */
	'@', '^', '&', '*', '(', ')', '_', '-', '+', '=',
	'`', '|', '\\', '~', 'Y', 'y', '\\', '[', '|', ']',
	'/', ':', ';', '"', 39, '*', 'Z', 'z', '<', '>',
	'?', '/', '-', ',', '+', '#', 'Q', 'q', 'W', 'w',
	'A', 'a', K_NA, K_NA, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', K_NA, K_NA, K_DEL,
	'^', '~', '{', '|', '}', '[', '\\', ']', '#', '`',
	'@', '!', '$', '%', 'M', K_NA, K_NA, K_NA, K_NA, '.',
};

langtab k_n_frencha = {		/* NIMITZ French - AZERTY */
	34, 43, 47, 40, 41, 61, 63, 39, 171, 170,
	60, 91, 93, 62, 89, 121, 181, 200, 42, 38,
	92, 201, 197, 179, 203, 64, 'W', 'w', 59, 58,
	95, 45, 96, 124, 126, '#', 'A', 'a', 'Z', 'z',
	'Q', 'q', K_NA, K_NA, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', K_NA, K_NA, K_DEL,
	'^', '~', '{', '|', '}', '[', '\\', ']', '#', '`',
	'@', '!', '$', '%', 'M', K_NA, K_NA, K_NA, K_NA, '.',
};

langtab k_i_usascii = {		/* ITF United States & ITF Katakana */
	'@', '^', '&', '*', '(', ')', '_', '-', '+', '=',
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', '{', '[', '}', ']',
	'/', ':', ';', '"', 39, '*', 'Z', 'z', '<', '>',
	'?', '/', '-', ',', '+', '#', 'Q', 'q', 'W', 'w',
	'A', 'a', '`', '\\', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', '~', '|', K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '!', '$', '%', 'M', K_NA, K_NA, K_NA, K_NA, '.',
};

langtab k_i_belgian = {		/* ITF Belgian & ITF French (AZERTY) */
	'2', '6', '7', '8', '9', '0', R_DEGREE, ')', '_', '-',
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', R_DDOT, R_HAT, '*', '`',
	'/', 'M', 'm', '%', R_u_FQUOTE, '*', 'W', 'w', '.', '/',
	'+', '=', '-', ',', '+', '3', 'A', 'a', 'Z', 'z',
	'Q', 'q', '$', '<', '&', R_e_BQUOTE, '"', 39, '(', R_SO,
	R_e_FQUOTE, '!', R_c_BEARD, R_a_FQUOTE, ';', ':', ',', R_SPOUND, '>', K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '1', '4', '5', '?', '#', R_MICRO, '<', '>', '.',
};

langtab k_a_french = {		/* PC-AT French */
	'2', '6', '7', '8', '9', '0', R_DEGREE, ')', '_', '-',
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', R_DDOT, R_HAT, R_SPOUND, '$',
	'/', 'M', 'm', '%', R_u_FQUOTE, '*', 'W', 'w', '.', '/',
	R_SO, '|', '-', ',', '+', '3', 'A', 'a', 'Z', 'z',
	'Q', 'q', '2', '<', '&', R_e_BQUOTE, '"', 39, '(', R_SO,
	R_e_FQUOTE, '!', R_c_BEARD, R_a_FQUOTE, ';', ':', ',', '2', '>', K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '1', '4', '5', '?', '#', R_MICRO, '<', '>', '.',
};

langtab k_i_canadian = {		/* ITF Canadian English/French */
	'"', '?', '&', '*', '(', ')', '_', '-', '+', '=',
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', R_DDOT, R_HAT, R_C_BEARD, R_c_BEARD,
	'/', ':', ';', 39, R_FQUOTE, '*', 'Z', 'z', '<', '>',
	R_E_BQUOTE, R_e_BQUOTE, '-', ',', '+', '/', 'Q', 'q', 'W', 'w',
	'A', 'a', ']', '@', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', '[', '#', K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '!', '$', '%', 'M', '<', '>', R_LEFTSHIFT, R_RIGHTSHIFT, '.',
};

langtab k_i_danish = {		/* ITF Danish */
	'"', '&', '/', '(', ')', '=', '?', '+', R_FQUOTE, R_BQUOTE,
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', R_A_DOT, R_a_DOT, '^', R_DDOT,
	'/', R_AE, R_ae, R_ZERO, R_zero, '*', 'Z', 'z', ';', ':',
	'_', '-', '-', ',', '+', R_SO, 'Q', 'q', 'W', 'w',
	'A', 'a', '<', '@', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', '>', '*', K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '!', '$', '%', 'M', '`', '*', '<', '>', '.',
};

langtab k_i_dutch = {		/* ITF Dutch */
	'"', '&', '_', '(', ')', 39, '?', '/', '\\', '|',
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', R_HAT, R_DDOT, '>', '<',
	'/', '+', ':', R_FQUOTE, R_BQUOTE, '*', 'Z', 'z', ';', '*',
	'=', '-', '-', ',', '+', '#', 'Q', 'q', 'W', 'w',
	'A', 'a', '@', R_c_BEARD, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', R_SO, R_F, K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '!', '$', '%', 'M', K_NA, K_NA, K_NA, K_NA, '.',
};

langtab k_i_finnish = {		/* ITF Finnish & ITF Swedish */
	'"', '&', '/', '(', ')', '=', '?', '+', R_E_BQUOTE, R_e_BQUOTE,
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', R_A_DOT, R_a_DOT, R_U_DDOT, R_u_DDOT,
	'/', R_O_DDOT, R_o_DDOT, R_A_DDOT, R_a_DDOT, '*', 'Z', 'z', ';', ':',
	'_', '-', '-', ',', '+', '#', 'Q', 'q', 'W', 'w',
	'A', 'a', '<', 39, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', '>', '*', K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '!', '$', '%', 'M', '`', '*', '<', '>', '.',
};

langtab k_i_swiss_french = {		/* ITF Swiss French */
	'"', '&', '/', '(', ')', '=', '?', '!', R_FQUOTE, R_HAT,
	K_NA, K_NA, K_NA, K_NA, 'Z', 'z', R_u_DDOT, R_e_FQUOTE, R_BQUOTE, R_DDOT,
	'/', R_o_DDOT, R_e_BQUOTE, R_a_DDOT, R_a_FQUOTE, '*', 'Y', 'y', ';', ':',
	'_', '-', '-', ',', '+', '*', 'Q', 'q', 'W', 'w',
	'A', 'a', R_SO, '$', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', R_DEGREE, R_SPOUND, K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '+', R_c_BEARD, '%', 'M', '$', R_SPOUND, '<', '>', '.',
};

langtab k_i_german = {		/* ITF German */
	'"', '&', '/', '(', ')', '=', '?', R_BETA, '`', 39,
	K_NA, K_NA, K_NA, K_NA, 'Z', 'z', R_U_DDOT, R_u_DDOT, '*', '+',
	'/', R_O_DDOT, R_o_DDOT, R_A_DDOT, R_a_DDOT, '*', 'Y', 'y', ';', ':',
	'_', '-', '-', ',', '+', R_SO, 'Q', 'q', 'W', 'w',
	'A', 'a', '<', R_SPOUND, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', '>', '^', K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '!', '$', '%', 'M', '#', '\'', '<', '>', '.',
};

langtab k_a_german = {		/* PC-AT German */
	'"', '&', '/', '(', ')', '=', '?', R_BETA, '`', 39,
	K_NA, K_NA, K_NA, K_NA, 'Z', 'z', R_U_DDOT, R_u_DDOT, '*', '+',
	'/', R_O_DDOT, R_o_DDOT, R_A_DDOT, R_a_DDOT, '*', 'Y', 'y', ';', ':',
	'_', '-', '-', ',', '+', R_SO, 'Q', 'q', 'W', 'w',
	'A', 'a', '^', R_SPOUND, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', R_DEGREE, '^', K_BS,
	'0', ',', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '!', '$', '%', 'M', '#', '\'', '<', '>', ',',
};

langtab k_i_swiss_german = {		/* ITF Swiss German */
	'"', '&', '/', '(', ')', '=', '?', '!', R_FQUOTE, R_HAT,
	K_NA, K_NA, K_NA, K_NA, 'Z', 'z', R_e_FQUOTE, R_u_DDOT, R_BQUOTE, R_DDOT,
	'/', R_e_BQUOTE, R_o_DDOT, R_a_FQUOTE, R_a_DDOT, '*', 'Y', 'y', ';', ':',
	'_', '-', '-', ',', '+', '*', 'Q', 'q', 'W', 'w',
	'A', 'a', R_SO, '$', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', R_DEGREE, R_SPOUND, K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '+', R_c_BEARD, '%', 'M', '$', R_SPOUND, '<', '>', '.',
};

langtab k_i_italian = {		/* ITF Italian */
	'2', '6', '7', '8', '9', '0', R_DEGREE, ')', '+', '-',
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', '=', R_i_FQUOTE, '&', '$',
	'/', 'M', 'm', '%', R_u_FQUOTE, '*', 'W', 'w', '.', '/',
	'!', R_o_FQUOTE, '-', ',', '+', '3', 'Q', 'q', 'Z', 'z',
	'A', 'a', '<', '*', R_SPOUND, R_e_BQUOTE, '"', 39, '(', '_',
	R_e_FQUOTE, R_HAT, R_c_BEARD, R_a_FQUOTE, ';', ':', ',', '>', R_SO, K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '1', '4', '5', '?', R_MICRO, R_SO, '<', '>', '.',
};

langtab k_a_italian = {		/* PC-AT Italian */
	'"', '&', '/', '(', ')', '=', '?', '\'', '^', R_i_FQUOTE,
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', R_e_BQUOTE, R_e_FQUOTE, '*', '+',
	'/', R_c_BEARD, R_o_FQUOTE, R_DEGREE, R_a_FQUOTE, '*', 'Z', 'z', ';', ':',
	'_', '-', '-', ',', '+', R_SPOUND, 'Q', 'q', 'W', 'w',
	'A', 'a', '\\', K_NA, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', '|', K_NA, K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '!', '$', '%', 'M', R_u_FQUOTE, R_SO, '<', '>', '.',
};

langtab k_i_norwegian = {		/* ITF Norwegian */
	'"', '&', '/', '(', ')', '=', '?', '+', R_FQUOTE, R_BQUOTE,
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', R_A_DOT, R_a_DOT, '^', R_DDOT,
	'/', R_ZERO, R_zero, R_AE, R_ae, '*', 'Z', 'z', ';', ':',
	'_', '-', '-', ',', '+', '#', 'Q', 'q', 'W', 'w',
	'A', 'a', '<', '@', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', '>', '*', K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '!', '$', '%', 'M', '\'', '*', '<', '>', '.',
};

langtab k_i_euro_spanish = {		/* ITF European Spanish */
	'"', '&', '/', '(', ')', '=', '?', 39, R_i, R_FQUOTE,
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', '#', '@', '*', '+',
	'/', R_N_TILTA, R_n_TILTA, R_DDOT, R_BQUOTE, '*', 'Z', 'z', ';', ':',
	'_', '-', '-', ',', '+', R_UQUES, 'Q', 'q', 'W', 'w',
	'A', 'a', '<', R_c_BEARD, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', '>', R_DEGREE, K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '!', '$', '%', 'M', R_c_BEARD, R_C_BEARD, '<', '>', '.',
};

langtab k_i_lat_spanish = {		/* ITF Latin Spanish */
	'@', '^', '&', '*', '(', ')', '_', '-', '+', '=',
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', R_DDOT, R_BQUOTE, '"', 39,
	'/', R_N_TILTA, R_n_TILTA, ':', ';', '*', 'Z', 'z', '<', '>',
	'?', '/', '-', ',', '+', '#', 'Q', 'q', 'W', 'w',
	'A', 'a', '`', R_UQUES, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', R_c_BEARD, R_i, K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '!', '$', '%', 'M', R_c_BEARD, R_C_BEARD, '<', '>', '.',
};

langtab k_i_uk = {		/* ITF United Kingdom */
	'"', '&', '^', '(', ')', '=', '?', '+', '/', 39,
	K_NA, K_NA, K_NA, K_NA, 'Y', 'y', '{', '[', '}', ']',
	'/', '@', '*', '|', '\\', '*', 'Z', 'z', ';', ':',
	'_', '-', '-', ',', '+', R_SPOUND, 'Q', 'q', 'W', 'w',
	'A', 'a', '`', '<', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', '~', '>', K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '!', '$', '%', 'M', '#', '~', '\\', '|', '.',
};

langtab k_i_swiss_french_II = {		/* ITF Swiss French II */
	'"', '&', '/', '(', ')', '=', '?', '\'', R_FQUOTE, R_HAT,
	K_NA, K_NA, K_NA, K_NA, 'Z', 'z', R_u_DDOT, R_e_FQUOTE, '!', R_DDOT,
	'/', R_o_DDOT, R_e_BQUOTE, R_a_DDOT, R_a_FQUOTE, '*', 'Y', 'y', ';', ':',
	'_', '-', '-', ',', '+', '*', 'Q', 'q', 'W', 'w',
	'A', 'a', R_SO, '$', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', R_DEGREE, R_SPOUND, K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '+', R_c_BEARD, '%', 'M', '$', R_SPOUND, '<', '>', '.',
};

langtab k_i_swiss_german_II = {		/* ITF Swiss German II */
	'"', '&', '/', '(', ')', '=', '?', '\'', R_FQUOTE, R_HAT,
	K_NA, K_NA, K_NA, K_NA, 'Z', 'z', R_e_FQUOTE, R_u_DDOT, '!', R_DDOT,
	'/', R_e_BQUOTE, R_o_DDOT, R_a_FQUOTE, R_a_DDOT, '*', 'Y', 'y', ';', ':',
	'_', '-', '-', ',', '+', '*', 'Q', 'q', 'W', 'w',
	'A', 'a', R_SO, '$', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', ',', '.', 'm', R_DEGREE, R_SPOUND, K_BS,
	'0', '.', '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '+', R_c_BEARD, '%', 'M', '$', R_SPOUND, '<', '>', '.',
};

unsigned char *k_langtab[28][2] = {
	/* ITF */		/* PC-AT */
	k_n_usascii,		k_n_usascii,		/* K_N_USASCII */
	k_n_frenchq,		k_n_frenchq,		/* K_N_FRENCHQ */
	k_n_german,		k_n_german,		/* K_N_GERMAN */
	k_n_swedish,		k_n_swedish,		/* K_N_SWEDISH */
	k_n_spanish,		k_n_spanish,		/* K_N_SPANISH */
	k_n_katakana,		k_n_katakana,		/* K_N_KATAKANA */
	k_n_frencha,		k_n_frencha,		/* K_N_FRENCHA */
	k_i_usascii,		k_i_usascii,		/* K_I_USASCII */
	k_i_belgian,		k_i_belgian,		/* K_I_BELGIAN */
	k_i_canadian,		k_i_canadian,		/* K_I_CANENG */
	k_i_danish,		k_i_danish,		/* K_I_DANISH */
	k_i_dutch,		k_i_dutch,		/* K_I_DUTCH */
	k_i_finnish,		k_i_finnish,		/* K_I_FINNISH */
	k_i_belgian,		k_a_french,		/* K_I_FRENCH */
	k_i_canadian,		k_i_canadian,		/* K_I_CANFRENCH */
	k_i_swiss_french,	k_i_swiss_french,	/* K_I_SWISSFRENCH */
	k_i_german,		k_a_german,		/* K_I_GERMAN */
	k_i_swiss_german,	k_i_swiss_german,	/* K_I_SWISSGERMAN */
	k_i_italian,		k_a_italian,		/* K_I_ITALIAN */
	k_i_norwegian,		k_i_norwegian,		/* K_I_NORWEGIAN */
	k_i_euro_spanish,	k_i_euro_spanish,	/* K_I_EUROSPANISH */
	k_i_lat_spanish,	k_i_lat_spanish,	/* K_I_LATSPANISH */
	k_i_finnish,		k_i_finnish,		/* K_I_SWEDISH */
	k_i_uk,			k_i_uk,			/* K_I_UNITEDK */
	k_i_usascii,		k_i_usascii,		/* K_I_KATAKANA */
	k_i_swiss_french_II,	k_i_swiss_french_II,	/* K_I_SWISSFRENCH2 */
	k_i_swiss_german_II,	k_i_swiss_german_II,	/* K_I_SWISSGERMAN2 */
	k_i_usascii,		k_i_usascii,		/* K_I_KANJI */
};
