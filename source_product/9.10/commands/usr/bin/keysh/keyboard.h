/* RCS ID: @(#) $Header: keyboard.h,v 66.8 90/09/30 17:50:42 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef KEYBOARDincluded
#define KEYBOARDincluded


#define KEYBOARDf(c)         ((c) >= keyboardF1 && (c) <= keyboardF8)
#define KEYBOARDfToN(c)      ((c) - keyboardF1)
#define KEYBOARDnToF(n)      (keyboardF1 + n)

#define KEYBOARDchar(c)      (((c) < keyboardNull)?(c):(0))
#define KEYBOARDavailChar(c) (((c)<keyboardNull && (isalnum(c) || (c)=='_'))?\
                              (c):(0))
#define KEYBOARDaction(c)    (((c) > minAction && (c) < maxAction)?(c):(0))
#define KEYBOARDverify(c)    (((c) > minVerify && (c) < maxVerify)?(c):(0))
#define KEYBOARDaToV(c)      (minVerify + ((c) - minAction))

#define KEYBOARDsoftKeyCount ((globalOptions.enableHelpKey)?(7):(8))

#define KEYBOARDhelp (globalOptions.enableHelpKey?keyboardF1:0)
#define KEYBOARDetc  (globalTailExtent->displayed->etceteras?keyboardF8:0)

#define CNTL(c)              ((c) - '@')
#define ESC                  '\033'
#define DEL                  '\177'

typedef enum KeyboardChar {
/* MISC */
  keyboardNull = 0x10000,    /* must be first in enumeration */
  keyboardFlush,
  keyboardTimeout,
  keyboardClear,
/* BLOCKS */
  keyboardDo,
  keyboardEndDo,             /* count */
  keyboardExtendedDo,
  keyboardRedo,              /* count */
  keyboardUndo,              /* count */
  keyboardUndoAll,
/* INSERT */
  keyboardInsert,
  keyboardOverwrite,
  keyboardToggleInsertOverwrite,
/* MOVE */
  keyboardForward,
  keyboardForwardWord,
  keyboardForwardBWord,
  keyboardForwardVWord,
#ifndef KEYSHELL
  keyboardForwardBlank,
#endif
  keyboardBackward,
  keyboardBackwardWord,
  keyboardBackwardBWord,
  keyboardBackwardVWord,
#ifndef KEYSHELL
  keyboardBackwardBlank,
  keyboardBeginWord,
  keyboardBeginBWord,
  keyboardBeginVWord,
#endif
  keyboardEndWord,
  keyboardEndBWord,
  keyboardEndVWord,
  keyboardBeginLine,
  keyboardBeginBLine,
  keyboardEndLine,
#ifndef KEYSHELL
  keyboardEndBLine,
#endif
  keyboardViAdjust,
  keyboardViBackward,
  keyboardViForward,
  keyboardViDFAdjust,
  keyboardViCWAdjust,
#ifndef KEYSHELL
  keyboardSetMargin,
  keyboardClearMargin,
#endif
/* FIND */
  keyboardFindForward,       /* param */
  keyboardFindBackward,      /* param */
  keyboardFindForwardVi,     /* param */
  keyboardFindBackwardVi,    /* param */
  keyboardFindAgain,
  keyboardFindReverse,
  keyboardFindAdjust,
/* MARKS */
  keyboardSetMark,
  keyboardSetTempMark,
  keyboardExchange,
  keyboardExchangeTemp,
/* CUT */
  keyboardCut,
  keyboardCutTemp,
/* COPY */
  keyboardCopy,
  keyboardCopyTemp,
/* BUFFER */
#ifndef KEYSHELL
  keyboardCopyTempBuffer,
#endif
/* COMPOUND MOVE/CUT */
  keyboardDelete,
  keyboardDeleteWord,
#ifndef KEYSHELL
  keyboardDeleteBWord,
  keyboardDeleteVWord,
#endif
  keyboardBackspace,
  keyboardBackspaceWord,
  keyboardBackspaceBWord,
#ifndef KEYSHELL
  keyboardBackspaceVWord,
#endif
  keyboardDeleteLine,
  keyboardClearEOL,
/* PASTE */
  keyboardPaste,
  keyboardPasteTemp,
/* CASE */
  keyboardFlipCase,
  keyboardUpperCase,
  keyboardLowerCase,
  keyboardUpperCaseWord,
  keyboardLowerCaseWord,
  keyboardCapitalWord,
/* HISTORY */
  keyboardPrevious,          /* param */
  keyboardNext,              /* param */
  keyboardPrevious1,
  keyboardNext1,
  keyboardFirst,
  keyboardLast,
  keyboardGoto,              /* param */
  keyboardSearchBackward,
  keyboardSearchForward,
  keyboardSearchAgain,
  keyboardSearchReverse,
  keyboardParam,             /* param */
  keyboardFC,                /* param */
/* SCROLL */
  keyboardScrollBackward,
  keyboardScrollForward,
  keyboardPageBackward,
  keyboardPageForward,
/* EXPANSION */
  keyboardExpandOne,
  keyboardExpandAll,
  keyboardExpandList,
  keyboardEndInputMode,
/* ACTIONS/VERIFYS */
  minAction,                 /* must maintain parallel sequences! */
  keyboardTranslate,
  keyboardOperate,
  keyboardExecute,
  keyboardInterrupt,
  maxAction,
  minVerify,
  keyboardTranslateVerify,
  keyboardOperateVerify,
  keyboardExecuteVerify,
  keyboardInterruptVerify,   /* never used! */
  maxVerify,
/* SOFTKEYS */
  keyboardF1,                /* F1 thru F8 must be contiguous */
  keyboardF2,
  keyboardF3,
  keyboardF4,
  keyboardF5,
  keyboardF6,
  keyboardF7,
  keyboardF8,
/* HIGHER LEVEL ACTIONS */
  editAdd,
  editDelete,
  editEmpty,
  wordAdd,
  wordDelete,
  wordUpdate,
} KeyboardChar;


extern int keyboardQuote;

extern void KeyboardInitialize();
extern void KeyboardInputFeeder(/* int (*feeder)() */);
extern void KeyboardWrite(/* int c */);

extern void KeyboardBegin();

extern int KeyboardRead();
extern int KeyboardPeek();
extern int KeyboardAvail();

extern void KeyboardEnd();

#endif
