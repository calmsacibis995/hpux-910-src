default:	primes.c
	 $(CC) primes.c -o primes -lm
	 findmsg primes.c > primes.msg
	 gencat primes.cat primes.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f primes

# copy /usr/bin/primes to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/primes
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	primes
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp primes ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/primes
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/primes to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/primes
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp primes ../../../../Filesets.sl/CMDS-AUX/usr/bin/primes
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/primes
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
