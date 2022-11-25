#include <sys/types.h>
#include <sys/param.h>
#include <sys/fs.h>
#include <sys/inode.h>
#include <sys/ino.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <nlist.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/conf.h>
#ifdef hp9000s800
#include <sys/vmmac.h>
#endif
#include <regex.h>
#include <dirent.h>

extern char *bread();
extern struct cg *read_cg_block();


#define MAXMOUNT 50

struct opendiskstruct 
{
    char path[100], rpath[100];
    dev_t dev, rdev;
    int fd, rfd;
    struct fs *fs;
    
};

#define DIRECT 1
#define INDIRECT 2
#define DOUBLE_INDIRECT 3
#define TRIPLE_INDIRECT 4

struct unrm_table_t 
{
    dev_t dir_dev;
    int dir_inum;
    char fname[MAXNAMLEN+1];
    struct dinode din;
};    
