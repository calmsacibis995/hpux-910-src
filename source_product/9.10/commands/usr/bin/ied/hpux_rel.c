/* 
 *   Careful: e_version[5] onward is what is used by the emacs ^V and 
 *  (if supported) vi V commands.
 *
 * The makefile for this command will modify hpux_rel.c to massage
 * the string to "ied version xx.x  YY/MM/DD".
 *
 */
const char e_version[] = "@(#) ied version $Revision: 66.12 $ $Date: 91/01/11 12:09:41 $";
