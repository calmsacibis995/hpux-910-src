From: Cristina Mahon
.br
Subject: NFS project development environment overview
.sp 3
.ad
The NFS project has been using a development environment that has proved 
to be very successful.  It has saved us a significant amount of time in 
several occasions and in a continuous basis.  For that reason we want to
make the development environment more visible to other groups in the lab 
that might be able to take advantage of the same tools we have been using.
.sp
At the beginning of this project we could see that more than one person
would be working on the same pieces of code (the kernel for example).  
We also wanted some way of making it easy for the developers to build 
their respective pieces of code without interfering with the work done by 
other members of the group.  However, we did not want to have more than one 
copy of all the source and object code because of disc space considerations.
.sp
After investigating the problem we found out that there was a 
development environment created by Karl Jensen for the HP9000 
s500 kernel group at SSO that seemed to fit our requirements.  
That development environment is called SDCS and it was the basis for 
the development environment we are using now. 
.sp
The development environment (DE) consists of some main directories 
which contain all the files that we develop with, SCCS directories 
(it could be RCS directories) and several personal development environments 
that are links to the main directories.  Several scripts manipulate the 
links when we check out and check in the source files.  Other scripts allow
us to build those files.
.sp
We have been using the DE since the beginning of this project and have found 
several advantages to it.  Among those advantages we can mention:
.sp
.AL
.LI
Controlled development environment:
.sp
.BL
.LI
The DE allows us to control all the tools we need to implement
the software.  It is self-contained and does not depend on what is
available on the system at any given time.
.LI
Development environments for different versions can easily coexist on the
same system. 
.LI
We can turn on or off any part of our build process.  
.LI
The development environment keeps track of all changes made
to all modules by date.  
.LE
.LI
Quality Improvement:
.sp
.BL
.LI
The night after a piece of a software system has been changed,
it becomes visible to all other developers and it is built
into the system.  
.LI
We get maximum exposure on the code since as soon as it is checked-in
other people start compiling with it and integrating it with their
own pieces of code.
.LI
The DE provides an extra incentive for people to make sure their
modules don't break other modules. If there are any problems they 
will be noticed the day after the changes were made.
.LE
.LI
Productivity Improvement:
.BL
.LI
Any defects found can be more easily fixed, since the change that caused 
it was made a short time ago and is probably still clear in the developer's 
mind.
.LI
All modules are built every night.  No need to wait for them to build
during the day.
.LI
The DE simplifies the process of releasing the product, since
the product is continuously being integrated.  
.LE
.LI
Reduced use of resources:
.BL
.LI
The DE uses links to maintain the different pieces of the
software organized, and so saves disc space.
.LI
There is no need to slow the machine with massive builds during the
day, since most of the parts that need to be built are built overnight.
.LE
.sp 2
The development environment has proved to be a very flexible
tool that adapted well to all the changes that were required during
the course of this project.  It saved us a significant amount of
effort in both managing and building the NFS commands and kernel.
If you would like to get more information on the development
environment please contact me.  I will be glad to send you the
paper we have that describes the development environment as well
as answer any questions that you might have.
.na
