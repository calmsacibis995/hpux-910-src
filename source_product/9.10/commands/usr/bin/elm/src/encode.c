/**			encode.c			**/

/*
 *  @(#) $Revision: 64.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  Encryption is not supported in this version of HP-UX because of 
 *  inhibition of exporting 'crypt' command by U.S. government.
 *  In this source, the function 'makekey()' calls 'crypt(1)' 
 *  in 'crypt()' function.
 */


#include "headers.h"


#ifdef ENCRYPTION_SUPPORTED


#define RS	94
#define RN	4
#define RMASK	0x7fff		/* use only 15 bits */

static char     r[RS][RN];	/* rotors */
static char     ir[RS][RN];	/* inverse rotors */
static char     h[RS];		/* half rotor */
static char     s[RS];		/* shuffle vector */
static int      p[RN];		/* rotor indices */

static char     the_key[SLEN];	/* unencrypted key */
static char    *encrypted_key;	/* encrypted key   */

char		*getpass(), 
		*strncpy(), 
		*strcpy();

unsigned long   sleep();


int 
getkey( send )

	int             send;

{
	/*
	 *  this routine prompts for and returns an encode/decode
	 *  key for use in the rest of the program.  If send == 1
	 *  then need to mess with rawmode. 
	 */


	char            buffer[NLEN];
	int             gotkey = 0, 
			x, 
			y;


	GetXYLocation( &x, &y );

	/*
	 *  ClearLine(21);
  	 *
	 *  if (send) Raw(OFF);
	 */

	while ( !gotkey ) {
		MoveCursor( LINES - 2, 0 );
		CleartoEOS();

		if ( send )
			strcpy( buffer, getpass("Enter encryption key: ") );
		else
			strcpy( buffer, getpass("Enter decryption key: ") );

		MoveCursor( LINES - 2, 0 );

		if ( send && strcmp(buffer, getpass("Please enter it again: ")) ) {
			error( "Your keys were not the same!" );
			sleep( 1 );
			clear_error();
			continue;
		}

		strcpy( the_key, buffer );		/* save unencrypted key */
		makekey( buffer );
		gotkey = 1;
	}

	/* 
	 * if ( send ) Raw( ON );
	 */

	setup();					/** initialize the rotors etc. **/

	MoveCursor( LINES - 2 );
	CleartoEOS();

	MoveCursor( x, 0 );				/* move back to where we were..  */
}


int 
get_key_no_prompt()

{
	/*
	 *  This performs the same action as get_key, but assumes that
	 *  the current value of 'the_key' is acceptable.  This is used
	 *  when a message is encrypted twice... 
	 */


	char            buffer[SLEN];


	strcpy( buffer, the_key );

	makekey( buffer );

	setup();
}


int 
encode( line )

	char           *line;

{
	/*
	 *  encrypt or decrypt the specified line.  Uses the previously
	 *  entered key... 
	 */


	register int    i, 
			index, 
			j, 
			ph = 0;


	for ( index = 0; index < strlen(line); index++ ) {
		i = (int) line[index];

		if ( (i >= ' ') && (i < '~') ) {
			i -= ' ';

			for ( j = 0; j < RN; j++ )	/* rotor forwards */
				i = r[(i + p[j]) % RS][j];

			i = ((h[(i + ph) % RS]) - ph + RS) % RS;	/* half rotor */

			for ( j--; j >= 0; j-- )	/* rotor backwards */
				i = (ir[i][j] + RS - p[j]) % RS;

			j = 0;				/* rotate rotors */
			p[0]++;

			while ( p[j] == RS ) {
				p[j] = 0;
				j++;

				if ( j == RN )
					break;

				p[j]++;
			}

			if ( ++ph == RS )
				ph = 0;

			i += ' ';
		}

		line[index] = (char) i;			/* replace with altered one */
	}

	strcat( line, " [decoded]" );
}


int 
makekey( rkey )

	char           *rkey;

{
	/*
	 *  encrypt the key using the system routine 'crypt' 
	 */

	char            key[8], 
			salt[2], 
			*crypt();


	strncpy( key, rkey, 8 );
	salt[0] = key[0];
	salt[1] = key[1];
	encrypted_key = crypt( key, salt );
}

/*
 * shuffle rotors. shuffle each of the rotors indiscriminately.  shuffle the
 * half-rotor using a special obvious and not very tricky algorithm which is
 * not as sophisticated as the one in crypt(1).
 * After all this is done build the inverses of the rotors. 
 */


int 
setup()

{
	register long   i, 
			j, 
			k, 
			temp;
	long            seed;


	for ( j = 0; j < RN; j++ ) {
		p[j] = 0;

		for ( i = 0; i < RS; i++ )
			r[i][j] = i;
	}

	seed = 123;

	for ( i = 0; i < 13; i++ )			/* now personalize the seed */
		seed = (seed * encrypted_key[i] + i) & RMASK;

	for ( i = 0; i < RS; i++ )			/* initialize shuffle vector */
		h[i] = s[i] = i;

	for ( i = 0; i < RS; i++ ) {	 		/* shuffle the vector */
		seed = (5 * seed + encrypted_key[i % 13]) & RMASK;
		k = ((seed % 65521) & RMASK) % RS;
		temp = s[k];
		s[k] = s[i];
		s[i] = temp;
	}

	for ( i = 0; i < RS; i += 2 ) {			/* scramble the half-rotor */
		temp = h[s[i]];				/* swap rotor elements ONCE */
		h[s[i]] = h[s[i + 1]];
		h[s[i + 1]] = temp;
	}

	for ( j = 0; j < RN; j++ ) {			/* select a rotor */

		for ( i = 0; i < RS; i++ ) {		/* shuffle the vector */
			seed = (5 * seed + encrypted_key[i % 13]) & RMASK;
			k = ((seed % 65521) & RMASK) % RS;
			temp = r[i][j];
			r[i][j] = r[k][j];
			r[k][j] = temp;
		}

		for ( i = 0; i < RS; i++ )		/* create inverse rotors */
			ir[r[i][j]][j] = i;
	}
}

#endif
