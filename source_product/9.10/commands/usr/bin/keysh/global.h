/* RCS ID: @(#) $Header: global.h,v 66.11 91/01/01 15:17:49 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef GLOBALincluded
#define GLOBALincluded

#include <sys/types.h>

#include "display.h"
#include "keyboard.h"

#include "chunk.h"
#include "hierarchy.h"
#include "linklist.h"

typedef struct {
  int enableTranslations;
  int enableAccelerators;
  int enableHelpKey;
  int enableBackups;
  int enableInvisibles;
  int enableVisibles;
  int enablePrompts;
} GlobalOptions;

extern GlobalOptions globalOptions;

#define GLOBALstateCommand  1
#define GLOBALstateFilter   2
#define GLOBALstateHPPA     4
#define GLOBALstateMotorola 8

typedef struct GlobalSoftKey {
  Hier hier;
  struct {
    int softKey:2;
    int string:2;
    int empty:2;
    int etcetera:2;   /* special */
    int keyshell:2;   /* special */
    int disabled:2;   /* modifier */
    int backup:2;     /* modifier */
    int automatic:2;  /* modifier */
  } attrib;
  short states;
  short disable;
  short enable;
  short position;
  short etceteras;
  char label[DISPLAYsoftKeyLines][DISPLAYsoftKeyLength+1];
  short accelerator;
  char *literal;
  char *alias;
  char *editRule;
  char *cleanUpRule;
  off_t help;
  char *hint;
  char *required;
  struct GlobalSoftKey *nextRequired;
  short code;
  char *file;       /* config */
  time_t st_mtime;  /* config */
  Chunk *chunk;     /* config */
} GlobalSoftKey;

typedef enum GlobalType {
  typeUnknown,
  typeSoftKey,
  typeSeparator,
  typePipe,
  typeUnSoftKey,
  typeComment,
  typeIncomplete,
} GlobalType;

typedef struct GlobalExtent {
  Link link;
  struct {
    int errored:2;
    int visible:2;
  } flags;
  GlobalSoftKey *displayed;
  short enabled;
  short disabled;
  short state;
  short etcetera;
  char *word;
  GlobalType type;
  GlobalSoftKey *selected;
  char *error;
} GlobalExtent;


extern GlobalExtent *globalExtents;          /* base */
extern GlobalExtent *globalHeadExtent;
extern GlobalExtent *globalLastExtent;
extern GlobalExtent *globalTailExtent;

extern GlobalSoftKey *globalSoftKeys;        /* root */
extern GlobalSoftKey *globalKeyshell;        /* root */
extern GlobalSoftKey *globalBackupSoftKeys;  /* root */
extern GlobalSoftKey *globalHelpSoftKey;     /* root */
extern GlobalSoftKey *globalEmptySoftKey;

extern int globalSoftKeyBlank;
extern int globalSoftKeyBackup;
extern int globalSoftKeyHelp;
extern int globalSoftKeyTopics;
extern int globalAccelerate;

extern int globalState;

extern int globalInteractive;
extern int globalNonInteractive;

extern void GlobalInitialize();

extern void GlobalBegin();

extern
  GlobalSoftKey *GlobalSoftKeyCheck(/* GlobalExtent *e, GlobalSoftKey *sk */);
extern GlobalSoftKey *GlobalDisplayedSoftKeyN(/* int n */);
extern int GlobalFirstDisplayedSoftKey(/* GlobalSoftKey *psk */);

extern GlobalSoftKey *GlobalFindSoftKey(/* GlobalSoftKey *root,
                                           char *literal */);

extern void GlobalEtcNext();
extern void GlobalEtcThis();

extern void GlobalEnd();

#endif
