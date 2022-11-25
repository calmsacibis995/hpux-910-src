# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/Makefile,v $
# $Revision: 1.4.84.3 $	$Author: kcs $
# $State: Exp $   	$Locker:  $
# $Date: 93/09/17 20:46:55 $

# To include the .sa -> .s dependencies, uncomment the following line.
### .SUFFIXES: .sa

RM=/bin/rm
AS=as
ASFLAGS=-m

OBJS =  do_func.o get_op.o res_func.o x_unimp.o x_unsupp.o \
        sto_res.o kernel_ex.o round.o \
        slogn.o setox.o ssinh.o stanh.o satan.o sasin.o satanh.o \
        ssin.o stan.o sacos.o stwotox.o scosh.o slog2.o srem_mod.o \
        bindec.o binstr.o decbin.o x_store.o sgetem.o smovecr.o \
        skeleton.o util.o x_bsun.o x_operr.o x_ovfl.o scale.o \
        x_snan.o x_unfl.o tbldo.o sint.o gen_except.o x_fline.o \
        bugfix.o

# The object module order on the following lines is important.
# Must be 'a.o' then all other objects and then 'z.o'.
# Kernal_fp.o is for kernel debugging, and contains no user code.
fpsp.o:		a.o ${OBJS} kernal_fp.o z.o
	@${RM} -f $@
	ld -r a.o ${OBJS} kernal_fp.o z.o -o fpsp.o

# These are here so that the kernel will know when we are in the fpsp.
# This is for time charging; if between a.o and z.o, charge to user time,
# otherwise charge to system time.
z.o:
	@${RM} -f $@
	echo " global _soft_emulation_end\n_soft_emulation_end:\n" | ${AS} -o z.o

a.o:
	@${RM} -f $@
	echo " global _soft_emulation_beg\n_soft_emulation_beg:\n" | ${AS} -o a.o

kernal_fp.s:	kernal_fp.sa
	@${RM} -f $@
	cp $? $@

.sa.s:
	@${RM} -f $*.s
	./hpasm -m < $*.sa > $*.s

.sa.o:
	@${RM} -f $*.s $*.o
	./hpasm -m < $*.sa > $*.s
	${AS} ${ASFLAGS} $*.s

clean:
	${RM} -f *.s *.o

bindec.sa binstr.sa bugfix.sa decbin.sa do_func.sa gen_except.sa get_op.sa \
kernel_ex.sa res_func.sa round.sa satan.sa scale.sa setox.sa sgetem.sa \
sint.sa skeleton.sa slogn.sa smovecr.sa srem_mod.sa ssin.sa stan.sa stanh.sa \
sto_res.sa stwotox.sa util.sa x_bsun.sa x_fline.sa x_operr.sa x_ovfl.sa \
x_snan.sa x_store.sa x_unfl.sa x_unimp.sa x_unsupp.sa: fpsp.h.s

# This uncomments the .SUFFIXES in the first part of this makefile
# so that the .sa stuff works right
fix:
	@${RM} -f Makefil
	sed -e 's/^##* .SUFFIXES/.SUFFIXES/' < Makefile > Makefil
	@${RM} -f Makefile
	mv -f Makefil Makefile

# This puts the comment back in before checking into shared source.
break:
	@${RM} -f Makefil
	sed -e 's/^.SUFFIXES/### .SUFFIXES/' < Makefile > Makefil
	@${RM} -f Makefile
	mv -f Makefil Makefile
