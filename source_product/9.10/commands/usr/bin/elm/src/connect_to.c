/**			connect_to.c			**/

/*
 *  @(#) $Revision: 70.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This contains the routine(s) needed to have the Elm mailer 
 *  figure out what machines the current machine can talk to. 
 *  This can be done in one of two ways - either the program 
 *  can read the L.sys file, or (if it fails or "UUNAME" define 
 *  is present) will invoke uuname to a file, then read the file
 *  in.
 */


#include "headers.h"

#ifdef NLS
# define NL_SETN   8
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


#ifndef USE_DBM
int 
get_connections()

{
	/*
	 *  get the direct connections that this machine has, by hook
	 *  or by crook (so to speak) 
	 */


#ifndef USE_UUNAME
	FILE            *lsysfile;
	char            buffer[SLEN], 
			sysname[NLEN];
	struct lsys_rec *system_record, 
			*previous_record;
	int             loc_on_line;
#endif


	if ( !warnings ) {			/* skip this - they don't care! */
		talk_to_sys = NULL;
		return;
	}


#ifndef USE_UUNAME

	previous_record = NULL;

	if ( (lsysfile = fopen(Lsys, "r")) == NULL ) {
		dprint( 1, (debugfile,
		        "Warning: Can't open L.sys file %s (read_lsys)\n", Lsys) );
#endif


		if ( read_uuname() == -1 ) {
			error( catgets(nl_fd,NL_SETN,1,"Warning: couldn't figure out system connections...") );
			talk_to_sys = NULL;
		}


#ifndef USE_UUNAME
		/*
		 *  ELSE: already read in uuname() output if we're here!! 
		 */

		return;
	}

	while ( fgets(buffer, SLEN, lsysfile) != NULL ) {
		sscanf( buffer, "%s", sysname );

		if ( previous_record == NULL ) {
			dprint( 2, (debugfile,
			        "L.sys\tdirect connection to %s, ", sysname) );
			loc_on_line = 30 + strlen( sysname );
			previous_record =
			    (struct lsys_rec *)
				my_malloc(sizeof(*talk_to_sys), "sysname");

			strcpy(previous_record->name, sysname);
			previous_record->next = NULL;
			talk_to_sys = previous_record;

		} else if ( !talk_to(sysname) && sysname[0] != '#' ) {
			if ( loc_on_line + strlen(sysname) > 80 ) {
				dprint( 2, (debugfile, "\n\t") );
				loc_on_line = 8;
			}

			dprint( 2, (debugfile, "%s, ", sysname) );
			loc_on_line += ( strlen(sysname) + 2 );
			system_record =
			    (struct lsys_rec *)
				my_malloc(sizeof(*talk_to_sys), "sysname");

			strcpy( system_record->name, sysname );
			system_record->next = NULL;
			previous_record->next = system_record;
			previous_record = system_record;
		}
	}

	fclose( lsysfile );

	if ( loc_on_line != 8 )
		dprint( 2, (debugfile, "\n") );

	dprint( 2, (debugfile, "\n") );		/* for an even nicer format... */
#endif

}
#endif /* not USE_DBM */

int
read_uuname()

{
	/*
	 *  This routine trys to use the uuname routine to get the names of
	 *  all the machines that this machine connects to...it returns
	 *  -1 on failure.
	 */


	FILE            *fd;
	int             loc_on_line;
	char		buffer[SLEN], 
			filename[SLEN];
	struct lsys_rec *system_record, 
			*previous_record;


	sprintf( filename, "%s/%s%d", tmpdir, temp_uuname, getpid() );
	sprintf( buffer, "%s > %s", uuname, filename );

	if ( system_call(buffer, SH, (char **)NULL) != 0 ) {
		dprint( 1, (debugfile, "Can't get uuname info - system() failed!\n") );
		unlink( filename );		/* insurance */
		return ( -1 );
	}

	if ( (fd = fopen(filename, "r")) == NULL ) {
		dprint( 1, (debugfile,
		        "Can't get uuname info - can't open file %s for reading\n",
			filename) );
		unlink( filename );		/* insurance */
		return ( -1 );
	}

	previous_record = NULL;

	while ( fgets(buffer, SLEN, fd) != NULL ) {
		no_ret( buffer );

		if ( previous_record == NULL ) {
			dprint( 2, (debugfile, 
				"uuname\tdirect connection to %s, ", buffer) );
			loc_on_line = 30 + strlen( buffer );
			previous_record =
			    (struct lsys_rec *)
				my_malloc(sizeof(*talk_to_sys), "sysname");

			strcpy( previous_record->name, buffer );
			previous_record->next = NULL;
			talk_to_sys = previous_record;
		} else {

			/*
			 * don't have to check uniqueness - uuname does that! 
			 */

			if ( loc_on_line + strlen(buffer) > 80 ) {
				dprint( 2, (debugfile, "\n\t") );
				loc_on_line = 8;
			}

			dprint( 2, (debugfile, "%s, ", buffer) );
			loc_on_line += ( strlen(buffer) + 2 );
			system_record =
			    (struct lsys_rec *)
				my_malloc(sizeof(*talk_to_sys), "sysname");

			strcpy( system_record->name, buffer );
			system_record->next = NULL;
			previous_record->next = system_record;
			previous_record = system_record;
		}
	}

	fclose( fd );

	(void) unlink( filename );		/* kill da temp file!! */

	dprint( 2, (debugfile, "\n") );	

	return ( 0 );			

}
