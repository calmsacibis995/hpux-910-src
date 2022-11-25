/* RCS ID: @(#) $Header: keyshell.h,v 66.11 91/08/27 16:39:38 ssa Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef KEYSHELLincluded
#define KEYSHELLincluded

/* EXPORTED TO KSH */

extern void KeyshellInitialize();
extern int KeyshellRead(/* int fd, char *buffer, int length */);
extern void KeyshellSigwinch();

/* EXPORTED TO KEYSHELL */

#ifndef TRUE
#  define TRUE   1
#  define FALSE  0
#endif

#ifndef NULL
#  define NULL   0
#endif

#define KEYSHELLstatus     "KEYSH"
#define KEYSHELLtimeOut    "KEYESC"
#define KEYSHELLsimulate   "KEYSIM"
#define KEYSHELLbell       "KEYBEL"
#define KEYSHELLenviron    "KEYENV"
#define KEYSHELLlocal      "KEYLOC"
#define KEYSHELLtsm        "KEYTSM"
#define KEYSHELLprompt     "KEYPS1"
#define KEYSHELLksh        "KEYKSH"
#define KEYSHELLetcetera   "KEYMORE"

extern char *HPUX_ID;
extern char *keyshellHelp;

extern int keyshellEOF;
extern int keyshellInterrupt;
extern int keyshellPSn;
extern int keyshellInitialized;
extern int keyshellTsmSoftKeys;
extern int keyshellTsm;

extern int keyshellSigwinch;

#endif
