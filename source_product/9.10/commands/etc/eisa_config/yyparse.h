/*****************************************************************************
 (C) Copyright Hewlett-Packard Co. 1991. All rights
 reserved.  Copying or other reproduction of this program except for archival
 purposes is prohibited without prior written consent of Hewlett-Packard.

			  RESTRICTED RIGHTS LEGEND

Use, duplication, or disclosure by Government is subject to restrictions
set forth in paragraph (b) (3) (B) of the Rights in Technical Data and
Computer Software clause in DAR 7-104.9(a).

HEWLETT-PACKARD COMPANY
Fort Collins Engineering Operation, Ft. Collins, CO 80525

******************************************************************************/
/*****************************************************************************
*
*                           inc/yyparse.h
*
*     This file contains the defines used by the parser to keep track of
*     tokens. This file is generated by yacc and should not be touched.
*
*****************************************************************************/


typedef union 					     /* YACC's parsing stack type    */
{
   unsigned long val;
   char  *str;
} YYSTYPE;
extern YYSTYPE yylval;
#define ADDRESS 257
#define AMPERAGE 258
#define BEGINOVL 259
#define BOARD 260
#define BUSMASTER 261
#define BYTE 262
#define CACHE 263
#define CATEGORY 264
#define CHOICE 265
#define COMBINE 266
#define COMMENTS 267
#define CONNECTION 268
#define COUNT 269
#define DECODE 270
#define DEFAULT 271
#define DIP 272
#define DISABLE 273
#define DMA 274
#define DWORD 275
#define EDGE 276
#define EISA 277
#define EISACMOS 278
#define EMB 279
#define ENDGROUP 280
#define ENDOVL 281
#define EXP 282
#define FACTORY 283
#define FIRST 284
#define FREE 285
#define FREEFORM 286
#define FUNCTION 287
#define GROUP 288
#define HELP 289
#define ID 290
#define INCLUDE 291
#define INIT 292
#define INITVAL 293
#define INLINE 294
#define INVALID 295
#define IOCHECK 296
#define IOPORT 297
#define IRQ 298
#define ISA16 299
#define ISA32 300
#define ISA8 301
#define ISA8OR16 302
#define JTYPE 303
#define JUMPER 304
#define KILOBYTES 305
#define LABEL 306
#define LENGTH 307
#define LEVEL 308
#define LINK 309
#define LOC 310
#define MEGABYTES 311
#define MEMORY 312
#define MEMTYPE 313
#define MFR 314
#define NAME 315
#define NO 316
#define NONVOLATILE 317
#define OTH 318
#define OTHER 319
#define PAIRED 320
#define PORT 321
#define PORTADR 322
#define PORTVAR 323
#define READID 324
#define REVERSE 325
#define ROTARY 326
#define SHARE 327
#define SHOW 328
#define SIZE 329
#define SIZING 330
#define SKIRT 331
#define SLIDE 332
#define SLOT 333
#define SOFTWARE 334
#define STEP 335
#define STYPE 336
#define SUBCHOICE 337
#define SUBFUNCTION 338
#define SUBTYPE 339
#define SUPPORTED 340
#define SWITCH 341
#define SYS 342
#define SYSTEM 343
#define TIMING 344
#define TOTALMEM 345
#define TRIGGER 346
#define TRIPOLE 347
#define TYPE 348
#define TYPEA 349
#define TYPEB 350
#define TYPEC 351
#define UNSUPPORTED 352
#define VALID 353
#define VERTICAL 354
#define VIR 355
#define WORD 356
#define WRITABLE 357
#define YES 358
#define BINNUM 359
#define DECNUM 360
#define HEXNUM 361
#define EISAPORT 362
#define MASK_BITS 363
#define TOGGLES 364
#define STRING 365
