/* @(#) $Revision: 1.8.83.4 $ */     
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/hilioctl.h,v $
 * $Revision: 1.8.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:27:03 $
 */

#ifndef _SYS_HILIOCTL_INCLUDED /* allows multiple inclusion */
#define _SYS_HILIOCTL_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _KERNEL_BUILD
#  include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#  include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

#ifdef _WSIO
struct eft2 {
	unsigned char b[2];
};
struct eft4 {
	unsigned char b[4];
};
struct eft5 {
	unsigned char b[5];
};

#define EFTRIUR _IOR('H',0x02,char)	/* Read the in use register. */
#define EFTRIM  _IOR('H',0x04,char)	/* Read the interrupt mask. */
#define EFTRKS  _IOR('H',0x05,char)	/* Read the keyboard status register */
#define EFTRCC  _IOR('H',0x11,char)	/* Read the configuration code. */
#define EFTRLC  _IOR('H',0x12,char)	/* Read the language code. */
#define EFTRRT  _IOR('H',0x31,struct eft5)/* Read the real time. */
#define EFTSIM  _IOW('H',0x40,char)	/* Set the interrupt mask. */
#define EFTSRD  _IOW('H',0xa0,char)	/* Set the repeat delay. */
#define EFTSRR  _IOW('H',0xa2,char)	/* Set the repeat rate. */
#define EFTSBI  _IOW('H',0xa3,struct eft2)/* Set the bell information. */
#define EFTSRPG _IOW('H',0xa6,char)	/* Set rate at which RPG 
					      can interrupt. */
#define EFTSRT  _IOW('H',0xad,struct eft5)/* Set the real time. */
#define EFTSBP  _IOW('H',0xc4,struct eft4)/* Send data to the beeper. */
#define EFTSKBD _IOW('H',0xe9,char)	/* Set "cooked" keyboard mask. */
#define EFTSLPC _IOW('H',0xeb,char)	/* Set HIL interface control
                                              register. */
#define EFTRR0  _IOR('H',0xf0,struct eft4)/* Read general purpose data
                                              buffer. */
#define EFTRT   _IOR('H',0xf4,struct eft4)/* Read the timers for the
                                              four voices. */
#define EFTRKBD _IOR('H',0xf9,char)	/* Read "cooked" keyboard mask. */
#define EFTRLPS _IOR('H',0xfa,char)	/* Read HIL interface status
                                              register. */
#define EFTRLPC _IOR('H',0xfb,char)	/* Read HIL interface control
                                              register. */
#define EFTRREV _IOR('H',0xfe,char)	/* Read the 8042 REVID. */

struct hil2 {
	unsigned char b[2];
};

#endif /* _WSIO  */

struct hil11 {
	unsigned char b[11];
};
struct hil16 {
	unsigned char b[16];
};

#define HILID  _IOR('h',0x03,struct hil11)/* Identify and Describe */
#define HILPST _IOR('h',0x05,char)	/* Perform Self Test */
#define HILRR  _IOWR('h',0x06,char)	/* Read Register */
#ifdef _WSIO
#define HILWR  _IOW('h',0x07,struct hil2)	/* Write Register */
#else
#define HILWR  _IOW('h',0x07,char)	/* Write Register */
#endif /* _WSIO */
#define HILRN  _IOR('h',0x30,struct hil16)/* Report Name */
#define HILRS  _IOR('h',0x31,struct hil16)/* Report Status */
#define HILED  _IOR('h',0x32,struct hil16)/* Extended Describe*/
#define HILSC  _IOR('h',0x33,struct hil16)/* Report Security Code */
#define HILDKR _IO('h',0x3d)		/* Disable Key Repeat */
#define HILER1 _IO('h',0x3e)		/* Enable Repeat, 1/30 */
#define HILER2 _IO('h',0x3f)		/* Enable Repeat, 1/60 */
#define HILP1  _IO('h',0x40)		/* Prompt 1 */
#define HILP2  _IO('h',0x41)		/* Prompt 2 */
#define HILP3  _IO('h',0x42)		/* Prompt 3 */
#define HILP4  _IO('h',0x43)		/* Prompt 4 */
#define HILP5  _IO('h',0x44)		/* Prompt 5 */
#define HILP6  _IO('h',0x45)		/* Prompt 6 */
#define HILP7  _IO('h',0x46)		/* Prompt 7 */
#define HILP   _IO('h',0x47)		/* Prompt */
#define HILA1  _IO('h',0x48)		/* Acknowledge 1 */
#define HILA2  _IO('h',0x49)		/* Acknowledge 2 */
#define HILA3  _IO('h',0x4a)		/* Acknowledge 3 */
#define HILA4  _IO('h',0x4b)		/* Acknowledge 4 */
#define HILA5  _IO('h',0x4c)		/* Acknowledge 5 */
#define HILA6  _IO('h',0x4d)		/* Acknowledge 6 */
#define HILA7  _IO('h',0x4e)		/* Acknowledge 7 */
#define HILA   _IO('h',0x4f)		/* Acknowledge */

#ifdef __hp9000s800

#define KBD_STATUS	    _IOR('H',0x05,char)	/* Read keyboard status. */
#define KBD_READ_CONFIG	    _IOR('H',0x11,char)	/* Read configuration code. */
#define KBD_READ_LANGUAGE   _IOR('H',0x12,char)	/* Read the language code. */
#define KBD_REPEAT_DELAY    _IOW('H',0xa0,char)	/* Set the repeat delay. */
#define KBD_REPEAT_RATE	    _IOW('H',0xa2,char)	/* Set the repeat rate. */
#define KBD_BEEP	    _IOW('H',0xa3,char)	/* Ring the bell. */

#define KBD_IDCODE_MASK		0x07
#define KBD_IDTYPE_ITF		0
#define KBD_IDTYPE_98203C	2

#define KBD_STAT_LEFTSHIFT	0x10
#define KBD_STAT_RIGHTSHIFT	0x40
#define KBD_STAT_SHIFT		0x01
#define KBD_STAT_CTRL		0x02
#define KBD_MAXVOLUME		255

/************************************************************************/
/* Definition of Language codes returned from KBD_READ_LANGUAGE		*/
/************************************************************************/

/* Language codes for ITF and 46030A keyboards.				*/
#define KBD_JAPANESE		0x02
#define KBD_SWISS_FRENCH	0x03
#define KBD_PORTUGESE		0x04
#define KBD_ARABIC		0x05
#define KBD_HEBREW		0x06
#define KBD_CANADIAN_ENGLISH	0x07
#define KBD_TURKISH		0x08
#define KBD_GREEK		0x09
#define KBD_THAI		0x0A
#define KBD_ITALIAN		0x0B
#define KBD_HANGUL		0x0C
#define KBD_DUTCH		0x0D
#define KBD_SWEDISH		0x0E
#define KBD_GERMAN		0x0F
#define KBD_CHINESE_PRC		0x10
#define KBD_CHINESE_ROC		0x11
#define KBD_SWISS_FRENCH_II	0x12
#define KBD_SPANISH		0x13
#define KBD_SWISS_GERMAN_II	0x14
#define KBD_BELGIAN		0x15
#define KBD_FINNISH		0x16
#define KBD_UNITED_KINGDOM	0x17
#define KBD_FRENCH_CANADIAN	0x18
#define KBD_SWISS_GERMAN	0x19
#define KBD_NORWEGIAN		0x1A
#define KBD_FRENCH		0x1B
#define KBD_DANISH		0x1C
#define KBD_KATAKANA		0x1D
#define KBD_LATIN_AMERICAN	0x1E
#define KBD_UNITED_STATES	0x1F

/* Language codes for 98203C keyboards.					*/
#define KBD_98203C_UNITED_STATES	0x00
#define KBD_98203C_FRENCHQ	0x01	/* French QWERTY */
#define KBD_98203C_GERMAN	0x02
#define KBD_98203C_FINNISH	0x03
#define KBD_98203C_SPANISH	0x04
#define KBD_98203C_KATAKANA	0x05
#define KBD_98203C_FRENCHA	0x06	/* French AZERTY */
#endif /* __hp9000s800 */

#endif /* ! _SYS_HILIOCTL_INCLUDED */
