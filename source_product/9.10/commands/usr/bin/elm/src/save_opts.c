/**			save_opts.c			**/

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
 *  This file contains the routine needed to allow the users to change the
 *  Elm parameters and then save the configuration in a ".elm/elmrc" file in
 *  their home directory.  With any luck this will allow them never to have
 *  to actually EDIT the file!!
 */

#include <stdio.h>

#include "headers.h"
#include "save_opts.h"

#undef onoff
#define   onoff(n)	( n == 1? "ON":"OFF" )

#define absolute(x)	( (x) < 0? -(x) : (x) )


#ifdef NLS
# define NL_SETN  36
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


FILE            *elminfo;	/* informational file as needed... */


int
save_options()

{
	/*
	 *  Save the options currently specified to a file.  This is a
	 *  fairly complex routine since it tries to put in meaningful
	 *  comments and such as it goes along.  The comments are
	 *  extracted from the file ELMRC_INFO as defined in the sysdefs.h
	 *  file.  THAT file has the format;
 	 *
	 *	varname
	 *	  <comment>
	 *	  <comment>
	 *	<blank line>
 	 *
	 *  and each comment is written ABOVE the variable to be added.  This
	 *  program also tries to make 'pretty' stuff like the alternatives
	 *  and such.
	 */


	FILE            *newelmrc;
	char            oldfname[SLEN], 
			newfname[SLEN];


	if ( strlen(home) + strlen(old_elmrcfile) < SLEN ) {
		sprintf( newfname, "%s/%s", home, elmrcfile );
		sprintf( oldfname, "%s/%s", home, old_elmrcfile );
	} else {
		error( catgets(nl_fd,NL_SETN,7,"Can't save configuration: too long pathname") );
		return;
	}

	/*
	 *  first off, let's see if they already HAVE a .elm/elmrc file 
	 */

	if ( access(newfname, ACCESS_EXISTS) != -1 ) {

		/*
		 *  YES!  Copy it to the file ".old.elmrc".. 
		 */
		if ( unlink(oldfname) < 0 )
			dprint( 1, (debugfile, "Unable to unlink %s\n", oldfname) );

		if ( link(newfname, oldfname) < 0 )
			dprint( 2, (debugfile, "Unable to link %s to %s\n",
			 	newfname, oldfname) );
		if ( unlink(newfname) < 0 )
			dprint( 1, (debugfile, "Unable to unlink %s\n", newfname) );

		(void) chown( oldfname, userid, groupid );
	}

	/*
	 *  now let's open the datafile if we can... 
	 */

	if ( (elminfo = fopen(ELMRC_INFO, "r")) == NULL )
		error1( catgets(nl_fd,NL_SETN,1,"Warning: saving without comments - can't get to %s"),
		        ELMRC_INFO );

	/*
	 *  next, open the new .elm/elmrc file... 
	 */

	if ( (newelmrc = user_fopen(newfname, "w")) == NULL ) {
		error2( catgets(nl_fd,NL_SETN,2,"Can't save configuration: can't write to %s [%s]"),
		        newfname, error_name(errno) );
		return;
	}

	save_user_options( elminfo, newelmrc );

	user_fclose( newelmrc );
	fclose( elminfo );

	error1( catgets(nl_fd,NL_SETN,3,"Options saved in file %s"), newfname );
}


int
save_user_options( elminfo_fd, newelmrc )

	FILE            *elminfo_fd, 
			*newelmrc;

{
	/*
	 *  save the information in the file.  If elminfo_fd == NULL don't look
	 *  for comments!
	 */


	if ( elminfo_fd != NULL )
		build_offset_table( elminfo_fd );

	fprintf( newelmrc,
	     "#\n# .elm/elmrc - options file for the Elm mail system\n#\n" );

	if ( strlen(full_username) > 0 )
		fprintf( newelmrc, "# Saved automatically by Elm %s for %s\n#\n\n",
			revision_id, full_username );
	else
		fprintf( newelmrc, "# Saved automatically by Elm %s\n#\n\n", revision_id );

	save_option_alternatives( ALTERNATIVES, alternative_addresses, newelmrc );
	save_option_string( CALENDAR, calendar_file, newelmrc, FALSE );
	save_option_string( EDITOR, editor, newelmrc, FALSE );
	save_option_char( ESCAPE_CHAR, escape_char, newelmrc );
	save_option_string( FULLNAME, full_username, newelmrc, FALSE );
	save_option_string( MAILBOX, mailbox, newelmrc, FALSE );
	save_option_string( MAILDIR, folders, newelmrc, FALSE );
	save_option_string( PAGER, pager, newelmrc, FALSE );
	save_option_string( PREFIX, prefixchars, newelmrc, TRUE );
	save_option_string( PRINT, printout, newelmrc, FALSE );
	save_option_string( SAVEMAIL, savefile, newelmrc, FALSE );
	save_option_string( SHELL, shell, newelmrc, FALSE );
	save_option_string( LOCALSIGNATURE, local_signature, newelmrc, FALSE );
	save_option_string( REMOTESIGNATURE, remote_signature, newelmrc, FALSE );

	save_option_sort( SORTBY, newelmrc );

	save_option_weedlist( WEEDOUT, newelmrc );

	save_option_number( TIMEOUT, (int) timeout, newelmrc );
	save_option_number( USERLEVEL, user_level, newelmrc );

	save_option_on_off( ALWAYSDELETE, always_del, newelmrc );
	save_option_on_off( ALWAYSLEAVE, always_leave, newelmrc );
	save_option_on_off( ARROW, arrow_cursor, newelmrc );
	save_option_on_off( ASK, question_me, newelmrc );
	save_option_on_off( ASKBCC, prompt_for_bcc, newelmrc );
	save_option_on_off( ASKCC, prompt_for_cc, newelmrc );
	save_option_on_off( AUTOCOPY, auto_copy, newelmrc );

#ifdef BOUNCEBACK_ENABLED
	/*
	 * In this case, you add 'bounceback' variable in file
	 * /usr/lib/elm/elmrc-info
	 */
	save_option_number( BOUNCEBACK, bounceback, newelmrc ); 
#endif

	save_option_on_off( COPY, auto_cc, newelmrc );
	save_option_on_off( EXPAND, expand_tabs, newelmrc );
	save_option_on_off( FORMS, (allow_forms != NO), newelmrc );
	save_option_on_off( KEEP, keep_empty_files, newelmrc );
	save_option_on_off( KEYPAD, hp_terminal, newelmrc );
	save_option_on_off( MENU, mini_menu, newelmrc );
	save_option_on_off( MOVEPAGE, move_when_paged, newelmrc );
	save_option_on_off( NAMES, names_only, newelmrc );
	save_option_on_off( NOHEADER, noheader, newelmrc );
	save_option_on_off( POINTNEW, point_to_new, newelmrc );
	save_option_on_off( RESOLVE, resolve_mode, newelmrc );
	save_option_on_off( SAVENAME, save_by_name, newelmrc );
	save_option_on_off( SKIPDELETED, skip_deleted, newelmrc );
	save_option_on_off( SOFTKEYS, hp_softkeys, newelmrc );
	save_option_on_off( TITLES, title_messages, newelmrc );
	save_option_on_off( WARNINGS, warnings, newelmrc );
	save_option_on_off( WEED, filter, newelmrc );

	fflush( elminfo_fd );			/* make sure we're clear... */
}


int
save_option_string( indx, value, fd, underscores )

	int             indx, 
			underscores;
	char            *value;
	FILE            *fd;

{
	/*
	 *  Save a string option to the file... only subtlety is when we
	 *  save strings with spaces in 'em - translate to underscores!
	 */


	register int    i;
	char            buffer[LONG_FILE_NAME];


	add_comment( indx, fd );

	if ( strlen(value) == 0 ) {
		fprintf( fd, "\n\n" );
		return;	
	}

	if ( (strlen(home) > 0) && (strlen(value) >= strlen(home)) && first_word(value, home) )
		sprintf( buffer, "$HOME%s", value+strlen(home) );
	else
		strcpy( buffer, value );

	if ( underscores )
		for ( i = 0; i < strlen(buffer); i++ )
			if ( buffer[i] == SPACE )
				buffer[i] = '_';

	if ( fprintf(fd, "%s = %s\n\n", save_info[indx].name, buffer) == EOF )
	        error1( catgets(nl_fd,NL_SETN,4,"Failed to save \"%s\" ..... continue"), 
			save_info[indx].name );
}


int
save_option_sort( indx, fd )

	int             indx;
	FILE            *fd;

{
	/*
	 *  save the current sorting option to a file 
	 */

	add_comment( indx, fd );

	if ( fprintf(fd, "%s = %s\n\n", save_info[indx].name,
					sort_name(SHORT)) == EOF )
	        error1( catgets(nl_fd,NL_SETN,4,"Failed to save \"%s\" ..... continue"), 
			save_info[indx].name );
}


int
save_option_char( indx, value, fd )

	int             indx, 
			value;
	FILE            *fd;

{
	/*
	 *  Save a character option to the file  
	 */

	add_comment( indx, fd );

	if ( fprintf(fd, "%s = %c\n\n", save_info[indx].name, value) == EOF )
	        error1( catgets(nl_fd,NL_SETN,4,"Failed to save \"%s\" ..... continue"), 
			save_info[indx].name );
}


int
save_option_number( indx, value, fd )

	int             indx, 
			value;
	FILE            *fd;

{
	/*
	 *  Save a binary option to the file 
	 */

	add_comment( indx, fd );

	if ( fprintf(fd, "%s = %d\n\n", save_info[indx].name, value) == EOF )
	        error1( catgets(nl_fd,NL_SETN,4,"Failed to save \"%s\" ..... continue"), 
			save_info[indx].name );
}


int
save_option_on_off( indx, value, fd )

	int             indx, 
			value;
	FILE            *fd;

{
	/*
	 *  Save a binary option to the file 
	 */

	add_comment( indx, fd );

	if ( fprintf(fd, "%s = %s\n\n", save_info[indx].name, onoff(value)) == EOF )
	        error1( catgets(nl_fd,NL_SETN,4,"Failed to save \"%s\" ..... continue"),
			save_info[indx].name );
}


int
save_option_weedlist( indx, fd )

	int             indx;
	FILE            *fd;

{
	/*
	 *  save a list of weedout headers to the file 
	 */


	int             length_so_far = 0, i;


	add_comment( indx, fd );

	if ( weedcount == 0 ){
		fprintf( fd, "\n\n" );
		return;
	}
		
	length_so_far = strlen( save_info[indx].name ) + 4;

	if ( fprintf(fd, "%s = ", save_info[indx].name) == EOF )
	        error1( catgets(nl_fd,NL_SETN,4,"Failed to save \"%s\" ..... continue"), 
			save_info[indx].name );

	for ( i=0; i<weedcount; i++ ){
		if ( strlen(weedlist[i]) + length_so_far > 72 ) {
			if ( fprintf(fd, "\n\t") == EOF )
			        error( catgets(nl_fd,NL_SETN,5,"Couldn't save \"weedlist\" completely") );
			length_so_far = 8;
		}

		if ( fprintf(fd, "\"%s\" ", weedlist[i]) == EOF )
		        error( catgets(nl_fd,NL_SETN,5,"Couldn't save \"weedlist\" completely") );
		length_so_far += ( strlen(weedlist[i]) + 3 );
	}

	fprintf( fd, "\n\n" );
}


int
save_option_alternatives( indx, list, fd )

	int             indx;
	struct addr_rec *list;
	FILE            *fd;

{
	/*
	 *  save a list of options to the file 
	 */


	int             length_so_far = 0;
	struct addr_rec *alternate;


	add_comment( indx, fd );

	if ( list == NULL ) {
		fprintf( fd, "\n\n" );
		return;				/* nothing to do! */
	}

	alternate = list;			/* don't LOSE the top!! */

	length_so_far = strlen( save_info[indx].name ) + 3;

	if ( fprintf(fd, "%s = ", save_info[indx].name) == EOF )
	        error1( catgets(nl_fd,NL_SETN,4,"Failed to save \"%s\" ..... continue"), 
				save_info[indx].name );

	while ( alternate != NULL ) {
		if ( strlen(alternate->address) + length_so_far > 72 ) {
			if ( fprintf(fd, "\n\t") == EOF )
			        error( catgets(nl_fd,NL_SETN,6,"Couldn't save \"alternatives\" completely") );
			length_so_far = 8;
		}

		if ( fprintf(fd, "%s  ", alternate->address) == EOF )
		        error( catgets(nl_fd,NL_SETN,6,"Couldn't save \"alternatives\" completely") );

		length_so_far += ( strlen(alternate->address) + 3 );
		alternate = alternate->next;
	}

	fprintf( fd, "\n\n" );
}


int
add_comment( indx, fd )

	int             indx;
	FILE            *fd;

{
	/*
	 *  get to and add the comment to the file 
	 */


	char            buffer[SLEN];


	/*
	 *  first off, add the comment from the comment file, if available 
	 */

	if ( save_info[indx].offset > 0L ) {

		if ( fseek(elminfo, save_info[indx].offset, 0) == -1 ) {
			dprint( 1, (debugfile,
			   "** error %s seeking to %ld in elm-info file!\n",
			   error_name(errno), save_info[indx].offset) );
		} else
			while ( fgets(buffer, SLEN, elminfo) != NULL ) {
				if ( buffer[0] != '#' )
					break;
				else
					fprintf( fd, "%s", buffer );
			}
	}
}


int
build_offset_table( elminfo_fd )

	FILE           *elminfo_fd;

{
	/*
	 *  read in the info file and build the table of offsets.
	 *  This is a rather laborious puppy, but at least we can
	 *  do a binary search through the array for each element and
	 *  then we have it all at once!
	 */


	char            line_buffer[SLEN];


	while ( fgets(line_buffer, SLEN, elminfo_fd) != NULL ) {
		if ( strlen(line_buffer) > 1 )
			if ( line_buffer[0] != '#' && !whitespace(line_buffer[0]) ) {
				no_ret( line_buffer );
				if ( find_and_store_loc(line_buffer, ftell(elminfo_fd)) ) {
					dprint( 1, (debugfile, 
						"** Couldn't find and store \"%s\" **\n",
						line_buffer) );
				}
			}
	}
}


int
find_and_store_loc( name, offset )

	char            *name;
	long            offset;

{
	/*
	 *  given the name and offset, find it in the table and store it 
	 */


	int             first = 0, 
			last, 
			middle, 
			compare;


	last = NUMBER_OF_SAVEABLE_OPTIONS;

	while ( first <= last ) {

		middle = ( first + last ) / 2;

		if ( (compare = strcmp(name, save_info[middle].name)) < 0 )	/* a < b */
			last = middle - 1;
		else if ( compare == 0 ) {					/* a = b */
			save_info[middle].offset = offset;
			return ( 0 );
		} else								/* a > b */
			first = middle + 1;
	}

	return ( -1 );
}
