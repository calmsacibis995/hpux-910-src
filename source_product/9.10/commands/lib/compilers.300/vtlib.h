/* file vtlib.h */
/*    SCCS    REV(64.2);       DATE(92/04/03        14:22:40) */
/* KLEENIX_ID @(#)vtlib.h	64.2 91/10/09 */
/* -*-C-*-
********************************************************************************
*
* File:         vtlib.h
* RCS:          $Header: vtlib.h,v 70.3 92/04/03 14:19:44 ssa Exp $
* Description:  Types for library routines to create VT table -- used by compilers
* Author:       Jim Wichelman, SES
* Created:      Fri Jun 16 13:08:57 1989
* Modified:     Mon Sep 11 14:37:27 1989 (Jim Wichelman) jww@hpfcjww
* Language:     C
* Package:      N/A
* Status:       Experimental (Do Not Distribute)
*
* (c) Copyright 1989, Hewlett-Packard Company, all rights reserved.
*
********************************************************************************
*/
#if __STDC__
extern void remove_vt_tempfile(void);
extern void register_vt_error_callback(int (*error_routine)());
extern long add_to_vt(char *s,int check_duplicates,int add_null);
extern char *dump_vt(void);
extern void dump_vt_to_dot_s(void (*output_func)(),int remove_tempfile);
extern void dump_vt_to_ucode(void (*output_func)(),int remove_tempfile);
#else
extern void remove_vt_tempfile();
extern void register_vt_error_callback();
extern long add_to_vt();
extern char *dump_vt();
extern void dump_vt_to_dot_s();
extern void dump_vt_to_ucode();
#endif
