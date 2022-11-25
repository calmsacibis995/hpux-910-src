.nr Cl 4
.nr Ej 1
.ds HF 3 3 2 2 2 2 2
.ds HP 12 12 10 10 10 10 10
.PH "'Development Environment and Build Process"
\" .EF "'HP Internal Use Only' - \\\\nP - ' \\\\n(mo/\\\\n(dy/\\\\n(yr'"
\" .OF "'HP Internal Use Only' - \\\\nP - ' \\\\n(mo/\\\\n(dy/\\\\n(yr'"
.PF "'HP Internal Use Only' - \\\\nP - ' \\\\n(mo/\\\\n(dy/\\\\n(yr'"
.nr Hu 1	\" define Appendix header and format
.nr a 0
.de aH
.nr a +1
.PH "'Development Environment and Build Process''Appendix \\na'"
.HU "\\$1"
..
.ce 2
DEVELOPMENT ENVIRONMENT AND BUILD PROCESS
S300/S800 NFS PROJECT
.sp 6
.ce 1
COLORADO NETWORKS DIVISION
.ce 2
$Date: 91/11/19 14:27:56 $
$Revision: 1.5.109.1 $
.sp 3
.ce 2
Authors: Cristina Mahon and John Dilley
.sp 6
.ad
This paper describes the software development environment and
build process used by the NFS 300 and NFS 800 development groups
at Colorado Networks Division.  The purpose of this paper is to
document this process so that new hires coming into the project
will be able to find a single source for all the information
needed.  Also, it will allow for a smooth transition between 
different people responsible for the process.  
.sp 
The development environment (DE) was based on SDCS (Software Development
Control System) developed by Karl Jensen and used by the development group 
for the HP9000 s500 kernel.  The concepts underlying this system are
very adaptable and transportable to other projects and HP-UX systems.
.sp
.na
.bp
.H 1 "INTRODUCTION"
.sp
.ad
The DE is a means of coordinating the development 
of software when several people work on the same or closely related
pieces of code.  It also provides a controlled environment for the
different tools required (for example, cc, include files, etc.).
It consists of some main directories which contain all the files that 
we develop with, SCCS directories (it could be RCS directories) and several 
personal development environments that are links to the main directories.  
Several scripts manipulate the links when we check out and check in the source 
files.  Other scripts allow us to build those files.
.sp
The build process attempts to automate as many of the every day
tasks as possible so that more time can be spent on the actual code
development.  It is fully integrated into the DE.
.na
.sp
.H 2 "Advantages"
.sp
.ad
The NFS development project has been using the DE since the beginning 
of the project and has found several advantages to it.  Among those advantages 
we can mention:
.sp
.AL
.LI
Controlled development environment:
.sp
.BL
.LI
The DE allows us to control all the tools we need to implement
the software.  For example, we are building with the most up to date
include files, libraries and C compilers.  We can also revert 
to a previous version and rebuild it from scratch if necessary,
since we know what pieces went into it and what tools were
used to build it.  The DE is self-contained.
.LI
Multiple development environments can easily coexist on the
same system.  We can have a frozen version of the software
on which other groups depend coexisting with an unfrozen version
on which we are making changes.  If some time in the future we
need to go back to a previously released version, it can exist
side by side with the current release.
.LI
We can turn on or off any part of our build process.  
This is particularly helpful when we want to freeze part
of the product but not another part, like the kernel but
not the user level commands.  It also allows us to decide whether
we want to pick up the most up to date libc and include files
to use what we have for the moment.
.LI
The development environment keeps track of all changes made
to all modules by date.  So before a release or between releases
it is much easier to find out exactly what modules have changed,
who made the change and why the change was made.
.LE
.LI
Quality Improvement:
.sp
.BL
.LI
The night after a piece of a software system has been changed,
it becomes visible to all other developers and it is built
into the system.  That way if a change to one piece of the
software affects other parts of that software that will be noticed
immediately instead of only immediately before a release.
.LI
We get maximum exposure on the code since as soon as it is checked-in
other people start compiling with it and integrating it with their
own pieces of code.
.LI
The DE provides an extra incentive for people to make sure their
modules don't break other modules since if there is a problem
the day after they make the change all the other developers that
interface with that module will see the problem and complain about it.
.LE
.LI
Productivity Improvement:
.BL
.LI
Since a module is integrated with all other modules the day after
it is checked into the source control system, any defects found 
can be more easily fixed, since the change that caused it was
made a short time ago and is probably still clear in the developer's mind.
.LI
All modules are built every night.  So if the developer
wants to build a module that depends on other modules, he does not
have to build all of them, but only the one that he has been making changes to.
.LI
The DE simplifies the process of releasing the product, since
the product is continuously being integrated.  There is no need
for all the pieces to be put together for the first time in
x weeks and tested as such before a release, the pieces are 
integrated \fBevery\fR night.
.LE
.LI
Reduced use of resources:
.BL
.LI
The DE uses links to maintain the different pieces of the
software organized.  For that reason, it saves disc space
that would otherwise be used to maintain separate copies 
of the software.
.LI
During the day people working on the kernel for example,
did not have to rebuild all the parts of the kernel
they needed and therefore didn't slow down the machine
for everyone else.  Instead all the parts they needed
were already built overnight (when the machine is normally
idle) and were available to be linked with whatever other
changes needed to be tested during the day.
.LE
.sp 2
The development environment has proved to be a very flexible
tool that adapted well to all the changes that were required during
the course of this project.  It saved us a significant amount of
effort in both managing and building the NFS commands and kernel.
.na 
.sp
.H 2 "Restrictions"
.sp
.ad
The current implementation of the DE requires it
to reside on a single file system to take advantage of file linking.
All developers are members of a single user group, in this case called
"nerfs".  Access to the files under development is limited to the group
nerfs only to protect the development effort from changes made outside
the DE framework.  The DE maintains the correct file permissions independent 
of the developers' umask values.
.sp
Each developer has a unique user name and ID.  In addition, there is one
special developer called "nfsmgr".  Nfsmgr manages and administers the 
DE and it owns most of the files.
.na
.sp
.H 2 "Development Environment Organization"
.sp
.ad
The entire software development environment resides in a single directory.
The name of that directory is kept in the system wide variable $Nfs and
all scripts key on that variable to perform their different functions.
$Nfs is defined at a system wide level on /etc/profile and /etc/csh.login.
By having the scripts key on $Nfs we have an easy method of changing 
the root of the development environment without having to change any
other components of the system.  This permits development on archived 
versions of NFS.  For example, we were using /nfs as our root directory,
but during the freeze period for EB2 we wanted to keep working on fixes that 
were needed while still being able to go back to EB2 frozen bits.  To achieve
that we changed $Nfs from /nfs to /nfs/working were we currently reside.
.sp
Under the main directory for the development environment we have the following
directories (ll $Nfs):
.sp
.nf
total 44
   2 -r--r--r--   1 nfsmgr   nerfs       1003 Sep 18 17:05 Makefile
   2 drwxrwx---   3 nfsmgr   nerfs       1024 Jun 23 22:04 SAVEDIR/
   6 drwxrwxr-x   4 nfsmgr   nerfs       3072 Oct  7 09:51 bin/
   2 drwxrwxr-x   2 nfsmgr   nerfs         64 Jun 22 10:29 checkout/
   2 drwxrwxr-x  10 nfsmgr   nerfs       1024 Oct  7 05:12 cmds/
   2 drwxrwxr-x  15 nfsmgr   nerfs       1024 Jul 24 12:40 develop/
   2 drwxrwxr-x   4 nfsmgr   nerfs       1024 Oct  6 13:33 doc/
   4 drwxrwxr-x  11 nfsmgr   nerfs       2048 Oct  4 11:58 include/
   2 drwxrwxr-x   5 nfsmgr   nerfs       1024 Aug  6 13:03 kernel/
   4 drwxr-x---   3 nfsmgr   nerfs       2048 Oct  7 10:52 log/
   2 drwxrwxr-x   8 nfsmgr   nerfs       1024 Oct  2 09:31 man/
   2 drwxrwxr-x   2 nfsmgr   nerfs       1024 Sep 21 12:01 releases/
   2 drwxrwxr-x  12 nfsmgr   nerfs       1024 Sep 18 14:48 sccs/
   2 drwxrwxr-x   3 nfsmgr   nerfs       1024 Jul 31 12:41 system/
   2 drwxrwxr-x   7 nfsmgr   nerfs       1024 Oct  6 15:04 test/
   2 drwxrwxrwx   2 nfsmgr   nerfs       1024 Oct  6 13:55 tmp/
.fi
.sp
One of the directories under the main directory is sccs.  We are currently
using SCCS as our source control system.  To use a different source control
system all that would be required would be to change certain scripts.  The
every day user of the development environment is not required to have any
knowledge about the source control system since that system is hidden 
from him by front-end scripts.
.sp
The sccs files reside under $Nfs/sccs:
.sp
.nf
total 40
   8 drwxrwxr-x   4 nfsmgr   nerfs       4096 Oct  7 09:51 bin/
   2 drwxrwxr-x   9 nfsmgr   nerfs       1024 Sep 18 15:02 cmds/
   4 drwxrwxr-x   4 nfsmgr   nerfs       2048 Oct  6 13:33 doc/
   4 drwxrwxr-x  11 nfsmgr   nerfs       2048 Aug  7 17:18 include/
   2 drwxrwxr-x   4 nfsmgr   nerfs       1024 Aug  6 12:59 kernel/
   2 drwxrwxr-x   2 nfsmgr   nerfs       1024 Sep  8 17:39 log/
   2 drwxrwxr-x   8 nfsmgr   nerfs       1024 Oct  2 09:31 man/
   2 drwxrwxr-x   2 nfsmgr   nerfs       1024 Sep 21 12:01 releases/
   2 drwxrwxr-x   3 nfsmgr   nerfs       1024 Jul 31 12:41 system/
   2 drwxrwxr-x   3 nfsmgr   nerfs       1024 Jun 22 11:10 test/
.fi
.sp
Note that the directories under $Nfs/sccs also exist in $Nfs.  This is 
true for the entire $Nfs/sccs file tree.  The SCCS files retain a history of
their corresponding $Nfs files, which are called the "working" files.
Working files are the latest revisions get-ed from each SCCS file. 
Some of the directories under $Nfs don't have a counter-part under $Nfs/sccs.
One specific directory $Nfs/sccs/hp-ux does not have a counter-part under
$Nfs.  The reason for that is that $Nfs/hp-ux was a directory that we used.
However, after a while we did not need that directory for constant development
anymore, but did not want to loose the history of the changes we made to some
of the files and therefore kept the sccs directory.
.sp
The source files for the product under development reside under two directories,
cmds and kernel.  As the name implies cmds contain the user level code for
NFS while kernel contains the kernel code.  The man directory contains our
on-line versions of the man pages.  The man pages are owned by the manual
writer for most of the project.  The doc directory contains the documents for
the project like ERS, IRS and life cycle.  The include directory contains 
the user level include files required by the commands to compile.  These
are both NFS specific include files and system include files.  The system
include files are automatically updated whenever they change.  The $Nfs/include
directory is the directory used by all the compiles.  That way we do not depend
on whatever include files happen to exist on the system at the time we build
the commands.  This allows us an extra level of control over our development
environment.  The test directory contains some performance tests that have
not yet been folded into the test scaffold.  The SAVEDIR directory keeps the
previous night's files used to build the kernel so that we can always easily
back track to the previous night's build.
.sp
The bin directory contains all the scripts needed to use the DE as
well as the build process.  The log directory contain the log files that
both store the results of the different scripts when they run but also
control whether those scripts will run or not..
.sp
.nf
total 1704
  12 -r--r--r--   2 nfsmgr   nerfs       5309 Oct  7 09:51 800_files
   8 -rwxr-xr-x  13 nfsmgr   nerfs       3923 Sep 18 16:29 800_subm*
   8 -r--r--r--  13 nfsmgr   nerfs       3135 Sep 21 13:44 800sub_files
  10 -rwxr-xr-x  13 nfsmgr   nerfs       4240 Aug  3 09:28 800transfer*
  54 -r-xr-xr-x  91 nfsmgr   nerfs      25925 Jun 22 10:26 Admin*
   6 -rw-rw-r--  13 jad      nerfs       2553 Sep 10 17:11 DESCRIPTIONS
   2 -r-xr-xr-x  13 nfsmgr   nerfs        185 Sep 18 15:46 Debug_all*
   2 -r-xr-xr-x  13 nfsmgr   nerfs        181 Sep 18 15:46 Debug_cmd*
  54 -r-xr-xr-x  91 nfsmgr   nerfs      25925 Jun 22 10:26 Delta*
  66 -r-xr-xr-x  13 nfsmgr   nerfs      32044 Aug 14 17:10 Get*
   4 -rwxr-xr-x  12 nfsmgr   nerfs       1284 Sep 15 14:30 MAKE*
   2 -r-xr-xr-x  13 nfsmgr   nerfs        541 Sep 18 15:47 Make*
   2 -rwxr-xr-x  13 nfsmgr   nerfs        380 Sep  9 12:37 Make_all*
   2 -rwxr-xr-x  13 nfsmgr   nerfs        118 Sep 10 17:06 Make_bfa*
   2 -rwxr-xr-x  13 nfsmgr   nerfs        888 Sep  9 12:38 Make_cmd*
   2 -rwxr-xr-x  13 nfsmgr   nerfs        776 Sep  9 12:39 Make_cmd800*
   2 -r-xr-xr-x  13 nfsmgr   nerfs        615 Sep 18 15:46 Make_install*
   2 -rwxr-xr-x  13 nfsmgr   nerfs        719 Sep  9 12:39 Make_kern_2*
   4 -rwxr-xr-x  13 nfsmgr   nerfs       1691 Sep 25 09:42 Make_kernel*
  54 -r-xr-xr-x  91 nfsmgr   nerfs      25925 Jun 22 10:26 Prs*
  54 -r-xr-xr-x  91 nfsmgr   nerfs      25925 Jun 22 10:26 Rmdel*
  54 -r-xr-xr-x  91 nfsmgr   nerfs      25925 Jun 22 10:26 Sccsdiff*
  54 -r-xr-xr-x  91 nfsmgr   nerfs      25925 Jun 22 10:26 Unget*
   2 -rwxr-xr-x  13 nfsmgr   nerfs        502 Sep  9 12:40 backup*
   8 -rwxr-xr-x  13 nfsmgr   nerfs       3990 Jul 20 17:32 bfatrans*
   4 -r-xr-xr-x  13 nfsmgr   nerfs       1193 Sep 18 15:46 changes*
  58 -r-xr-xr-x  13 nfsmgr   nerfs      28400 Jun 22 10:26 checkdate*
   2 drwxrwxr-x   2 nfsmgr   nerfs       1024 Jun 22 10:29 checkdate.src/
   2 -r-xr-xr-x  13 nfsmgr   nerfs        703 Sep 18 15:46 checked_out*
  20 -rwxr-xr-x  13 nfsmgr   nerfs      10168 Sep 21 12:03 checkin*
  10 -rwxr-xr-x  13 nfsmgr   nerfs       4890 Aug 14 17:15 checkout*
  10 -rwxr-xr-x  13 nfsmgr   nerfs       4722 Sep  9 12:41 create_rel*
   2 drwxrwxr-x   2 nfsmgr   nerfs       1024 Jul 13 13:56 d.rmtrcs/
   2 -r-xr-xr-x  13 nfsmgr   nerfs        185 Sep 18 15:46 debug_all*
   2 -r-xr-xr-x  13 nfsmgr   nerfs        185 Sep 18 15:46 debug_cmd*
   4 -rwxr-xr-x  13 nfsmgr   nerfs       1597 Sep 17 07:44 do_netunam*
   2 -rwxr-xr-x  13 nfsmgr   nerfs        485 Sep  9 12:42 fullbackup*
   4 -rwxr-xr-x  13 nfsmgr   nerfs       1534 Sep  9 12:42 get_800inc*
   4 -rwxr-xr-x  13 nfsmgr   nerfs       2000 Sep  9 12:43 get_all*
   2 -rwxr-xr-x  13 nfsmgr   nerfs        751 Sep  9 12:43 get_include*
   8 -rwxr-xr-x  13 nfsmgr   nerfs       3906 Sep  9 12:43 get_kernel*
   6 -rwxr-xr-x  13 nfsmgr   nerfs       2829 Sep  9 12:44 get_kobjects*
   6 -rwxr-xr-x  13 nfsmgr   nerfs       2641 Sep  9 12:44 get_libc*
   2 -r-xr-xr-x  13 nfsmgr   nerfs        835 Sep 18 15:46 get_net*
   4 -rwxr-xr-x  13 nfsmgr   nerfs       1769 Oct  1 09:08 get_smelter*
   2 -rwxr-xr-x  39 nfsmgr   nerfs        762 Jun 23 10:03 in*
   2 -rwxr-xr-x  13 nfsmgr   nerfs        748 Sep  9 12:45 inc_depend*
  24 -rwxr-xr-x  13 nfsmgr   nerfs      10241 Aug  3 10:03 include.mk*
   6 -r-xr-xr-x  13 nfsmgr   nerfs       2823 Sep 18 15:46 install*
   4 -r-xr-xr-x  13 nfsmgr   nerfs       1256 Sep 18 15:47 listtd*
   2 -r-xr-xr-x  13 nfsmgr   nerfs        691 Sep 18 15:46 make_all*
   2 -r-xr-xr-x  13 nfsmgr   nerfs        339 Sep 18 15:46 make_bfa*
   2 -r-xr-xr-x  13 nfsmgr   nerfs        960 Sep 18 15:46 make_cmd*
   6 -rwxr-xr-x  13 nfsmgr   nerfs       3047 Sep  9 12:46 make_cscope*
   8 -rwxr-xr-x  13 nfsmgr   nerfs       3788 Sep  9 12:46 make_kbfa*
   8 -rwxr-xr-x  13 nfsmgr   nerfs       3858 Sep 25 17:33 make_ktrig*
   8 -rwxr-xr-x  13 nfsmgr   nerfs       3532 Sep  9 12:47 make_nfsklib*
   2 -r-xr-xr-x  13 nfsmgr   nerfs        521 Sep 18 15:47 make_path*
   4 -r-xr-xr-x  13 nfsmgr   nerfs       1688 Sep 18 15:46 maketd*
   2 -rwxr-xr-x  39 nfsmgr   nerfs        762 Jun 23 10:03 out*
  26 -rwxr-xr-x  13 nfsmgr   nerfs      11473 Oct  6 10:59 overnight*
   4 -r-xr-xr-x  13 nfsmgr   nerfs       1146 Sep 18 15:47 rmt_include*
   4 -rwxr-xr-x  13 nfsmgr   nerfs       1057 Sep  8 10:56 rmtci*
  80 -rwxrwxr-x  13 nfsmgr   nerfs      39060 Jul 13 13:56 rmtrcs*
   2 -rwxr-xr-x  13 nfsmgr   nerfs        913 Jul 10 09:06 sdiff*
   4 -rwxr-xr-x  13 nfsmgr   nerfs       1879 Oct  5 14:02 setup*
   8 -rwxr-xr-x  13 nfsmgr   nerfs       3314 Sep  9 12:48 setup_rel*
   2 -rwxr-xr-x  13 nfsmgr   nerfs        475 Sep  9 12:48 takedown*
  12 -rwxr-xr-x  13 nfsmgr   nerfs       5363 Sep 18 12:28 transfer*
   6 -rwxr-xr-x  13 nfsmgr   nerfs       2675 Aug 14 17:16 uncheckout*
  88 -r-xr-xr-x  13 nfsmgr   nerfs      43902 Jun 22 10:26 unifdef*
  86 -r-xr-xr-x  13 nfsmgr   nerfs      43008 Jul 16 13:19 unifdef.800*
   2 -rwxr-xr-x  39 nfsmgr   nerfs        762 Jun 23 10:03 unout*
   8 -rwxr-xr-x  13 nfsmgr   nerfs       3614 Aug 17 12:16 up_release*
  16 -r-xr-xr-x  13 nfsmgr   nerfs       7794 Sep 18 15:46 vdiff*
 188 -rwxrwxr-x  13 nfsmgr   nerfs      95232 Jun 22 10:26 vmake*
   6 -rwxr-xr-x  13 nfsmgr   nerfs       2216 Sep  9 12:49 wait_for*
   4 -r-xr-xr-x  13 nfsmgr   nerfs       2042 Sep 18 15:46 whohas*
  92 -r-xr-xr-x  13 nfsmgr   nerfs      45616 Jun 22 10:26 workingcpp*
.sp
.fi
As the directory listing above indicates, there are a lot of scripts and
commands used by the DE and build process.  Part of the reason for some many
commands is that the build process for example is formed by different parts
that can be turned on or off independently.  This allows us for example to
build commands independently of kernel or to retrieve of not the most up to date
libc for our builds.  The file DESCRIPTIONS contains a brief description of
the commands found in $Nfs/bin.
.sp
.na
.H 2 "User development environments"
.sp
.ad
So far the system described above is suitable for development by a single
user, the nfs manager nfsmgr.  The nfsmgr can operate on the working files.
But the other developers are only permitted read access to the working files, 
so they can't edit them.  The idea now is to provide any number of separate
development environments, one for each developer, where one developer's
experimental code doesn't interfere with another developers efforts, but
still can profit from the most up to date code from the other developers.
One way of doing that would be to regularly make copies of the working
files for each developer, but this approach consumes disc space.  A better
approach is to make many links to a single copy of each working file.  This 
has the advantage of saving disc space and therefore allowing as many
developers to access the code as necessary.  However, it means also that
the code has to reside in a single disc partition to allow for links.
.sp
The individual development environments are grouped together in $Nfs/develop.
Here is an example of its contents:
.sp
.nf
total 26
   2 drwxr-xr-x  12 chm      nerfs       1024 Oct  7 07:24 chm
   2 drwxr-xr-x  10 dae      nerfs       1024 Oct  7 09:00 dae
   2 drwxr-xr-x  14 dds      nerfs       1024 Oct  7 07:34 dds
   2 drwxr-xr-x  12 dvs      nerfs       1024 Oct  7 07:24 dvs
   2 drwxr-xr-x  11 gmf      nerfs       1024 Oct  7 07:17 gmf
   2 drwxr-xr-x  14 jad      nerfs       1024 Oct  7 07:25 jad
   2 drwxr-xr-x  11 jason    nerfs       1024 Oct  7 07:25 jason
   2 drwxr-xr-x  11 jbl      nerfs       1024 Oct  7 07:25 jbl
   2 drwxr-xr-x  13 mak      nerfs       1024 Oct  7 07:33 mak
   2 drwxr-x---  13 mikey    nerfs       1024 Oct  7 06:42 mikey
   2 drwxr-xr-x  11 mjk      nerfs       1024 Oct  7 07:26 mjk
   2 drwxr-xr-x  13 nfsmgr   nerfs       1024 Oct  7 07:33 nfsmgr
   2 drwxr-xr-x   4 toolbox  nerfs       1024 Jun 22 13:42 toolbox
.fi
.sp
The name of each directory in $Nfs/develop corresponds to the name of the
developer who owns it.  It's easy to add a development directory.
The new developer (e.g. "newguy") logs on and runs the command $Nfs/bin/setup
which creates $Nfs/develop/newguy and all the files underneath it.  The
new developer should also add $nfs/bin to his or her path so that the
DE commands become easily available.  The variable $nfs is defined in
/etc/profile and /etc/csh.login to be $Nfs/develop/$LOGNAME.  This
variable is setup for all users logging into the system and allows the
developers to quickly access their personal development environment.
.sp
Each development environment has a complete set of links to the working files,
which is customized by the developer's experimental files.  Here is an 
example of a development environment.  This one belongs to chm:
.sp
.nf
drwxrwxr-x   3 chm      nerfs       3072 Jun 27 12:53 bin
drwxr-xr-x   4 chm      nerfs       1024 Jun 27 12:53 checkout
drwxrwxr-x  10 chm      nerfs       1024 Jun 27 13:01 cmds
drwxrwxr-x   4 chm      nerfs       1024 Jun 27 13:18 doc
drwxrwxr-x  11 chm      nerfs       1056 Jun 27 13:06 include
drwxr-x---   2 chm      nerfs       1024 Jun 22 13:40 log
drwxrwxr-x   8 chm      nerfs       1024 Jun 27 13:08 man
drwxr-xr-x   2 chm      nerfs         64 Jun 27 12:53 tmp
.sp
.fi
chm's current experimental files reside in checkout.  Here is an example
(output of ll -R $nfs/checkout for user chm):
.sp
.nf
drwxr-xr-x   3 chm      nerfs       1024 Jun 27 12:53 cmds
drwxrwx---   2 chm      nerfs       1024 Jun 27 12:55 doc
.sp
/nfs/working/develop/chm/checkout/cmds:
drwxr-xr-x   3 chm      nerfs       1024 Jun 27 12:53 libc
.sp
/nfs/working/develop/chm/checkout/cmds/libc:
drwxr-xr-x   2 chm      nerfs       1024 Jun 27 12:53 net
.sp
/nfs/working/develop/chm/checkout/cmds/libc/net:
-rw-r--r--   2 chm      nerfs       8274 Jun 27 12:53 gtnetgrent.c
.sp
/nfs/working/develop/chm/checkout/doc:
-rw-rw----   2 chm      nerfs      14296 Jun 27 13:05 readme
.fi
.sp
Chm is working on two files: cmds/libc/net/gtnetgrent.c and doc/readme.
Note that checkout and all of the files in checkout belong to and are writable
by chm.  When chm's personal development environment is setup the files in
$Nfs/develop/chm/checkout, if there are any, will take precedence over the 
equivalent working files.  Overnight each developer's personal
development environment will be removed except for the files in log,
checkout and any other nonworking directories.  So, developer's should
not keep files under development anywhere else other than in their checkout
directory if they don't want to loose them overnight.  The next chapter will 
explain some of the commands that a developer will need to use on a regular 
basis.
.na
.bp
.H 1  "EVERY DAY DEVELOPMENT ENVIRONMENT COMMANDS"
.ad
.sp
Of all the commands available in the $Nfs/bin directory only some 
are needed by the developers on a regular basis.  These commands are:
checkout, checkin, uncheckout, checked_out, sdiff, in, out, unout and whohas.
.sp
Before editing a file, a developer must first check it out from the code 
control system (SCCS for example).  This prevents two developers from working 
on the same file simultaneously because SCCS (or RCS) maintain locks on the 
files.  In the example on the previous chapter, chm entered the 
"checkout cmds/libc/net/gtnetgrent.c".  Checkout is a shell script
which acts as a human interface to SCCS.  It remembers to prepend the 
needed "s." to gtnetgrent.c when it gets the file from SCCS, for example,
so the developer doesn't have to do that.  It also manages file linking 
between $Nfs, $nfs, and $nfs/checkout.  It creates directories as needed,
with correct file ownership and permissions.  If the file is already
checked out then checkout shows this:
.sp
.nf
> checkout cmds/etc/inetd.c
Checking out "cmds/etc/inetd.c":
3.1
ERROR [/nfs/working/cmds/etc/s.inetd.c]: being edited: '3.1 3.2 jad 87/06/27 11:41:5'
.fi
.sp 2
In this case chm can talk to jad (the developer who has inetd.c checked 
out already) to find out when jad will be done with it.  If jad has not made
any changes to inetd.c yet he can back out his checkout by running 
"uncheckout cmds/etc/inetd.c", which makes inetd.c available for chm to check 
out.
.sp
.nf
> uncheckout cmds/etc/inetd.c
Unchecking "cmds/etc/inetd.c":
3.2
.fi
.sp 2
If chm thought that someone had a file checked out and wanted to verify 
that, without actually trying to check out that file, she could use the command
"whohas". 
.sp
.nf
> whohas cmds/etc/inetd.c
/nfs/working/bin/whohas:
jad has "cmds/etc/inetd.c" checked out
.fi
.sp 2
If the file is not already checked out, then checkout works like this:
.sp
.nf
> checkout cmds/etc/inetd.c
Checking out "cmds/etc/inetd.c":
3.1
new delta 3.2
2894 lines
.fi
.sp 2
Now $Nfs/develop/chm/checkout/cmds/etc/inetd.c exists and is linked to
$Nfs/develop/chm/cmds/etc/inetd.c.  Chm can edit it, compile it, and test
it without affecting the other developers by setting her current directory
to $nfs (which for chm is equivalent to $Nfs/develop/chm).  All the other
developers still have in their own personal development environments
a copy of cmds/etc/inetd.c.  That copy is not modified by the changes
chm is making on her own copy on her development environment since her copy
is only linked to another version on her checkout directory.  The working
copy under $Nfs/cmds/etc/inetd.c is linked to the copies of all the
other developers.
.sp
Developers can see what files they have checked out by running "checked_out":
.sp
.nf
> checked_out 
Files in /nfs/working/develop/chm/checkout:
cmds/etc/inetd.c
doc/readme
.fi
.sp 2
After testing shows that the experimental edits work properly, chm can
release the file to SCCS and the working files.  That is done by the
script "checkin":
.sp
.nf
> checkin cmds/etc/inetd.c
File to be updated is "$nfs/checkout/cmds/etc/inetd.c" - Comments?
Added support for positional parameters within print statements (NLS).
3.2
10 lines added
10 lines deleted
2884 lines unchanged
.fi
.sp 2
Checkin recognizes whether the file being checked in already exists in
SCCS.  If it does, then check in deltas the new revision of the file into SCCS.
If it doesn't, then check in knows how to create directories as needed and admin
the file into SCCS.  The SCCS header for the file should be added by hand
(it used to be done automatically unless otherwise indicated) since there are
certain conventions being used in the project for SCCS headers.  The newly
released file appears in all of the other developers' directories the following
morning due to the overnight build process.  If the checked in file is part
of a set of experimental files which must all be added to the system 
simultaneously (e.g. a procedure with a new parameter, which requires edits
to every source file that calls the procedure), the developer has all day
to release the related files.  This simplifies coordinated file check-ins 
between developers, too.
.sp
Something very important to remember about checkin is that the file that
is actually put back into SCCS is the file that resides on the developers
checkout directory.  So, if you, as a developer, happened to keep a separate
copy of a file you were changing outside the development directory (and had 
already checked out) and now you want to make that separate copy the real file 
you are going to check in, do not move the outside file on top of the file in 
$nfs since that will break the link between the file in $nfs and $nfs/checkout.
You can either copy the outside file on top of $nfs or $nfs/checkout files 
(that will maintain the links), or mv the side file on top of $nfs/checkout 
file.  The move will still break the link, but the changes will be on the file 
in $nfs/checkout and when you say "checkin" those changes will go in.  As a 
general rule it is not good to make changes to files outside the development 
environment.
.sp
When you have several files checked out on the same directory or want to
check in all the files you currently have checked out you can take the
following shortcut:
.nf
.sp
> checkin `checked_out`
.br
to check in \fBall\fR the files you have checked out or
.sp
> checkin `checked_out cmds/etc`
.br
for example, to check in all files that are checked out in the directory
cmds/etc.
.fi
.sp 
When you are checking in more than one file and you want to use the same
comments for all of them, you can enter the first comment and then the next
time the checkin script asks for a comment just type * and all the other
files you will be checking in will have the same comment as the first one.
For example,
.nf
.sp
> checkin cmds/etc/inetd.c cmds/libc/net/gtnetgrent.c
File to be updated is "$nfs/checkout/cmds/etc/inetd.c" - Comments?
> Added support for positional parameters within print statements (NLS).
3.2
10 lines added
10 lines deleted
2884 lines unchanged
File to be updated is "$nfs/checkout/cmds/libc/net/gtnetgrent.c" - Comments?
> *
Added support for positional parameters within print statements (NLS).
3.2
1 line added
38 lines unchanged
.fi
.sp
What you had to type on the example above is preceded by a ">".  The
second file used the same comment as the first file checked in.
.sp
Checkin maintains a log of all updates in $Nfs/log/update.  It looks like
this:
.nf
.sp
87.06.22 15	dds	bin/make_nfsklib
Removed hpfcls from list of target machines for nightly distribution for EB2.
87.06.22 15	jbl	doc/product_req
updated 800 quality goal to .1080 SRs/KNCSS, assuming 3/88 shipments.
87.06.23 10	jad	bin/in
fixed variable DEVELOP, use double quotes so $Nfs gets expanded
.fi
.sp
The log file shows who made what change to what file when.  This is helpful
when tracking down problems in the system.  The comments tie checkins
to a bug reporting mechanism.
.sp
The nfsmgr can block updates to the working files by write protecting group
access to the log file $Nfs/log/update.  This log file belongs to nfsmgr,
so the nfsmgr can continue to update the working files even when it is closed
to group checkins.  If the working files become disorganized in some way the
nfsmgr can group write protect the checkin log file and then put the working
files back in order.  This feature is also useful when releasing the developed
software product, to prevent errors caused by updates in the middle of the
build.
.sp
If the development environment has several directories levels it might get 
tiresome to type all the directories before the file you want to check in 
or out.  To avoid that John Dilley wrote some front-end scripts that can
be used when you are in your own development environment and in the 
directory that contains the file you want to deal with.
.sp
Those scripts are described below:
.sp
.AL
.LI
in, out, unout: 
.br
Front-end scripts for checkin/checkout/uncheckout.  These will
prepend the path through your development environment onto the
file names supplied, and invoke checkin, checkout or uncheckout.
.LI
sdiff:
.br
Front-end for sccsdiff.  Like in, out, and unout , sdiff takes a
list of files in the current directory and prepends the proper
path information to /nfs/sccs/..../s.FILE and then does the
diff.  These are to keep you from having to type long path
names; not an issue if you love to type...
.LE
.sp
For example, if you are in $nfs/cmds/etc and want to check out inetd.c
you can either type "checkout cmds/etc/inetd.c" or "out inetd.c".  If
you are any other place and want to checkout inetd.c you have to use the
checkout command.
.na
.bp
.H 1 "ADMINISTRATION TOOLS"
.ad
Most of the other tools available with the DE are used mainly by 
nfsmgr during the build process.  Some of those tools are basically scripts that
are run at night by cron and perform several administrative tasks.
.sp
Most of the scripts that are used as administration tools are invoked by the 
"overnight" script.  Those scripts perform tasks from backing up the system to 
rebuilding all the commands and the kernel.  So, those scripts will be
explained through their use in overnight.
While overnight is running checkins are disabled so that the system has
a consistent view of the working files.  The overnight process currently
starts at 10 PM.
.sp
All the scripts described below are executed or not depending on whether a file
by the same name as the script resides in $Nfs/log and is writable.
The exceptions are noted.
That allows us to turn on or off specific parts of our build process
without having to turn on or off the whole process.
.sp
.na
.H 2 "Takedown"
.sp
.ad
The overnight script removes all the links between the working files and 
$Nfs/develop.  The script within overnight that performs that job is called
"takedown".  Overnight used to remove all the personal development environments
whether a file called takedown existed on the developer's $nfs/log
directory or not.  Now it will only takedown the personal development 
environment if the takedown file exists in $nfs/log.  This feature was
added to allow for special kernels to be saved temporarily.  It should not
be used on a continuous basis since it goes against one of the goals
of the DE which is to keep all developers up to date in relation to the work
done by other people in the group.  To enforce this, the overnight
script will create the "takedown" file towards the end of the process, so the
next night the user's development environment will be taken down.
Mail is sent to the user informing him or her of this fact.
.sp
Once takedown is executed all that remains in $Nfs/develop is current 
experimental source and log files, and any nonworking directories.
For example, if chm creates a directory called "temp" under her
personal development environment the overnight process will not remove it
since it is not one of the normal working directories (like bin, cmds, kernel).
.sp
Chm's development directory will look like this after a takedown:
.sp
.nf
drwxr-xr-x   4 chm      nerfs       1024 Jun 27 12:53 checkout
drwxr-x---   2 chm      nerfs       1024 Jun 22 13:40 log
drwxr-xr-x   2 chm      nerfs         64 Jun 27 12:53 temp
.fi
.sp
.na
.H 2 "get_libc, get_include, inc_depend and get_800inc"
.sp
.ad
Once all the directories under $Nfs/develop have been taken down, the
main part of the build process starts.  The scripts below
are used to prepare for the commands build.  
.sp
Since certain NFS routines are part of libc, we pick up libc every night to 
make sure we are building with the most current versions of our routines. 
The commands build deposits the most up to date libc.a at
the end of their build on hpcndhf so that it is available to all groups
that need it.  Get_libc is a script that waits for libc.a to 
appear in the /tmp directory of hpcndhf.  As soon as the library is there
it picks it up, otherwise it times out at 2 AM.  If it successfully
picked up libc.a it replaces the libc.a under $Nfs/cmds/libc which is
used by all the Makefiles to build the commands.
.sp
The script get_include picks up include files from the different systems
that provide them if they have changed since the last time we picked them up
(usually every night).  The include files in question are system include files
and networking include files provided by SSO and other groups in CND.
We only get the include files that are used by the commands we build so
that we don't need to get all include files available.  The include
files are got remotely using the "Get" command (in $Nfs/bin) that understands
how to get SCCS or RCS files from a remote system using RFA.
.sp
The get_include script used to depend on the get_libc scripts.  Basically,
if we did not get a new libc.a we did not get new include files.  The 
reasoning behind that was that certain routines we were using were heavily
dependent on structures defined in certain header files and if those 
header files changed but we did not successfully get libc.a the rest
of the build might fail.  However, since header files are now changing less,
it is acceptable to pick up the header files independently of libc.a
.sp
Get_include uses $Nfs/bin/checkdate.  Checkdate checks the date on the
source control files we access on remote systems against the date on
the SCCS file with the pointer that we have on the local system.  If the date 
on the remote file is newer than the local date, checkdate touches the
SCCS file on the local system.
.sp
Get_include after running checkdate uses the Makefile $Nfs/bin/include.mk
to retrieve, unifdef (using $Nfs/bin/unifdef) and place in the correct
location on the development system (under $Nfs/include) any include
files that have changed.
.sp
When explaining which header files to get we mentioned that only the
header files needed by the commands build were picked up if they changed.
The script inc_depend goes through all the commands looking for include
files they depend on and builds a list of transient dependencies.  If
it doesn't find some header file it needs under $Nfs/include it will
print a message with the header file name in the $NFs/log/inc_depend
file.  The nfsmgr should check that file periodically to see if any new
header files need to be picked up.  It uses $Nfs/bin/listtd which is
described further down.
.sp
The last script of this type is get_800inc which picks up some 800
include files that are not part of shared source directly from the
ISO source tree.  The reason for us picking up 800 include files is
that we also build NFS commands for the 800.   This script invokes
a script called rmt_include on hpisoa1.  Hpisoa1 is the system that
has the master files for the 800 include files that are not shared.
The local copy of rmt_include is the same as the copy of rmt_include
on hpisoa1.
.sp
.na
.H 2 "Make_kernel: the kernel build"
.sp
.ad
The kernel build is invoked if the file $Nfs/log/Make_kernel
exists and is writable.  It is started from the overnight script along
with the other builds and copys, and consists of a number of separate
scripts to do the assorted tasks needed for the complete build.
.sp
The first of those scripts is called get_kernel and it gets all the
source files in the kernel directory both local and remote.  It uses
the Get command mentioned above.  After all the source files required are
available another script called make_nfsklib makes the archive for the
the NFS kernel segment and sends it to all the machine that need it.  
Currently the machines that receive our archive are hpcndhf and hpfcde.  
The name of the kernel archive is libnfs.a.
Once the archive has been built the program checkdate (available under
$Nfs/bin) goes through all the SCCS files and touches the ones that have
changed since the last time we verified.  This is used specially for
the remote files.
Finally, the cscope database is built based on the most up to date files
available in the kernel directory.
Note: this is subject to change (and it has changed); see the
overnight script itself for exact details on what scripts are called
during which phases of the overnight process.
.sp
.na
.H 2 "NFS Commands Build"
.ad
.sp
Once we have obtained the most up to date include files and libraries 
we can do the actual commands build.  The overnight script checks 
to see if any of the log files for the following scripts exist and are
writable.
.AL
.LI
Make_cmd:
.br
The log file for this script is $Nfs/log/make_cmd.  This script goes through 
all the Makefiles for the s300 NFS and compiles the commands.
It accepts arguments.  For example, a useful way to remove
all targets of the makefiles and other things like BFA versions of the
commands for all commands is to do "Make_cmd clobber".  This will execute
the clobber target on all the makefiles.
.LI
Make_cmd800:
.br
The log file for this script is $Nfs/log/make_cmd800.  This script goes
through all the Makefiles for the s800 NFS commands and compiles the commands.
It also accepts arguments like those described for Make_cmd800.  To
invoke this script we do a remsh to the 800 we use for our build and
execute a script called init_make on that machine.  Init_make sets up an
RFA connection, defines $Nfs to be /net/hpcndu/$Nfs (where hpcndu is our
build machine) and invokes Make_cmd800.
.LI
Make_cscope:
.br
The log file for this script is $Nfs/log/make_cscope.  This script creates
the cscope database for the user level files.
.LI
Make_bfa:
.br
The log file for this script is $Nfs/log/make_bfa.  This script creates
the BFA versions of the commands.  The BFA versions of the commands 
all have a ".b" suffix to differentiate them from the standard versions.
.LE
.na
.sp
.H 2 "Second part of kernel build"
.ad
.sp
The second part of the kernel build runs in parallel with the commands
build.  That script is called Make_kern_2 and it uses the same log
file $Nfs/log/Make_kernel as the first part of the kernel build.  
It consists of two scripts that in the future will have separate log files for 
turning them on or off independently.
.sp
The first script is called get_kobjects.  It gets the kernel archives and hp-ux,
that is the results of the build from hpcndhf.  That allows us to rebuild 
kernels during the day if we need to do so.  This script waits until 4 AM for
hp-ux and if hp-ux is not there by then it times out.
The second script is called make_kbfa and it builds a BFA version of
the kernel.
Note: this too has changed; the overnight script should be consulted.
.sp
.na
.H 2 "Backup"
.ad
.sp
After the personal development environments have been taken down and 
the commands and kernel have been rebuilt the system is backed up.
Depending on whether it is the weekend or not we do two types of backup,
a full backup or an incremental backup.
.sp
The full backup ($Nfs/bin/fullbackup) occurs once a week on Fridays.  
It touches a file called $Nfs/log/fulltime that is used to determine when 
the last full backup occurred.  The other backups during the week 
($Nfs/bin/backup) backup anything that has changed since $Nfs/log/fulltime.
That way if we need to it would be easy to recover the development 
environment to the previous night by using two tapes: the weekends full
backup and one of the nightly backups since they have all the changes since
the full backup.
.sp
We also use two sets of tapes for our weekend backups (a "black" and a "red"
set, depending on the color of the labels).  That way if something goes wrong
with the system during the weekend backup we can still revert to the previous'
week backup and the Thursday backup tape.
.sp
.na
.H 2 "Setup"
.ad
.sp
Once the backup has finished we enable updates again by making $Nfs/log/update
writable and setup the personal development environments.
The setup script allows for three different types of setup: commands, kernel
and both.  Since it takes a while to make all the links required for the
setup we divided the working files into two sets: the ones used by the 
kernel developers and the ones used by the commands developers.  A developer
can choose to setup either set of files or both sets by touching a different 
file in $nfs/log.  To setup the kernel a developer would touch 
$nfs/log/set_kern, for the commands he would touch $nfs/log/set_cmds and
to setup both sets of working files he would touch setup.
.sp
A certain set of the working files are setup if you choose setup_cmds or
setup_kern.  Those files are the ones in the bin, include, doc and man 
directories. If the developer does not have a $nfs/log/takedown file, but has 
a setup file overnight will not re-setup its development environment.
.sp
.na
.H 2 "Releases"
.ad
.sp
A useful script to be used when NFS needs to be delivered either for
an EB release or for the final release is up_release.
It changes the SCCS level of all the files in the development environment.
It should be run before giving the bits to SSIT for a release.
Other changes that are associated with changing SCCS levels before a
release and that need to be done by hand are marking the end of 
$Nfs/log/update file and changing the admin level in the checkin script.
.sp
The update file keeps the information about all the changes that have occurred
since the last release.  We can either rename the current file to keep the
information separate for different releases or we can just put a mark at
the time we freeze for a release so we have an easy way of determining
what changes have occurred between releases.
.sp
The other manual step before unfreezing after a release is to change the
SCCS levels used on the checkin script for the admin command.  The admin
command is used for creating new files.  If we create new files after
a release we want them to have the new number for the SCCS level.
.sp
The DE gives you the possibility to keep several version of the development
environment on the system at the same time.  That way we can keep a frozen 
and an unfrozen source tree in parallel.  To do that all that is required
is to change the value of $Nfs since the scripts key on that value to 
perform their different operations.  Note: this is highly discouraged,
since the acrobatics to do so (which follow) are non-trivial, and
leave possible problems if partners are not informed soon enough, etc.
(many processes are involved, and most of them have to change when
this happens).  The recommended way of doing this is by keeping a file
of the revisions used to build a certain product (called a tags file).
The command "create_rel" will create this file, and "setup_rel" will
setup a development environment with all these source files.
.sp
When changing $Nfs (if you choose to do so) there are a number of
things that need to be done:
.sp
.AL
.LI
Change in /etc/profile, /etc/csh.login (Nfs=/nfs to Nfs=<new_tree>) 
.LI
Change the definition of $Nfs on the overnight cron script.
.LI
Change the definition of $Nfs on the script that starts the build over RFA of
the 800 executables.
.LI
Let all partners know that we are about to change $Nfs since they
pull both library routines and include files from us.  The groups that
need to be informed if we change $Nfs are Peking and the SSO kernel groups
for include files and maybe some kernel routines (they have to change their
pointers to our system) and the commands group (they pick up routines that
are part of libc.a from us).  Also, if the password for nfsmgr is ever 
changed at least Peking will need to know about it since they use it with RFA
for some of their scripts.
.sp
.na
.H 2 "Other useful scripts"
.ad
.sp
Other than the scripts used by overnight and the release process there are 
other scripts and programs that can be useful in performing certain system 
functions.  Some of these are described below:
.sp
.AL
.LI
Make_install
.br
This script is similar to the other Make_* scripts, but it makes
the install target in the Makefiles.  This will actually call
the Make_cmd script with an argument of "install".
.LI
bfatrans
.br
This script copies the BFA database files and the version of the commands
compiled with BFA to any test system specified on the command line.
It can also be used to copy regular files to a target machine.
.LI
do_netunam
.br
do_netunam looks up the target machine in the naccess files and
verifies that the values given will work, then echos an appropriate
command to be executed.  The default naccess file resides on the home 
directory of nfsmgr.
.LI
install
.br
Sun compatible install program; install is used by Sun Makefiles
to put source code in the desired destination with the desired
user, group and mode bits.  Note that /nfs/bin must be before
/etc in your $PATH for the Makefile to get the right version!
The latest version looks in your environment for $INSTALL_OPTS,
and uses those as options to install; this is especially useful
to apply the "-c" option (copy, don't move) to all installs.
.LI
maketd
.br
This script goes takes all the C source and header files named
on the command line and determines which header files they
include.  It is used by the "depend" target in the Makefiles to
make the dependency lists used to create new executables when a
header file changes.
.LI
listtd
.br
Works the same way as maketd, but instead of putting the results
in the format used by Makefiles it creates a list of include 
file dependencies for the files given, one dependency per line.
.LI
transfer
.br
This script transfers the user level commands from the
NFS's development machine hpcndu to hpcndhf which is the machine 
that distributes commands and kernel to all test machines.
.LI
wait_for
.br
Wait until a file exists or until a "timeout" occurs.  This is an operation
done in several scripts an for that reason it has been combined into a 
single script.
.LE
.na
.bp
.H 1 "PARTNERS"
.ad
.sp
The NFS project requires close interactions between our group and 
other groups at SSO and CND.  We provide and receive different files
to and from those groups.  The groups we deal with are the SSO commands
group, the SSO kernel group and the CND convergence group.
.sp
The SSO commands group provide libc.a to us every night unless they
are frozen.  That library contains RPC, XDR and YP library routines that
have been incorporated into libc because of NFS.  The commands group has
a process in place that checks our SCCS files to see if they have been
modified since they last checked.  If they have changed they get our
whole SCCS tree and convert it to RCS.  They use the RCS files to store
the routines they build for libc.  The files they are interested in are
the directories $Nfs/cmds/libc/rpc, $Nfs/cmds/libc/yp, $Nfs/include/rpc
and $Nfs/include/nfs. 
.sp
The SSO kernel group picks up nfslib.a, the NFS kernel archive.  We
get from them their kernel C files and their kernel header files.  The header 
files are used for our build, but the C files are only picked up for reference.
Through hpcndhf (the machine for the CND convergence group) we get 
SSO's kernel archives.  We also get the user level header files from them,
even though the original header files reside on hpfclj, the commands group
machine.  The reason we get them from the SSO kernel group is that hpfclj
is a s500 and we had problems networking directly to it.  The SSO kernel
group maintains in their system a shadow copy of the user level include
files and those are the ones we get.
.sp
The CND convergence group machine (hpcndhf) is also the distribution machine 
for the test systems.  So, hpcndhf gets all the NFS commands we build every
night through a script called "transfer" in $Nfs/bin.  They also get
from us out kernel library nfslib.a.  We get from them the network header
files and their C files.  The C files are only picked up for reference.
.na
