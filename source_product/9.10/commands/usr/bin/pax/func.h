/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/func.h,v $
 *
 * $Revision: 66.1 $
 *
 * func.h - function type and argument declarations
 *
 * DESCRIPTION
 *
 *	This file contains function delcarations in both ANSI style
 *	(function prototypes) and traditional style. 
 *
 * AUTHOR
 *
 *     Mark H. Colburn, Open Systems Architects, Inc. (mark@osa.com)
 *
 * COPYRIGHT
 *
 *	Copyright (c) 1989 Mark H. Colburn.  All rights reserved.
 *
 *	Redistribution and use in source and binary forms are permitted
 *	provided that the above copyright notice and this paragraph are
 *	duplicated in all such forms and that any documentation,
 *	advertising materials, and other materials related to such
 *	distribution and use acknowledge that the software was developed
 *	by Mark H. Colburn.
 *
 *	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 *	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _PAX_FUNC_H
#define _PAX_FUNC_H

/* Function Prototypes */

#ifdef __STDC__
#define P(x)    x
#else
#define P(x)	()
#endif

/*
 * Theortically, this should not be necessary, however, some ANSI compilers
 * (notably GCC), do not yet have an ANSI conforming support library.
 */
#ifdef VOIDSIG
void                (*signal()) ();
#else /* VOIDSIG */
int                 (*signal()) ();
#endif /* VOIDSIG */

/* append.c */
void append_archive P((void ));

/* buffer.c */
int inentry P((char *name , Stat *asb ));
void outdata P((int fd , char *name , OFFSET size ));
void write_eot P((void ));
void outwrite P((char *idx , uint len ));
void passdata P((char *from , int ifd , char *to , int ofd ));
void buf_allocate P((OFFSET size ));
int buf_skip P((OFFSET len ));
int buf_read P((char *dst , uint len ));
int ar_read P((void ));

/* cpio.c */
int do_cpio P((int argc , char **argv ));

/* create.c */
int create_archive P((void ));

/* dbug.c */
void _db_push_ P((char *control ));
void _db_pop_ P((void ));
void _db_enter_ P((char *_func_ , char *_file_ , int _line_ , char **_sfunc_ , char **_sfile_ , int *_slevel_ ));
void _db_return_ P((int _line_ , char **_sfunc_ , char **_sfile_ , int *_slevel_ ));
void _db_pargs_ P((int _line_ , char *keyword ));
void _db_doprnt_ P((char *format , long ARGLIST ));
void _db_printf_ P((int _line_ , char *keyword , char *format , long ARGLIST ));
void _db_setjmp_ P((void ));
void _db_longjmp_ P((void ));
int Delay P((void ));

/* extract.c */
int read_archive P((void ));
int get_header P((char *name , Stat *asb ));

/* fileio.c */
int open_archive P((int mode ));
void close_archive P((void ));
int openout P((char *name , Stat *asb , Link *linkp , int ispass ));
int openin P((char *name , Stat *asb ));

/* link.c */
Link *linkfrom P((char *name , Stat *asb ));
Link *islink P((char *name , Stat *asb ));
Link *linkto P((char *name , Stat *asb ));
void linkleft P((void ));

/* list.c */
int read_header P((char *name , Stat *asb ));
void print_entry P((char *name , Stat *asb ));

/* mem.c */
char *mem_get P((uint len ));
char *mem_str P((char *str ));

/* msdos.c */
void dio_str P((char *s ));
int dio_write P((int drive , char *from_buf , unsigned int from_cnt ));
int dio_read P((int drive , char *to_buf , unsigned int to_cnt ));
int dio_to_binary P((int h ));
int dio_open_check P((char *s ));
int dio_open2 P((char *p , int f ));
int dio_open3 P((char *p , int f , int m ));
int dio_close P((int h ));
long dio_lseek P((int h , long o , int r ));
struct passwd *getpwuid P((int x ));
struct passwd *getpwnam P((char *s ));
struct group *getgrgid P((int x ));
struct group *getgrnam P((char *s ));
void setgrent P((void ));
int getuid P((void ));
int getgid P((void ));
int link P((char *from , char *to ));
int chown P((char *name , int uid , int gid ));

/* namelist.c */
void add_name P((char *name ));
int name_match P((char *p ));
void notfound P((void ));
void name_init P((int argc , char **argv ));
int name_next P((char *name , Stat *statbuf ));
void name_gather P((void ));

/* names.c */
char *finduname P((UIDTYPE uuid ));
UIDTYPE finduid P((char *uname ));
char *findgname P((GIDTYPE ggid ));
GIDTYPE findgid P((char *gname ));

/* pass.c */
int pass P((char *dirname ));
int passitem P((char *from , Stat *asb , int ifd , char *dir ));

/* pathname.c */
int dirneed P((char *name ));
int nameopt P((char *begin ));
int dirmake P((char *name , Stat *asb ));

/* pax.c */
int main P((int argc , char **argv ));
int do_pax P((int ac , char **av ));
void get_archive_type P((void ));

/* port.c */
int mkdir P((char *dpath , int dmode ));
int rmdir P((char *dpath ));
#ifndef STRERROR
char *strerror P((int errnum));
#endif
int getopt P((int argc , char **argv , char *opts ));
#ifndef MEMCPY
char *memcpy P((char *s1 , char *s2 , unsigned int n ));
#endif
#ifndef MEMSET
char *memset P((char *s1 , int c , unsigned int n ));
#endif

/* regexp.c */
regexp *regcomp P((char *exp ));
int regexec P((Reg1 regexp *prog , Reg2 char *string ));
void regdump P((regexp *r ));
void regsub P((regexp *prog , char *source , char *dest ));
void regerror P((char *s ));

/* replace.c */
void add_replstr P((char *pattern ));
void rpl_name P((char *name ));
int get_disposition P((char *mode , char *name ));
int get_newname P((char *name , int size ));

/* tar.c */
int do_tar P((int argc , char **argv ));

/* ttyio.c */
int open_tty P((void ));
int nextask P((char *msg , char *answer , int limit ));
int lineget P((FILE *stream , char *buf ));
void next P((int mode ));

/* warn.c */
void warnarch P((char *msg , OFFSET adjust ));
void fatal P((char *why ));
void warn P((char *what , char *why ));

/* wildmat.c */
int wildmat P((char *pattern , char *source ));

#ifndef __STDC__
char *malloc();
char *rindex();
char *index();
char *strtok();
char *getenv();
#endif /*__STDC__*/

#undef P
#endif /* _PAX_FUNC_H */
