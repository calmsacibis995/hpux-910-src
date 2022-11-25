/* $Source: /misc/source_product/9.10/commands.rcs/etc/mkboot/lifio.c,v $
 * $Revision: 66.2 $	$Author: lkc $
 * $State: Exp $   	$Locker:  $
 * $Date: 90/10/18 15:32:32 $
 */

#include <stdio.h>
#define OPEN( a, b, c )	  open( a, b )
#define CLOSE( a )        close( a )

#define L_SET 0

#include <fcntl.h>
#include "lifio.h"
#include "global.h"

#define BOF(lfp)  lfp->lifent.start
#undef EOF
#define EOF(lfp)  (lfp->lifent.start+lfp->lifent.size)

#define LIFTYPE_PURGED	((short int)0)
#define LIFTYPE_EOF	((short int)-1)

static
LIFDIR lifdir_pool[MAXLIFOPEN] = { 0 };

int lifutl_debug = 0;

void
WRITE(fd, buf, bytes)
int fd;
char *buf;
unsigned bytes;
{
    if(write(fd, buf, bytes) != bytes) {
	perror("lifio write failed");
	exit(1);
    }
}


READ(fd, buf, bytes)
int fd;
char *buf;
unsigned bytes;
{
    if(read(fd, buf, bytes) != bytes) {
	perror("lifio read failed");
	exit(1);
    }
    return(bytes);
}

LSEEK(fd, offset, whence)
int fd;
off_t offset;
int whence;
{
    extern off_t lseek();
    
    if(lseek(fd, offset, whence) == (off_t)-1) {
	perror("lifio lseek failed");
	exit(1);
    }
}

static
LIFDIR *
lif_alloc()
{
	int i;
	LIFDIR *ldp = (LIFDIR *)0;

	for( i = 0; i < MAXLIFOPEN; i++ ){
		if( !lifdir_pool[i].inuse ) {
			ldp = &lifdir_pool[i];
			ldp->inuse = 1;
			break;
		}
	}
	return(ldp);
}

static
LIFDIR *
lif_dealloc( ldp )
LIFDIR *ldp;
{
	ldp->inuse = 0;
	return( (LIFDIR *)0 );
}

static
LIFDIR *
_lopen( file, mode )
char *file;
int mode;
{
	int fd, i;
	struct lvol hdr;
	LIFDIR *ldp, *lif_alloc();

	fd = OPEN( file, mode, 0 );
	if( fd < 0 ) 
		return( (LIFDIR *)0 );

	if((ldp = lif_alloc()) == (LIFDIR *)0) {
		(void) CLOSE(fd);
		return((LIFDIR *)0);
	}

	ldp->fd = fd;

	(void) READ( fd, (char *)&hdr, (unsigned)sizeof( hdr ) );
	if( lifutl_debug ) print_hdr( &hdr ); 
	ldp->loc = 0;
	ldp->size = hdr.dsize;
	ldp->start = hdr.dstart * 0x100;

	for( i = 0; i < MAXVOLNAME; i++ ){
		ldp->lifent.fname[i] = hdr.volname[i];
		if(ldp->lifent.fname[i] == ' ' ) break;
	}
	ldp->lifent.fname[i] = '\0';

	LSEEK( fd, (off_t)ldp->start, L_SET );
	ldp->loc = ldp->start;

	return(ldp);
}

static
LIFDIR *
_lclose( ldp )
LIFDIR *ldp;
{
	LIFDIR *lif_dealloc();

	(void) CLOSE( ldp->fd );
	(void) lif_dealloc( ldp );
	return( (LIFDIR *)0 ); 
}

LIFDIR *
lif_opendir( file )
char *file;
{
	LIFDIR *_lopen();

	return(_lopen( file, O_RDONLY ));
}

lif_seekdir( dirp, loc, d )
LIFDIR *dirp;
int loc, d;
{
	/*
	 * Current problems with this routine.
	 * 1) We don't check bounds (before start or after end).
	 * 2) We don't implement seek from end.
	 */
	switch( d ) {
	case 0:
		/* Rel to beginning of directory */
		loc = loc * sizeof( struct dentry ) + dirp->start;
		break;
	case 1:
		/* Rel to current position */
		loc = loc * sizeof( struct dentry ) + dirp->loc;
		break;
	case 2: 
		/* Rel to end of directory */
		return(-1);
	}
	LSEEK( dirp->fd, (off_t)loc, L_SET );
	dirp->loc = loc;

	return( (loc - dirp->start)/sizeof(struct dentry) );
}

struct lif_dirent *
lif_readdir( dirp )
LIFDIR *dirp;
{
	int i, status;
	struct dentry lifentry;

	/* Skip over purged entry */
	do {
		status = READ(dirp->fd, (char *)&lifentry,
			      (unsigned)sizeof( lifentry )); 
		if( status < sizeof(lifentry) ) 
			return( (struct lif_dirent *)0 );
	} while( lifentry.ftype == LIFTYPE_PURGED );
	if( lifutl_debug ) print_entry( &lifentry ); 
	dirp->loc += sizeof( lifentry );

	dirp->lifent.start = lifentry.start * 0x100;
	dirp->lifent.size = lifentry.size * 0x100;
	dirp->lifent.ftype = lifentry.ftype;
	for( i = 0; i < MAXFILENAME; i++ ){
		dirp->lifent.fname[i] = lifentry.fname[i];
		if(dirp->lifent.fname[i] == ' ' ) break;
	}
	dirp->lifent.fname[i] = '\0';

	return((lifentry.ftype == LIFTYPE_EOF)? 
		(struct lif_dirent *)0: &dirp->lifent);
}

struct lif_dirent *
lif_writedir( dirp, dent )
LIFDIR *dirp;
struct lif_dirent *dent;
{
	int i, status;
	struct dentry lifentry;

	/* Find purged entry */
	do {
		status = READ(dirp->fd, (char *)&lifentry,
			      (unsigned)sizeof( lifentry )); 
		if( status < sizeof(lifentry) ) 
			return( (struct lif_dirent *)0 );
	} while((lifentry.ftype != LIFTYPE_PURGED) &&
		(lifentry.ftype != LIFTYPE_EOF));

	if( lifentry.ftype == LIFTYPE_PURGED ) { 
		lifentry.start = dent->start / 0x100;
		lifentry.size = dent->size / 0x100;
		lifentry.ftype = dent->ftype;

		for( i = 0; i < MAXFILENAME; i++ )
			lifentry.fname[i] = ' ';

		for( i = 0; i < MAXFILENAME; i++ ){
			if( dent->fname[i] )
				lifentry.fname[i] = dent->fname[i];
			else 
				break;
		}
	}

	WRITE( dirp->fd, (char *)&lifentry, (unsigned)sizeof(lifentry) );
	return(dent);

}

static int
translate_mode( cmode )
char *cmode;
{
	int flags = 0;

	while( *cmode != '\0' ) {
		switch(*cmode){
		case 'r':
			flags |= 4;
			break;
		case 'w':
			flags |= 2;
			break;
		default:
			(void) fprintf(stderr, "lif_open failed ");
			(void) fprintf(stderr, "unknown open flag\n");
			exit(1);
			break;
		}
		cmode++;
	}
	switch( flags ){
	case	2:
		flags = O_WRONLY;
		break;
	case	4:
		flags = O_RDONLY;
		break;
	case	6:
		flags = O_RDWR;
		break;
	}

	return(flags);
}

LIFFILE *
lif_fopen( file, mode )
char *file;
char *mode;
{
	char path[256], *f;
	LIFDIR *ldp, *_lopen();
	struct lif_dirent *lep, *lif_readdir();
	int open_flags;

	(void) strncpy( path, file, sizeof(path) - 1 );
	path[sizeof(path) - 1] = '\0';

	for(f = path; *f != '\0'; f++){
		if( *f == ':' ) { 
			*f++ = '\0';
			break; 
		}
	}

	/* p points to path, f points to file */

	open_flags = translate_mode( mode );
	ldp = _lopen( path, open_flags );
	if( ldp == (LIFDIR *)0 )
		return( (LIFFILE *)0 );

	do {
		lep = lif_readdir( ldp );
	} while( lep && strcmp(lep->fname, f) );
	if( lep == (struct lif_dirent *)0 ) 
		return( (LIFFILE *)0 );

	/* Found it */
	LSEEK(ldp->fd, (off_t)lep->start, L_SET);
	ldp->loc = lep->start;
	
	return( (LIFFILE *)ldp );
}

lif_fclose( lfp )
LIFFILE *lfp;
{
	LIFDIR *_lclose();

	(void) _lclose( (LIFDIR *)lfp );
	return( 0 ); 
}

lif_fread( lfp, buf, n )
LIFFILE *lfp;
char *buf;
int n;
{
	int resid = lfp->lifent.size -
			(lfp->loc - lfp->lifent.start);

	/* Check End of file */
	if( resid <= 0 )
		return( 0 );

	/* Dont overflow file */
	if( n > resid ) 
		n = resid;

	(void) READ( lfp->fd, buf, (unsigned)n );
	lfp->loc += n;
	return(n);
}

lif_fwrite( lfp, buf, n )
LIFFILE *lfp;
char *buf;
int n;
{
	int resid = lfp->lifent.size -
			(lfp->loc - lfp->lifent.start);

	/* Check End of file */
	if( resid <= 0 )
		return( 0 );

	/* Dont overflow file */
	if( n > resid ) 
		n = resid;

	WRITE( lfp->fd, buf, (unsigned)n );
	lfp->loc += n;
	return(n);
}

lif_fseek( lfp, loc, d )
LIFFILE *lfp;
int loc, d;
{
	int nloc = BOF(lfp);

	switch( d ) {
	case 0:
		nloc = BOF(lfp) + loc;
		break;
	case 1:
		nloc = lfp->loc + loc;
		break;
	case 2: 
		nloc = EOF(lfp) - loc;
		break;
	}
	if( nloc >= EOF(lfp) )
		nloc = EOF(lfp);
	if( nloc <= BOF(lfp) )
		nloc = BOF(lfp);

	LSEEK( lfp->fd, (off_t)nloc, L_SET );
	lfp->loc = nloc;
	
	return( nloc - BOF(lfp) ); 
}

#define PRINT(fmt,var) (void) printf(" var = fmt \n", var )
print_hdr( hdr )
struct lvol *hdr;
{
	PRINT(%d, hdr->discid);
	PRINT(%s, hdr->volname);
	PRINT(%x, hdr->dstart );
	PRINT(%d, hdr->dsize);
	PRINT(%d, hdr->version);
	PRINT(%d, hdr->tps);
	PRINT(%d, hdr->spm);
	PRINT(%d, hdr->spt);
	PRINT(%d, hdr->iplstart); PRINT(%x, hdr->iplstart);
	PRINT(%d, hdr->ipllength);
	PRINT(%d, hdr->iplentry);			
}

print_entry( entry )
struct dentry *entry;
{
	PRINT(%s, entry->fname);
	PRINT(%d, entry->start); PRINT(%x, entry->start);
	PRINT(%d, entry->size);
	/*
	char fname[MAXFILENAME];	* Name of file *
	short ftype;			* Type of file or EOD (end-of-dir) *
	int start;			* Starting sector *
	int size;			* Size in sectors *
	char date[6];			* Should be date_fmt, but on
					   some machines (series 500)
					   date_fmt gets padded to 8 bytes *
	short lastvolnumber;		* Both lastvolume flag & volume # *
	int extension;			* Bizarre user-defined field *
	*/
}
