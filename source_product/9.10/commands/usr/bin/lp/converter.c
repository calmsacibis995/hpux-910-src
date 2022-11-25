/*
 *  file converter ( pstatus, qstatus ) for XSPOOL 7.0
 */

#include <stdio.h>
#include <errno.h>
#include <ndir.h>
#include <sys/utsname.h>

#define OLD_UTSLEN	9
#define	DESTMAX	DIRSIZ
#define SPOOLING_VERSION 2	/* spooler version for XSPOOL */
#define PSTATUS "/usr/spool/lp/pstatus"
#define QSTATUS "/usr/spool/lp/qstatus"

/* Note: SP_MAXHOSTNAMELEN should be a multiple of 4 bytes to allow
   things to line up on specific bounderies */

#if UTSLEN > 256
#define SP_MAXHOSTNAMELEN UTSLEN + 3
#else
#define SP_MAXHOSTNAMELEN 260
#endif

#define OLD_Q_RSIZE	81
#define NEW_Q_RSIZE	164

#define OLD_P_RSIZE	81
#define NEW_P_RSIZE	164

struct old_qstat {	/* old format of qstatus file */
	char q_dest[DESTMAX+1];
	short q_accept;
	time_t q_date;
	char q_reason[OLD_Q_RSIZE];
	short q_ob3;
};

struct new_qstat {	/* new format of qstatus file */
	time_t q_date;
	short q_accept;
	short q_ob3;
	char q_reason[NEW_Q_RSIZE];
	char q_dest[DESTMAX+1];
};

struct old_pstat {	/* old format of pstatus file */
	char p_dest[DESTMAX+1];
	int p_pid;
	char p_rdest[DESTMAX+1];
	int p_seqno;
	time_t p_date;
	char p_reason[OLD_P_RSIZE];
	short p_flags;
	char p_remotedest[OLD_UTSLEN];
	char p_remoteprinter[DESTMAX+1];
	char p_host[OLD_UTSLEN];
	short p_remob3;
	short p_rflags;
};

struct new_pstat{	/* new format of pstatus file */
	long p_version;	
	int p_pid;
	int p_seqno;
	time_t p_date;
	short p_flags;
	short p_remob3;
	short p_rflags;
	short p_fence;
	short p_default;
	char p_reason[NEW_P_RSIZE];
	char p_remotedest[SP_MAXHOSTNAMELEN];
	char p_host[SP_MAXHOSTNAMELEN];
	char p_dest[DESTMAX+1];
	char p_rdest[DESTMAX+1];
	char p_remoteprinter[DESTMAX+1];
};


/* NotNewVersion -- check the version of spooler */
/*		return 1 -- old spooler		 */
/*		return 0 -- new spooler		 */
int
NotNewVersion()
{
	FILE	*fp;
	long	version;

	if ( (fp=fopen( PSTATUS, "r" )) == NULL ){
		perror("fopen");
		exit(1);
	}

	if( fread( &version, sizeof( long ), 1, fp ) != 1 ){
		perror("fread");
		exit(1);
	}

	fclose(fp);
	if( version != SPOOLING_VERSION )
		return(1);
	else
		return(0);
}

/* ConvertPstatus -- pstatus file format conversion */
void
ConvertPstatus( new_pfile )
char	*new_pfile;
{
	FILE	*fp_old, *fp_new;
	struct old_pstat Old_pstat;
	struct new_pstat New_pstat;

	if ( (fp_old=fopen( PSTATUS, "r")) == NULL ){
		perror("fopen");
		exit(1);
	}

	if ( (fp_new=fopen( new_pfile, "r+")) == NULL ){
		perror("fopen");
		exit(1);
	}

	while( fread( &Old_pstat, sizeof( struct old_pstat ), 1, fp_old)== 1 ){
		New_pstat.p_version = SPOOLING_VERSION;
		New_pstat.p_pid = 0;
		New_pstat.p_seqno = 0;
		New_pstat.p_date = Old_pstat.p_date;
		New_pstat.p_flags = Old_pstat.p_flags;
		New_pstat.p_remob3 = Old_pstat.p_remob3;
		New_pstat.p_rflags = Old_pstat.p_rflags;
		New_pstat.p_fence = 0;
		New_pstat.p_default = 0;
		strncpy(New_pstat.p_reason, Old_pstat.p_reason, NEW_P_RSIZE);
		strncpy(New_pstat.p_reason, Old_pstat.p_reason, NEW_P_RSIZE);
		strncpy(New_pstat.p_remotedest, Old_pstat.p_remotedest, SP_MAXHOSTNAMELEN);
		strncpy(New_pstat.p_host,
			Old_pstat.p_host, SP_MAXHOSTNAMELEN);
		strncpy(New_pstat.p_dest, Old_pstat.p_dest, DESTMAX+1);
		strncpy(New_pstat.p_rdest, Old_pstat.p_rdest, DESTMAX+1);
		strncpy(New_pstat.p_remoteprinter,
			Old_pstat.p_remoteprinter, DESTMAX+1);
		
		fwrite( &New_pstat, sizeof( struct new_pstat ), 1, fp_new);
	}

	fclose(fp_new);	fclose(fp_old);
}

/* ConvertQstatus -- qstatus file format conversion */
void
ConvertQstatus( new_qfile )
char	*new_qfile;
{
	FILE	*fp_old, *fp_new;
	struct old_qstat Old_qstat;
	struct new_qstat New_qstat;

	if ( (fp_old=fopen( QSTATUS, "r")) == NULL ){
		perror("fopen");
		exit(1);
	}

	if ( (fp_new=fopen( new_qfile, "r+")) == NULL ){
		perror("fopen");
		exit(1);
	}

	while( fread( &Old_qstat, sizeof( struct old_qstat ), 1, fp_old)== 1 ){
		New_qstat.q_date = Old_qstat.q_date;
		New_qstat.q_accept = Old_qstat.q_accept;
		New_qstat.q_ob3 = Old_qstat.q_ob3;
		strncpy(New_qstat.q_reason, Old_qstat.q_reason, NEW_Q_RSIZE);
		strncpy(New_qstat.q_dest, Old_qstat.q_dest, DESTMAX+1);
		
		fwrite( &New_qstat, sizeof( struct new_qstat ), 1, fp_new);
	}

	fclose(fp_new);	fclose(fp_old);
}


void
main( argc, argv )
int	argc;
char	*argv[];
{
	if( NotNewVersion() ) {
		ConvertPstatus( argv[1] );
		ConvertQstatus( argv[2] );
		fprintf(stderr, "Conversion complete\n");
	} else {
		fprintf(stderr
			,"The %s file format is for XSPOOL, not converted.\n"
			, argv[1] );
	}
}
