/* @(#) $Revision: 51.1 $ */      
/********************************************************/
/*							*/
/*			LANLOGIN			*/
/*							*/
/*   This library routine takes a user-supplied login   */
/*   as its argument. If that login ends in a colon,    */
/*   the user is prompted for a password (with echoing	*/
/*   turned off).					*/
/*   If there is a colon in the middle of the login     */
/*   string, no prompt will be issued                   */
/*                                                      */
/*                                                      */
/*   This code was obtained from Bob Lenk of FSD.       */
/*   Modified by Mike Shipley  Sept 7 1984.             */
/*							*/
/********************************************************/


#include <stdio.h>
#define MAXLOGIN	64		/* should never need more than 17 */

#ifdef NLS
#define NL_SETN 1
#endif


char *lanlogin (original)
char *original;
{
	static char	with_password[MAXLOGIN];
	char		*password;
	int		original_length;
	char		*getpass();
	char		*strchr();

	original_length = strlen (original);
	if (strchr(original, ':') != (original + (original_length -1) ) ) /*MS*/
		return (original);
	password = getpass ((nl_msg(501, "Password:")));
	if (original_length + strlen (password) >= MAXLOGIN)
	{
		fprintf (stderr, (nl_msg(502, "name and password too long\n")));
		exit (1);
	}

	strcpy (with_password, original);
	strcpy (&with_password[original_length], password);
	return (with_password);
}
	

