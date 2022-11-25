/**			forms.c				**/

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
 *  This set of files supports the 'forms' options to the mail system.
 * Some notes on the format of a FORM;
 *
 *	each form must have three sections.
 *
 *		[options-section]
 *		***
 *		[form-image]
 *		***
 *		[rules-section]
 *
 *	this program will ignore the first and third sections completely.  The
 *	program will assume that the user merely enteres the form-image section,
 *	and will append and prepend the triple asterisk sequences that *MUST*
 *	be part of the message.  The messages are also expected to have a 
 *	specific header - "Content-Type: mailform" - which will be added on all
 *	outbound mail and checked on inbound...
 */


#include "headers.h"

#ifdef FORMS_MODE_SUPPORTED

#ifdef NLS
# define NL_SETN  18
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS

int
check_form_file( filename )

	char           *filename;

{
	/*
	 *  This routine returns the number of fields in the specified file,
	 *  or -1 if an error is encountered. 
	 */


	FILE           *form;
	char            buffer[VERY_LONG_STRING];
	register int    field_count = 0;


	if ( (form = fopen(filename, "r")) == NULL ) {
		error2( catgets(nl_fd,NL_SETN,1,"Error %s trying to open %s to check fields!"),
		        error_name(errno), filename );
		return ( -1 );
	}

	while ( fgets(buffer, VERY_LONG_STRING, form) != NULL ) {
		field_count += occurances_of( COLON, buffer );
	}

	fclose( form );

	return ( field_count );
}


int 
format_form( filename )

	char           *filename;

{
	/*
	 *  This routine accepts a validated file that is the middle 
	 *  section of a form message and prepends and appends the
	 *  appropriate instructions.  It's pretty simple. 
	 *  This returns the number of forms in the file, or -1 on errors
	 */


	FILE            *form, 
			*newform;
	char            newfname[SLEN], 
			buffer[VERY_LONG_STRING];
	register        form_count = 0;
	int		err;


	dprint( 4, (debugfile, "Formatting form file '%s'\n", filename) );

	/*
	 *  first off, let's open the files... 
	 */

	if ( (form = fopen(filename, "r")) == NULL ) {
		error( catgets(nl_fd,NL_SETN,2,"Can't read the message to validate the form!") );
		dprint( 1, (debugfile,
			   "** Error encountered opening file \"%s\" - %s (check_form) **\n",
			   filename, error_name(errno)) );
		return ( -1 );
	}

	sprintf( newfname, "%s/%s%d", tmpdir, temp_form_file, getpid() );

	if ( (newform = fopen(newfname, "w")) == NULL ) {
		error( catgets(nl_fd,NL_SETN,3,"Couldn't open newform file for form output!") );
		dprint( 1, (debugfile,
			   "** Error encountered opening file \"%s\" - %s (check_form) **\n",
			   newfname, error_name(errno)) );
		return ( -1 );
	}

	/*
	 *  the required header... 
	 *  these are actually the defaults, but let's be sure, okay? 
	 */

	if ( fprintf(newform, "WIDTH=79\nTYPE=SIMPLE\nOUTPUT=TEXT\n***\n") == EOF ){
	        err = errno;
		error2( catgets(nl_fd,NL_SETN,4,"Failed to make temp form file %s : %s"), 
			newfname, error_description(err) );
		fclose( form );
		fclose( newform );
		unlink( newfname );
		return( -1 );
	}	

	/*
	 *  and let's have some fun transfering the stuff across... 
	 */

	while ( fgets(buffer, VERY_LONG_STRING, form) != NULL ) {
		if ( fputs(buffer, newform) == EOF ){
		        err = errno;
		        error2( catgets(nl_fd,NL_SETN,5,"Fail to make temp form file %s : %s"),
				newfname, error_description(err) );
		        fclose( form );
		        fclose( newform );
			unlink( newfname );
			return( -1 );
		}	

		form_count += occurances_of( COLON, buffer );
	}

	if ( fprintf(newform, "***\n") == EOF ){		/* that closing bit! */
	        err = errno;
	        error2( catgets(nl_fd,NL_SETN,5,"Fail to make temp form file %s : %s"),
			newfname, error_description(err) );
		fclose( form );
		fclose( newform );
		unlink( newfname );
		return( -1 );
	}	

	fclose( form );
	fclose( newform );

	if ( form_count > 0 ) {
		if ( unlink(filename) != 0 ) {
			error2( catgets(nl_fd,NL_SETN,7,"Error %s unlinking file %s"), 
				error_name(errno), filename );
			return ( -1 );
		}

		if ( link(newfname, filename) ) {
			error3( catgets(nl_fd,NL_SETN,8,"Error %s linking %s to %s"), error_name(errno),
			        newfname, filename );
			return ( -1 );
		}
	}

	if ( unlink(newfname) ) {
		error2( catgets(nl_fd,NL_SETN,7,"Error %s unlinking file %s"), 
			error_name(errno), newfname );
		return ( -1 );
	}

	return ( form_count );
}


int
mail_filled_in_form( address, subject )

	char            *address, 
			*subject;

{
	/*
	 *  This is the interesting routine.  This one will read the
	 *  message and prompt the user, line by line, for each of
	 *  the fields...returns non-zero if it needs redrawing screen
	 */


	FILE           *fd;
	register int    lines = 0, 
			count;
	char            buffer[VERY_LONG_STRING], 
			*ptr;


	dprint( 4, (debugfile,
		"replying to form with;\n\taddress=%s and\n\t subject=%s\n",
		address, subject) );

	if ( fseek(mailfile, header_table[current - 1].offset, 0) == -1 ) {
		dprint(1, (debugfile,
			   "Error: seek %ld resulted in errno %s (%s)\n",
			   header_table[current - 1].offset, error_name(errno),
			   "mail_filled_in_form") );

		error2( catgets(nl_fd,NL_SETN,10,"ELM [seek] couldn't read %d bytes into file (%s)"),
		        header_table[current - 1].offset, error_name(errno) );
		return ( 0 );
	}

	/*
	 * now we can fly along and get to the message body... 
	 */

	while ( (ptr = fgets(buffer, VERY_LONG_STRING, mailfile)) != NULL ) {
		if ( strlen(buffer) == 1 )		/* <return> only */
			break;
		else if ( real_from(buffer, (struct header_rec *)0) && lines++ > 0 ) {
			error( catgets(nl_fd,NL_SETN,11,"No form in this message!?") );
			return ( 0 );
		}
	}

	if ( ptr == NULL ) {
		error( catgets(nl_fd,NL_SETN,11,"No form in this message!?") );
		return ( 0 );
	}

	dprint( 6, (debugfile, "- past header of form message -\n") );

	/*
	 * at this point we're at the beginning of the body of the message 
	 * now we can skip to the FORM-IMAGE section by reading through a
	 * line with a triple asterisk... 
	 */

	while ( (ptr = fgets(buffer, VERY_LONG_STRING, mailfile)) != NULL ) {
		if ( equal(buffer, "***\n") )
			break;
		else if ( real_from(buffer, (struct header_rec *)0) ) {
			error( catgets(nl_fd,NL_SETN,13,"Badly constructed form.  Can't reply!") );
			return ( 0 );
		}
	}

	if ( ptr == NULL ) {
		error( catgets(nl_fd,NL_SETN,13,"Badly constructed form.  Can't reply!") );
		return ( 0 );
	}

	dprint( 6, (debugfile, "- skipped the non-forms-image stuff -\n") );

	/*
	 * one last thing - let's open the tempfile for output... 
	 */

	sprintf( buffer, "%s/%s%d", tmpdir, temp_form_file, getpid() );

	dprint( 2, (debugfile, "-- forms sending using file %s --\n", buffer) );

	if ( (fd = fopen(buffer, "w")) == NULL ) {
		error2( catgets(nl_fd,NL_SETN,15,"Can't open \"%s\" as output file! (%s)"), buffer,
		        error_name(errno) );
			
		dprint( 1, (debugfile,
		        "** Error %s encountered trying to open temp file %s;\n",
			error_name(errno), buffer) );
		return ( 0 );
	}

	/*
	 * NOW we're ready to read the form image in and start prompting... 
	 */

	if ( hp_terminal )
		softkeys_off();

	Raw( OFF );
	ClearScreen();

	while ( (ptr = fgets(buffer, VERY_LONG_STRING, mailfile)) != NULL ) {
		dprint( 9, (debugfile, "- read %s", buffer) );

		if ( equal(buffer, "***\n") )	/* end of form! */
			break;

		switch ( (count = occurances_of(COLON, buffer)) ) {
		case 0:
			printf( "%s", buffer );		/* output line */

			if ( fprintf(fd, "%s", buffer) == EOF ){
			        printf( catgets(nl_fd,NL_SETN,16,"(Failed to append to edit buffer: %s)\r\n"), 
					error_description(errno) );
				(void) sleep( 3 );
				return( 1 );
			}	
			break;

		case 1:
			if ( buffer[0] == COLON ) {
				printf( catgets(nl_fd,NL_SETN,17,"(Enter as many lines as needed, ending with a '.' by itself on a line)\n"));
				while ( gets(buffer) != NULL )
					if ( equal(buffer, ".") )
						break;
					else
						if ( fprintf(fd, "%s\n", buffer) == EOF ){
							printf(catgets(nl_fd,NL_SETN,18,"(Failed to append to edit buffer: %s)\r\n"),
							   error_description(errno));
							(void) sleep( 3 );
							return( 1 );
						}
			} else
				if ( !prompt_for_multiple_entries(buffer, fd, count) )
					return( 1 );
			break;

		default:
			if ( !prompt_for_multiple_entries(buffer, fd, count) )
				return( 1 );
		}
	}

	Raw( ON );
	fclose( fd );

	/*
	 *  let's just mail this off now... 
	 */

	mail_form( address, subject );

	return ( 1 );
}


int
prompt_for_multiple_entries( buffer, fd, entries )

	char           *buffer;
	FILE           *fd;
	int             entries;

{
	/*
	 *  Almost the same as the above routine, this one deals with lines
	 *  that have multiple colons on them.  It must first figure out how
	 *  many spaces to allocate for each field then prompts the user, 
	 *  line by line, for the entries...
	 */


	char            mybuffer[SLEN], 
			prompt[SLEN], 
			spaces[SLEN];
	int 		err;

	register int    field_size, 
			i, 
			j, 
	                tab_space,
			offset = 0, 
			line_image = 0;


	dprint( 7, (debugfile,
		   "prompt-for-multiple [%d] -entries \"%s\"\n", entries,
		   buffer) );
	
	strcpy( prompt, "No Prompt Available:" );

	while ( entries-- ) {
		j = 0;
		i = chloc( (char *) buffer + offset, COLON ) + 1;

		while ( j < i - 1 ) {
			prompt[j] = buffer[j + offset];
			j++;
		}

		prompt[j] = '\0';

		field_size = 0;
		line_image += (strlen(prompt) + 1);

		while ( whitespace(buffer[i + offset]) ) {
		        tab_space = 0;

			if ( buffer[i + offset] == TAB ) {
			        tab_space = 8 - (line_image % 8);
				field_size += tab_space;
			} else 
				field_size += 1;

			line_image += (tab_space ? tab_space : 1);
			i++;
		}

		offset += i;

		if ( field_size == 0 )		/* probably last prompt in line... */
			field_size = COLUMNS - line_image -1;

		prompt_for_sized_entry( prompt, mybuffer, field_size );

		spaces[0] = ' ';		/* always at least ONE trailing space... */
		spaces[1] = '\0';

		for ( j = strlen(mybuffer); j < field_size; j++ )
			strcat( spaces, " " );

		if ( fprintf(fd, "%s: %s%s", prompt, mybuffer, spaces) ==EOF ){
		        err = errno;
			printf( catgets(nl_fd,NL_SETN,19,"Failed to append to edit buffer: %s"), 
				error_description(err) );
			(void) sleep( 3 );
			return( 0 );
		}

		fflush( fd );
	}

	if ( fprintf(fd, "\n") == EOF ){
	        err = errno;
		printf( catgets(nl_fd,NL_SETN,19,"Failed to append to edit buffer: %s"),
			error_description(err) );
		(void) sleep( 3 );
		return( 0 );
	}

	return( 1 );
}


int
prompt_for_sized_entry( prompt, buffer, field_size )

	char            *prompt,
			*buffer;
	int             field_size;

{
	/*
	 * This routine prompts for an entry of the size specified. 
	 */


	register int    i;


	dprint( 7, (debugfile, "prompt-for-sized-entry \"%s\" %d chars\n",
		   prompt, field_size) );

	printf( "%s:", prompt );

	if ( field_size >= COLUMNS )
	        field_size = COLUMNS -1;

	for ( i = 0; i < field_size; i++ )
		(void) putchar( '_' );

	for ( i = 0; i < field_size; i++ )
		(void) putchar( BACKSPACE );

	fflush( stdout );

	gets( buffer );

	if ( strlen(buffer) > field_size )
		buffer[field_size] = '\0';
}

#endif
