static char *HPUX_ID = "@(#) $Revision: 64.13 $";
/****************************************************************************

       audusr - selectively start or stop auditing of users

       SYNOPSIS

            audusr [ [ -a user ] ... ] [ [ -d user ] ... ] [ -A | -D ]


       DESCRIPTION

            Audusr selectively starts or stops recording of the
            auditing information for a given user or a list of
            users.  Without arguments, audusr displays the usage of
            the command.  This command is restricted to users with
            appropriate privilege.


              -a        Audit the indicated users when they execute
                        any event specified in audevent(AMIN)

              -d        Do not audit the indicated users when they
                        execute any event specified in audevent(ADMIN).

              -A        Audit all users executing the events
                        specified in audevent(ADMIN).  The -A and
                        -D options are mutually exclusive; they
                        cannot both be specified.  Furthermore, if
                        -A option is specified, -d option cannot be
                        specified.

              -D        Exclude from auditing all users executing
                        the events specified in audevent(ADMIN).
                        If -D option is specified, -a option cannot
                        be specified.

            No options take effect for a user currently on the system
            until the user logs off and then logs in again.  However,
            any new users who log in after the specified option is 
            invoked are audited or excluded from auditing accordingly.



        EXIT CODES:

        0  - complete successfully.
        1  - wrong usage or option.
        2  - not previleged user.
        3  - cannot suspend auditing.
        4  - input user does not exist.
        5  - cannot lock /.scure/etc/passwd because /.secure/etc/ptmp
                is already being used.
        6  - cannot stat or read /.scure/etc/passwd.
        7  - cannot write any entry to /.secure/etc/ptmp.
        8  - input number of users are more than allowed.

***************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/audit.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pwd.h>
#include <errno.h>
#include "aud_def.h"

#ifdef NLS
#include <nl_types.h>
#define NL_SETN	1
#include "nl_aud_err.h"
#include "nl_aud_msg.h"
#else
#include "aud_err.h"
#include "aud_msg.h"
#endif

#ifdef NLS
#define AUD_EXIT(r)		catclose(catd); exit(r)
#else
#define AUD_EXIT(r)		( exit(r) )
#endif

main(argc, argv)
int argc;
char *argv[];

{
        int c;
        extern char *optarg;
        extern int optind;

	extern int errno;

        int i=0, n=0, u_err=0, aflg=0, dflg=0, Aflg=0, Dflg=0;
        char *a_usr[USR_SZ], *d_usr[USR_SZ], *u_ptr, usr_name[9];
        FILE *fp, *ft;
        int fd;
        struct s_passwd *pwd_ent;
        struct self_audit_rec audrec;
        char *audbody= audrec.aud_body.text;
        struct timeval *tp;
        struct timezone *tzp;
        struct stat buf;
#ifdef NLS
	nl_catd catd;
#endif


#ifdef NLS

        /* open message catalog */
        catd=catopen("audsysusr",0);

#endif


        /* Verify the user is privileged and do self auditing */

        if (audswitch(AUD_SUSPEND) == -1) {
		fprintf(stderr,E_SUSER);
                AUD_EXIT(2);
        }


        /* get arguments */

        while ((c=getopt(argc, argv, "a:d:AD")) != EOF)
                switch (c) {
                case 'a':
                        aflg++;
                        if (n >= USR_SZ) {
                            fprintf (stderr, E_USR_SZ);
                            AUD_EXIT (8);
                        }
                        a_usr[n++]=optarg;
                        break;
                case 'd':
                        dflg++;
                        if (i >= USR_SZ) {
                            fprintf (stderr, E_USR_SZ);
                            AUD_EXIT (8);
                        }
                        d_usr[i++]=optarg;
                        break;
                case 'A':
                        Aflg++;
                        break;
                case 'D':
                        Dflg++;
                        break;
                default:
                        fprintf (stderr, E_AUDUSR_USE);
                        AUD_EXIT(1);
                }

        /* Validate the input options */

        if (Aflg && Dflg) {
                fprintf (stderr,E_ADFLG);
                AUD_EXIT(1);
        }

        if ((!Aflg) && (!Dflg) && (!aflg) && (!dflg)) {
                fprintf (stdout,E_AUDUSR_USE);
                AUD_EXIT(1);
        }

        if (Aflg && dflg) {
                fprintf (stderr,E_AdFLG);
                AUD_EXIT(1);
        }

        if (Dflg && aflg) {
                fprintf (stderr,E_aDFLG);
                AUD_EXIT(1);
        }

        if (Aflg && aflg) 
                aflg = 0;

        if (Dflg && dflg) 
                dflg = 0;


        if (aflg) {

                /* set the last entry to null pointer in the table
                   of users to be added */

                a_usr[n]=0;

                /* check the existence of input users */

                setspwent();
                for (n=0; a_usr[n]; n++) {
                        if ((pwd_ent = getspwnam(a_usr[n])) == NULL) {
                                u_err++;
                        }
                }
        }
        if (dflg) {

                /* set the last entry to NULL pointer in the table
                   of users to be deleted */

                d_usr[i]=0;

                /* check the existence of input users */

                setspwent();
                for (i=0; d_usr[i]; i++) {
                        if ((pwd_ent = getspwnam(d_usr[i])) == NULL) {
                                u_err++;
                        }
                }
        }
        if (u_err) {
                fprintf (stderr,E_U_NEX);
                fprintf (stderr,UNCHANGED);
                AUD_EXIT(4);
        }


        /* Get the passwd file's mode and owner */

        if ((stat(PASSWD_FILE, &buf)) != 0) {
                fprintf (stderr,E_STAT);
                AUD_EXIT(6);
        }
        
        /* create temp-lock-file file */

        if ((fd=(aud_lockfile(TMP_PASS_FILE, buf.st_mode, buf.st_uid, buf.st_gid))) <0) { 
		if (errno==EEXIST) fprintf (stderr,E_PWDLK);
		else fprintf (stderr,E_PWDTMP);
                AUD_EXIT(5);
        }


        /* Open the passwd file to read */

        if ((fp = fopen (PASSWD_FILE, "r")) == NULL) {
                fprintf (stderr,E_PWD_OPEN);
                unlink (TMP_PASS_FILE);
                AUD_EXIT(6);
        }
        /* Open the tmp file to write */

        if ((ft = fdopen (fd, "w")) == NULL) {
                fprintf (stderr,E_PWDT_OPEN);
                unlink (TMP_PASS_FILE);
                AUD_EXIT(5);
        }

        /* Read all records in the password file */

        setspwent();
        while ((pwd_ent = getspwent()) != NULL) {
                if ((pwd_ent->pw_audflg) == 0) {
                        if (Aflg) {
                                pwd_ent->pw_audflg = 1;
                        } else if (aflg) {
                                strcpy(usr_name, pwd_ent->pw_name);
                                if (match_usr (a_usr, usr_name) == 0) 
                                        pwd_ent->pw_audflg = 1;
                        }
                } else if ((pwd_ent->pw_audflg) == 1) {
                        if (Dflg) {
                                pwd_ent->pw_audflg = 0;
                        } else if (dflg) {
                                strcpy(usr_name, pwd_ent->pw_name);
                                if (match_usr (d_usr, usr_name) == 0) 
                                        pwd_ent->pw_audflg = 0;
                        }
                }

                if (putspwent(pwd_ent, ft) < 0) {
                        fprintf (stderr,E_PWDT_WR);
                        unlink (TMP_PASS_FILE);
                        AUD_EXIT(7);
                }
        }

        /* close and rename tmp file to password file */

        fclose(fp);
        fclose(ft);
        rename (TMP_PASS_FILE, PASSWD_FILE);


        /* write audit record body */

        strcat(audbody, NULL_STRING);
        if (Aflg || Dflg) {
                strcat(audbody, ALLUSR);
		if (Aflg) {
			strcat(audbody, A_MSG);
		} else {
			strcat(audbody, D_MSG);
		}
        } else if ((aflg) && (dflg)) {
                for (i=0; a_usr[i]; i++) {
                        strcat(audbody, a_usr[i]);
                        strcat(audbody, " ");
                }
                strcat(audbody, A_MSG);
                for (i=0; d_usr[i]; i++) {
                        strcat(audbody, d_usr[i]);
                        strcat(audbody, " ");
                }
                strcat(audbody, D_MSG);
        } else {
                if (aflg) {
                        for (i=0; a_usr[i]; i++) {
                                strcat(audbody, a_usr[i]);
                                strcat(audbody, " ");
                        }
                        strcat(audbody, A_MSG);
                }
                if (dflg) {
                        for (i=0; d_usr[i]; i++) {
                                strcat(audbody, d_usr[i]);
                                strcat(audbody, " ");
                        }
                        strcat(audbody, D_MSG);
                }
        }


        /* write audit record header */

        audrec.aud_head.ah_error = 0;
        audrec.aud_head.ah_event = EN_AUDUSR;
        audrec.aud_head.ah_len = strlen(audbody);

        /* write the audit record */

        audswitch(AUD_RESUME);
        audwrite(&audrec);
        audswitch(AUD_SUSPEND);

        AUD_EXIT(0);
}

aud_lockfile(filename, mode, owner, grp)
char *filename;
int mode, owner, grp;
{
        int fd;

        /*  create the lock file */

        if ( (fd = open(filename, O_WRONLY|O_CREAT|O_EXCL, mode)) < 0 )
                return -1;
        if ( owner != -1 && grp != -1 )
                if ( fchown(fd, owner, grp) < 0 ) {
                        close(fd);
                        unlink(filename);
                        return -1;
                }               
        return(fd);
}

match_usr (table, usr)
char *table[], *usr;

{
        int n=0;

        /* if the user in the current passwd entry matches one in the table */
        /* return 0 */

        while (table[n]) {
                if (strcmp(table[n++], usr) == 0)
                        return(0);
        }

        /* return -1 if the user is not in the table */

        return(-1);
}
