/* @(#) $Revision: 37.3 $ */     
#ifdef NLS
#include	<msgbuf.h>
#endif
# include	"ctype.h"
# include	"../../hdr/defines.h"

#ifdef NLS16
#include	<nl_ctype.h>
#endif NLS16

char *strchr();

chkid(line,idstr)

char *line;
char *idstr;

{
	register char *lp;
	register char *p;
	extern int Did_id;

	if (!Did_id && any('%',line))
		if (! *idstr)
#ifdef NLS16
			for(lp=line; *lp != 0; ADVANCE(lp)) {
#else NLS16
			for(lp=line; *lp != 0; lp++) {
#endif NLS16
				if(lp[0] == '%' && lp[1] != 0 && lp[2] == '%')
					if (isupper(lp[1]))
						switch (lp[1]) {
						case 'J':
							break;
						case 'K':
							break;
						case 'N':
							break;
						case 'O':
							break;
						case 'V':
							break;
						case 'X':
							break;
						default:
							return(Did_id++);
						}
			}
		else
			{
			 p=idstr;
			 lp=line;
#ifdef NLS16
			 while(*lp)
				if(!(strncmp(lp,p,strlen(p))))
					return(Did_id++);
				else
					ADVANCE(lp);
#else NLS16
			 while(lp=strchr(lp,*p))
				if(!(strncmp(lp,p,strlen(p))))
					return(Did_id++);
				else
					++lp;
#endif NLS16
			}

	return(Did_id);
}
