/* @(#) $Revision: 32.1 $ */      
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */


#define	N_ENVCHG 0020
#define N_RDONLY 0010
#define N_EXPORT 0004
#define N_ENVNAM 0002
#define N_FUNCTN 0001

#define N_DEFAULT 0

struct namnod
{
	struct namnod	*namlft;
	struct namnod	*namrgt;
	char	*namid;
	char	*namval;
	char	*namenv;
	int	namflg;
};

struct tnamnod
{
	struct tnamnod	*namlft;
	struct tnamnod	*namrgt;
	tchar	*namid;
	tchar	*namval;
	tchar	*namenv;
	int	namflg;
};

