/**			domains.c			**/

/*
 *  @(#) $Revision: 66.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This file contains all the code dealing with the expansion of 
 *  domain based addresses in Elm.  It uses the file "domains" as
 *  defined in the sysdefs.h file.
 */


#include <ctype.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN   10
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


/*
 *  define the various characters that we can encounter after a "%" sign
 *  in the template file...
 */


#define USERNAME	'U'	/* %U = the name of the remote user */
#define HOSTNAME	'N'	/* %N = the remote machine name     */
#define FULLNAME	'D'	/* %D = %N + domain info given      */
#define NPATH		'R'	/* %R = path to %N from pathalias   */
#define PPATH		'P'	/* %P = path to 'P' from pathalias  */
#define OBSOLETE	'S'	/* %S = (used to be suffix string)  */


/*
 *  and finally some characters that are allowed in user/machine names 
 */

#define okay_others(c)	(c == '-' || c == '^' || c == '$' || c == '_')

/*
 *  and some allowed ONLY in the username field 
 */


#define special_chars(c)	(c == '%' || c == ':')


char		*find_path_to(), 
		*expand_domain(), 
		*match_and_expand_domain(),

		*strcpy(), 
		*strcat(), 
		*strtok();

unsigned long   sleep();
void            rewind();


char		expanded_domain[SLEN];		/* buffer for expanded domain name*/
extern char	path_to_machine[LONG_SLEN];     /* buffer for find_path_to()      */


int
open_domain_file()

{

	if ( (domainfd = fopen(domains, "r")) == NULL ) {
		dprint( 2, (debugfile, "Warning: can't open file %s as domains file\n",
			    domains) );
	} else {
		dprint( 3, (debugfile,
			   "Opened '%s' as the domain database\n\n", domains) );
	}

	/*
	 * if it fails it'll instantiate domainfd to NULL which is exactly
	 * what we want to have happen!! 
	 */
}


char  
*expand_domain( buffer )

	char           *buffer;

{
	/*
	 *  Expand the address 'buffer' based on the domain information, 
	 *  if any.  Returns NULL if it can't expand it for any reason.
	 */


	char            name[2 * NLEN], 
			address[2 * NLEN], 
			domain[2 * NLEN];
	char            *match_and_expand_domain();


	if ( domainfd == NULL )
		return ( NULL );		/* no file present! */

	if ( explode(buffer, name, address, domain) )
		return ( match_and_expand_domain(domain, name, address) );

	else {					/* invalid format-not "user@host.domain" */
		dprint( 6, (debugfile,
		        "Invalid format for domain expansion (no domain?) : %s\n",
		 	buffer) );
		return ( NULL );
	}
}


int
explode( buffer, name, address, domain )

	char            *buffer, 
			*name, 
			*address, 
			*domain;

{
	/*
	 *  Break buffer, if in format name@machine.domain, into the
	 *  component parts, otherwise return ZERO and don't worry
	 *  about the values of the parameters!
	 */


	register int    i, 
			j = 0;


	/*
	 *  First get the name... 
	 */

	for ( i = 0; buffer[i] != '@'; i++ ) {
		if ( !isalnum(buffer[i]) && !okay_others(buffer[i]) && !
		     special_chars(buffer[i]) )
			return ( 0 );		/* invalid character in string! */
		name[i] = buffer[i];
	}

	name[i++] = '\0';

	/*
	 *  now let's get the machinename 
	 */

	while ( buffer[i] != '.' ) {
		if ( !isalnum(buffer[i]) && !okay_others(buffer[i]) )
			return ( 0 );		/* invalid character in string! */

		address[j++] = buffer[i++];
	}

	address[j] = '\0';

	j = 0;

	/*
	 *  finally let's get the domain information (there better be some!) 
	 */

	while ( buffer[i] != '\0' ) {
		if ( !isalnum(buffer[i]) && !okay_others(buffer[i])
		      			 && buffer[i] != '.' )
			return ( 0 );		/* an you fail again, bozo! */

		domain[j++] = toupper(buffer[i]);
		i++;
	}

	domain[j] = '\0';

	return ( j );				/* if j == 0 there's no domain info! */
}


char 
*match_and_expand_domain( domain, name, machine )

	char            *domain, 
			*name, 
			*machine;
{
	/*
	 *  Given the domain, try to find it in the domain file and
   	 *  if found expand the entry and return the result as a 
	 *  character string...
	 */


	char            buffer[SLEN], 
			domainbuff[NLEN];

	char            field1[2 * NLEN], 
			field2[2 * NLEN], 
			field3[2 * NLEN];

	char            *path, 
			*template, 
			*expanded, 
			*mydomain, 
			*strtok();

	int             matched = 0, 
			in_percent = 0;

	register int    i, 
			j = 0;


	expanded_domain[j] = '\0';

	domainbuff[0] = '\0';
	mydomain = (char *) domainbuff;			/* set up buffer etc */

	do {
		rewind( domainfd );			/* back to ground zero! */

		if ( strlen(mydomain) > 0 ) {		/* already in a domain! */
			mydomain++;			/* skip leading '.' */

			while ( *mydomain != '.' && *mydomain != ',' )
				mydomain++;		/* next character   */

			if ( *mydomain == ',' )
				return ( NULL );	/* didn't find domain!  */
		} else
			sprintf( mydomain, "%s,", domain );	/* match ENTIRELY! */

		/*
		 * whip through file looking for the entry, please... 
		 */

		while ( fgets(buffer, SLEN, domainfd) != NULL ) {
			if ( buffer[0] == '#' )		/* skip comments */
				continue;

			if ( first_word(buffer, mydomain) ) {
				matched++;		/* Gotcha!  Remember this
							 * momentous event! */
				break;
			}
		}

		if ( !matched )
			continue;			/* Nothing.  Not a sausage!  Step
					 		 * through! */

		/*
		 *  We've matched the domain! 
		 */

		no_ret( buffer );

		(void) strtok( buffer, "," );		/* skip the domain info */

		strcpy( field1, strtok(NULL, ",") );	/* fun 		 */
		strcpy( field2, strtok(NULL, ",") );	/* stuff     */
		strcpy( field3, strtok(NULL, ",") );	/* eh?    */

		path = (char *) NULL;

		/*
		 * now we merely need to figure out what permutation this is! 
		 */

		if ( field3 == NULL || strlen(field3) == 0 )
			if ( field2 == NULL || strlen(field2) == 0 )
				template = (char *) field1;
			else {
				path = (char *) field1;
				template = (char *) field2;
			}

		else {
			dprint( 1, (debugfile,
				"Domain info for %s from file broken into THREE fields!!\n",
				domain) );
			dprint( 1, (debugfile,
			  	"-> %s\n-> %s\n-> %s\n", field1, field2, field3) );
			dprint(1, (debugfile, "(this means it's using a defunct field)\n"));

			error1( catgets(nl_fd,NL_SETN,1,"Warning: domain %s uses a defunct field!!"),
				domain);
			sleep( 2 );
			path = (char *) field1;
			template = (char *) field3;
		}

		if ( strlen(path) > 0 && path[0] == '>' )
			path++;			/* skip the '>' character, okay? */

		j = 0;				/* expanded_domain is zero, right now, right?? */
		expanded_domain[j] = '\0';		/* make sure string is too! */

		for ( i = 0; i < strlen(template); i++ ) {
			if ( template[i] == '%' ) {

				if ( !in_percent )	/* just hit a NEW percent! */
					in_percent = 1;
				else {	/* just another percent sign on the
					 * wall... */
					expanded_domain[j++] = '%';
					expanded_domain[j] = '\0';	/* ALWAYS NULL terminate */
					in_percent = 0;
				}

			} else if ( in_percent ) {		/* a real command string */
				in_percent = 0;

				switch ( template[i] ) {
				case USERNAME:
					     strcat( expanded_domain, name );
					     break;

				case HOSTNAME:
				     	     strcat( expanded_domain, machine );
					     break;

				case FULLNAME:
					     strcat( expanded_domain, machine );
					     strcat( expanded_domain, domain );
					     break;

				case NPATH:
					  if ( (expanded = find_path_to(machine, FALSE)) 
							== NULL ) {
					       dprint( 3, (debugfile,
					     	   "\nCouldn't expand system path '%s' (%s)\n\n",
					      	   machine, "domains") );
						error1( catgets(nl_fd,NL_SETN,2,"Couldn't find a path to %s!"), machine);
						sleep( 2 );
						return ( NULL );	/* failed!! */
					  }

					  strcat( expanded_domain, expanded );

					  break;

				case PPATH:

					  if ((expanded = find_path_to(path, FALSE)) == NULL) {
						dprint( 3, (debugfile,
						      "\nCouldn't expand system path '%s' (%s)\n\n",
						      path, "domains") );
						error1( catgets(nl_fd,NL_SETN,3,"I Couldn't find a path to %s!"), path);
						sleep( 2 );
						return ( NULL );	/* failed!! */
					}

					strcat( expanded_domain, expanded );

					break;

				case OBSOLETE:				/* fall through.. */
				default:
				       dprint( 1, (debugfile,
						 "\nError: Bad sequence in template file for domain '%s': %%%c\n\n",
						 domain, template[i]) );
				}

				j = strlen( expanded_domain );

			} else {
				expanded_domain[j++] = template[i];
				expanded_domain[j] = '\0';			/* null terminate */
			}
		}

		expanded_domain[j] = '\0';

	} while ( strlen(expanded_domain) < 1 );

	return ( (char *) expanded_domain );
}
