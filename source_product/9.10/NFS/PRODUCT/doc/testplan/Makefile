SHELL=/bin/sh

ORIGTESTS = NFS.testplan async_tests cmds_one cmds_two           \
          cmds_three cmds_four                                 \
          cmds_five conn_tests daemon_tests destruct_tes       \
	  func_tests gateway_test getdirent_ts hardware_tes    \
	  longname_tes memory_tests nfs_dux_test nls_tests     \
	  pcnfsd_tests perf_tests qr_tests recover_test        \
	  rel_tests rename_tests rpcinfo_test rup_tests        \
          rusers_tests rwall_tests                             \
	  security_tes showmount_ts specific_tes spray_tests   \
	  stress_tests symlnk_tests udp_cksum vhe_tests xserv_tests      \
          yp_tests


NEWTESTS = NEW.testplan lock_tests rpcgen_tests rex_tests           \
          fifo_tests posix_tests acl_tests                                 

orig_plan.out:	${ORIGTESTS}
	nroff -mm -Tlp -rN2 -rB3 -rW68 NFS.testplan >orig_plan.out

new_plan.out:	${NEWTESTS}
	nroff -mm -Tlp -rN2 NEW.testplan >new_plan.out

clean:;
clobber:;	-rm -f orig_plan.out new_plan.out 
