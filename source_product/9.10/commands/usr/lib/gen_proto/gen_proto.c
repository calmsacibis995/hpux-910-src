static char *HPUX_ID = "@(#) $Revision: 66.2 $";
/*
	gen_proto - create mkfs proto files

*/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mknod.h>
#include <sys/param.h>
#include <stdio.h>


/* file information structure */
struct file_info {
	char	filetype[7];	/* mkfs file type		*/
	char	*name;		/* path on proto file system	*/
	char	*source_name;	/* path on "real" file system	*/
	uid_t	uid;		/* uid of file			*/
	gid_t	gid;		/* gid of file			*/
	int	size;		/* file size rounded to next 1k	*/
	int	nlink;		/* link count for file		*/
	int	inode;		/* inode number of file		*/
	int	dev;		/* device number of file	*/
	int	st_rdev;	/* major/minor device number	*/
	struct file_info *next; /* next item in cur directory	*/
	struct file_info *contents; /* list head for directory	*/
};

/* tree structure for inode/device searching */
struct node {
	struct file_info	*f;
	struct node		*left;
	struct node		*right;
};

struct file_info	*root=0;
struct node		*inode_tree=0;
int			ignore=0;
char			*strdup();


#define max(a,b) (((a)>(b))?(a):(b))


struct node *new_node(f)
struct file_info *f;
{
	struct node *n;

	n=(struct node *)malloc(sizeof(struct node));
	if (!n) { perror("malloc"); exit(1); }
	n->f=f;
	n->left=n->right=(struct node*)0;
	return n;
}


/*
	Check to see if a given device/inode pair is already part of
	the file system tree.  If yes, create a link.
*/
struct file_info *inode_match(f)
struct file_info *f;
{
	struct node *n;
	struct file_info *f2;

	if (!inode_tree) {
		inode_tree=new_node(f);
		return (struct file_info *)0;
	}
	n=inode_tree;
	while (1) {
		f2=n->f;
		if (f2->inode==f->inode && f2->dev==f->dev)
			return(f2);
		if (f->dev<f2->dev || f->inode<f2->inode) {
			if (n->left) n=n->left;
			else {
				n->left=new_node(f);
				return (struct file_info *)0;
			}
		} else {
			if (n->right) n=n->right;
			else {
				n->right=new_node(f);
				return (struct file_info *)0;
			}
		}
	}
}


/*
	Read a line from file database and fill in the fields of the
	file_info structure.

	Returns:
		success: pointer to file_info structure
		failure: 0
*/
struct file_info *readentry()
{
	char			s[MAXPATHLEN];
	char			src[MAXPATHLEN];
	char			dst[MAXPATHLEN];
	struct file_info	*f,*m;

	f=(struct file_info *)malloc(sizeof(struct file_info));
	if (!f) { perror("malloc"); exit(1); }
	if (gets(s)) {
		src[0]='\0';
		sscanf(s,"%s %s",dst,src);
		f->source_name=strdup((src[0])?src:dst);
		massage_name(dst);
		f->name=strdup(dst);
		if (!getfiletype(f)) {
			return((struct file_info *)-1);
		} else if (f->filetype[0]!='d' && f->nlink>1 && (m=inode_match(f))) {
			free(f->source_name);
			if (*root->source_name=='/') {
				f->source_name=strdup(m->name);
			} else {
				int n;

				n=strlen(root->source_name);
				f->source_name=strdup(m->name+n);
			}
			f->filetype[0]='L';	/* hard link */
		}
		return(f);
	} else {
		free(f);
		return(0);
	}
}

/*
	Get information about file.

	Returns:
		success: 1
		failure: 0
*/
getfiletype(f)
struct file_info *f;
{
	struct stat sbuf;
	char s[MAXPATHLEN];
	int n;

	if (lstat(f->source_name,&sbuf) == -1) {
		if (!ignore) {
			fputs(f->source_name,stderr);
			perror(": stat");
		}
		return(0);
	}
	switch (sbuf.st_mode&S_IFMT) {
		case S_IFREG:
			f->filetype[0]='-';break;
		case S_IFDIR:
			f->filetype[0]='d';break;
		case S_IFCHR:
			f->filetype[0]='c';break;
		case S_IFBLK:
			f->filetype[0]='b';break;
		case S_IFLNK:
			n=readlink(f->source_name,s,MAXPATHLEN);
			if (n!=-1) {
				s[n]='\0';
				f->filetype[0]='l';
				free(f->source_name);
				f->source_name=strdup(s);
			} else {
				f->filetype[0]='-';
			}
			break;
	}
	f->filetype[1]=(sbuf.st_mode&S_ISUID)?'u':'-';
	f->filetype[2]=(sbuf.st_mode&S_ISGID)?'g':'-';
	sprintf(f->filetype+3,"%3o",sbuf.st_mode&0777);
	f->uid=sbuf.st_uid;
	f->gid=sbuf.st_gid;
	f->size=roundup(sbuf.st_size,DEV_BSIZE);
	f->nlink=sbuf.st_nlink;
	f->inode=sbuf.st_ino;
	f->dev=sbuf.st_dev;
	f->st_rdev=sbuf.st_rdev;
	f->next=0;
	f->contents=0;
	return(1);
}

/*
	remove multiple slashes, trailing slashes
*/
massage_name(s)
char *s;
{
	char n[MAXPATHLEN];
	char *ps,*pn;

	ps=s;
	pn=n;
	while (*ps=='.') ps++;
	if (*ps=='\0') {
		strcpy(s,"/");
		return;
	}
	while (*ps) {
		if (*ps=='/' && *(ps-1)=='/' && ps!=s)
			ps++;
		else
			*pn++ = *ps++;
	}
	if (pn>n+1 && *(pn-1)=='/')
		pn--;
	*pn='\0';
	strcpy(s,n);
}


/*
	Initialize file system tree
*/
initialize_fs_tree(f)
struct file_info *f;
{
	struct file_info *f2;
	char s[MAXPATHLEN];

	if (strcmp(f->name,"/")==0) {
		root=f;
		return 0;
	}
	f2=(struct file_info *)malloc(sizeof(struct file_info));
	if (!f2) { perror("malloc"); exit(1); }
	strcpy(s,f->source_name);
	if (*f->source_name=='/') {
		f2->name=strdup("/");
		f2->source_name=strdup("/");
	} else {
		f2->source_name=strdup(strtok(s,"/"));
		massage_name(s);
		f2->name=strdup(s);
	}
	getfiletype(f2);
	root=f2;
	return insert_file(f,root);
}


/*
	Search for location at which datum f can be inserted
	in tree t.  Then, insert it.
	returns:
		success: 0
		failure: 1
*/
insert_file(f,t)
struct file_info *f,*t;
{
	if (!t)
		return initialize_fs_tree(f);
	while (t && !same(f->name,t->name)) {
		if (!t->next)
			return insert_next(f,t);
		t=t->next;
	}
	if (t->contents)
		return insert_file(f,t->contents);
	else if (strcmp(f->name,t->name)==0)
		/* nodes are identical */
		return 1;
	else
		return insert_contents(f,t);
}


/*
	Insert path components into tree->contents
*/
insert_contents(f,t)
struct file_info *f,*t;
{
	char s[MAXPATHLEN];
	struct file_info *f2;
	int n;

	if (strcmp(f->name,t->name)) {	/* make sure we're not already done */
		n=strlen(t->name);
		if (f->name[n]=='/') n++;
		strcpy(s,f->name);
		strtok(s+n,"/");
		if (strcmp(s,f->name)) {/* is this the last time around? */
			f2=(struct file_info *)malloc(sizeof(struct file_info));
			if (!f2) { perror("malloc"); exit(1); }
			f2->name=strdup(s);
			f2->source_name=strdup(s);
			getfiletype(f2);
			t->contents=f2;
			return insert_contents(f,f2);
		} else {
			t->contents=f;
			return 0;
		}
	}
	return 0;
}


/*
	Insert path components into tree->next
*/
insert_next(f,t)
struct file_info *f,*t;
{
	char s[MAXPATHLEN];
	struct file_info *f2;
	char *p;
	int slash=1;

	if (strcmp(f->name,t->name)) {	/* make sure we're not already done */
		p=t->name;
		while (*p) { if ((*p++)=='/') slash++; }
		strcpy(s,f->name);
		p=s;
		while (*p) {
			if (*p=='/') slash--;
			if (!slash) *p=0;
			else p++;
		}
		if (strcmp(s,f->name)) {/* is this the last time around? */
			f2=(struct file_info *)malloc(sizeof(struct file_info));
			if (!f2) { perror("malloc"); exit(1); }
			f2->name=strdup(s);
			f2->source_name=strdup(s);
			getfiletype(f2);
			t->next=f2;
			return insert_contents(f,f2);
		} else {
			t->next=f;
			return 0;
		}
	}
	return 0;
}


same(f,t)
char *f,*t;
{
	int n;

	n=strlen(t);
	if (t[n-1]=='/')
		return strncmp(f,t,n)==0;
	else
		return strncmp(f,t,n)==0 && (f[n]=='/' || f[n]=='\0');
}


char *basename(s)
char *s;
{
	char *p1,*p2;

	p1=p2=s;
	while (*p1) {
		if (*p1++=='/')
			p2=p1;
	}
	return(p2);
}


visit(t,level)
struct file_info *t;
int level;	/* level */
{
	if (!t) return;
	switch(t->filetype[0]) {
		case 'd':
			if (level)
				printf("%*s%s\t%s %3d %3d\n",level*5,"",basename(t->name),t->filetype,t->uid,t->gid);
			else
				printf("%*s%s %3d %3d\n",level*5,"",t->filetype,t->uid,t->gid);
			break;
		case 'b':
		case 'c':
			printf("%*s%s\t%s %3d %3d %d %#x\n",level*5,"",basename(t->name),t->filetype,t->uid,t->gid,major(t->st_rdev),minor(t->st_rdev));
			break;
		case '-':
		case 'l':
		case 'L':
			printf("%*s%s\t%s %3d %3d %s\n",level*5,"",basename(t->name),t->filetype,t->uid,t->gid,t->source_name);
			break;
	}
	if (t->filetype[0]=='d') {
		level++;
		visit(t->contents,level);
		level--;
		printf("%*s$\n",level*5,"");
	}
	visit(t->next,level);
}


/*
	Recalculate sizes for all directories based on the
	number of directory entries used.
*/
int recalculate_sizes(t)
struct file_info *t;
{
	if (!t) return 0;
	if (t->filetype[0]=='d') {
		t->size=roundup(24+recalculate_sizes(t->contents),DEV_BSIZE);
	}
	return max(14,strlen(basename(t->name))+1)+recalculate_sizes(t->next);
}


/*
	Traverse fs tree to get size of fs
*/
int get_fss(t)
struct file_info *t;
{
	if (!t) return 0;
	return t->size+get_fss(t->next)+get_fss(t->contents);
}


usage()
{
	fputs("usage: gen_proto [-i]	# Ignore missing files.\n",stderr);
#ifdef hp9000s300
	fputs("\t[-b bootfile]	# Specify bootfile.\n",stderr);
#endif
	fputs("\t[-o nblks]	# Specify nblks additional 1k file system blocks.\n",stderr);
	fputs("\t[-p pcnt]	# Specify pcnt% overhead file system space.\n",stderr);
	fputs("\t[-s nblks]	# Specify absolute file system size of nblks 1k blocks.\n",stderr);
	fputs("\t\t\t# Only one of the -o, -p, or -s options may be specified.\n",stderr);
	exit(1);
}


main(argc,argv)
int argc;
char *argv[];
{
	struct file_info *f;
	char c;
	extern char *optarg;
	extern int optind,opterr;
	char *bootfile;
	int percent=0;
	int overhead=0;
	int fss=0;
	int size,bsize,nbkup_sb;

#ifdef hp9000s300
	bootfile="/etc/boot";
	while ((c=getopt(argc,argv,"ib:o:p:s:"))!=EOF)
#else
	bootfile="\"\"";
	while ((c=getopt(argc,argv,"io:p:s:"))!=EOF)
#endif
		switch(c) {
			case 'b':
				bootfile=optarg;
				break;
			case 'o':
				if (percent || fss) {
					fputs("-p or -s already specified\n\n",stderr);
					usage();
				}
				overhead=atoi(optarg);
				if (overhead<0) {
					fprintf(stderr,"%s: absurd overhead of %d blocks\n",argv[0],overhead);
					exit(1);
				}
				break;
			case 'p':
				if (overhead || fss) {
					fputs("-o or -s already specified\n\n",stderr);
					usage();
				}
				percent=atoi(optarg);
				if (percent<0) {
					fprintf(stderr,"%s: absurd overhead of %d%%\n",argv[0],percent);
					exit(1);
				}
				break;
			case 's':
				if (overhead || percent) {
					fputs("-o or -p already specified\n\n",stderr);
					usage();
				}
				fss=atoi(optarg);
				if (fss<0 || fss<10) {
					fprintf(stderr,"%s: absurd file system size of %d blocks\n",argv[0],fss);
					exit(1);
				}
				break;
			case 'i':
				ignore++;
				break;
			case '?':
				usage();
				break;
		}

	while (f=readentry()) {
		if (f!=(struct file_info *)-1)
			insert_file(f,root);
	}
	printf("%s\n",bootfile);
	recalculate_sizes(root);
	size=get_fss(root);
	size+=16384;	/* add for boot block and super-block */
	bsize=roundup(size,DEV_BSIZE)/DEV_BSIZE;
	nbkup_sb=(bsize+511)/512;	/* # of backup super blocks */
	bsize += 8*nbkup_sb;	/* 8k each */
	bsize += 100;		/* inodes, minfree, etc */
	if (percent)
		bsize+=(bsize*percent)/100;
	else if (overhead)
		bsize+=overhead;
	else if (fss)
		bsize=fss;
	printf("%d\n",bsize);
	visit(root,0);
}
