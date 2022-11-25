/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/graf/RCS/kbd_code.c,v $
 * $Revision: 1.10.83.7 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/06/10 12:18:34 $
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

#ifndef lint
static char revision[] = "$Header: kbd_code.c,v 1.10.83.7 94/06/10 12:18:34 rpc Exp $";
#endif

#include "../h/types.h"
#include "../h/tty.h"
#include "../graf/hil.h"
#include "../wsio/beeper.h"
#include "../wsio/audio.h"
#include "../h/param.h"
#include "../graf/kbd.h"
#include "../graf/kbd_chars.h"

extern void (*audiobeeper)();

#ifndef	TRUE
#   define TRUE		1
#   define FALSE	0
#endif

/* Keyboard command defines */
#define BELL	0xA3		/* set bell information   */

/* Keyboard status register constants (negative true) */
#define KEYSHIFT	0x10	/* key was shifted     */
#define KEYCNTL		0x20	/* key was control key */
#define	CNTLKEY_MASK	0x1f	/* maps keys to control key range */

#define swap(a, b) if (t_key==a) t_key=b; else if (t_key==b) t_key=a;

#ifndef	CR
#    define CR '\r'
#endif

#ifdef __hp9000s300
struct k_keystate k_keystate;
#endif
#ifdef __hp9000s700
struct k_keystate *global_k_keystate;
#endif

/* duration = 60ms ((256-248) * 10) */
/* freq = 8 * 104.17 HZ */
unsigned char BELLSTR[]  = { 0xf8, 0x08, 0x00 };
unsigned char ARP40MS[]  = { 0xfc, 0x00 };	/* 40 ms auto repeat period */
unsigned char ARD300MS[] = { 0xe2, 0x00 };	/* 300 ms auto repeat delay */
unsigned char RPG30MS[]  = { 0x03, 0x00 };	/* 30 ms interrupt rate for RPG */

struct mutes {
	unsigned char mute,	/* The accent itself, e.g., an umlaut */
		      old,	/* The pre-accent character e.g., "o" */
		      new;	/* The merged character, e.g., o-umlaut */
} mutes[] = {
	{ R_BQUOTE, 'a', R_a_BQUOTE, },	{ R_BQUOTE, 'A', R_A_BQUOTE, },
	{ R_BQUOTE, 'e', R_e_BQUOTE, },	{ R_BQUOTE, 'E', R_E_BQUOTE, },
	{ R_BQUOTE, 'i', R_i_BQUOTE, },	{ R_BQUOTE, 'I', R_I_BQUOTE, },
	{ R_BQUOTE, 'o', R_o_BQUOTE, },	{ R_BQUOTE, 'O', R_O_BQUOTE, },
	{ R_BQUOTE, 'u', R_u_BQUOTE, },	{ R_BQUOTE, 'U', R_U_BQUOTE, },
	{ R_BQUOTE, 'y', R_y_BQUOTE, },	{ R_BQUOTE, 'Y', R_Y_BQUOTE, },
	{ R_BQUOTE, ' ', R_BQUOTE, },	{ R_BQUOTE, CR,  CR, },
	{ R_FQUOTE, 'a', R_a_FQUOTE, },	{ R_FQUOTE, 'A', R_A_FQUOTE, },
	{ R_FQUOTE, 'e', R_e_FQUOTE, },	{ R_FQUOTE, 'E', R_E_FQUOTE, },
	{ R_FQUOTE, 'i', R_i_FQUOTE, },	{ R_FQUOTE, 'I', R_I_FQUOTE, },
	{ R_FQUOTE, 'o', R_o_FQUOTE, },	{ R_FQUOTE, 'O', R_O_FQUOTE, },
	{ R_FQUOTE, 'u', R_u_FQUOTE, },	{ R_FQUOTE, 'U', R_U_FQUOTE, },
	{ R_FQUOTE, ' ', R_FQUOTE, },	{ R_FQUOTE, CR,  CR, },
	{ R_HAT,    'a', R_a_HAT, },	{ R_HAT,    'A', R_A_HAT, },
	{ R_HAT,    'e', R_e_HAT, },	{ R_HAT,    'E', R_E_HAT, },
	{ R_HAT,    'i', R_i_HAT, },	{ R_HAT,    'I', R_I_HAT, },
	{ R_HAT,    'o', R_o_HAT, },	{ R_HAT,    'O', R_O_HAT, },
	{ R_HAT,    'u', R_u_HAT, },	{ R_HAT,    'U', R_U_HAT, },
	{ R_HAT,    ' ', R_HAT, },	{ R_HAT,    CR,  CR, },
	{ R_DDOT,   'a', R_a_DDOT, },	{ R_DDOT,   'A', R_A_DDOT, },
	{ R_DDOT,   'e', R_e_DDOT, },	{ R_DDOT,   'E', R_E_DDOT, },
	{ R_DDOT,   'i', R_i_DDOT, },	{ R_DDOT,   'I', R_I_DDOT, },
	{ R_DDOT,   'o', R_o_DDOT, },	{ R_DDOT,   'O', R_O_DDOT, },
	{ R_DDOT,   'u', R_u_DDOT, },	{ R_DDOT,   'U', R_U_DDOT, },
	{ R_DDOT,   'y', R_y_DDOT, },	{ R_DDOT,   'Y', R_Y_DDOT, },
	{ R_DDOT,   ' ', R_DDOT, },	{ R_DDOT,   CR,  CR, },
	{ R_TILTA,  'a', R_a_TILTA, },	{ R_TILTA,  'A', R_A_TILTA, },
	{ R_TILTA,  'o', R_o_TILTA, },	{ R_TILTA,  'O', R_O_TILTA, },
	{ R_TILTA,  'n', R_n_TILTA, },	{ R_TILTA,  'N', R_N_TILTA, },
	{ R_TILTA,  ' ', R_TILTA, },	{ R_TILTA,  CR,  CR, },
	{ 0, 0, 0 }
};

/*
 * There are twelve keys defined by ISO to be language-dependent
 * when operating in 7-bit mode.  Here they are for each keyboard.
 */

unsigned char k_isus[] = { 0 };
unsigned char k_isfr[] = { '#', R_SPOUND, '@', R_a_FQUOTE, '[', R_DEGREE,
	'\\', R_c_BEARD, ']', R_SO, '^', R_HAT, '{', R_e_BQUOTE,
	'|', R_u_FQUOTE, '}', R_e_FQUOTE, '-', R_DDOT, 0 };
unsigned char k_isgm[] = { '#', R_SPOUND, '@', R_SO, '[', R_A_DDOT,
	'\\', R_O_DDOT, ']', R_U_DDOT, '{', R_a_DDOT, '|', R_o_DDOT,
	'}', R_u_DDOT, '-', R_BETA, 0 };
unsigned char k_issw[] = { '@', R_E_BQUOTE, '[', R_A_DDOT, '\\', R_O_DDOT,
	']', R_A_DOT, '^', R_U_DDOT, '`', R_e_BQUOTE, '{', R_a_DDOT,
	'|', R_o_DDOT, '}', R_a_DOT, '-', R_u_DDOT, 0 };
unsigned char k_isnsp[] = { '[', R_i, '\\', R_N_TILTA, ']', R_UQUES,
	'^', R_DEGREE, '|', R_n_TILTA, 0 };
unsigned char k_iscn[] = { '\\', R_c_BEARD, '^', R_HAT, '`', R_FQUOTE,
	'{', R_e_BQUOTE, '|', R_C_BEARD, '}', R_E_BQUOTE, '-', R_DDOT, 0 };
unsigned char k_isdn[] = { '#', R_SO, '\'', R_BQUOTE, '[', R_AE, '\\', R_ZERO,
	']', R_A_DOT, '`', R_FQUOTE, '{', R_ae, '|', R_zero, '}', R_a_DOT,
	'-', R_DDOT, 0 };
unsigned char k_isdu[] = { '[', R_c_BEARD, ']', R_SO, '^', R_HAT,
	'`', R_FQUOTE, '{', R_F, '}', R_BQUOTE, '-', R_DDOT, 0 };
unsigned char k_isss[] = { '#', R_SPOUND, '\'', R_BQUOTE, '@', R_a_FQUOTE,
	'[', R_DEGREE, '\\', R_c_BEARD, ']', R_SO, '^', R_HAT, '`', R_FQUOTE,
	'{', R_a_DDOT, '|', R_o_DDOT, '}', R_u_DDOT, '-', R_DDOT,
	'>', R_e_BQUOTE, '<', R_e_FQUOTE, 0 };
unsigned char k_isit[] = { '#', R_SPOUND, '@', R_SO, '[', R_DEGREE,
	'\\', R_c_BEARD, ']', R_e_BQUOTE, '^', R_HAT, '`', R_u_FQUOTE,
	'{', R_a_FQUOTE, '|', R_o_FQUOTE, '}', R_e_FQUOTE, '-', R_i_FQUOTE, 0 };
unsigned char k_isnw[] = { '\'', R_BQUOTE, '[', R_AE, '\\', R_ZERO,
	']', R_A_DOT, '`', R_FQUOTE, '{', R_ae, '|', R_zero, '}', R_a_DOT,
	'-', R_DDOT, 0 };
unsigned char k_ises[] = { '\'', R_BQUOTE, '[', R_i, '\\', R_N_TILTA, ']',
	R_UQUES, '^', R_DEGREE, '{', '\'', '|', R_n_TILTA, '}', R_c_BEARD,
	'-', R_DDOT, 0 };
unsigned char k_isls[] = { '\'', R_BQUOTE, '[', R_i, '\\', R_N_TILTA,
	']', R_UQUES, '{', '\'', '|', R_n_TILTA, '}', R_c_BEARD,
	'-', R_DDOT, 0 };
unsigned char k_isuk[] = { '#', R_SPOUND, 0 };

extern unsigned char ntz_katab[], *k_langtab[][2];

extern struct { unsigned char unshift, shift; } itf_extend[];

extern char lang_jump[];
extern unsigned short mute_enable[];
extern struct { unsigned short unshift, shift; } k_code[];
#ifdef __hp9000s300
extern char kbd_present;
extern struct hilrec *hils[];
#endif
#ifdef __hp9000s700
extern int ite_queue(), ite_out_char(), ite_flush_check_ext();
#endif

#define K_PC_AT 0x100

/*
 * Initialize the keyboard -- should only be called once from at power-up.
 */
#ifdef __hp9000s300
kbd_init()							    /* ENTRY */
#endif
#ifdef __hp9000s700
kbd_init(unit, k)						    /* ENTRY */
int unit;
register struct k_keystate *k;
#endif
{
	unsigned register jump;
	char data;
	register struct hilrec *g;

#ifdef __hp9000s300
	register struct k_keystate *k = &k_keystate;

	kbd_present = TRUE;
	g = hils[0];

	/*
	 * Set hilbase value in keyboard data structure.
	 * Actually, it's not a hilbase, it's a pointer to an
	 * entire hil structure.  Hey, it's a magic value, so what.
	 */
#endif

#ifdef __hp9000s700
	extern struct hilrec *global_hil_state;
	  
	k->unit = unit;
	g = global_hil_state; /* there ought to be a better way */
	if (g == NULL) return;
#endif
	g->has_ite_kbd = TRUE;
	k->hilbase = (long)g;

	/* Find out type of keyboard family */
	hil_cmd(k->hilbase, H_READCONFIG, NULL, 0, &data);
	jump = data;
	if ((jump&0x20)==0)
		k->type = K_NIMITZ;		/* No HIL, must be NIMITZ */
	else {
		switch (jump & 07) {
		default:			/* reserved, make itf */
		case 0:  k->type = K_ITF150;	break;
		case 2:  k->type = K_NIMITZ;	break;
		}
	}

	/* auto repeat period = 40 ms */
	hil_cmd(k->hilbase, H_REP_RATE, ARP40MS, 1, NULL);
	g->repeat_rate = 0xFC;		/* -4*10ms */

	/* auto repeat delay = 300 ms */
	hil_cmd(k->hilbase, H_REP_DELAY, ARD300MS, 1, NULL);

	/* RPG interrupt rate = 30ms */
	hil_cmd(k->hilbase, H_RPG_RATE, RPG30MS, 1, NULL);

	/* get language jumper */
	hil_cmd(k->hilbase, H_READLANG, NULL, 0, &data);
	jump = data;

	/* Reject values we don't handle */
	switch (k->type) {
	case K_NIMITZ:
		if (jump>9)
			jump = 0;
		break;
	default:
	case K_ITF150:
		if (jump>31)
			jump = 31;
		jump += 10;
		break;
	}
	k->language = lang_jump[jump];
	k->pwr_language = k->language;	/* Power-up Value */

	/* reset terminal configuration to defaults */
#ifdef __hp9000s300
	kbd_reset();
#endif
#ifdef __hp9000s700
	kbd_reset(k);
#endif

	hil_cmd(k->hilbase, H_INTON, NULL, 0, NULL);

	/* Do a pretty display */
	lights(k);
}

#include "../wsio/timeout.h"
#undef timeout

/*
 * Do a bouncing lights display on the AT keyboard, like this:
 *
 * O..	step 0 mod 4
 * .O.	step 1 mod 4
 * ..O	step 2 mod 4
 * .O.	step 3 mod 4
 * O..	step 0 mod 4
 * .O.	step 1 mod 4
 * ..O	step 2 mod 4
 * .O.	step 3 mod 4
 */
lights(k)
register struct	k_keystate *k;
{
	static int count, duration = (HZ) << 7;
	register int n, i;
	register struct hilrec *g = (struct hilrec *) k->hilbase;
	register int x;

	if (!(k->flags & K_PC_AT))		/* Do we have an AT keyboard? */
		return;				/* No, don't bother. */

	n = "\4\2\1\2"[--count & 03];		/* Get bouncing light */
	duration -= duration >> 4;
	if (duration <= 1 << 4)			/* Final time? */
		n=0;				/* Clear the lights. */
	x = spl6();
	for (i=1; i<=7; i++)
		if (g->current_mask & (1<<(i-1)))	/* If a keyboard: */
			hil_lcommand(g, i, 0xc0+n);	/* Set the lights */
	splx(x);
	if (n)					/* If not final time: */
		timeout(lights, k, duration >> 7);	/* Call us again. */
}

/*
 * Set the caps lock light appropriately for PC-AT keyboards.
 */
set_caps_light(k)
register struct	k_keystate *k;
{
	register int i, x;
	register struct hilrec *g = (struct hilrec *) k->hilbase;

	if (!(k->flags & K_PC_AT))		/* Do we have an AT keyboard? */
		return;				/* If not, no lights then! */

	/*
	 * The keyboard may not be interrupting, so go to
	 * interrupt level 1 to force polling.
	 */
	x=spl6();
	for (i=1; i<=7; i++) {
		/* Only do this for keyboards. */
		if (g->current_mask & (1<<(i-1))) {
			hil_lcommand(g, i, k->flags&K_CAPSLOCK ? 0x42 : 0x4a);
		}
	}
	splx(x);
}

/*
 * Software Reset state of keyboard driver.
 */
#ifdef __hp9000s300
kbd_reset()							    /* ENTRY */
#endif
#ifdef __hp9000s700
kbd_reset(k)
register struct	k_keystate *k;
#endif
{
	register int i, x;
	register struct hilrec *g;
#ifdef __hp9000s300
	register struct	k_keystate *k = &k_keystate;
#endif

	g = (struct hilrec *) k->hilbase;

	k->language = k->pwr_language;
	k->flags = K_CAPSMODE|K_EXTEND|K_ATTACH|K_CONTROL|K_SHIFT|
			K_ANYCHAR|mute_enable[(int)k->language];
	k->last_char = 0;
	k->anychar_state = 0;
	k->extend = FALSE;

#undef KBD_DEBUG
	/* Do we have a PC-AT keyboard? */
	for (i=1; i<=7; i++) {
		/* Is this device isn't a keyboard, don't bother. */
		if (!(g->current_mask & (1<<(i-1))))
			continue;
		x = spl6();
		hil_lcommand(g, i, 0x03);	  /* Identify & describe */
		splx(x);
		if (g->loop_response[1] & 0x10) { /* Extended describe? */
			k->flags |= K_PC_AT;	  /* Only AT does that. */
#ifdef KBD_DEBUG
			printf("AT keyboard\n");
#endif
		}
#ifdef KBD_DEBUG
		{
		int index;
		static char *language[] = {
		    "language 00", "language 01", "Kanji", "Swiss/French",
		    "Portuguese", "Arabic", "Hebrew", "Canadian English",
		    "Turkish", "Greek", "Thai", "Italian", "Hangul (Korea)",
		    "Dutch", "Swedish", "German", "Chinese PRC", "Chinese ROC",
		    "Swiss French 2", "European Spanish", "Swiss German 2",
		    "Belgian (Flemish)", "Finnish", "United Kingdom",
		    "Canadian French", "Swiss German", "Norwegian", "French",
		    "Danish", "Katakana", "Latin American Spanish", "US" };

		printf("HIL device %d is a %s keyboard:",
			i, language[g->loop_response[0] & 0x1f]);
		for (index=0; index<g->response_length; index++)
			printf(" %02x", g->loop_response[index] & 0xff);
		printf("\n");
		}
#endif
	}

	/* Must know if K_PC_AT before we do this. */
#ifdef __hp9000s300
	kbd_language();
#endif
#ifdef __hp9000s700
	kbd_language(k);
#endif
}

#ifdef __hp9000s300
kbd_beep()							    /* ENTRY */
#endif
#ifdef __hp9000s700
kbd_beep(k)
register struct	k_keystate *k;
#endif
{
#ifdef __hp9000s300
	register struct k_keystate *k = &k_keystate;
	extern struct audio_descriptor *audio_devices;

	/* If we have digital audio hardware then we no longer
	 * have the TI beeper. Call the audio_beeper routine
	 * instead.
	 */

	if (audio_devices == (struct audio_descriptor *)0)
		hil_cmd(k->hilbase, BELL, BELLSTR, 2, NULL);
	else {
		(*audiobeeper)((struct audio_descriptor *)0,
				BELLSTR[1] & 0xff, 0, BELLSTR[0] & 0xff,
				BEEPTYPE_200);
	}
#endif
#ifdef __hp9000s700
	hil_cmd(k->hilbase, BELL, BELLSTR, 2, NULL);
#endif
	return(0);
}

/*
 * kbd_map_keys() takes an HIL keycode & status and passes on something
 * to ite_filter().  That something is ASCII, if it is a normal key,
 * or something >256 if it is a funny key, like an up arrow.
 *
 * Processing of control, shift, and extend happens here.
 * Control & shift are true modifiers, just bits in the status word.
 * The extend keys (left & right) are just regular keys, so we have
 * to keep track of their state when we see the up & down transitions.
 *
 * You will see occasional references to Nimitz.  This is a keyboard
 * type used on the now-obsolete 9836/9826.  There is also an HIL Nimitz,
 * which seems to work, but it is not supported for HP-UX.
 */

kbd_map_keys(key, status)					    /* ENTRY */
register unsigned key, status;
{
	register control, shift, numpad;
	unsigned register t_key;
#ifdef __hp9000s300
	register struct k_keystate *k = &k_keystate;
#endif
#ifdef __hp9000s700
	register struct k_keystate *k = global_k_keystate;
#endif

	control = !(status & KEYCNTL);
	shift = !(status & KEYSHIFT);
	key &= 0xff;

	/* Avoid bad Nimitz keycodes from bad hardware. */
	if (k->type==K_NIMITZ && key>127)
		return;

	switch (key) {
	case K_KNOB_UP:
	case K_KNOB_DOWN:
	case K_KNOB_LEFT:
	case K_KNOB_RIGHT:
		t_key = key | (K_SPECIAL<<8);
		goto kbd_done;

	/* k->extend & 0x01:	Left extend key is down*/
	/* k->extend & 0x02:	Right extend is down */

	case K_EXTEND_LEFT+128:			/* Left extend key up */
		k->extend &= ~0x01;
		return;

	case K_EXTEND_LEFT:			/* Left extend key down */
		k->extend |= 0x01;	/* Indicate Extend Down */
		if (k->flags&K_EXTEND && k->language==K_I_KATAKANA) {
			k->flags &= ~K_KANAKBD;	/* Roman */
			t_key = K_GO_ROMAN;
			k->extend &= ~0x01;	/* Indicate Extend Up */
			goto kbd_done;
		}
		return;

	case K_EXTEND_RIGHT+128:		/* Right extend key up */
		k->extend &= ~0x02;
		return;

	case K_EXTEND_RIGHT:			/* Right extend key down */
		k->extend |= 0x02;
		/* For Katakana keyboard, this goes to kana mode. */
		if (k->language==K_I_KATAKANA) {
			k->flags |= K_KANAKBD;
			t_key = K_GO_KATAKANA;
			goto kbd_done;
		}

		/* For Kanji keyboard, this toggles. */
		if (k->language==K_I_KANJI) {
			k->flags ^= K_KANAKBD;	/* flip Katakana/Roman */
			t_key = (k->flags & K_KANAKBD)
				? K_GO_KATAKANA : K_GO_ROMAN;
			goto kbd_done;
		}
		return;
	}

	if ((k->flags&K_ANYCHAR) == 0)
		k->anychar_state = 0;

	/* Check for the KATAKANA Alternate Keyboard */
	if (((k->flags&(K_KANAKBD|K_EXTEND|K_ASCII8))==
		(K_KANAKBD|K_EXTEND|K_ASCII8)) &&
		(k->language==K_N_KATAKANA || k->language==K_I_KATAKANA || k->language==K_I_KANJI) &&
		(key>=80 || key==1 || key==2)) {
			k->anychar_state = 0;			      /* Disable anychar key */
			if (control && key == 96) {
			/* Control <  (ROMAN) */
			k->flags &= ~K_KANAKBD;
			t_key = K_GO_ROMAN;
			goto kbd_done;
		}
		if (key!=1 && key!=2)
			t_key = ntz_katab[key-80+(shift ? 46:0)];
		switch (key) {
		case 1: t_key = (shift ? 176 : 219); break;
		case 2: t_key = (shift ? 163 : 209); break;
		case 92:
			if (k->type==K_NIMITZ) break;
			t_key = 222; break;
		case 93:
			if (k->type==K_NIMITZ) break;
			t_key = (shift ? 162 : 223); break;
		}
		goto kbd_done;
	}

	/* <extend>-key? */
	if (k->extend && k->language!=K_I_KATAKANA && k->language!=K_I_KANJI &&
		((k->flags&(K_EXTEND|K_ASCII8))==(K_EXTEND|K_ASCII8)) &&
		(key>=80 && key<=127 || key==1 || key==2)) {
		k->anychar_state = 0; /* Disable anychar key */
		/* Look up ALT SHIFT Set via keycode shifted */
		switch (key) {
		case 1: t_key = (shift ? R_RIGHTSHIFT : R_LEFTSHIFT); break;
		case 2: t_key = R_MICRO; break;
		default: t_key = shift ? itf_extend[key-80].shift : itf_extend[key-80].unshift; break;
		}
		/* Check for CAPSLOCK */
		if ((k->flags&(K_CAPSMODE|K_CAPSLOCK))==(K_CAPSMODE|K_CAPSLOCK)) {
			swap(R_ZERO,	R_zero);
			swap(R_IP,	R_ip);
			swap(R_A_DOT,	R_a_DOT);
			swap(R_AE,	R_ae);
			swap(R_D_CROSS,	R_d_CROSS);
			swap(R_S_V,	R_s_V);
			swap(R_C_BEARD,	R_c_BEARD);
		}
	}
	else {
		t_key = shift ? k_code[key].shift : k_code[key].unshift;
		numpad = t_key&(K_NPAD<<8);
		t_key &= ~(K_NPAD<<8);

		/* Handle Special Keys First */
		if (t_key>255) {
			k->anychar_state = 0;	/* Disable anychar key */
			switch (t_key) {
			case K_CAPS_LOCK:	/* Handle CAPS Key */
				if (k->flags&K_CAPSMODE) {
					k->flags ^= K_CAPSLOCK;
					t_key = (k->flags&K_CAPSLOCK ? K_CAPS_ON : K_CAPS_OFF);
					set_caps_light(k);
				}
				break;
			case K_ANY_CHAR:	/* any char key */
				if (k->flags&K_ANYCHAR) {
					k->anychar_state = 1;
					k->anychar_value = 0;
				}
			}
			goto kbd_done;
		}

		/* If it is a language dependent key, look up again. */
		if ((t_key>=128) && (t_key<=256)) {
			/* set klang to 0 if not in range 0 to 74 */
			t_key = k->langtab[t_key-160];
		}

		/* Check for caps mode and ASR33TTY mode */
		if ((k->flags&(K_ASR33TTY|K_CAPSMODE)) && (k->flags&K_KANAKBD) == 0) {
			/* Check for keys affected by capitalization. */
			if ((t_key>='a' && t_key<='z') ||
			    (t_key>='A' && t_key<='Z')) {
				/* Invert Case */
				if ((k->flags&(K_CAPSLOCK|K_CAPSMODE))==(K_CAPSLOCK|K_CAPSMODE))
					t_key ^= 0x20;
				/* Caps only */
				if (k->flags&K_ASR33TTY)
					t_key &= ~0x20;
			}
			else if (k->flags&K_ASR33TTY) {
				/* TTY codes only, Sorry. */
				if (t_key>0x7f || t_key=='`' || t_key=='~') {
					t_key = K_ILLEGAL;
					goto kbd_done;
				} else if (t_key>0x5f && t_key<0x7f)
					t_key &= ~0x20;
			}
			else {
				if ((k->flags&(K_CAPSLOCK|K_CAPSMODE)) ==
					(K_CAPSLOCK|K_CAPSMODE))
					/* Check for roman characters */
				switch (k->language) {
				case K_N_KATAKANA:
				case K_I_KATAKANA:
				case K_I_KANJI:
				case K_I_SWISSFRENCH:
				case K_I_SWISSGERMAN:
				case K_I_SWISSFRENCH2:
				case K_I_SWISSGERMAN2:
					break;	/* Nothing */
				case K_I_CANENG:
				case K_I_CANFRENCH:
					swap(R_C_BEARD, R_c_BEARD);
					/* Fall through */
				case K_N_SWEDISH:
				case K_I_FINNISH:
				case K_I_SWEDISH:
					swap(R_E_BQUOTE, R_e_BQUOTE);
					/* Fall through */
				default:
					swap(R_U_DDOT,	R_u_DDOT);
					swap(R_O_DDOT,	R_o_DDOT);
					swap(R_A_DDOT,	R_a_DDOT);
					swap(R_A_DOT,	R_a_DOT);
					swap(R_N_TILTA,	R_n_TILTA);
					swap(R_AE,	R_ae);
					swap(R_ZERO,	R_zero);
				}
			}
		}
	}

	if (control) {
		k->anychar_state = 0;	/* Disable anychar key */
		/* If Katakana then check for <> */
		if (k->language == K_N_KATAKANA &&
		   (k->flags&(K_EXTEND|K_ASCII8))==(K_EXTEND|K_ASCII8) &&
		   key == 97) {
			/* Control > (KATAKANA) */
			k->flags |= K_KANAKBD;
			t_key = K_GO_KATAKANA;
			goto kbd_done;
		}
		/* Only convert codes codes 64 to 127
		** as does the 2622 only if enabled
		*/
		if (k->flags&K_CONTROL && t_key>=64 && t_key<=127)
			t_key &= CNTLKEY_MASK;	/* Convert to control */
	}

	/* Disable Collapsing of Shifts on A-Z if enabled */
	if (shift && (k->flags&K_SHIFT) == 0 && t_key>='A' && t_key<='Z')
		t_key |= 0x20;

	/* Handle anychar key */
	if (k->anychar_state) {
		if ((t_key>'9') || (t_key<'0'))
			k->anychar_state = 0;
		else {
			k->anychar_value *= 10;
			k->anychar_value += (t_key - '0');
			if (k->anychar_value <= 255)
			switch (k->anychar_state++) {
			default:
				k->anychar_state = 0;
				break;
			case 1:
				if ((t_key>'2') || (t_key<'0')) {
					k->anychar_state = 0;
					break;
				}
			case 2:
				return;
			case 3:
				k->anychar_state = 0;
				t_key = k->anychar_value;
			}
		}
	}

	/* Handle ISO 7 Bit */
	if ((k->flags&K_ASCII8)==0 && t_key<256)
		t_key = k->k_iso8to7[t_key]&0x7F;

	kbd_done:

	/* Add in control, shift, extend, numeric pad bits */
	if (t_key != K_ILLEGAL) {
		if (shift)	t_key |= (K_SHIFT_B<<8);
		if (control)	t_key |= (K_CONTROL_B<<8);
		if (k->extend)	t_key |= (K_EXTEND_B<<8);
		if (numpad)	t_key |= (K_NPAD<<8);
	}

	/* Send the code to the attached driver */
#ifdef __hp9000s300
	ite_filter(t_key);
#endif
#ifdef __hp9000s700
	/* Send the code to the attached driver */
	{
		/*
		*  The s800 ITE input code executes assuming the protection
		*  bit is turned on.  This used to be guaranteed by the graphics
		*  interface driver which handled HIL interrupts.
		*/
		int sm;

		sm = rsm(PSW_P);
		ite_queue(k->unit, t_key);
		if (sm & PSW_P)
			ssm(PSW_P);
	}
#endif
}


/*
 * This routine handles "muting" of characters, that is, the processing of
 * non-advancing diacriticals (accents for European character sets).  Let us
 * imagine how this would work on a real paper typewriter, say to generate
 * an o-umlaut.  You would press the umlaut key.  Oddly enough, the
 * typewriter would not advance.  Then press the "o" key.  The o would
 * print over the umlaut, giving you an o-umlaut.
 *
 * Now, how do you get an o-umlaut on a USASCII roman-8 keyboard?  Make
 * sure you are in eight-bit mode (from the "terminal config" screen).  You
 * press the umlaut key (meta-u), which looks like it does nothing.  Then,
 * press the "o" key, and an o-umlaut appears!
 *
 * It would be nice if the ITE printed the accent when you pressed the key,
 * and then converted it to the accented character when you pressed the
 * second key, as the 2392 does.  We print nothing when you press the accent,
 * and then replace it with the accented character.
 *
 * Some oddities:
 *	<accent> <space> becomes <accent>
 *	<accent> <CR>	 becomes <accent> <CR>
 * These are both reasonable when you think about the typewriter model.
 *
 * This code has the useful feature that if you type two mutes in a row,
 * the second one counts.
 */

#ifdef __hp9000s300
kbd_mute_char(c)						    /* ENTRY */
#endif
#ifdef __hp9000s700
kbd_mute_char(k, c)
register struct k_keystate *k;
#endif
register unsigned c;
{
#ifdef __hp9000s300
    register struct k_keystate *k = &k_keystate;
#endif

    if ((k->flags&(K_MUTE|K_ASCII8))==(K_MUTE|K_ASCII8)) {
	register last = k->last_char;
	register struct mutes *m;

	/* Handle mutes or non-advancing diacriticals */
	k->last_char = c;

	/* Look for a match in the table of mutes */
	for (m=mutes; m->mute; m++) {
	    if (m->mute==c)		/* did we just type a mute? */
		return;			/* accept it and return */
					/* without printing it */

	    if (m->mute==last && m->old==c) {   /* mute & vowel? */
		if (c==CR) {			/* CR is special */
#ifdef __hp9000s300
		    ite_flush_check(3);		/* make room */
		    ite_out_char(last);		/* emit the accent */
#endif
#ifdef __hp9000s700
		    ite_flush_check_ext(k->unit, 3);	/* make room */
		    ite_out_char(k->unit, last);	/* emit the accent */
#endif
		}
		c = m->new;		/* replace character */
		break;
	    }
	}
    }

#ifdef __hp9000s300
    ite_out_char(c);	/* emit new accented character */
#endif
#ifdef __hp9000s700
    ite_out_char(k->unit, c);	/* emit new accented character */
#endif
}


unsigned char *language_to_table[] = {
	k_isus,		/* K_N_USASCII */
	k_isfr,		/* K_N_FRENCHQ */
	k_isgm,		/* K_N_GERMAN */
	k_issw,		/* K_N_SWEDISH */
	k_isnsp,	/* K_N_SPANISH */
	k_isus,		/* K_N_KATAKANA */
	k_isfr,		/* K_N_FRENCHA */
	k_isus,		/* K_I_USASCII */
	k_isfr,		/* K_I_BELGIAN */
	k_iscn,		/* K_I_CANENG */
	k_isdn,		/* K_I_DANISH */
	k_isdu,		/* K_I_DUTCH */
	k_issw,		/* K_I_FINNISH */
	k_isfr,		/* K_I_FRENCH */
	k_iscn,		/* K_I_CANFRENCH */
	k_isss,		/* K_I_SWISSFRENCH */
	k_isgm,		/* K_I_GERMAN */
	k_isss,		/* K_I_SWISSGERMAN */
	k_isit,		/* K_I_ITALIAN */
	k_isnw,		/* K_I_NORWEGIAN */
	k_ises,		/* K_I_EUROSPANISH */
	k_isls,		/* K_I_LATSPANISH */
	k_issw,		/* K_I_SWEDISH */
	k_isuk,		/* K_I_UNITEDK */
	k_isus,		/* K_I_KATAKANA */
	k_isss,		/* K_I_SWISSFRENCH2 */
	k_isss,		/* K_I_SWISSGERMAN2 */
	k_isus,		/* K_I_KANJI */
};

/*
 * Switch to a different keyboard language
 */
#ifdef __hp9000s300
kbd_language()							    /* ENTRY */
#endif
#ifdef __hp9000s700
kbd_language(k)
register struct k_keystate *k;
#endif
{
#ifdef __hp9000s300
	register struct k_keystate *k = &k_keystate;
#endif
	register unsigned char *p;
	register i;

	/* Point to Language Dependent Keycode Table */
	k->langtab = k_langtab[k->language][!!(k->flags & K_PC_AT)];

	/* Preload iso translation tables */
	for (i=0; i<256; i++) {
		k->k_iso7to8[i] = i;
		k->k_iso8to7[i] = i;
	}

	/* Set up ISO 7 Bit Conversion Routine */
	p = language_to_table[k->language];

	/* Fill in 7 to 8 bit translations */
	for (; *p; p+=2) {
		k->k_iso8to7[p[1]] = p[0];
		k->k_iso7to8[p[0]] = p[1];
	}
}
