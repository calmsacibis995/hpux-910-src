/*
static char rcsId[] =
  "@(#) $Header: config.c,v 66.12 91/01/01 15:12:58 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


/*********************************** config.c ********************************/
/*                                                                           */
/* This module is responsible for reading all softkey files.  It reads an    */
/* entire softkey file internally and then returns individual softkeys       */
/* to the intrinsics module as needed.  This module only physically rereads  */
/* a softkey file if its modify time changes.                                */
/*                                                                           */
/* This module maintains a linked-list of "open" softkey files.  Within      */
/* that list are pointers to the corresponding softkey hierarchies.  In      */
/* these hierarchies are the individual softkeys themselves.                 */
/*                                                                           */
/*****************************************************************************/


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <values.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "keyshell.h"
#include "global.h"
#include "config.h"
#include "display.h"
#include "debug.h"

#include "message.h"
#include "chunk.h"
#include "hierarchy.h"
#include "linklist.h"
#include "string2.h"
#include "quote.h"
#include "buffer.h"


/********************************* COUNTall **********************************/
/*                                                                           */
/* This enable/disable count should be greater than the number of children   */
/* we ever expect to encounter in a single level of a softkey definition.    */
/* It must also fit in a "short".                                            */
/*                                                                           */
/*****************************************************************************/

#define COUNTall      1000


/************************ errorDetail / errorWord ****************************/
/*                                                                           */
/* The errorDetail pointer points to a string with additional detail about   */
/* the most recent "syntax error" in a softkey file being read.  errorWord   */
/* is the word which was read when the error occurred -- it corresponds to   */
/* the "%s" in errorDetail.                                                  */
/*                                                                           */
/*****************************************************************************/

static char *errorDetail;
static char *errorWord;


/******************************* SoftKeyFile *********************************/
/*                                                                           */
/* This structure identifies an open softkey file.  The structures           */
/* corresponding to all open softkey files are linked into a list so that    */
/* when the intrinsics module asks for a particular softkey from a           */
/* particular softkey file, we can scan the list to see if the file is       */
/* already open.  If it isn't, we read in the file and create a new entry    */
/* in the linked list.  Then we search the file's softkey hierarchy for      */
/* the particular softkey and return it if found.                            */
/*                                                                           */
/* In particular, the fields of this structure are:                          */
/*                                                                           */
/*   link       - used to link this entry into the "softKeyFiles" list       */
/*   file       - a pointer to this file's path name                         */
/*   st_mtime   - the modification time of this file                         */
/*   chunk      - the chunk used to allocate storage for this file           */
/*   root       - a pointer to the softkeyhierarchy for this file            */
/*                                                                           */
/* Note that the "chunk" in the SoftKeyFile structure is used only to        */
/* allocate file-wide data structures.  In particular, it is not used to     */
/* allocate the structures associated with individual softkeys, as they      */
/* each contain their own chunk.                                             */
/*                                                                           */
/*****************************************************************************/

typedef struct SoftKeyFile {
  Link link;
  char *file;
  time_t st_mtime;
  Chunk *chunk;
  GlobalSoftKey *root;
} SoftKeyFile;


/****************************** softKeyFiles *********************************/
/*                                                                           */
/* This variable points to the base of the linked list of SoftKeyFile        */
/* structures corresponding to the softkey files already opened and read     */
/* by this module.                                                           */
/*                                                                           */
/*****************************************************************************/

static SoftKeyFile *softKeyFiles;


/*****************************************************************************/
/*****************************************************************************/
/**************************** accelerators ***********************************/
/*****************************************************************************/
/*****************************************************************************/


/**************************** MAXchar / MASKlength ***************************/
/*                                                                           */
/* These constants define the maximum possible number of different           */
/* characters which can be represented in a "char" (MAXchar) and the         */
/* number of integers it would take to allocate a single bit for each of     */
/* those characters (MASKlength).                                            */
/*                                                                           */
/*****************************************************************************/

#define MAXchar     (1 << BITS(char))
#define MASKlength  (MAXchar/BITS(int))


/*************************** acceleratorMask *********************************/
/*                                                                           */
/* This array holds one bit for each possible character we represent with    */
/* an 8 bit codeset.  We set the bit corresponding to a character when we    */
/* pick that character to be a keyboard accelerator (or "selector", as       */
/* it is called in the man-page) character.  This way we avoid ever          */
/* allocating duplicate accelerator characters within the same softkey       */
/* bank.                                                                     */
/*                                                                           */
/*****************************************************************************/

static int acceleratorMask[MASKlength];


/*************************** TAKE() / TAKEN() ********************************/
/*                                                                           */
/* The "TAKE" function just marks the bit in "acceleratorMask" corresponding */
/* to the character "c" as being picked as an accelerator character.  The    */
/* "TAKEN" function just checks to see if the character "c" has already      */
/* been picked as an accelerator character.                                  */
/*                                                                           */
/*****************************************************************************/

#define TAKE(c)     { acceleratorMask[(c)/BITS(int)] |= 1 << ((c)%BITS(int)); }
#define TAKEN(c)    ( acceleratorMask[(c)/BITS(int)] & 1 << ((c)%BITS(int)) )


/****************************** ClearAccelerators() **************************/
/*                                                                           */
/* This function resets all of the bits in "acceleratorMask" to 0 --         */
/* indicating that no keyboard accelerator characters have yet been          */
/* assigned.  It then sets the bits corresponding to accelerator for the     */
/* etcetera and help softkeys (usually, "E" and "H") to 1 so that            */
/* no other softkey will try to use them.                                    */
/*                                                                           */
/*****************************************************************************/

static void ClearAccelerators()
{
  int i;

  for (i = 0; i < MASKlength; i++) {
    acceleratorMask[i] = 0;
  }
  TAKE(tolower(*MESSAGEhelpAccelerator));
  TAKE(tolower(*MESSAGEetcAccelerator));
}


/****************************** AssignAccelerator() **************************/
/*                                                                           */
/* This function assigns an accelerator to the specified softkey.  It        */
/* checks each letter of the label, in turn, checking to see if is both      */
/* a letter with an upper case equivalent and not already taken as an        */
/* accelerator character in this particular bank of softkeys.  The           */
/* first such letter found is chosen to be the accelerator character         */
/* for the softkey.  If no letter is found, any available upper case letter  */
/* is used.                                                                  */
/*                                                                           */
/*****************************************************************************/

static void AssignAccelerator(sk)
GlobalSoftKey *sk;
{
  int c;
  int C;
  char *s;
  int skLine;

  if (sk->attrib.empty) {
    return;
  }

  /* make sure the softkey label is all lower-case to begin with */
  for (skLine = 0; skLine < DISPLAYsoftKeyLines; skLine++) {
    (void)StringUnCapitalize(sk->label[skLine]);
  }

  /* if accelerators aren't enabled, then if this is a top-level */
  /* softkey, just capitalize its first letter and return.  otherwise */
  /* just return. */
  if (! globalAccelerate) {
    if (HierParent(sk) == globalSoftKeys) {
      sk->label[0][0] = toupper(sk->label[0][0]);
    }
    return;
  }

  /* thenlook for a letter with an upper-case equivalent which isn't an */
  /* accelerator yet.  pick it if found. */
  for (skLine = 0; skLine < DISPLAYsoftKeyLines; skLine++) {
    for (s = sk->label[skLine]; c = *s; s++) {
      C = toupper(c);
      if (c != C && ! TAKEN(c)) {
        TAKE(c);
        *s = C;
        sk->accelerator = C;
        return;
      }
    }
  }

  /* otherwise just pick any available upper case letter */
  skLine--;
  for (c = 0; c < MAXchar; c++) {
    C = toupper(c);
    if (c != C && ! TAKEN(c)) {
      TAKE(c);
      sk->label[skLine][DISPLAYsoftKeyLength-1] = C;
      sk->accelerator = C;
      return;
    }
  }
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/******************************* VisibleChildCount() *************************/
/*                                                                           */
/* This function returns the count of visible children associated with       */
/* the specified softkey.  Children following the "Keysh_config" built-in    */
/* command in the top-level softkey menu are never visible.                  */
/*                                                                           */
/*****************************************************************************/


static int VisibleChildCount(root)
GlobalSoftKey *root;
{
  int count;
  GlobalSoftKey *sk;

  count = 0;
  for (sk = HierHeadChild(root); sk; sk = HierNextSibling(sk)) {
    count++;
    if (sk->attrib.keyshell) {
      break;
    }
  }
  return count;
}


/***************************** LinkRequisitesEtc() ***************************/
/*                                                                           */
/* This is probably the most complex function in the entire module.  Its     */
/* primary intent is to link the "nextRequired" pointers from each           */
/* softkey node to the next node the user can see or has seen in the         */
/* hierarchy which has a "required" attribute.  The algorithm is described   */
/* briefly in the softkeys(4) man-page.  This routine returns a pointer      */
/* to the first immediate child of "root" which has a "required" attribute.  */
/*                                                                           */
/* This routine is very recursive.                                           */
/*                                                                           */
/*****************************************************************************/


static GlobalSoftKey *LinkRequisitesEtc(root)
GlobalSoftKey *root;
{
  int first;
  int position;
  GlobalSoftKey *sk;
  GlobalSoftKey *sibling;
  GlobalSoftKey *parent;
  GlobalSoftKey *requiredChild;
  GlobalSoftKey *requiredSibling;

  position = 0;
  requiredChild = NULL;
  requiredSibling = NULL;

  /* for each immediate child of "root", do... */
  for (sk = HierHeadChild(root); sk; sk = HierNextSibling(sk)) {
    sk->disable += position;
    sk->enable += position;
    sk->position = position;

    /* if this child has its own children, then check to see if any */
    /* of them are required.  link our nextRequired pointer to the */
    /* first one, if so. */
    if (HierChildCount(sk)) {
      if (requiredChild = LinkRequisitesEtc(sk)) {
        sk->nextRequired = requiredChild;
        goto required;
      }
    }

    /* then check to see if any of this child's siblings are required. */
    /* if so, link our nextRequired pointer to the first one. */
    parent = sk;
    while (HierParent(parent)) {
      first = parent->disable+1;

      /* if we are undisabling more than just one softkey back, then */
      /* force the user to satisfy any required nodes all over again. */
      /* otherwise, we just have a parameter where the user is allowed */
      /* to select it more than one time -- only the first time is */
      /* actually required. */
      if (first == parent->position) {
        first++;
      }
      sibling = HierNthChild(HierParent(parent), first);
      while (sibling) {
        if (sibling->required) {
          sk->nextRequired = sibling;
          goto required;
        }
        sibling = HierNextSibling(sibling);
      }

    /* next check if any of this child's parent's siblings, (and so-on, */
    /* moving up the softkey hierarchy) are required -- link them if so. */
      parent = HierParent(parent);
    }

    /* then check to see if this softkey itself is the first required */
    /* softkey of its siblings -- save its pointer if so */
  required:
    if (! requiredSibling && sk->required) {
      requiredSibling = sk;
    }

    position++;
  }

  /* then return a pointer to the first required child of "root" */
  return requiredSibling;
}


/*****************************************************************************/
/*****************************************************************************/
/******************************** label manipulation *************************/
/*****************************************************************************/
/*****************************************************************************/


/******************************** NextWord() *********************************/
/*                                                                           */
/* This routine returns a pointer to the beginning of the next real word     */
/* in the softkey label stringpointed to by "this".  The pointer points      */
/* either to the end of the string, to the next "_" character, or just past  */
/* the next '+' or '-' char.                                                 */
/*                                                                           */
/*****************************************************************************/


static char *NextWord(this)
char *this;
{
  if (*this) {
    do {
      this++;
    } while (*this && *this != '_' && *(this-1) != '-' && *(this-1) != '+');
  }
  return this;
}

/******************************* CopyWords() *********************************/
/*                                                                           */
/* This routine copies whole words from the "src" string to the "dest"       */
/* string for a maximum of "len" characters.  It is used to create           */
/* properly hyphenated/divided softkey labels from softkey names.            */
/*                                                                           */
/*****************************************************************************/

static void CopyWords(dest, src, len)
char *dest;
char *src;
int len;
{
  while (*src && len--) {
    if (*src == '_') {
      *dest = ' ';
    } else if (*src == '+') {
      *dest = '-';
    } else {
      *dest = *src;
    }
    src++;
    dest++;
  }
  *dest = '\0';
}


/*****************************************************************************/
/*****************************************************************************/
/****************************** softkey file input ***************************/
/*****************************************************************************/
/*****************************************************************************/


/******************************* ReadString() ********************************/
/*                                                                           */
/* This routine reads a (possibly quoted) word from the softkey file and     */
/* returns a pointer to it.  An empty string is returned at EOF.             */
/*                                                                           */
/*****************************************************************************/

static char *ReadString(extended)
int extended;
{
  char *s;

  s = QuoteReadWord(BufferPeekChar, BufferReadChar, extended, ';');
  if (! s) {
    return "";
  }
  return s;
}

/****************************** Count() **************************************/
/*                                                                           */
/* This routine reads a disable or enable count from the softkey file and    */
/* returns its integer value.  If the count is specified as either "all"     */
/* or "none", then COUNTall or -COUNTall is returned, respectively.          */
/*                                                                           */
/*****************************************************************************/

static int Count()
{
  char *p;

  p = ReadString(FALSE);
  if (strcmp(p, "all") == 0) {
    return COUNTall;
  } else if (strcmp(p, "none") == 0) {
    return -COUNTall;
  }
  return atoi(p);
}

/******************************* LoadSoftKey() *******************************/
/*                                                                           */
/* This routine reads a single softkey node definition from a softkey file,  */
/* allocating all storage to the specified chunk.  If the softkey is read    */
/* successfully, a pointer to the GlobalSoftKey struct is returned.          */
/* Otherwise, NULL is returned to indicate an error.                         */
/*                                                                           */
/*****************************************************************************/

static GlobalSoftKey *LoadSoftKey(chunk)
Chunk *chunk;
{
  int c;
  char *s;
  GlobalSoftKey *sk;

  sk = (GlobalSoftKey *)ChunkCalloc(chunk, sizeof(*sk));

  /* get the first word of the softkey definition */
  s = ReadString(FALSE);

  /* check if this is a backup softkey -- set the backup attribute if so */
  if (strcmp(s, "backup") == 0) {
    sk->attrib.backup = TRUE;
    s = ReadString(FALSE);
  }

  /* then check if this is s "string" or "option" type softkey */
  if (strcmp(s, "option") == 0 || strcmp(s, "softkey") == 0) {
    sk->attrib.softKey = TRUE;
  } else if (strcmp(s, "string") == 0) {
    sk->attrib.string = TRUE;
  } else {
    errorDetail = MESSAGEsoftKeyExpected;
    errorWord = s;
    return NULL;
  }

  /* then get the official softkey name from the softkey file */
  s = ReadString(FALSE);
  sk->alias = ChunkString(chunk, s);
  sk->literal = sk->alias;

  /* and then create the default softkey label from the official */
  /* softkey name */
  ConfigDefaultLabel(sk);

  /* then read any optional softkey attributes until we hit a semi-colon */
  /* or an open curly-brace */
  for (;;) {
    c = QuoteSkipSpace(BufferPeekChar, BufferReadChar);
    if (! c || c == ';' || c == '{') {
      break;
    }

    s = ReadString(FALSE);

    if (strcmp(s, "disable") == 0) {
      sk->disable = Count();
    } else if (strcmp(s, "enable") == 0) {
      sk->enable = Count();
    } else if (strcmp(s, "empty") == 0) {
      sk->attrib.empty = TRUE;
    } else if (strcmp(s, "automatic") == 0) {
      sk->attrib.automatic = TRUE;
    } else if (strcmp(s, "filter") == 0) {
      sk->states |= GLOBALstateFilter;
    } else if (strcmp(s, "command") == 0) {
      sk->states |= GLOBALstateCommand;
    } else if (strcmp(s, "motorola") == 0) {
      sk->states |= GLOBALstateMotorola;
    } else if (strcmp(s, "precision") == 0) {
      sk->states |= GLOBALstateHPPA;
    } else if (strcmp(s, "disabled") == 0) {
      sk->attrib.disabled = TRUE;
    } else if (strcmp(s, "literal") == 0) {
      sk->literal = ChunkString(chunk, ReadString(FALSE));
    } else if (strcmp(s, "editrule") == 0) {
      sk->editRule = ChunkString(chunk, ReadString(TRUE));
    } else if (strcmp(s, "cleanuprule") == 0) {
      sk->cleanUpRule = ChunkString(chunk, ReadString(TRUE));
    } else if (strcmp(s, "hint") == 0) {
      sk->hint = ChunkString(chunk, ReadString(FALSE));
    } else if (strcmp(s, "help") == 0) {
      sk->help = BufferReadOffset();
      (void)ReadString(FALSE);
    } else if (strcmp(s, "required") == 0) {
      sk->required = ChunkString(chunk, ReadString(FALSE));
    } else if (strcmp(s, "code") == 0) {
      sk->code = atoi(ReadString(FALSE));
      if (! sk->code) {
        errorDetail = MESSAGEnonZeroCodeExpected;
        errorWord = s;
        return NULL;
      }
    } else {
      errorDetail = MESSAGEattributeExpected;
      errorWord = s;
      return NULL;
    }
  }

  /* if the user specified neither "command" or "filter", then default */
  /* to both. */
  if (! (sk->states & (GLOBALstateCommand | GLOBALstateFilter))) {
    sk->states |= GLOBALstateCommand | GLOBALstateFilter;
  }

  /* if the user specified neither "motorola" or "precision", then */
  /* default to both. */
  if (! (sk->states & (GLOBALstateMotorola | GLOBALstateHPPA))) {
    sk->states |= GLOBALstateMotorola | GLOBALstateHPPA;
  }

  /* and return a pointer to the softkey */
  return sk;
}


/******************************** LoadSoftKeyTree() **************************/
/*                                                                           */
/* This routine reads a single softkey definition (i.e., hierarchy of        */
/* softkey nodes) from a softkey file.  All allocation is charged to the     */
/* specified chunk.  All nodes are tagged with the specified file name       */
/* and modify time.  If the hierarchy of nodes is read successfully, it      */
/* is attached to the specified "root" and its pointer is returned.          */
/* Otherwise, NULL is returned.                                              */
/*                                                                           */
/*****************************************************************************/

static GlobalSoftKey *LoadSoftKeyTree(chunk, file, st_mtime, root)
Chunk *chunk;
char *file;
time_t st_mtime;
GlobalSoftKey *root;
{
  int c;
  GlobalSoftKey *sk;

  /* read the next softkey node from the softkey file */
  if (! (sk = LoadSoftKey(chunk))) {
    return NULL;
  }
  HierCreateRoot(sk);
  sk->chunk = chunk;
  sk->file = file;
  sk->st_mtime = st_mtime;

  /* if the softkey node we just read has children nodes, read each of */
  /* them in sequence, and attach them to the current node. */
  if ((c = BufferReadChar()) == '{') {
    while ((c = QuoteSkipSpace(BufferPeekChar, BufferReadChar)) && c != '}') {
      if (! LoadSoftKeyTree(chunk, file, st_mtime, sk)) {
        return NULL;
      }
    }

    /* make sure there is a "}" at the end of the list of child nodes */
    if (c == '}') {
      (void)BufferReadChar();
    } else {
      errorDetail = MESSAGEcurlyExpected;
      errorWord = "";
      return NULL;
    }

  /* otherwise, this is a leaf node -- ensure it is followed by a ";" */
  } else if (c != ';') {
    errorDetail = MESSAGEsemiColonExpected;
    errorWord = ReadString(FALSE);
    return NULL;
  }

  /* finally, attach the hierarchy we just read to the specified "root" */
  HierAddTailChild(root, sk, TRUE);
  return sk;
}


/******************************* ReSynchronize() *****************************/
/*                                                                           */
/* This routine just synchronizes the softkey file to the beginning of the   */
/* next softkey following a softkey error.  This allows us to continue       */
/* finding subsequent errors in the softkey file.                            */
/*                                                                           */
/*****************************************************************************/

static void ReSynchronize()
{
  int c;
  int lastC;

  c = '\0';
  do {
    lastC = c;
    c = BufferReadChar();
  } while (c && (c != '}' || lastC != '\n'));
}


/******************************** LoadSoftKeyFile() **************************/
/*                                                                           */
/* This routine reads an entire file's worth of softkey definitions.  It     */
/* allocates a separate chunk for each softkey, reads the definition,        */
/* and finally links up the "nextRequired" pointers.  It returns a count     */
/* of softkeys actually read, or -1 in case of an error.                     */
/*                                                                           */
/*****************************************************************************/

static int LoadSoftKeyFile(file, st_mtime, root)
char *file;
time_t st_mtime;
GlobalSoftKey *root;
{
  char *s;
  int fd;
  int oldFd;
  int count;
  int errors;
  Chunk *chunk;
  char *newFile;
  GlobalSoftKey *sk;

  fd = open(file, O_RDONLY);
  if (fd >= 0) {
    count = 0;
    errors = 0;
    oldFd = BufferReadFd(fd);
    while (QuoteSkipSpace(BufferPeekChar, BufferReadChar)) {
      chunk = ChunkCreate();
      newFile = ChunkString(chunk, file);
      sk = LoadSoftKeyTree(chunk, newFile, st_mtime, root);
      if (! sk) {
        DisplayPrintf(MESSAGEerrorSoftKeyFile, file, bufferLine, bufferChar);
        if (s = strpbrk(StringBeginSig(errorWord), " \t\n")) {
          strncpy(s, "...", strlen(s));
        }
        DisplayPrintf(errorDetail, errorWord);
        DisplayFlush();
        ChunkDestroy(chunk);
        if (++errors < 10) {
          ReSynchronize();
        } else {
          DisplayPrintf(MESSAGEtooManyErrors);
          DisplayFlush();
          break;
        }
      } else {
        sk->nextRequired = LinkRequisitesEtc(sk);
        count++;
      }
    }
    close(fd);
    (void)BufferReadFd(oldFd);
    return count;
  }
  return -1;
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/********************************* FindFile() ********************************/
/*                                                                           */
/* This routine just scans the private linked list of file structures to     */
/* see if the requested file has already been read.  It returns a pointer    */
/* to the appropriate SoftKeyFile structure if so.  It returns NULL          */
/* otherwise.                                                                */
/*                                                                           */
/*****************************************************************************/

static SoftKeyFile *FindFile(file)
char *file;
{
  SoftKeyFile *skf;

  for (skf = LinkHead(softKeyFiles); skf; skf = LinkNext(skf)) {
    if (strcmp(file, skf->file) == 0) {
      return skf;
    }
  }
  return NULL;
}


/********************************* DestroyFile() *****************************/
/*                                                                           */
/* This routine deallocates all storage associated with the specified        */
/* SoftKeyFile structure.  It is typically called when a SoftKeyFile         */
/* structure is found to be out-of-date with the real softkey file and the   */
/* real softkey file is about to be reread.                                  */
/*                                                                           */
/*****************************************************************************/

static void DestroyFile(skf)
SoftKeyFile *skf;
{
  GlobalSoftKey *sk;
  GlobalSoftKey *nsk;

  /* deallocate the chunks for each softkey */
  for (sk = HierHeadChild(skf->root); sk; sk = nsk) {
    nsk = HierNextSibling(sk);
    ChunkDestroy(sk->chunk);
  }

  /* then remove this structure form the linked list of softkey files */
  (void)LinkRemove(skf);

  /* then deallocate the chunk associated with this structure itself. */
  ChunkDestroy(skf->chunk);
}


/*****************************************************************************/
/*****************************************************************************/
/***************************** external routines *****************************/
/*****************************************************************************/
/*****************************************************************************/

/******************************* ConfigInitialize() **************************/
/*                                                                           */
/* This routine performs local initialization for the config module.         */
/*                                                                           */
/*****************************************************************************/

void ConfigInitialize()
{
  static SoftKeyFile softKeyFile;

  softKeyFiles = &softKeyFile;
  LinkCreateBase(softKeyFiles);
}


/****************************** ConfigSoftKeyRoot() **************************/
/*                                                                           */
/* This routine returns the root of the softkeys hierarchy for the           */
/* specified file.  It first checks to see if that file is already loaded    */
/* and its modification time hasn't changed.  If not, it actually loads      */
/* (or reloads) the entire softkey file.                                     */
/*                                                                           */
/*****************************************************************************/

GlobalSoftKey *ConfigSoftKeyRoot(file)
char *file;
{
  int count;
  Chunk *chunk;
  SoftKeyFile *skf;
  GlobalSoftKey *sk;
  struct stat statBuf;

  /* check to see if the requested file is already loaded */
  file = QuotePath(file);
  skf = FindFile(file);
  if (stat(file, &statBuf) < 0) {
    DisplayPrintf(MESSAGEcannotOpenSoftKeyFile, file);
    DisplayFlush();
    sk = NULL;

  /* if so, and the modify time hasn't changed for the file, then we */
  /* are done -- just return the old root pointer. */
  } else if (skf && skf->st_mtime == statBuf.st_mtime) {
    sk = skf->root;

  /* otherwise we need to reload the specified softkey file. */
  } else {
    if (skf) {
      DestroyFile(skf);
    }
    chunk = ChunkCreate();
    skf = (SoftKeyFile *)ChunkMalloc(chunk, sizeof(*skf));
    skf->chunk = chunk;
    skf->file = ChunkString(chunk, file);
    skf->st_mtime = statBuf.st_mtime;
    skf->root = (GlobalSoftKey *)ChunkCalloc(chunk, sizeof(*skf->root));
    HierCreateRoot(skf->root);
    skf->root->literal = skf->file;
    count = LoadSoftKeyFile(file, statBuf.st_mtime, skf->root);

    /* if we had an error loading the softkey file, print a message. */
    /* otherwise link it into our list of loaded softkey files. */
    if (count <= 0) {
      ChunkDestroy(chunk);
      if (count < 0) {
        DisplayPrintf(MESSAGEcannotOpenSoftKeyFile, file);
        DisplayFlush();
      }
      sk = NULL;
    } else {
      LinkAddTail(softKeyFiles, skf);
      sk = skf->root;
    }
  }

  return sk;
}


/******************************* ConfigGetSoftKey() **************************/
/*                                                                           */
/* This routine looks up the specified softkey definition in the specified   */
/* softkey file.  It returns a pointer to the top of the node hierarchy      */
/* if found, and NULL otherwise.  These softkeys are effectively "given away"*/
/* to the caller of this routine -- that is, the config module no longer     */
/* knows about them until they are "put back".                               */
/*                                                                           */
/*****************************************************************************/

GlobalSoftKey *ConfigGetSoftKey(file, literal)
char *file;
char *literal;
{
  GlobalSoftKey *sk;
  GlobalSoftKey *root;

  if (! (root = ConfigSoftKeyRoot(file))) {
    return NULL;
  }
  sk = GlobalFindSoftKey(root, literal);
  if (sk) {
    (void)HierRemove(sk);
  }
  return sk;
}


/**************************** ConfigGetOtherSoftKey() ************************/
/*                                                                           */
/* This routine returns either a backup softkey or any remaining softkey     */
/* from the specified file.  These softkeys are effectively "given away"     */
/* to the caller of this routine -- that is, the config module no longer     */
/* knows about them until they are "put back".  Note that there is no        */
/* way to specify "which" of these "other" softkeys you want to get.         */
/*                                                                           */
/*****************************************************************************/

GlobalSoftKey *ConfigGetOtherSoftKey(file, backup)
char *file;
int backup;
{
  GlobalSoftKey *sk;
  GlobalSoftKey *root;

  if (! (root = ConfigSoftKeyRoot(file))) {
    return NULL;
  }
  for (sk = HierHeadChild(root); sk; sk = HierNextSibling(sk)) {
    if (sk->attrib.backup == backup) {
      break;
    }
  }
  if (sk) {
    (void)HierRemove(sk);
  }
  return sk;
}


/************************** ConfigPutSoftKeyBack() ***************************/
/*                                                                           */
/* This routine returns the specified softkey to the config module for       */
/* ownership.  If that softkey is now out of date, it will be thrown out     */
/* (since we already loaded a newer version of it).                          */
/*                                                                           */
/*****************************************************************************/

void ConfigPutSoftKeyBack(sk)
GlobalSoftKey *sk;
{
  SoftKeyFile *skf;

  skf = FindFile(sk->file);
  if (! skf || sk->st_mtime != skf->st_mtime) {
    ConfigThrowSoftKeyAway(sk);
    return;
  }
  sk->literal = sk->alias;
  HierAddTailChild(skf->root, sk, TRUE);
}


/******************************* ConfigThrowSoftKeyAway() ********************/
/*                                                                           */
/* This routine just deallocates all storage associated with the specified   */
/* softkey.                                                                  */
/*                                                                           */
/*****************************************************************************/

void ConfigThrowSoftKeyAway(sk)
GlobalSoftKey *sk;
{
  ChunkDestroy(sk->chunk);
  return;
}


/****************************** ConfigDefaultLabel() *************************/
/*                                                                           */
/* This routine creates a default label string (2 lines by 8 chars) for      */
/* the specified softkey based on its current official name.  The label      */
/* will be properly hyphenated/separated.                                    */
/*                                                                           */
/*****************************************************************************/

void ConfigDefaultLabel(sk)
GlobalSoftKey *sk;
{
  char *first;
  char *this;
  char *next;

  first = sk->literal;
  next = NextWord(first);
  do {
    this = next;
    next = NextWord(this);
  } while (next > this && next-first <= DISPLAYsoftKeyLength);
  CopyWords(sk->label[0], first, this-first);
  if (*this == '_') {
    this++;
  }
  CopyWords(sk->label[1], this, DISPLAYsoftKeyLength);

  this = sk->literal;
  next = sk->literal;
  while (*next) {
    if (*next == '+') {
      next++;
    } else {
      *this++ = *next++;
    }
  }
  *this = '\0';
}


/***************************** ConfigAssignAcceleratorsEtc() *****************/
/*                                                                           */
/* This routine assigns accelerator characters for all of the immediate      */
/* children node of "root".  It also assigns an etcetera count to "root"     */
/* based on the number of its visible children.                              */
/*                                                                           */
/*****************************************************************************/

void ConfigAssignAcceleratorsEtc(root)
GlobalSoftKey *root;
{
  int count;
  int position;
  GlobalSoftKey *sk;

  position = 0;
  count = VisibleChildCount(root);
  ClearAccelerators();

  /* for each child node of "root"... */
  for (sk = HierHeadChild(root); sk; sk = HierNextSibling(sk)) {

    /* allow accelerator characters to be reused in each new bank of */
    /* etceteras */
    if (! globalSoftKeyHelp && count != KEYBOARDsoftKeyCount &&
        position % (KEYBOARDsoftKeyCount-1) == 0) {
      ClearAccelerators();
    }

    /* and then assign an unused accelerator character */
    AssignAccelerator(sk);

    /* make sure disbale counts for all top level softkeys are set */
    if (root == globalSoftKeys) {
      sk->disable = COUNTall;
    }
    position++;
  }

  /* assign the etcetera count for "root" */
  if (count <= KEYBOARDsoftKeyCount) {
    root->etceteras = 0;
  } else {
    root->etceteras = (count-1)/(KEYBOARDsoftKeyCount-1) + 1;
  }
}


/*****************************************************************************/
