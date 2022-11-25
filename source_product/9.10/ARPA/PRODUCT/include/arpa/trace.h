/*	@(#)$Header: trace.h,v 1.9.109.1 91/11/21 12:00:10 kcs Exp $	*/

#ifdef TRACEON

/** see if ctype.h has already been include by program **/
#ifndef _U
#include <ctype.h>
#endif _U

int traceon = 0;
FILE *trace_fd;
char *ctime();
char *strchr();
long _trace_clock, time();
char trace_buf[BUFSIZ];

/*
**	time.h stuff used for timestamping in trace file ...
*/
# include	<time.h>
struct	timeval		tv;
struct	timezone	tz;

/*
**	errno needs to be saved because ctime changes it!  Ouch!
**	MUST be defined as a LOCAL variable each time it has to be saved,
**	in case TRACE calls get stacked, or TRACE and oob() get stacked.
*/

/*
**	macro to actually do the work of tracing ...
**	writes string x to file after a time stamp
*/
#define ETRACE(x)  {	int errno_save_for_tracing = errno;\
			gettimeofday(&tv, &tz);\
			nl_fprintf(trace_fd, "%d (%.11s.%03d): %s", getpid(),\
				ctime(&tv.tv_sec)+8, (int)(tv.tv_usec/1000),x);\
			if( strchr(x,'\n') == NULL) putc('\n', trace_fd);\
			fflush(trace_fd);\
			errno = errno_save_for_tracing;\
		  }

/*
**	macros to trace with zero or more parameters ... have to use
**	separate macros since variable argument macros are not allowed
*/
#define TRACE(y) if( traceon )\
		{ ETRACE(y); }
#define TRACE2(x,y)  if ( traceon )\
		{ nl_sprintf(trace_buf,x,y); ETRACE(trace_buf); }
#define TRACE3(x,y,z) if ( traceon )\
		{ nl_sprintf(trace_buf,x,y,z); ETRACE(trace_buf); }
#define TRACE4(w,x,y,z) if ( traceon ) \
		{ nl_sprintf(trace_buf,w,x,y,z); ETRACE(trace_buf); }
#define TRACE5(v,w,x,y,z) if ( traceon ) \
		{ nl_sprintf(trace_buf,v,w,x,y,z); ETRACE(trace_buf); }
#define TRACE6(u,v,w,x,y,z) if ( traceon ) \
		{ nl_sprintf(trace_buf,u,v,w,x,y,z); ETRACE(trace_buf); }

/*
**	byte tracing macro -- writes trace of bytes to the trace file
**	prints octal equivalent of non-printable characters
*/
#define BYTETRACE(buf,x) if( traceon && x>0 ) {\
				int i; char *p;unsigned char ch;\
				int errno_save_for_tracing = errno;\
				nl_sprintf(trace_buf,"%d: %d chars =",getpid(),x);\
				p = trace_buf + strlen(trace_buf);\
				for(i=0;i<x;i++){\
				    ch = *(buf + i);\
				    if(isprint(ch) || ch == '\n' || ch == '\r')\
					nl_sprintf(p,"%c",ch);\
				    else\
					nl_sprintf(p,"\\%.3o",ch);\
				    p = p + strlen(p);\
				    if ( p > &trace_buf[BUFSIZ-5] ) {\
					nl_fprintf(trace_fd,trace_buf);\
					p = trace_buf;\
				    }\
				}\
				nl_sprintf(p,"\n");\
				nl_fprintf(trace_fd,trace_buf);\
				fflush(trace_fd);\
				errno = errno_save_for_tracing;\
			}


/*
**	macro to start tracing ... opens trace file and sets "traceon"
*/
#define STARTTRACE(trace_file)	{ \
			int errno_save_for_tracing = errno;\
			if( ( trace_fd= fopen(trace_file,"a+")) == NULL)\
			{\
			  perror("error on open of trace file");\
			  exit(1);\
			}\
			else {\
				traceon = 1;\
				nl_fprintf(trace_fd,"\n\n");\
				_trace_clock = time( (long *)0 );\
				TRACE2("STARTED AT %s",ctime(&_trace_clock));\
			}\
			errno = errno_save_for_tracing;\
		    }

/*
**	macro to end tracing ... logs ending message and closes trace file
*/
#define ENDTRACE() {\
		int errno_save_for_tracing = errno;\
		_trace_clock = time( (long *)0 );\
		TRACE2("TRACE FILE CLOSED AT %s",ctime(&_trace_clock));\
		fclose(trace_fd);\
		errno = errno_save_for_tracing;\
		}

#else

/*
**	this stuff takes care of the TRACE macros in non-trace code ...
**	if tracing is off we don't want these guys to generate errors!!
*/
#define TRACE(x)
#define TRACE2(y,z)
#define TRACE3(x,y,z)
#define TRACE4(w,x,y,z)
#define TRACE5(v,w,x,y,z)
#define TRACE6(u,v,w,x,y,z)
#define BYTETRACE(buf,n)
#define STARTTRACE(x);
#define ENDTRACE() 

#endif TRACEON
