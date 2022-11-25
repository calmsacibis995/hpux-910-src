/*
 *	@(#)Version.c	$Header: Version.c,v 1.1.109.2 91/11/21 13:58:02 kcs Exp $
 */

#ifndef lint
#ifdef hpux
char sccsid[] = "@(#)nslookup $Revision: 1.1.109.2 $ (ALPHA 9.0 rev A) %WHEN%\n";
#else
char sccsid[] = "@(#)nslookup $Revision: 1.1.109.2 $ %WHEN% %WHOANDWHERE%\n";
#endif
#endif /* not lint */

#ifdef hpux
char Version[] = "nslookup $Revision: 1.1.109.2 $ %WHEN%\n";
#else
char Version[] = "nslookup $Revision: 1.1.109.2 $ %WHEN%\n\t%WHOANDWHERE%\n";
#endif
