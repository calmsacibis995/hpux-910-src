/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/display.c,v $
 * $Revision: 1.57.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:30:20 $
 */

/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/

#include "../standard/inc.h"
#include "../standard/defs.h"
#include "../standard/types.h"
#include "../standard/externs.h"
#include <fcntl.h>

#define DEBUG

#ifdef hp9000s800
display_help(redir,path)
int redir;
char *path;
{
	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	fprintf(outf,"\n");

	fprintf(outf,"Display structures\n");
	fprintf(outf,"===================\n\n");

	fprintf(outf,"buf    [addr][n|d|a]    Display buffer headr.\n");
	fprintf(outf,"swbuf  [addr][n|a]      Display swbuf buffer headr.\n");
	fprintf(outf,"pfdat  [addr][n|d|a]    Display pfdat entry.\n");
	fprintf(outf,"sysmap [addr][n|a]      Display sysmap entry.\n");
	fprintf(outf,"shmmap [addr][n|a]      Display seglist entry.\n");
	fprintf(outf,"dbd    [addr][n]        Display dbd entry.\n");
	fprintf(outf,"swapt  [addr][n|a]      Display swaptab entry.\n");


#ifdef iostuff
	fprintf(outf,"frame  [addr]|[ndex n]  Display message frame entry.\n");
	fprintf(outf,"port   [addr]|[ndex n]  Display imcport frame entry.\n");
	fprintf(outf,"io     [mgr_name] | [port n] | [help] | [mgr options]     i/o manager analysis\n");
#endif
	fprintf(outf,"inode  [addr][n|d|a]    Display inode frame entry.\n");
	fprintf(outf,"pdir   [addr][n|a|i]    Display pdir entry.\n");
	fprintf(outf,"proc   [addr][n|p|a]    Display proc entry.\n");
	fprintf(outf,"shmem  [addr][n|a]      Display shmem_ds entry.\n");
	fprintf(outf,"text   [addr][n|a]      Display text entry.\n");
	fprintf(outf,"savestate   [addr]      Display savestate entry.\n");
	fprintf(outf,"file   [addr][n|a]      Display file table entry.\n");
	fprintf(outf,"vnode  [addr]           Display vnode entry.\n");
	fprintf(outf,"rnode  [addr] [a]       Display NFS rnode entry.\n");
	fprintf(outf,"cred   [addr]           Display credentials struct.\n");
	fprintf(outf,"pdirhash    [addr]      Display pdirhash for addr.\n");
	fprintf(outf,"print  [struct name] [addr] Display structure.\n");
#ifdef NETWORK
	fprintf(outf,"net type [addr][n|a]    Display network data structure.\n");
	fprintf(outf,"net    option [opt]     Display or toggle network options.\n");
	fprintf(outf,"net    help             Display network help.\n");
	fprintf(outf,"muxcamdata[addr][n|a]   Display muxcamdata entry.\n");
	fprintf(outf,"muxhwdata [addr][n|a]   Display muxhwdata entry.\n");
	fprintf(outf,"muxdata   [addr][n|a]   Display muxdata entry.\n");

	fprintf(outf,"ptyinfo[addr][n|a]      Display ptyinfo entry.\n");
	fprintf(outf,"ptty   [addr][n|a]      Display pty tty entry.\n");
	fprintf(outf,"tty    [addr][n|a]      Display mux tty entry.\n");
	fprintf(outf,"n = index, a= all entries, d = disc blk, p = pid.\n");
#endif NETWORK
	fprintf(outf,"\nDump or scan structures\n");
	fprintf(outf,"=======================\n\n");
	fprintf(outf,"listcount[space][addr][offset]            Number of nodes starting at addr.\n");
	fprintf(outf,"listdump [space][addr][len][a|p][offset]  Hex/ascii dump virtual addresses.\n");
	fprintf(outf,"xdump  [addr]  [len]  Hex/ascii dump virtual addresses.\n");
	fprintf(outf,"dump   [addr]  [len] [p|s] Dump virtual [physical] addresses.\n");
	fprintf(outf,"stktr  [addr] [addr2] Kernel stack trace for pc=addr, sp=addr2 (virt addrs).\n");
	fprintf(outf,"binary [addr]  [len]  Dump virtual address in binary.\n");
	fprintf(outf,"ltor   [space] [addr] Long to real address translation.\n");
	fprintf(outf,"sym    [addr]         Display address as symbol+offset\n");
	fprintf(outf,"setmask[val][mask]    Set search value and mask\n");
	fprintf(outf,"search [addr][len]    Search for pattern in range\n");
	fprintf(outf,"!      Execute shell command.\n");
	fprintf(outf,"help   Help. (Give this message)\n");
	fprintf(outf,"q      Quit.\n");
	fprintf(outf,"show   [symbol|vmstats|vmaddr|iostats|network|dnlcstats]\n");
	fprintf(outf,"setopt Set verbose options [UEPZv]\n");
	fprintf(outf,"snap   re-snapshot data structures.\n");
	fprintf(outf,"real   make realtime priority.\n");
#ifdef NETWORK
	fprintf(outf,"scan [FCUHQPESMRAiIbBdDoOzZnNvVL] scan data structures.\n");
#else  NETWORK
	fprintf(outf,"scan [FCUHQSMRAiIbBdDoOzZvVL] scan data structures.\n");
#endif NETWORK
	fprintf(outf,"        F freelist entries printed.\n");
	fprintf(outf,"        C  coremap entries printed.\n");
	fprintf(outf,"        U additional uarea info and k-stack trace.\n");
	fprintf(outf,"        H text hash chains are printed.\n");
	fprintf(outf,"        Q run queues header are printed.\n");
	fprintf(outf,"        P Pregion, and regions dumpped on scan.\n");
	fprintf(outf,"        E Give complete dump of vfds and dbds.\n");
	fprintf(outf,"        S shared memory tables printed.\n");
	fprintf(outf,"        M vfs list, mount table, dnlc, mntinfo, and file table printed.\n");
	fprintf(outf,"        R make analyze realtime.\n");
	fprintf(outf,"        A dump misc addresses, and registers.\n");
	fprintf(outf,"        i inode checks performed, [I] printed.\n");
	fprintf(outf,"                                  [I] NFS rnodes printed.\n");
	fprintf(outf,"        b buffer checks performed, [B] printed.\n");
	fprintf(outf,"        d swap map checks performed, [D] printed.\n");
#ifdef iostuff
	fprintf(outf,"        o io message tables scanned, [O] printed.\n");
	fprintf(outf,"        z basic i/o manager analysis,[Z] detailed.\n");
#endif
#ifdef VNODE
	fprintf(outf,"        V directory name cache dumped.\n");
#endif
	fprintf(outf,"        L Discless information dumped.\n");
#ifdef NETWORK
	fprintf(outf,"        n networking checks performed.\n");
	fprintf(outf,"        N RFA info in proc struct dumped\n");
#endif NETWORK

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return;
}

#else /* hp9000s300 */

display_help(redir,path)
int redir;
char *path;
{
	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	fprintf(outf,"\n");

	fprintf(outf,"Display structures\n");
	fprintf(outf,"===================\n\n");

	fprintf(outf,"buf    [addr][n|d|a]    Display buffer headr.\n");
	fprintf(outf,"swbuf  [addr][n|a]      Display swbuf buffer headr.\n");
	fprintf(outf,"pfdat  [addr][n|d|a]    Display pfdat entry.\n");
	fprintf(outf,"sysmap [addr][n|a]      Display sysmap entry.\n");
	fprintf(outf,"shmmap [addr][n|a]      Display seglist entry.\n");
	fprintf(outf,"dbd    [addr][n]        Display dbd entry.\n");
	fprintf(outf,"swapt  [addr][n|a]      Display swaptab entry.\n");


	fprintf(outf,"inode  [addr][n|d|a]    Display inode frame entry.\n");
	fprintf(outf,"proc   [addr][n|p|a]    Display proc entry.\n");
	fprintf(outf,"shmem  [addr][n|a]      Display shmem_ds entry.\n");
	fprintf(outf,"text   [addr][n|a]      Display text entry.\n");
	fprintf(outf,"savestate   [addr]      Display savestate entry.\n");
	fprintf(outf,"file   [addr][n|a]      Display file table entry.\n");
	fprintf(outf,"vnode  [addr]           Display vnode entry.\n");
	fprintf(outf,"rnode  [addr] [a]       Display NFS rnode entry.\n");
	fprintf(outf,"cred   [addr]           Display credentials struct.\n");
	fprintf(outf,"print  [struct name] [addr] Display structure.\n");
	fprintf(outf,"xdump  [addr]  [len]  Hex/ascii dump virtual addresses.\n");
	fprintf(outf,"dump   [addr]  [len] [p|s] Dump virtual [physical] addresses.\n");
	fprintf(outf,"binary [addr]  [len]  Dump virtual address in binary.\n");
	fprintf(outf,"setmask[val][mask]    Set search value and mask\n");
	fprintf(outf,"search [addr][len]    Search for pattern in range\n");
	fprintf(outf,"!      Execute shell command.\n");
	fprintf(outf,"help   Help. (Give this message)\n");
	fprintf(outf,"q      Quit.\n");
	fprintf(outf,"show   [symbol|vmstats|vmaddr]\n");
	fprintf(outf,"setopt Set verbose options [UEPZv]\n");
	fprintf(outf,"snap   re-snapshot data structures.\n");
	fprintf(outf,"real   make realtime priority.\n");
	fprintf(outf,"scan [options] scan data structures.\n");
	fprintf(outf,"        F freelist entries printed.\n");
	fprintf(outf,"        C  coremap entries printed.\n");
	fprintf(outf,"        U additional uarea info and k-stack trace.\n");
	fprintf(outf,"        H text hash chains are printed.\n");
	fprintf(outf,"        Q run queues header are printed.\n");
	fprintf(outf,"        P Pregion, and regions dumpped on scan.\n");
	fprintf(outf,"        E Give complete dump of ptes and dbds.\n");
	fprintf(outf,"        S shared memory tables printed.\n");
	fprintf(outf,"        M vfs list, mount table, dnlc, mntinfo, and file table printed.\n");
	fprintf(outf,"        R make analyze realtime.\n");
	fprintf(outf,"        A dump misc addresses, and registers.\n");
	fprintf(outf,"        i inode checks performed, [I] printed.\n");
	fprintf(outf,"                                  [I] NFS rnodes printed.\n");
	fprintf(outf,"        b buffer checks performed, [B] printed.\n");
	fprintf(outf,"        d swap map checks performed, [D] printed.\n");

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return;
}

#endif /* s800 vs. s300 */

display_proc(param2,opt,redir,path)
int param2, redir;
char opt;
char *path;
{
	struct proc *addr, *unloc_addr;
	struct pregion *pregptr;
	int i;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_proc(0x%x, 0x%x, 0x%x, 0x%x)\n",param2,opt,redir,path);
#endif
	if (param2 == -1)
		param2 = cur_proc_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}
	allow_sigint = 1;

	switch (opt) {

	case 'p':
		/* pid */
		if ((addr = (struct proc *)pid_to_proc(param2)) == (struct proc *)-1)
			goto reset;

		break;

	case 'n':
		/* index into proc table */
		if ((addr = (struct proc *)ndx_toproc(param2)) == (struct proc *)-1)
			goto reset;
		break;
	case 'a':
		/* dump entire proc  table */
		addr = &proc[0];
		for (i = 0; i < nproc; i++) {
			dumpproc("entry ",addr,proc,vproc);
			/* skip if this entry is not being used */
			if (addr->p_upreg !=0) {
				getu(addr);
				fprintf(outf,"\n");
			}
			fprintf(outf,"\n");
			addr++;
		}
		goto reset;


	case '\0' :
		/* Check validity of proc address */
		if ((addr = (struct proc *)valid_proc(param2)) == (struct proc *)-1)
			goto reset;
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	dumpproc("entry ",addr,proc,vproc);
	if (addr->p_upreg != 0) {
		if (getu(addr) > 0)
			goto reset;
	} else
		goto  reset;

	if (Pflg) {
		vas_t *vas;
		unloc_addr = unlocalizer(addr,proc,vproc);

		if (addr->p_vas == 0) {
			fprintf(outf," Process has no pregions!\n");
		} else {
		    vas = GETBYTES(vas_t *, addr->p_vas, sizeof(vas_t));
		    if (vas == NULL) {
			fprintf(outf, "display_proc: localizing vas failed\n");
			goto reset;
		    }
		    followpregion(addr, vas->va_next);
		}
	}


	/* reset our default */
	cur_proc_addr = param2;

reset:
	allow_sigint = 0;
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

display_text(param2,opt,redir,path)
int param2, redir;
char opt;
char *path;
{
	fprintf(stderr, "display_text not supported with regons\n");
}

display_buf(param1,param2,opt,redir,path)
int param1,param2, redir;
char opt;
char *path;
{
	struct buf *vbp;
	unsigned absaddr;
	struct buf dummy;
	struct buf *bp = &dummy;
	int gotit, i;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_buf(0x%0x 0x%x, 0x%x, 0x%x, 0x%x)\n",param1,param2,opt,redir,path);
#endif

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {

	case 'a':	/* These options require us to search the buffer */
		allow_sigint = 1;
	case 'd':	/* header list. */
	case 'n':
	    {	
	        int count = 0;

		vbp = vbufhdr;
		while (vbp != NULL && (count++ <100000)) {
                  absaddr = getphyaddr ((unsigned) vbp);
                  longlseek(fcore, (long)clear(absaddr), 0);
                  if (longread(fcore, (char *)bp, sizeof (struct buf))
                        != sizeof (struct buf)) {
                        perror("display_buf longread");
                        goto reset;
                  }

		  switch (opt) {
		    case 'n':
			if (param1 == count) goto printit;
			break;
		    case 'd':
			if ((param1== (int)bp->b_dev) 
				&& (param2 == (int)bp->b_blkno)) goto printit;
			break;
		    case 'a':
			dumpbuf("entry ",bp, vbp);
			break;
		  }
		  vbp = bp->b_nexthdr;
		}

		if (opt != 'a') {
		    fprintf(outf," Could not find matching buffer header\n");
		    goto reset;
		}
		allow_sigint = 0;
		goto reset;
		break;


	  }
	case '\0' :
		/* Check validity of proc address */
		vbp = (struct buf *)param1;
                absaddr = getphyaddr ((unsigned) vbp);
                longlseek(fcore, (long)clear(absaddr), 0);
                if (longread(fcore, (char *)bp, sizeof (struct buf))
                        != sizeof (struct buf)) {
                        perror("display_buf longread");
			goto reset;
                }

		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

printit:
	dumpbuf("entry ",bp, vbp);

	/* reset our default */
	cur_buf_addr = param1;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

display_swbuf(param2,opt,redir,path)
int param2, redir;
char opt;
char *path;
{
	struct buf *addr;
	int i;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_swbuf(0x%x, 0x%x, 0x%x, 0x%x)\n",param2,opt,redir,path);
#endif
	if (param2 == -1)
		param2 = cur_swbuf_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {

	case 'n':
		/* index into swbuf table */
		if ((addr = (struct buf *)ndx_toswbuf(param2)) == (struct buf *)-1)
			goto reset;
		break;

	case 'a':
		/* dump entire swbuf table */
		allow_sigint = 1;
		addr = &swbuf[0];
		for (i = 0; i < nswbuf; i++) {
			dumpbuf("entry ",addr,(addr-swbuf)+vswbuf);
			fprintf(outf,"\n");
			addr++;
		}
		allow_sigint = 0;
		goto reset;

	case '\0' :
		/* Check validity of proc address */
		if ((addr = (struct buf *)valid_swbuf(param2)) == (struct buf *)-1)
			goto reset;
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	dumpbuf("entry ",addr,(addr-swbuf)+vswbuf);


	/* reset our default */
	cur_swbuf_addr = param2;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

display_inode(param1,param2,opt,redir,path)
int param1,param2, redir;
char opt;
char *path;
{
	register struct inode *ip;
	int gotit, i;

struct inode *addr;
	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_inode(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",param1,param2,opt,redir,path);
#endif
	/*
	if (param1 == -1)
		param1 = cur_inode_addr;
	*/

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {

	case 'n':
		/* index into inode table */
		if ((addr = (struct inode *)ndx_toinode(param1)) == (struct inode *)-1)
			goto reset;
		break;
	case 'd':
		/* dev, inode pair */
		gotit = 0;
		for (ip = &inode[0]; ip < &inode[ninode]; ip++) {
			if ((param1== (int)ip->i_dev) && (param2 == (int)ip->i_number)) {
				gotit++;
				break;
			}
		}
		if (!gotit) {
			fprintf(outf," No such inode in core inode table\n");
			goto reset;
		}
		addr = ip;
		break;

	case 'a':
		/* dump entire inode table */
		allow_sigint = 1;
		addr = &inode[0];
		for (i = 0; i < ninode; i++) {
			fprintf(outf,"\n");
			dumpinode("entry ",addr,inode,vinode);
			addr++;
		}
		allow_sigint = 0;
		goto reset;


	case '\0' :
		/* Check validity of proc address */
		if ((addr = (struct inode *)valid_inode(param1)) == (struct inode *)-1)
			goto reset;
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	dumpinode("entry ",addr,inode,vinode);


	/* reset our default */
	cur_inode_addr = param1;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

display_file(param1,opt,redir,path)
int param1, redir;
char opt;
char *path;
{
	register struct inode *ip;
	struct file *fp;
	struct rnode rn;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_file(0x%x 0x%x, 0x%x, 0x%x)\n",param1,opt,redir,path);
#endif
	if (param1 == -1)
		param1 = cur_file_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {

	case 'n':
		/* index into file table */
		if ((fp=(struct file *)ndx_tofile(param1)) == (struct file *)-1)
			goto reset;
		break;

	case 'a':
		/* dump entire file table */
		allow_sigint = 1;
		for (fp = file; fp < &file[nfile]; fp++) {
			fprintf(outf,"\n");
			dumpft(" ",fp,file,vfile);
			if (fp->f_data) {
				/* Does entry refer to a valid inode? */
				if (((struct vnode *)fp->f_data >= (struct vnode *)vinode)
				    && ((struct vnode *)fp->f_data < (struct vnode *)&vinode[ninode]))
					dumpinode(" ", &inode[(struct inode *)fp->f_data - vinode], inode, vinode);
				else if (getchunk(KERNELSPACE,
						  fp->f_data-sizeof(struct rnode *),
						  &rn, sizeof(struct rnode),
						  "display_file") == 0
					 && rn.r_vnode.v_fstype == VNFS)
					dumprnode("",&rn,fp->f_data-sizeof(struct rnode *));
			}
		}
		allow_sigint = 0;
		goto reset;


	case '\0' :
		/* Check validity of file address */
		if ((fp = (struct file *)valid_file(param1)) == (struct file *)-1)
			goto reset;
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	dumpft(" ",fp,file,vfile);
	if (fp->f_data) {
		/* Does entry refer to a valid inode? */
		if (((struct vnode *)fp->f_data >= (struct vnode *)vinode)
		    && ((struct vnode *)fp->f_data < (struct vnode *)&vinode[ninode]))
			dumpinode(" ", &inode[(struct inode *)fp->f_data - vinode], inode, vinode);
		else if (getchunk(KERNELSPACE,
				  fp->f_data-sizeof(struct rnode *),
				  &rn, sizeof(struct rnode),
				  "display_file") == 0
			 && rn.r_vnode.v_fstype == VNFS)
			dumprnode("",&rn,fp->f_data-sizeof(struct rnode *));
	}


	/* reset our default */
	cur_file_addr = param1;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

display_vnode(param1,redir,path)
int param1, redir;
char *path;
{
	struct inode *ip;
	struct vnode vn;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_vnode(0x%x, 0x%x, 0x%x)\n",param1,redir,path);
#endif
	if (param1 == -1)
		param1 = cur_vnode_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}
	if (getchunk(KERNELSPACE,param1,&vn,sizeof(struct vnode),"display_vnode"))
		goto reset;
	dumpvnode("",&vn,param1);

	/* reset our default */
	cur_vnode_addr = param1;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

display_cred(param1,redir,path)
int param1, redir;
char *path;
{
	struct inode *ip;
	struct ucred cred;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_cred(0x%x, 0x%x, 0x%x)\n",param1,redir,path);
#endif
	if (param1 == -1)
		param1 = cur_cred_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}
	if (dumpcred("Credential struct: ",param1) != -1)
		cur_cred_addr = param1;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

display_rnode(param1,opt,redir,path)
int param1, redir;
char opt;
char *path;
{
	struct rnode rn;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_rnode(0x%x, 0x%x, 0x%x, 0x%x)\n",
		param1,opt,redir,path);
#endif
	if (param1 == -1)
		param1 = cur_rnode_addr;


	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {

	case 'a':
		/* dump entire inode table */
		allow_sigint = 1;
		/* take the easy way out here */
		rcheck();
		allow_sigint = 0;
		goto reset;


	case '\0' :
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	if (getchunk(KERNELSPACE,param1,&rn,sizeof(struct rnode),"display_rnode"))
		goto reset;
	dumprnode("",&rn,param1);

	/* reset our default */
	cur_inode_addr = param1;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

#ifdef __hp9000s800

display_pdir(param2,opt,redir,path)
int param2, redir;
char opt;
char *path;
{
	struct hpde *addr, *pde;
	int i;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_pdir(0x%x, 0x%x, 0x%x, 0x%x)\n",param2,opt,redir,path);
#endif
	if (param2 == -1)
		param2 - cur_pdir_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {

	case 'n':
		/* index into pdir table */
		if ((addr = (struct hpde *)ndx_topde(param2)) == (struct hpde *)-1)
			goto reset;
		break;


	case 'a':
		/* dump entire pdir table */
		allow_sigint = 1;
		fprintf(outf,"HASH TABLE\n");
		addr = &htbl[0];
		for (i = 0; i < nhtbl; i++) {
		    dumppde("entry",addr,0);
		    fprintf(outf,"\n");
		    /*
		     * catch any dynamically allocated pde's hanging
		     * off of this chain
		     */
		    pde = addr->pde_next;
		    while ((pde - vpdir) >  scaled_npdir) {
			    if (getepde(pde, &tmppde))
				    fprintf(outf, "getedpe failed on 0x%x, display_pde aborted\n", pde);
			    dumppde("entry", &tmppde, 0);
			    pde = tmppde.pde_next;
		    }
		    addr++;
		}

		fprintf(outf,"PDIR TABLE\n");
		addr = &pdir[0];
		for (i = 0; i < scaled_npdir; i++) {
		    struct hpde *pde;
		    dumppde("entry",addr,0);
		    fprintf(outf,"\n");
		    /*
		     * catch any dynamically allocated pde's hanging
		     * off of this chain
		     */
		    pde = addr->pde_next;
		    while ((pde - vpdir) >  scaled_npdir) {
			    if (getepde(pde, &tmppde))
				    fprintf(outf, "getedpe failed on 0x%x, display_pde aborted\n", pde);
			    dumppde("entry", &tmppde, 0);
			    pde = tmppde.pde_next;
		    }
		    addr++;
		}
		allow_sigint = 0;
		goto reset;


	case '\0' :
		/* Check validity of pde address */
		if ((addr =(struct hpde *)valid_pde(param2))==(struct hpde *)-1)
			goto reset;
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	dumppde("entry",addr,0);

	/* reset our default */
	cur_pdir_addr = param2;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}
#endif

display_num(value)
int value;
{
	fprintf(outf,"\n");
	fprintf(outf," value = 0x%08x,  %d\n",value,value);
	return(0);
}

/* These masks control search routines */
setmask(value,mask)
unsigned int value, mask;
{
	fprintf(outf,"global_value  0x%x  <=  0x%x\n",global_value,value);
	fprintf(outf,"global_mask   0x%x  <=  0x%x\n",global_mask,mask);
	global_value = value;
	global_mask = mask;
}

search(space,addr,length,redir,path)
int space;
int *addr, redir;
int length;
char *path;
{
	register int i;
	register unsigned int word,phyaddr;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"search(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",space,
	addr,length,redir,path);
#endif
	if (addr == (int *)-1)
		addr = (int *)cur_mem_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}
	allow_sigint= 1;

	for (i = 0; i < length; i++) {
		phyaddr = ltor(space,addr);
		if (phyaddr == 0) {
			/* do nothing */
		} else {
			word = getreal(phyaddr);
			if ((word & global_mask) ==  global_value) {
				/* match satisfied print out value */
				fprintf(outf,"virt addr = 0x%08x  phys addr = 0x%08x  value 0x%08x\n",addr, phyaddr, word);
			}
		}
		/* Bump address */
		addr++;
	}
	fprintf(outf,"\n");


	allow_sigint= 0;
	/* reset our default */
	cur_mem_addr = (int)addr;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

dumpmem(space, addr,length,opt,redir,path)
int space, *addr, redir;
int length;
char opt;
char *path;
{
	register int phyaddr, word, i;
	int diff;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"dumpmem(0x%x, 0x%x, 0x%x, 0x%s, 0x%x, 0x%x)\n",
	space,addr,length,opt,redir,path);
#endif
	if (addr == (int *)-1)
		addr = (int *)cur_mem_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	allow_sigint= 1;

	/* sorry but lets keep it reasonable */
	if (length > 40000) {
		fprintf(outf,"Length truncated to 40000 words\n");
		length = 40000;
	}

	switch (opt) {
#ifdef hp9000s800
#define MAXPHYS 0xf0000000
#else
#undef MAXPHYS
#define MAXPHYS 0xffffffff
#endif
	 case 'p': /* physical dump */
		for (i = 0; i < length; i++) {
			phyaddr = (int)addr;
			addr++;
			if ((unsigned)phyaddr >= MAXPHYS) {
				fprintf(outf,"address out of range\n");
				break;
			} else {
				word = getreal(phyaddr);
				if ((i % 7) == 0)
					fprintf(outf,"\n");
				fprintf(outf,"0x%08x ",word);

			}
		}

		break;

	 case 's': /* translate address to symbols */

		for (i = 0; i < length; i++) {
			phyaddr = ltor(space,addr);
			if (phyaddr == 0) {
				fprintf(outf," 0x%x address not mapped\n", addr -1);
				break;
			} else {
				word = getreal(phyaddr);

				fprintf(outf,"Address 0x%08x, value 0x%08x ",
					addr, word);

				diff = findsym(word);
				if (diff < 0x0000ffff)
					fprintf(outf,"  %s+0x%04x\n",cursym,diff);
				else
					fprintf(outf," no match\n");


			}
			addr++;
		}
		break;

	 default: /* straight dump */
		for (i = 0; i < length; i++) {
			phyaddr = ltor(space,addr++);
			if (phyaddr == 0) {
				fprintf(outf," 0x%x address not mapped\n",addr - 1);
			break;
			} else {
				word = getreal(phyaddr);
				if ((i % 7) == 0)
					fprintf(outf,"\n");
				fprintf(outf,"0x%08x ",word);

			}
		}
	}

	fprintf(outf,"\n");
	allow_sigint = 0;


	/* reset our default */
	cur_mem_addr = (int)addr;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

/* NOTE, only 60 words of the savestate read and written... */

#ifdef hp9000s800
/* 60 * 4 = 240 */
#define SAVESTATESIZE 240
#else
#define SAVESTATESIZE 100
#endif

displaysavestate(space, addr,opt,redir,path)
int space, *addr, redir;
char opt;
char *path;
{
	register int phyaddr, word, i;
	int sstate[SAVESTATESIZE];
	int diff;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"displaysavestate(0x%x, 0x%x, 0x%s, 0x%x, 0x%x)\n",
	space,addr,opt,redir,path);
#endif
	if (addr == (int *)-1)
		addr = (int *)cur_mem_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	allow_sigint= 1;

	getchunk(space,addr,sstate,SAVESTATESIZE,"Displaysavestate");

	dumpsavestate(sstate);

	allow_sigint = 0;


	/* reset our default */
	cur_mem_addr = (int)addr;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

#ifdef hp9000s800

displaypdirhash(space, addr,opt,redir,path)
int space, *addr, redir;
char opt;
char *path;
{
	unsigned int hashval;
	unsigned int hashindex;
	struct hpde *myhtbl;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"displaypdirhash(0x%x, 0x%x, 0x%s, 0x%x, 0x%x)\n",
	space,addr,opt,redir,path);
#endif
	if (addr == (int *)-1)
		addr = (int *)cur_mem_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	/* Calculate the pdir hash */
	hashval = pdirhashproc(space, addr);
	hashindex = hashval % nhtbl;
	myhtbl = vhtbl + hashindex;
	fprintf(outf," Pdir hash = 0x%x,  htbl index = 0x%x , htbl address 0x%x\n",
	hashval, hashindex, myhtbl);



	/* reset our default */
	cur_mem_addr = (int)addr;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

#endif /* hp9000s800 */

display_ltor(space, addr, redir, path)
u_long space;
u_long addr;
int redir;
char *path;
{
	u_long phyaddr;

	fprintf(outf, "\n");
#ifdef DEBUG
	fprintf(stderr, "display_ltor(0x%x, 0x%x, 0x%x, 0x%x)\n",
	        space, addr, redir, path);
#endif

	if (redir) {
		if ((outf = fopen(path, (redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr, "Can't open file %s, errno = %d\n",
				path, errno);
			goto out;
		}
	}

	phyaddr = ltor(space, addr);
	if (phyaddr == 0) {
		fprintf(outf, "<space, addr> address NOT mapped in pdir");
	} else {
		fprintf(outf,
			"<space, addr> mapped to physical address 0x%08x",
			phyaddr);

	}
	fprintf(outf, "\n\n");

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

display_sym(addr, redir, path)
u_long addr;
int redir;
char *path;
{
	u_long diff;

	fprintf(outf, "\n");
#ifdef DEBUG
	fprintf(stderr, "display_sym(0x%x, 0x%x, 0x%x)\n", addr, redir, path);
#endif

	if (redir) {
		if ((outf = fopen(path, (redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr, "Can't open file %s, errno = %d\n",
				path, errno);
			goto out;
		}
	}

	diff = findsym(addr);
	if (diff == 0xffffffff)
		fprintf(outf, "  0x%04x", addr);
	else if (diff == 0)
		fprintf(outf, "  %s", cursym);
	else
		fprintf(outf, "  %s+0x%04x", cursym, diff);

	fprintf(outf, "\n");

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

xdumpmem(space,addr,length,redir,path)
int space,*addr, redir;
int length;
char *path;
{
#ifdef DEBUG
	fprintf(stderr,"xdumpmem(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",
	    space,addr,length,redir,path);
#endif

	if (addr == (int *)-1)
		addr = (int *)cur_mem_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",
			    path,errno);
			goto out;
		}
	}

	fprintf(outf,"\n");

	allow_sigint= 1;

	/* sorry but lets keep it reasonable */
	if (length > 40000) {
		fprintf(outf,"Length truncated to 40000 words\n");
		length = 40000;
	}

	dumphex(space,addr, length);

	allow_sigint = 0;

	/* reset our default */
	cur_mem_addr = (int)addr;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

display_sema(addr,redir,path)
int addr, redir;
char *path;
{
	fprintf(outf,"Not supported\n");
}

display_shmem(param2, opt, redir, path)
int param2, redir;
char opt;
char *path;
{
	struct shmid_ds *addr;
	int gotit, i;

	fprintf(outf, "\n");
#ifdef DEBUG
	fprintf(stderr,"display_shmem(0x%x, 0x%x, 0x%x, 0x%x)\n",
		param2, opt, redir, path);
#endif
	if (param2 == -1)
		param2 = cur_shmem_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr, "Can't open file %s, errno = %d\n",
				path, errno);
			goto out;
		}
	}

	switch (opt) {
	case 'n':
		/* index into proc table */
		if ((addr = (struct shmid_ds *)ndx_toshmem(param2)) ==
							(struct shmid_ds *)-1)
			goto reset;
		break;

	case 'a':
		/* dump entire shmem table */
		allow_sigint = 1;
		addr = &shmem[0];
		for (i = 0; i < shminfo.shmmni; i++) {
			dumpshmid("entry ",addr,shmem,vshmem);
			fprintf(outf,"\n");
			addr++;
		}
		allow_sigint = 0;
		goto reset;

	case '\0' :
		/* Check validity of proc address */
		if ((addr = (struct shmid_ds *)valid_shmem(param2)) ==
							(struct shmid_ds *)-1)
			goto reset;
		break;

	default:
		/* bad option */
		fprintf(outf, " bad option\n");
		goto reset;
	}

	dumpshmid("entry ", addr, shmem, vshmem);

	/* reset our default */
	cur_shmem_addr = param2;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

dumptable(opt,redir,path)
int opt, redir;
char *path;
{
	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"dumptable(0x%x, 0x%x, 0x%x)\n",opt,redir,path);
#endif

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {

	case 1:	/* dump symbols */
		dumpaddress();
		break;

	case 2: /* dump vm stats */
		dumpvmstats();
		break;

	case 3: /* dump vm table addresses */
		dumpvmaddr();
		break;

#ifdef iostuff
	case 4: /* dump io stats */
		dumpio(outf);
		break;
#endif

#ifdef NETWORK
	case 5: /* dump networking stats */
		dumpnet();
		break;
#endif

	case 6: /* dump dnlc stats */
		dump_dnlcstats();
		break;

	default:
		/* nothing */
		;
	}

reset:
	/* close file if redirected */
	if (redir)
		fclose(outf);
out:
	outf = stdout;
	return(0);
}

dumpbinary(space,addr,length,redir,path)
int space;
int *addr, redir;
int length;
char *path;
{
	register int i;
	int word, phyaddr;
	int fd;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"dumpbinary(0x%x 0x%x, 0x%x, 0x%x, 0x%x)\n",
	space,addr,length,redir,path);
#endif
	if (addr == (int *)-1)
		addr = (int *)cur_mem_addr;

	if (redir) {
		if ((fd = open(path,O_RDWR|O_CREAT|O_TRUNC,0660)) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}


	for (i = 0; i < length; i++) {
		phyaddr = ltor(space,addr++);
		if (phyaddr == 0) {
			fprintf(outf,"address not mapped\n");
			break;
		} else {
			word = getreal(phyaddr);
			write(fd,&word,4);

		}
	}

	/* reset our default */
	cur_mem_addr = (int)addr;

reset:
	/* close file if we redirected output */
	if (redir)
		close(fd);

out:
	outf = stdout;
	fprintf(outf,"\n");
	return(0);
}

dumpkernelbinary(redir, path)
int redir;
char *path;
{
	fprintf(outf,"No longer supported\n");
}

/* Have some fun */
fxbg()
{
	fprintf(outf,"\n");
	fprintf(outf,"Computing... please wait...\n");
	sleep(10);
	fprintf(outf,"Bug found, whats it worth to you?\n");
	fprintf(outf,"Try wizard command.\n");
	fprintf(outf,"\n");
}

int wzcnt = 0;

wzrd()
{
	int doit;
	struct timeval tp;
	struct timezone tz;

	wzcnt++;
	fprintf(outf,"\n");
	if (wzcnt < 2) {
		fprintf(outf,"Only wizards allowed to use this command, go away!\n");
		gettimeofday(&tp, &tz);
		srand(tp.tv_sec);
		return;
	}

	doit = rand();
	doit = doit % 18;
	switch (doit) {
	case 0:
		fprintf(outf,"Only wizards allowed to use this command, good bye!\n");
		exit();
		break;
	case 1:
		fprintf(outf,"You are near a stream, a lantern is on the ground.\n");
		break;
	case 2:
		fprintf(outf,"You must first give me the magic code.\n");
		break;
	case 3:
		fprintf(outf,"You're no wizard.\n");
		break;
	case 4:
		fprintf(outf,"There is a black rod near, and a bird chirping.\n");
		break;
	case 5:
		fprintf(outf,"This crash looks bad, you'll need wizard powers to fix this one.\n");
		break;
	case 6:
		fprintf(outf,"You have been promoted to wizard status.\n");
		break;
	case 7:
		fprintf(outf,"What is thy bidding?.\n");
		break;
	case 8:
		fprintf(outf,"Are you having fun yet?\n");
		break;
	case 9:
		fprintf(outf,"Encrypting crash dump... please wait\n");
		sleep(5);
		fprintf(outf,"Just kidding.\n");
		fprintf(outf,"done.\n");
		break;
	case 10:
		fprintf(outf,"Did anyone see a semaphore walk through here?\n");
		break;
	case 11:
		fprintf(outf,"Analyze is the product of a warped mind...\n");
		break;

	case 12:
		fprintf(outf,
#ifdef __hp9000s300
			"A nasty dwarf darts out of the 300, looks around\n"
#else
#ifdef __hp9000s700
			"A nasty dwarf darts out of the 700, looks around\n"
#else
			"A nasty dwarf darts out of the 800, looks around\n"
#endif /* s700 vs s800 */
#endif /* s300 vs s700/s800 */
		);
		fprintf(outf,"and jumps back in...\n");
		break;

	case 13:
		fprintf(outf,"whats up doc???\n");
		break;

	case 14:
		fprintf(outf,"This area under repair\n");
		break;

	case 15:
		fprintf(outf,"Wizard powers removed.\n");
		wzcnt = 0;
		break;

	default:
		fprintf(outf,"loshojlevadlvdlkgmalycatsgdp magic cookie????\n");
		break;
	}
	fprintf(outf,"\n");
}

lok()
{
	int doit;
	struct timeval tp;
	struct timezone tz;

	wzcnt++;
	fprintf(outf,"\n");
	if (wzcnt < 2) {
		fprintf(outf,"Only wizards allowed to use this command, go away!\n");
		gettimeofday(&tp, &tz);
		srand(tp.tv_sec);
		return;
	}
	doit = rand();
	doit = doit % 10;
	switch (doit) {
	case 0:
		fprintf(outf,"There are many proc structures here, ptes\n");
		fprintf(outf,"are dripping on the floor.\n");
		break;
	case 1:
		fprintf(outf,"You are in a maze of twisty little buffers.\n");
		break;
	case 2:
		fprintf(outf,"There are many inodes here.\n");
		fprintf(outf,"A sign says los was here.\n");
		break;
	case 3:
		fprintf(outf,"There are many pointers here. Watch out\n");
		fprintf(outf,"for the non-equivalent ones.\n");
		break;
	case 4:
		fprintf(outf,"There is a black rod near, and a bird chirping.\n");
		break;
	case 5:
		fprintf(outf,"There are lots of bits in a bucket. A sign\n");
		fprintf(outf,"posted above the door says /dev/null.\n");
		break;
	case 6:
		fprintf(outf,"The are many tlbs here, some look worried\n");
		fprintf(outf,"and are raising exceptions.\n");
		break;
	case 7:
		fprintf(outf,"There are realtime processes in this room,\n");
		fprintf(outf,"This seems to be the first class section\n");
		fprintf(outf,"as they are being given better treatment.\n");
		fprintf(outf,"A person know only as lvd sits behind\n");
		fprintf(outf,"a desk. He lets realtimers through.\n");
		break;
	case 8:
		fprintf(outf,"Are you having fun yet?\n");
		break;
	case 9:
		fprintf(outf,"You are in the cache room. Data is flying\n");
		fprintf(outf,"bye at tremendous speeds.\n");
		fprintf(outf,"A sign says Beware of Cache Coherency.\n");
		break;

	}
	fprintf(outf,"\n");
}

/* stub */

display_cmap(param1,param2,opt,redir,path)
int param1,param2, redir;
char opt;
char *path;
{ }

/* REGION */

display_pfdat(param1,param2,opt,redir,path)
int param1,param2, redir;
char opt;
char *path;
{
	struct pfdat *addr;
	struct pfdat *cp;
	int i;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_pfdat(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",param1,param2,opt,redir,path);
#endif

	if (param1 == -1)
		param1 = cur_cmap_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {

	case 'n':
		/* index into pfdat table */
		if ((addr = (struct pfdat*)ndx_topfdat(param1)) == (struct pfdat *)-1)
			goto reset;
		break;

	case 'd':
		/* dev, blk pair */
		cp = (struct pfdat *)pffinder(param1,param2);
		if (cp == (struct pfdat *)0) {
			fprintf(outf," No such dev,blk in text hash chains\n");
			goto reset;
		}
		addr = cp;
		break;

	case 'a':
		/* dump entire pfdat table */
		allow_sigint = 1;
		addr = &pfdat[0];
		for (i = 0; i < physmem; i++) {
			dumppfdat("pfdat ",addr, pfdat, vpfdat);
			fprintf(outf,"\n");
			addr++;
		}
		allow_sigint = 0;
		goto reset;

	case '\0' :
		/* Check validity of pfdat address */
		if ((addr = (struct pfdat *)valid_pfdat(param1)) == (struct pfdat *)-1)
			goto reset;
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	/* Dump pfdat structure */
	dumppfdat("pfdat ",addr, pfdat, vpfdat);

	/* reset our default */
	cur_cmap_addr = param1;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

display_pregion(param1,param2,opt,redir,path)
int param1,param2, redir;
char opt;
char *path;
{
	struct pregion *addr;
	struct pregion *cp;
	register int i;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_pregion(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",param1,param2,opt,redir,path);
#endif

	allow_sigint = 1;

	if (param1 == -1)
		param1 = cur_cmap_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {

	case 'n':
		/* index into pregion table */
		/* This option is not applicable to the new VM */
		fprintf(outf," option not supported in new VM\n");
		goto reset;

	case 'a':
		/* dump entire pregion table */
		/* Go down pregions , and dump all */
		/* This option is not applicable to the new VM */
		fprintf(outf," option not supported in new VM\n");
		goto reset;


	case '\0' :
		/* Check validity of cmap pregion */
		/* Checking of this parameter will have to be done
		   by means of other method, will address this later */
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	addr = GETBYTES(preg_t *, param1, sizeof(preg_t));
	if (addr == 0) {
		fprintf(outf,"display_pregion: localizing object failed\n");
		goto reset;
	}
	/* Dump pregion structure */
	dumppreg("pregion ",addr, param1);

	/* reset our default */
	cur_cmap_addr = param1;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	allow_sigint = 0;

	return(0);
}

display_region(param1,param2,opt,redir,path)
int param1,param2, redir;
char opt;
char *path;
{
	struct region *addr;
	struct region *cp;
	register int i;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_region(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",param1,param2,opt,redir,path);
#endif

	if (param1 == -1)
		param1 = cur_cmap_addr;

	allow_sigint = 1;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {

	case 'n':
		/* index into region table */
		/* This option is not applicable to the new VM */
		fprintf(outf," option not supported in new VM\n");
		goto reset;

	case 'a':
		/* dump entire region table */
		/* Go down regions , and dump all */
		fprintf(outf," option not supported in new VM\n");
		goto reset;

	case '\0' :
		/* Check validity of cmap region */
		/* will need to figure this out later */
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	/* Dump region structure */
	addr = GETBYTES(reg_t *, param1, sizeof(reg_t));
	if (addr == 0) {
		fprintf(outf,"display_region: localizing object failed\n");
		goto reset;
	}
	dumpreg("region ", addr, param1);

	/* reset our default */
	cur_cmap_addr = param1;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;

	allow_sigint = 0;

	return(0);
}

display_swaptab(param1,param2,opt,redir,path)
int param1,param2, redir;
char opt;
char *path;
{
	swpt_t *addr;
	swpt_t *cp;
	int i;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_swaptab(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",param1,param2,opt,redir,path);
#endif

	if (param1 == -1)
		param1 = cur_cmap_addr;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {

	case 'n':
		/* index into swaptab table */
		if ((addr = (swpt_t *)ndx_toswaptab(param1)) ==
			(swpt_t *)-1)
			goto reset;
		break;

	case 'a':
		/* dump entire swaptab table */
		allow_sigint = 1;
		addr = &swaptab[0];
		for (i = 0; i < MAXSWAPCHUNKS; i++) {
			dumpswaptab("swaptab ",addr, swaptab, vswaptab);
			fprintf(outf,"\n");
			addr++;
		}
		allow_sigint = 0;
		goto reset;


	case '\0' :
		/* Check validity of swaptab */
		if ((addr = (swpt_t *)valid_swaptab(param1)) == (swpt_t *)-1)
			goto reset;
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	/* Dump swaptab structure */
	dumpswaptab("swaptab ",addr, swaptab, vswaptab);

	/* reset our default */
	cur_cmap_addr = param1;

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}

display_dbd(param1,param2,opt,redir,path)
int param1,param2, redir;
char opt;
char *path;
{
	dbd_t *addr;
	dbd_t *cp;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_dbd(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",param1,param2,opt,redir,path);
#endif

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {
	case '\0' :
		/* Check validity of dbd, and get copy into local storage */
		if ((addr = (dbd_t *)valid_dbd(param1)) == (dbd_t *)-1)
			goto reset;
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	/* Dump dbd structure */
	dumpdbd("dbd ",addr);

reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	return(0);
}


display_sysmap(param1,param2,opt,redir,path)
int param1,param2, redir;
char opt;
char *path;
{
	struct mapent *addr;
	register int i;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_sysmap(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",param1,param2,opt,redir,path);
#endif


	allow_sigint = 1;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {

	case 'n':
		/* index into sysmap table */
		if ((addr = (struct mapent *)ndx_tosysmap(param1)) ==
			(struct mapent *)-1)
			goto reset;
		break;

	case 'a':
		/* dump entire sysmap table */
		/* skip first */
		allow_sigint = 1;
		addr = (struct mapent *)&sysmap[1];
		for (i = 1; i < SYSMAPSIZE; i++) {
			dumpsysmap("sysmap",addr,sysmap,vsysmap);
			addr++;
		}
		allow_sigint = 0;
		goto reset;


	case '\0' :
		/* Check validity of sysmap */
		if ((addr = (struct mapent *)valid_sysmap(param1)) == (struct mapent *)-1)
			goto reset;
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	/* Dump sysmap structure */
	dumpsysmap("sysmap",addr,sysmap,vsysmap);


reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	allow_sigint = 0;
	return(0);
}

display_shmmap(param1,param2,opt,redir,path)
int param1,param2, redir;
char opt;
char *path;
{}

#ifdef LATER
display_shmmap(param1,param2,opt,redir,path)
int param1,param2, redir;
char opt;
char *path;
{
	struct seglist *addr;
	register int i;

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_shmmap(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",param1,param2,opt,redir,path);
#endif

	allow_sigint = 1;

	if (redir) {
		if ((outf = fopen(path,(redir == 2)?"a+":"w+")) == NULL) {
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}

	switch (opt) {
	case 'n':
		/* index into seglist table */
		if ((addr = (struct seglist *)ndx_toseglist(param1)) ==
			(struct seglist *)-1)
			goto reset;
		break;

	case 'a':
		/* dump entire seglist table */
		/* skip first */
		allow_sigint = 1;
		addr = psegs;
		for (i = 1; i < shminfo.shmmni; i++) {
			dumpseglist("seglist",addr,psegs,vpsegs);
			addr++;
		}
		allow_sigint = 0;
		goto reset;


	case '\0' :
		/* Check validity of seglist */
		if ((addr = (struct seglist *)valid_seglist(param1)) == (struct seglist *)-1)
			goto reset;
		break;

	default:
		/* bad option */
		fprintf(outf," bad option\n");
		goto reset;
	}

	/* Dump sysmap structure */
	dumpseglist("seglist",addr,psegs,vpsegs);


reset:
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out:
	outf = stdout;
	allow_sigint = 0;
	return(0);
}
#endif LATER

#ifndef DEBREC
/* Stubs */
/* Dummy routine in case debrec turned off */

display_print()
{
}
#endif

#ifndef iostuff
/* Dummy routine in case iostuff turned off */
display_frame()
{
}

an_mgr_decode()
{
}

display_imcport()
{
}
#endif

#ifdef __hp9000s800

pgtopde(i)
int i;
{
	if (i >= physmem) {
		fprintf(stderr,
			"Page index 0x%x is out of range (physmem = 0x%x)\n",
			i, physmem);
		return(0);
	}

	return(pgtopde_table[i]);
}

#else

/* stubs for s300 */
display_pdir()
{
}

displaypdirhash()
{
}

pgtopde()
{
}
#endif /* s800 vs. s300 */
