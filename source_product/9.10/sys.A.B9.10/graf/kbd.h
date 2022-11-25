/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/graf/RCS/kbd.h,v $
 * $Revision: 1.10.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 15:25:13 $
 */
#ifndef _GRAF_KBD_INCLUDED /* allows multiple inclusion */
#define _GRAF_KBD_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

/*
 * Keyboard Types
 */
#define	K_NIMITZ	0	/* NIMITZ 9836 Style Keyboard */
#define	K_ITF150	1	/* ITF 150 Style Keyboard */

/*
 * Definition of Language Fields
 */
#define K_N_FIRST	0	/* The first Nimitz language */
#define	K_N_USASCII	0	/* (0) NIMITZ US Standard */
#define	K_N_FRENCHQ	1	/* (1) NIMITZ French - QWERTY */
#define	K_N_GERMAN	2	/* (2) NIMITZ German */
#define	K_N_SWEDISH	3	/* (3) NIMITZ Swedish-Finish */
#define	K_N_SPANISH	4	/* (4) NIMITZ Spanish */
#define	K_N_KATAKANA	5	/* (5) NIMITZ Katanana */
#define	K_N_FRENCHA	6	/* (6) NIMITZ French - AZERTY */
#define K_N_LAST	6	/* The last Nimitz language */

#define K_I_FIRST	7	/* The first ITF language */
#define	K_I_USASCII	7	/* (7) ITF150 United States */
#define	K_I_BELGIAN	8	/* (8) ITF150 Belgian */
#define	K_I_CANENG	9	/* (9) ITF150 Canadian English */
#define	K_I_DANISH	10	/* (10) ITF150 Danish */
#define	K_I_DUTCH	11	/* (11) ITF150 Dutch */
#define	K_I_FINNISH	12	/* (12) ITF150 Finnish */
#define	K_I_FRENCH	13	/* (13) ITF150 French (AZERTY) */
#define	K_I_CANFRENCH	14	/* (14) ITF150 Canadian French */
#define	K_I_SWISSFRENCH	15	/* (15) ITF150 Swiss French */
#define	K_I_GERMAN	16	/* (16) ITF150 German */
#define	K_I_SWISSGERMAN	17	/* (17) ITF150 Swiss German */
#define	K_I_ITALIAN	18	/* (18) ITF150 Italian */
#define	K_I_NORWEGIAN	19	/* (19) ITF150 Norwegian */
#define	K_I_EUROSPANISH	20	/* (20) ITF150 European Spanish */
#define	K_I_LATSPANISH	21	/* (21) ITF150 Latin Spanish */
#define	K_I_SWEDISH	22	/* (22) ITF150 Swedish */
#define	K_I_UNITEDK	23	/* (23) ITF150 United Kingdom */
#define	K_I_KATAKANA	24	/* (24) ITF150 Katakana */
#define	K_I_SWISSFRENCH2	25	/* (25) ITF150 Swiss French II */
#define	K_I_SWISSGERMAN2	26	/* (26) ITF150 Swiss German II */
#define K_I_KANJI	27	/* (27) ITF150 Kanji (Japanese) */
#define K_I_LAST	27	/* The last ITF language */


/*
 * Keyboard Control/State Flag Bits
 */

/*
 * Enable Bits (Default in Parenthesis)
 */
#define	K_ASR33TTY	0x00001	/* Keyboard ASR33 Teletype Subseting (0) */
#define	K_ASCII8	0x00002	/* 8 bit (versus ISO 7 bit) (0) */
#define	K_MUTE		0x00004	/* Non-advancing Diacriticals (8 bit only)
					(Lg. Dependent) */
#define	K_CAPSMODE	0x00008	/* CAPS Key (1) */
#define	K_EXTEND	0x00010	/* Alternate Keyboards (1) */
#define	K_ATTACH	0x00020	/* Attaching Keyboard To Kernel (0) */
#define	K_CONTROL	0x00040	/* Control Collapsing of Printables (1) */
#define K_SHIFT		0x00080	/* Shift Collasping of Capitals (1) */

/*
 * NIMITZ only (Default in Parenthesis)
 */
#define	K_ANYCHAR	0x00800	/* Enable NIMITZ ANYCHAR Key (1) */

/*
 * ITF only (Default in Parenthesis)
 */
#define	K_META		0x01000	/* Enable META Modifiers (1) */
#define	K_META_EXTEND	0x02000	/* Enable Left-EXTEND as META Modifier (0) */

/*
 * State Bits (Read Only) (Default in Parenthesis)
 */
#define	K_CAPSLOCK	0x08000	/* CAPS Mode State (Read Only) (1==Locked)(0)*/
#define	K_KANAKBD	0x10000	/* Katakana Keyboard (Read Only)(0) */

/*
 * Control Byte Bit Defines
 */
#define	K_SHIFT_B	0x001		/* Shift Bit */
#define	K_CONTROL_B	0x002		/* Control Bit */
#define	K_META_B	0x004		/* META */
#define	K_EXTEND_B	0x008		/* Extend Right */
#define	K_UP		0x010		/* UP==0 DOWN==1 */
#define	K_NPAD		0x020		/* Number Pad Key */
#define	K_MOUSE		0x040		/* Mouse Codes Follow */
#define	K_SPECIAL	0x080		/* CONTROL Keys such as CLR LINE,
					    ENTER, f1, etc. */

/*
 * Special Key Defines
 */
#define	K_ESC		27
#define	K_SPACE		32
#define	K_DEL		127
#define	K_ILLEGAL	0xffff
#define	K_NA		0
#define	K_BS		8
#define	K_EXTEND_LEFT	18
#define	K_EXTEND_RIGHT	19
#define	K_META_LEFT	126
#define	K_META_RIGHT	127
#define	K_KNOB_UP	132
#define	K_KNOB_DOWN	133
#define	K_KNOB_RIGHT	134
#define	K_KNOB_LEFT	135
#define	K_CAPS_ON	((K_SPECIAL<<8)+128)
#define	K_CAPS_OFF	((K_SPECIAL<<8)+129)
#define	K_GO_ROMAN	((K_SPECIAL<<8)+130)
#define	K_GO_KATAKANA	((K_SPECIAL<<8)+131)
			/* 132 - 135 are KNOB Key Directions */
#define	K_BUTTON0	((K_SPECIAL<<8)+136)
#define	K_BUTTON1	((K_SPECIAL<<8)+137)
#define	K_BUTTON2	((K_SPECIAL<<8)+138)
#define	K_BUTTON3	((K_SPECIAL<<8)+139)
#define	K_BUTTON4	((K_SPECIAL<<8)+140)
#define	K_BUTTON5	((K_SPECIAL<<8)+141)
#define	K_BUTTON6	((K_SPECIAL<<8)+142)
#define	K_BUTTON7	((K_SPECIAL<<8)+143)
#define	K_KANA_LMETA	((K_SPECIAL<<8)+144)
#define	K_KANA_RMETA	((K_SPECIAL<<8)+144)
			/* 146 - 147 are Up Codes for Left and Right Extend */

#define	K_BREAK		((K_SPECIAL<<8)+5)
#define	K_STOP		((K_SPECIAL<<8)+6)
#define	K_SELECT	((K_SPECIAL<<8)+7)
#define	K_NP_ENTER	(((K_NPAD|K_SPECIAL)<<8)+8)
#define	K_NP_TAB	(((K_NPAD|K_SPECIAL)<<8)+9)
#define	K_NP_K0		(((K_NPAD|K_SPECIAL)<<8)+10)
#define	K_NP_K1		(((K_NPAD|K_SPECIAL)<<8)+11)
#define	K_NP_K2		(((K_NPAD|K_SPECIAL)<<8)+12)
#define	K_NP_K3		(((K_NPAD|K_SPECIAL)<<8)+13)
#define	K_HOME_ARROW	((K_SPECIAL<<8)+14)
#define	K_PREV		((K_SPECIAL<<8)+15)
#define	K_NEXT		((K_SPECIAL<<8)+16)
#define	K_ENTER		((K_SPECIAL<<8)+17)
#define	K_SYSTEM	((K_SPECIAL<<8)+20)
#define	K_MENU		((K_SPECIAL<<8)+21)
#define	K_CLR_LINE	((K_SPECIAL<<8)+22)
#define	K_CLR_DISP	((K_SPECIAL<<8)+23)
#define	K_CAPS_LOCK	((K_SPECIAL<<8)+24)
#define	K_TAB		((K_SPECIAL<<8)+25)
#define	K_K0		((K_SPECIAL<<8)+26)
#define	K_K1		((K_SPECIAL<<8)+27)
#define	K_K2		((K_SPECIAL<<8)+28)
#define	K_K5		((K_SPECIAL<<8)+29)
#define	K_K6		((K_SPECIAL<<8)+30)
#define	K_K7		((K_SPECIAL<<8)+31)
#define	K_K3		((K_SPECIAL<<8)+32)
#define	K_K4		((K_SPECIAL<<8)+33)
#define	K_DOWN_ARROW	((K_SPECIAL<<8)+34)
#define	K_UP_ARROW	((K_SPECIAL<<8)+35)
#define	K_K8		((K_SPECIAL<<8)+36)
#define	K_K9		((K_SPECIAL<<8)+37)
#define	K_LEFT_ARROW	((K_SPECIAL<<8)+38)
#define	K_RIGHT_ARROW	((K_SPECIAL<<8)+39)
#define	K_INSERT_LINE	((K_SPECIAL<<8)+40)
#define	K_DELETE_LINE	((K_SPECIAL<<8)+41)
#define	K_RECALL	((K_SPECIAL<<8)+42)
#define	K_INSERT_CHAR	((K_SPECIAL<<8)+43)
#define	K_DELETE_CHAR	((K_SPECIAL<<8)+44)
#define	K_CLEAR_TO_END	((K_SPECIAL<<8)+45)
#define	K_BACKSPACE	((K_SPECIAL<<8)+46)
#define	K_RUN		((K_SPECIAL<<8)+47)
#define	K_EDIT		((K_SPECIAL<<8)+48)
#define	K_ALPHA		((K_SPECIAL<<8)+49)
#define	K_GRAPHICS	((K_SPECIAL<<8)+50)
#define	K_STEP		((K_SPECIAL<<8)+51)
#define	K_N_CLEAR_LINE	((K_SPECIAL<<8)+52)
#define	K_RESULT	((K_SPECIAL<<8)+53)
#define	K_PRT_ALL	((K_SPECIAL<<8)+54)
#define	K_CLR_IO	((K_SPECIAL<<8)+55)
#define	K_PAUSE		((K_SPECIAL<<8)+56)
#define	K_RETURN	((K_SPECIAL<<8)+57)
#define	K_CONTINUE	((K_SPECIAL<<8)+58)
#define	K_EXECUTE	((K_SPECIAL<<8)+59)
#define	K_NP_ZERO	((K_NPAD<<8)+'0')
#define	K_NP_PERIOD	((K_NPAD<<8)+'.')
#define	K_NP_COMMA	((K_NPAD<<8)+',')
#define	K_NP_PLUS	((K_NPAD<<8)+'+')
#define	K_NP_ONE	((K_NPAD<<8)+'1')
#define	K_NP_TWO	((K_NPAD<<8)+'2')
#define	K_NP_THREE	((K_NPAD<<8)+'3')
#define	K_NP_MINUS	((K_NPAD<<8)+'-')
#define	K_NP_FOUR	((K_NPAD<<8)+'4')
#define	K_NP_FIVE	((K_NPAD<<8)+'5')
#define	K_NP_SIX	((K_NPAD<<8)+'6')
#define	K_NP_MULT	((K_NPAD<<8)+'*')
#define	K_NP_SEVEN	((K_NPAD<<8)+'7')
#define	K_NP_EIGHT	((K_NPAD<<8)+'8')
#define	K_NP_NINE	((K_NPAD<<8)+'9')
#define	K_NP_DIV	((K_NPAD<<8)+'/')
#define	K_NP_E		((K_NPAD<<8)+'E')
#define	K_NP_LPAR	((K_NPAD<<8)+'(')
#define	K_NP_RPAR	((K_NPAD<<8)+')')
#define	K_NP_CARAT	((K_NPAD<<8)+'^')
#define	K_RESET_KEY	(((K_SHIFT_B|K_SPECIAL)<<8)+5)
#define	K_PRINT		(((K_SHIFT_B|K_SPECIAL)<<8)+17)
#define	K_USER		(((K_SHIFT_B|K_SPECIAL)<<8)+20)
#define	K_DISP_FCTNS	(((K_SHIFT_B|K_SPECIAL)<<8)+48)
#define	K_DUMP_ALPHA	(((K_SHIFT_B|K_SPECIAL)<<8)+49)
#define	K_DUMP_GRAPHICS	(((K_SHIFT_B|K_SPECIAL)<<8)+50)
#define	K_ANY_CHAR	(((K_SHIFT_B|K_SPECIAL)<<8)+51)
#define	K_N_CLEAR_SCR	(((K_SHIFT_B|K_SPECIAL)<<8)+52)
#define	K_SET_TAB	(((K_SHIFT_B|K_SPECIAL)<<8)+53)
#define	K_CLR_TAB	(((K_SHIFT_B|K_SPECIAL)<<8)+54)
#define	K_N_STOP	(((K_SHIFT_B|K_SPECIAL)<<8)+55)

struct k_keystate {
    int unit;
    unsigned int flags;		/* Control & State Flags (R/W) */
    char type;			/* Keyboard Type (Read Only) */
    char language;		/* Keyboard Nationality Language (R/W) */
    char meta;			/* State of META Key */
    char extend;		/* State of EXTEND Key */
    char anychar_value;		/* Current ANYCHAR Value */
    char anychar_state;		/* Current ANYCHAR State (0 == Not Active) */
    char pwr_language;		/* Keyboard Language At Power Up */
    unsigned short last_char;	/* Mute Prefix */
    unsigned char *langtab;	/* Pointer To Language Dependent Key Table */
#   ifdef _WSIO
	unsigned int flags_config;	/* temp version for config screen */
	char language_config;		/*		"		  */
	long hilbase;			/* Address of 8042 attached to ITE */
#   else
	int (*ite_q_func)();    /* pointer to ite_queue() if ITE configured */
	int (*ite_outc_func)(); /* pointer to ite_out_char()	"	    */
#   endif
    unsigned char k_iso8to7[256], k_iso7to8[256];
};

#define MAX_KBDS	2

#ifdef _KERNEL
#   ifdef __hp9000s300
	extern int kbd_mute_char(), kbd_command(), kbd_reset();
#   endif
#   ifdef __hp9000s800
	extern int kbd_init(), kbd_beep();
#   endif
#endif /* _KERNEL */

#endif /* ! _GRAF_KBD_INCLUDED */
