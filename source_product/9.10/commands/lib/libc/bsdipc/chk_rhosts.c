/*
 *  This is the definition of _check_rhosts_file for ruserok().
 *  It has to live in a separate file so that loaders don't pull
 *  rcmd.c for every executable.
 * 
 *  $Header: chk_rhosts.c,v 66.1 89/10/14 14:32:09 jmc Exp $
 */

int _check_rhosts_file = 1;
