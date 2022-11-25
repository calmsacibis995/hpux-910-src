/* $Source: /source/hpux_source/networking/rcs/arpa90_800/sendmail/src/RCS/mimefy.c,v $
 * $Revision: 1.2.109.5 $	$Author: mike $
 * $State: Exp $   	$Locker:  $
 * $Date: 95/02/21 16:08:12 $
 */

/*
 * Copyright (c) 1993, 1994 Hewlett-Packard Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * Hewlett-Packard Company'' in the documentation or other materials provided
 * with the distribution and in all advertising materials mentioning features
 * or use of this software.  The Hewlett-Packard name may not be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.  
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * In no event shall Hewlett-Packard be liable for direct, indirect, special,
 * incidental or consequential damages (including lost profits) related to
 * this software, whether based on contract, tort or any other legal theory.
 */

# ifndef lint
static char rcsid[] = "$Header: mimefy.c,v 1.2.109.5 95/02/21 16:08:12 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)mimefy.c	1.1 (Hewlett-Packard) Jan 20, 1994";
# 	endif /* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_3997="@(#) PATCH_9.03: mimefy.o $Revision: 1.2.109.5 $ 94/03/24 PHNE_3997";
# endif	/* PATCH_STRING */

# include <stdio.h>
# include "sendmail.h"

static bool first_time = TRUE;

# define STRING_MATCH	0

# ifndef MIN
# 	define	MIN(x, y)	((x < y) ? x : y)
# endif /* MIN */

/*
 * If the line to search contains "--" plus the boundary to match, it is a
 * boundary.  If is contains a final "--", it is a final boundary.
 */
# define NO_BOUNDARY	1
# define NORMAL_BOUNDARY	2
# define FINAL_BOUNDARY	3

boundary_match(line_to_search, boundary_to_match)
	char *line_to_search, *boundary_to_match;
{
	register int boundary_len = strlen(boundary_to_match);

# ifdef DEBUG
	if (tTd(63, 3))
		printf("boundary_match(%s)\n", boundary_to_match);
# endif /* DEBUG */
	if ((strncmp(line_to_search, "--", 2) == STRING_MATCH) &&
	    (strncmp(line_to_search+2, boundary_to_match,
		     MIN(strlen(line_to_search+2), boundary_len)) == STRING_MATCH)){
		if (strncmp(line_to_search+2+boundary_len, "--", 2) == STRING_MATCH){
# ifdef DEBUG
			if (tTd(63, 3))
				printf("Final boundary %s found.\n", boundary_to_match);
# endif /* DEBUG */
			return(FINAL_BOUNDARY);
		}
# ifdef DEBUG
		if (tTd(63, 3))
			printf("Normal boundary %s found.\n", boundary_to_match);
# endif /* DEBUG */
		return(NORMAL_BOUNDARY);
	}
# ifdef DEBUG
	if (tTd(63, 3))
		printf("No boundary found.\n");
# endif /* DEBUG */
	return(NO_BOUNDARY);
}

char *
get_next_line(in_file, buf)
	FILE *in_file;
	char *buf;
{
	char *line;
	register int line_len;

	/*
	 * Read in a line, up to MAXLINE bytes, and kill the trailing newline.
	 */
	line = fgets(buf, MAXLINE, in_file);
	line_len = strlen(line) - 1;
	if ((line_len >= 0) && (line[line_len] == '\n'))
		line[line_len--] = '\0';
	if ((line_len >= 0) && (line[line_len] == '\r'))
		line[line_len--] = '\0';
	return(line);
}

# define get_new_tmp_file(x, y)	y = tempnam(NULL, "sndml");\
   x = fopen(y, "w");

/*
 * Flush the data in the temp file to the out-file.
 * Use the num_high_bytes/num_bytes ratio to determine whether to use
 * base64 or quoted-printable.  If there are no high-bytes, it must be
 * an already encoded part of a multipart: just output the data.
 */
flush_tmp_file(file, out_file, name, num_bytes, num_high_bytes, e)
	FILE *file, *out_file;
	char *name;
	int *num_bytes, *num_high_bytes;
	ENVELOPE *e;
{
	char buf[MAXLINE], *line;
	FILE *tmp_in_file;

	if (*num_bytes > 0){
		fclose(file);
		tmp_in_file = fopen(name, "r");
		if (*num_high_bytes){
			if ((*num_high_bytes * 9) > *num_bytes){
				if (first_time){
					first_time = FALSE;
# ifdef LOG
					if (LogLevel > 8)
						syslog(LOG_INFO, "%s: MIMEfied into base64", e->e_id);
# endif /* LOG */
				}
				fprintf(out_file, "Content-Transfer-Encoding: base64\n\n");
				to64(tmp_in_file, out_file, 0);
			} else {
				if (first_time){
					first_time = FALSE;
# ifdef LOG
					if (LogLevel > 8)
						syslog(LOG_INFO, "%s: MIMEfied into quoted-printable", e->e_id);
# endif /* LOG */
				}
				fprintf(out_file, "Content-Transfer-Encoding: quoted-printable\n\n");
				toqp(tmp_in_file, out_file);
			}
		} else {
			/*
			 * First print the header/body separator.
			 */
			fprintf(out_file, "\n");
			while (!feof(tmp_in_file)){
				line = get_next_line(tmp_in_file, buf);
				if (!feof(tmp_in_file))
					fprintf(out_file, "%s\n", line);
			}
		}
		*num_bytes = 0;
		*num_high_bytes = 0;
		fclose(tmp_in_file);
	}
	unlink(name);
	free(name);
}

/*
 * These are used to keep track of the state by parse_multipart().
 * The mode is initially HEADER_MODE for the outer part, but PRE_MODE
 * for inner parts.  This is because inner parts can have data before
 * the initial boundary, typically MIME warning information.
 */
# define HEADER_MODE	1
# define	BODY_MODE	2
# define PRE_MODE	3

/*
 *  parse_multipart -- make the following conversion:
 *      [MIME headers]             MIME headers indicating new encoding
 *      <blank line>      -->      <blank line>
 *      body                       encoded body
 *
 *	Parameters:
 *		search_boundary -- the boundary to search for; if NULL, this is
 *			the outer part.
 *		in_file -- fd of intermediate file
 *		out_file -- fd of output file
 *		e -- the envelope for this transaction.
 *
 *	Returns:
 *		none
 *
 *	Side Effects:
 *		Will encode the body, and set the headers appropriately.
 */
void
parse_multipart(search_boundary, in_file, out_file, e)
	char *search_boundary;
	FILE *in_file, *out_file;
	ENVELOPE *e;
{
	char buf[MAXLINE], keep_boundary[80],
	     *t, *line, *u, *ch, *tmp_boundary, *tmp_file_name;
	int boundary_len, match_result;
	int mode = HEADER_MODE,
	    tmp_bytes_output = 0,
	    tmp_high_bytes_output = 0;
	bool multi_part = FALSE, rfc_822 = FALSE;
	FILE *tmp_file;

# ifdef DEBUG
	if (tTd(63, 2))
		printf("parse_multipart(%s).\n", search_boundary);
# endif /* DEBUG */
	get_new_tmp_file(tmp_file, tmp_file_name);
	if (search_boundary)
		mode = PRE_MODE;

	while(!feof(in_file)){
		line = get_next_line(in_file, buf);

		if (search_boundary){
			match_result = boundary_match(line, search_boundary);
			if (match_result == FINAL_BOUNDARY){
				/*
				 * Final boundary: flush the temp file, put the
				 * boundary line, and pop up one level.
				 */
				flush_tmp_file(tmp_file, out_file, tmp_file_name,
					       &tmp_bytes_output, &tmp_high_bytes_output, e);
				fprintf(out_file, "%s\n", line);
				return;
			} else if (match_result == NORMAL_BOUNDARY) {
				/*
				 * Normal boundary: go into header-mode for
				 * the next part, flush the temp file, put the
				 * boundary, get a new temp file, and continue.
				 */
				mode = HEADER_MODE;
				flush_tmp_file(tmp_file, out_file, tmp_file_name,
					       &tmp_bytes_output, &tmp_high_bytes_output, e);
				fprintf(out_file, "%s\n", line);
				get_new_tmp_file(tmp_file, tmp_file_name);
				continue;
			}
		}

		if ((mode == HEADER_MODE) && (strncasecmp(line, "content-type:", 13) == STRING_MATCH)){
			/*
			 * We're in header-mode and we found a content-type:
			 * header.  Look for continuation lines; then determine
			 * if it is a multipart.
			 */
			t = line + 13;
			while (*t && isspace(*t))
				++t;
			if (*t == '\0'){
				fprintf(out_file, "%s\n", line);
				line = get_next_line(in_file, buf);
				t = line;
				while (*t && isspace(*t))
					++t;
			}
			if (strncasecmp(t, "multipart", 9) == STRING_MATCH){
				u = t + 9;
				while (*u && isspace(*u))
					++u;
				if (*u == '\0'){
					fprintf(out_file, "%s\n", line);
					line = get_next_line(in_file, buf);
					u = line;
					while (*u && isspace(*u))
						++u;
				}
				tmp_boundary = strstr(u, "boundary");
				if (!tmp_boundary){
					fprintf(out_file, "%s\n", line);
					line = get_next_line(in_file, buf);
					u = line;
					while (*u && isspace(*u))
						++u;
					tmp_boundary = strstr(u, "boundary");
				}
				if (tmp_boundary){
					t = tmp_boundary + 8;
					while (*t && isspace(*t))
						++t;
					if ((*t) == '=')
						tmp_boundary = ++t;
					strcpy(keep_boundary, tmp_boundary);
					boundary_len = strlen(keep_boundary) - 1;
					if ((keep_boundary[0] == '"') &&
					    (keep_boundary[boundary_len] == '"')){
						keep_boundary[boundary_len] = '\0';
						tmp_boundary = &keep_boundary[1];
						strcpy(keep_boundary, tmp_boundary);
					}
				}
				multi_part = TRUE;
				fprintf(out_file, "%s\n", line);
				continue;
			}
			if (strncasecmp(t, "message", 7) == STRING_MATCH){
				u = t + 7;
				while (*u && isspace(*u))
					++u;
				if (strstr(u, "rfc822") != NULL)
					rfc_822 = TRUE;
				fprintf(out_file, "%s\n", line);
				continue;
			}
		}

		/*
		 * Header-mode and we found a C-T-E: header.  Look for continuation
		 * lines, and determine if we need to rewrite this header.
		 */
		if ((mode == HEADER_MODE) &&
		    (strncasecmp(line, "content-transfer-encoding:", 26) == STRING_MATCH)){
			char backup_line[MAXLINE];
		  
			backup_line[0] = '\0';
			t = line + 26;
			while (*t && isspace(*t))
				++t;
			if (*t == '\0'){
				strcpy(backup_line, line);
				line = get_next_line(in_file, buf);
				t = line;
				while (*t && isspace(*t))
					++t;
			}
			/*
			 * Any encoding other than the following 3 will have
			 * to be discarded, since it will be replaced with
			 * the appropriate one later.
			 */
			if ((strncasecmp(t, "base64", 6) == STRING_MATCH) ||
			    (strncasecmp(t, "quoted-printable", 16) == STRING_MATCH) ||
			    (strncasecmp(t, "7bit", 4) == STRING_MATCH)){
			  
				if (backup_line[0] != '\0')
					fprintf(out_file, "%s %s\n", backup_line, t);
				else
					fprintf(out_file, "%s\n", line);
				continue;
			} else
				continue;
		}
      
		/*
		 * A blank line: it could be a header/body separator, or EOF,
		 * or just a blank line in the body.
		 */
		if (line[0] == '\0'){
			if (!feof(in_file)){
				if (mode == HEADER_MODE){
					if (rfc_822){
						rfc_822 = FALSE;
						fprintf(out_file, "\n");
					} else {
						/*
						 * Don't print this header/body
						 * separator now; flush_tmp_file()
						 * will take care of it.
						 */
						mode = BODY_MODE;
					}
				} else if (mode == PRE_MODE)
					fprintf(out_file, "\n");
				else {
					fprintf(tmp_file, "\n");
					tmp_bytes_output++;
				}
				if (multi_part){
					/*
					 * In this case we must print the separator.
					 */
					if (!search_boundary){
						first_time = FALSE;
# ifdef LOG
						if (LogLevel > 8)
							syslog(LOG_INFO, "%s: MIMEfied into 7bit", e->e_id);
# endif /* LOG */
						fprintf(out_file, "Content-Transfer-Encoding: 7bit\n");
					}
					fprintf(out_file, "\n");
					parse_multipart(keep_boundary, in_file, out_file, e);
					multi_part = FALSE;
				}
			}
			continue;
		}

		/*
		 * Past all the BS: just output the line to the right file.
		 */
		if (!feof(in_file)){
			if ((mode == HEADER_MODE) || (mode == PRE_MODE))
				fprintf(out_file, "%s\n", line);
			else {
				fprintf(tmp_file, "%s\n", line);
				tmp_bytes_output += (strlen(line) + 1);
				for(ch=line; *ch; ++ch)
					if (*ch & 0x80)
						tmp_high_bytes_output++;
			}
		}
		/*
		 * this is the end of the while(!feof()) loop.
		 */
	}
	/*
	 * We're done: flush the temp file.
	 */
	flush_tmp_file(tmp_file, out_file, tmp_file_name, &tmp_bytes_output,
		       &tmp_high_bytes_output, e);
}

/*
 *  intermediate_to_out -- make the following conversion:
 *      headers
 *      <blank line>      -->      encoded body
 *      encoded body
 *  The headers will be taken care of by chompheader(); the blank line
 *  will no longer be needed.
 *
 *	Parameters:
 *		in_file -- fd of intermediate file
 *		out_file -- fd of output file
 *		e -- the envelope for this transaction.
 *
 *	Returns:
 *		none
 *
 *	Side Effects:
 *		May add some new headers.
 */
void
intermediate_to_out(in_file, out_file, e)
	FILE *in_file, *out_file;
	ENVELOPE *e;
{
	char *line, buf[MAXLINE], prev_buf[MAXLINE];
	char *prev_line = NULL;
	bool past_break = FALSE;

# ifdef DEBUG
	if (tTd(63, 2))
		printf("intermediate_to_out().\n");
# endif /* DEBUG */
	prev_buf[0] = '\0';
	while(!feof(in_file)){
		int line_count = 0;
		
		line = get_next_line(in_file, buf);

		/*
		 * If past the header/body break, just output the line.
		 */
		if (past_break){
			if (!feof(in_file))
				fprintf(out_file, "%s\n", line);
			continue;
		}

		/*
		 * If this is the break, process any saved header, and note that
		 * we're past the break.  We don't want to save the blank line!
		 */
		if (strlen(line) == 0){
			if (prev_line)
				chompheader(prev_line, FALSE, e);
			past_break = TRUE;
			continue;
		}

		/*
		 * If this line start with white-space, tack it onto the
		 * previous line, then keep going to see if there are more
		 * continuations to this header.
		 */
		if (isspace(*line)){
			prev_line = strcat(prev_buf, line);
			continue;
		}

		/*
		 * This line does not start with white-space, so it must be a
		 * new header.  Process any previous one, then save this one
		 * in case it is continued.
		 */
		if (prev_line)
			chompheader(prev_line, FALSE, e);
		if (strncasecmp(line, "content-transfer-encoding:", 26) == 0){
			strcpy(prev_buf, line);
			prev_line = prev_buf;
		}
	}
}

/*
 *  mimefy -- encode the input file and process headers for the encoded
 *	      output file.
 *
 *	Parameters:
 *		in_file_name -- name of the input, un-encoded file
 *		out_file_name -- name of the output, encoded file
 *		e -- the envelope for this transaction.
 *
 *	Returns:
 *		0 for success, 1 for failure
 *
 *	Side Effects:
 *		May add some new headers.
 */

mimefy(in_file_name, out_file_name, e)
	char *in_file_name, *out_file_name;
	ENVELOPE *e;
{
	FILE *in_file, *intermediate_file, *out_file;
	char *intermediate_file_name;

# ifdef DEBUG
	if (tTd(63, 1))
		printf("mimefy(%s, %s).\n", in_file_name, out_file_name);
# endif /* DEBUG */
	in_file = fopen(in_file_name, "r");
	if (in_file == NULL)
		return(1);
	get_new_tmp_file(intermediate_file, intermediate_file_name);
	if (intermediate_file == NULL){
		unlink(intermediate_file_name);
		return(1);
	}
	parse_multipart(NULL, in_file, intermediate_file, e);
	fclose(in_file);
	fclose(intermediate_file);

	/*
	 * parse_multipart() encoded the data, but left some headers which
	 * need to be processed.  intermediate_to_out() will handle that.
	 */
	intermediate_file = fopen(intermediate_file_name, "r");
	if (intermediate_file == NULL){
		unlink(intermediate_file_name);
		return(1);
	}
	out_file = fopen(out_file_name, "w");
	if (out_file == NULL){
		fclose(intermediate_file);
		unlink(intermediate_file_name);
		return(1);
	}
	intermediate_to_out(intermediate_file, out_file, e);
	fclose(intermediate_file);
	unlink(intermediate_file_name);
	free(intermediate_file_name);
	fclose(out_file);
	return(0);
}
