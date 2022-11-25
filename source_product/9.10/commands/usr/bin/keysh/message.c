/*
static char rcsId[] =
  "@(#) $Header: message.c,v 66.10 91/01/01 15:25:29 rpt Exp $";
static char copyright[] =
  "Copyright (c) HP, 1989";
*/


#include <nl_types.h>

#include "chunk.h"
#include "message.h"

#define NULL 0


char *message[] = {
  NULL,
  "/usr/keysh/C/softkeys"                                     /* catgets 1 */,
  "/usr/keysh/C/keyshrc"                                      /* catgets 2 */,
  "/usr/keysh/C/help"                                         /* catgets 3 */,
  "/usr/keysh/builtins"                                       /* catgets 4 */,
  "/.softkeys"                                                /* catgets 5 */,
  "/.keyshrc"                                                 /* catgets 6 */,
  ""                                                          /* catgets 7 */,
  ""                                                          /* catgets 8 */,
  ""                                                          /* catgets 9 */,

  "%s: [10] Terminfo initialization failed; check $TERM.\n"   /* catgets 10 */,
  "%s: [11] Exec'ing /bin/ksh...\n"                           /* catgets 11 */,
  "%s: [12] Continuing without terminfo...\n"                 /* catgets 12 */,
  "[13] Cannot find softkey %q in file %q.\n"                 /* catgets 13 */,
  "[14] Cannot find softkey %q.\n"                            /* catgets 14 */,
  "[15] %q overwritten; previous changes may be lost.\n"      /* catgets 15 */,
  "[16] Cannot write %q; changes cannot be saved.\n"          /* catgets 16 */,
  "[17] Cannot remove %q; nothing changed.\n"                 /* catgets 17 */,
  "[18] Syntax error in configuration file -- ignored.\n"     /* catgets 18 */,
  "[19] Cannot open softkey file %q.\n"                       /* catgets 19 */,
  "[20] Unknown user %q.\n"                                   /* catgets 20 */,
  "[21] Error in softkey file %q, line %d, character %d.\n"   /* catgets 21 */,
  "[22] (New softkey node expected; found %q.)\n"             /* catgets 22 */,
  "[23] (Softkey attribute expected; found %q.)\n"            /* catgets 23 */,
  "[24] (\";\" expected; found %q.)\n"                        /* catgets 24 */,
  "[25] (\"}\" expected; found end of file.)\n"               /* catgets 25 */,
  "[26] (Non-zero code value expected; found %q.)\n"          /* catgets 26 */,
  "[27] Too many errors; skipping remaining softkeys.\n"      /* catgets 27 */,
  "[28] %s parse error in softkey %q near character %d.\n"    /* catgets 28 */,
  "[29] Softkey file has changed; cannot read help.\n"        /* catgets 29 */,
  "[30] Cannot read configuration from %q or %q.\n"           /* catgets 30 */,
  ""                                                          /* catgets 31 */,
  ""                                                          /* catgets 32 */,
  ""                                                          /* catgets 33 */,
  ""                                                          /* catgets 34 */,
  ""                                                          /* catgets 35 */,
  ""                                                          /* catgets 36 */,
  ""                                                          /* catgets 37 */,
  ""                                                          /* catgets 38 */,
  ""                                                          /* catgets 39 */,

  "--Help--"                                                  /* catgets 40 */,
  "HELP"                                                      /* catgets 41 */,
  "--More--"                                                  /* catgets 42 */,
  "MORE"                                                      /* catgets 43 */,
  " of "                                                      /* catgets 44 */,

  "You have new mail"                                         /* catgets 45 */,
  "You have mail"                                             /* catgets 46 */,
  "No mail"                                                   /* catgets 47 */,
  ""                                                          /* catgets 48 */,
  ""                                                          /* catgets 49 */,

  "Select a softkey or press <Return> for help in context."   /* catgets 50 */,
  "Sorry.  No help is available for that key."                /* catgets 51 */,
  "Sorry.  No hint is available for that key."                /* catgets 52 */,
  "Sorry.  No help is available."                             /* catgets 53 */,
  "Softkey is disabled."                                      /* catgets 54 */,
  "help"                                                      /* catgets 55 */,
  "Select the topic you want help with..."                    /* catgets 56 */,
  "(topics)"                                                  /* catgets 57 */,
  "QUIT"                                                      /* catgets 58 */,
  ""                                                          /* catgets 59 */,

  "Keysh_config"                                              /* catgets 60 */,
  "kc"                                                        /* catgets 61 */,
  "Then, enter a space."                                      /* catgets 62 */,
  "tsm-"                                                      /* catgets 63 */,
  "Incomplete..."                                             /* catgets 64 */,
  ""                                                          /* catgets 65 */,
  "Select any options; then "                                 /* catgets 66 */,
  "Select an option or "                                      /* catgets 67 */,
  "Not allowed here."                                         /* catgets 68 */,
  ""                                                          /* catgets 69 */,

  NULL
};

void MessageInitialize()
{
  int n;
  char *s;
  nl_catd fd;
  Chunk *chunk;

  chunk = ChunkCreate();
  fd = catopen("keysh", 0);
  for (n = 0; n < sizeof(message)/sizeof(char *); n++) {
    if (s = catgets(fd, NL_SETD, n, NULL)) {
      message[n] = ChunkString(chunk, s);
    }
  }
  (void)catclose(fd);
}

