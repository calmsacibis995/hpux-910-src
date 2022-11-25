/* RCS ID: @(#) $Header: kshhooks.h,v 66.8 90/09/26 22:53:13 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef KSHHOOKSincluded
#define KSHHOOKSincluded

#include "keyshell.h"

extern char kshPrompt[];
extern char kshStatus[];
extern int kshPromptLines;
extern int kshPromptLength;

extern int KshEmacsMode();
extern int KshViMode();

extern char *KshGetEnv();
extern void KshPutEnv();

extern int KshEdSetup();
extern int KshTtyRaw();
extern void KshTtyCooked();
extern int KshOutputFd();
extern int KshInputFd();
extern int KshEdExpand();

extern char *KshArgv0();
extern int KshInteractive();
extern int KshBuiltin();

extern int KshInterrupt();
extern int KshEraseChar();
extern int KshKillChar();
extern int KshEOFChar();

extern int KshHistMin();
extern int KshHistMax();
extern int KshGetHistCurrent();
extern void KshSetHistCurrent();
extern int KshHistCopy();
extern int KshHistFind();
extern char *KshHistWord();
extern void KshHistWrite();

extern char *KshFCCommand();

extern char *KshLogDir();

extern void KshJobNotWaitSafe();

/*****************************************************************************/

#ifdef KSHELL

/* included into sh/edit.c */

#include <setjmp.h>

#include "jobs.h"

#undef TRUE

char kshPrompt[PRSIZE+1];
char kshStatus[PRSIZE+1];
int kshPromptLines;
int kshPromptLength;

static char kshPromptLine[PRSIZE+1];

int KshEmacsMode()
{
  return is_option(EMACS|GMACS);
}

int KshViMode()
{
  return is_option(EDITVI);
}

char *KshGetEnv(s)
char *s;
{
  struct namnod *n;

  n = env_namset(s, sh.var_tree, P_FLAG);
  if (n) {
    return n->value.namval.cp;
  }
  return NULL;
}

void KshPutEnv(s, readOnly, export)
char *s;
int readOnly;
int export;
{
  char *e;
  int flags;
  struct namnod *n;

  if (e = strchr(s, '=')) {
    *e = '\0';
    n = env_namset(s, sh.var_tree, P_FLAG);
    *e = '=';
    if (n) {
      n->value.namflg &= ~R_FLAG;
    }
  }
  flags = ((readOnly)?(R_FLAG):(0)) | ((export)?(X_FLAG):(0));
  n = env_namset(s, sh.var_tree, flags);
  if (n && readOnly && ! export) {
    n->value.namflg &= ~X_FLAG;
  }
}

int KshEdSetup(fd)
int fd;
{
  char *p;
  char *t;
  int psn;
  char *ps1;
  extern char *StringTail();

  *io_stdout.ptr = '\0';

  ps1 = KshGetEnv("PS1");
  if (ps1 && (t = StringTail(io_stdout.base, ps1))) {
    psn = 1;
    strncpy(kshPrompt, t, PRSIZE);
    io_stdout.ptr = t;
  } else {
    psn = 0;
    strncpy(kshPrompt, io_stdout.base, PRSIZE);
    io_stdout.ptr = io_stdout.base;
  }

  kshPromptLines = 0;
  p = kshPrompt;
  while (p = strchr(p, '\n')) {
    p++;
    kshPromptLines++;
  }

  editb.e_prompt = kshPromptLine;
  ed_setup(fd);
  kshPromptLength = editb.e_plen;

  if (psn == 1) {
    p = KshGetEnv(KEYSHELLstatus);
    if (p) {
      strncpy(kshStatus, mac_try(p), PRSIZE);
    } else {
      kshStatus[0] = '\0';
    }
    free(malloc(10240)); /* yuck */
  }

  return psn;
}

int KshTtyRaw()
{
  return tty_raw(ERRIO);
}

void KshTtyCooked()
{
  tty_cooked(ERRIO);
}

int KshOutputFd()
{
  return ERRIO;
}

int KshInputFd()
{
  return editb.e_fd;
}

int KshEdExpand(s, i, eol, mode)
char *s;
int *i;
int *eol;
int mode;
{
  int status;
  static jmp_buf *old;
  jmp_buf new;

  old = sh.freturn;
  sh.freturn = (jmp_buf *)new;
  if (! setjmp(new)) {
    status = ed_expand(s, i, eol, mode);
  } else {
    status = -1;
  }
  sh.freturn = old;
  return status;
}

char *KshArgv0()
{
  return st.dolv[0];
}

int KshInteractive()
{
  return is_option(INTFLG);
}

int KshBuiltin()
{
  return st.states&BUILTIN || st.loopcnt;
}

int KshInterrupt()
{
  return sh.trapnote & SIGSET;
}

int KshEraseChar()
{
  return editb.e_erase;
}

int KshKillChar()
{
  return editb.e_kill;
}

int KshEOFChar()
{
  return editb.e_eof;
}

int KshHistMin()
{
  return editb.e_hismin+1;
}

int KshHistMax()
{
  return editb.e_hismax;
}

int KshGetHistCurrent()
{
  return editb.e_hline;
}

void KshSetHistCurrent(n)
int n;
{
  editb.e_hline = n;
}

int KshHistCopy(s, n)
char *s;
int n;
{ 
  return hist_copy(s, n, -1);
}

int KshHistFind(s, n, dir)
char *s;
int n;
int dir;
{
  return hist_find(s, n, 1, dir).his_command;
}

char *KshHistWord(s, n)
char *s;
int n;
{
  return hist_word(s, n);
}

void KshHistWrite(s)
char *s;
{
  if (hist_ptr) {
    p_setout(hist_ptr->fixfd);
    p_str(s, '\0');
    st.states |= FIXFLG;
    hist_flush();
    p_setout(ERRIO);
  }
}

char *KshFCCommand()
{
  return e_runvi;
}

char *KshLogDir(user)
char *user;
{
  extern char *logdir();

  return logdir(user);
}

void KshJobNotWaitSafe()
{
  if (job.waitsafe) {
    job.waitsafe--;
  }
/*if (vfork() == 0) {
    exit(0);
  }
*/
}

#endif /* KSHELL */

/*****************************************************************************/

#endif
