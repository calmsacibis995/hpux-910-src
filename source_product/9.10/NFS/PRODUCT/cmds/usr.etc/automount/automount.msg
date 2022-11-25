$set 1
$ ==============
$ Message numbers 1-99 are for future use...
$ ==============
$
$ ==============
$ auto_all.c 
$ 100-199
$ ==============
101 unmount %s\n
102 unmount %s: BUSY\n
103 unmount %s: OK\n
104 pathok: couldn't find devid %04x(%04x) for %s
105 pathok: %s\n
106 pathok: %s is dead\n
107 pathok: %s is OK\n
$ ==============
$ auto_look.c
$ 200-299
$ ==============
201 automount trace = %d\n
202 verbose %s
203 on
204 off
205 link: %s/ %s 
206 dir : %s %s -%s\n
207 leading space in map entry \"%s\" in %s
208 bad key \"%s\" in direct map %s\n
209 bad key \"%s\" in indirect map %s\n
210 WARNING: \"%s\" ignored for %s in %s
211 WARNING: \"%s\" ignored for %s in %s
212 map %s, key %s: bad
213 Memory allocation failed: %m
214 bad entry in map %s \"%s\"
215 Memory allocation failed: %m
216 %s: exports: %s
217 Memory allocation failed: %m
218 client create: %s
$ ==============
$ auto_main.c
$ 300-399
$ ==============
301 Must be root to use automount\n
302 Can't get my address
303 Bad timeout value\n
304 Bad timeout value\n
305 no mount maps specified
306 NIS bind failed: %s
307 Cannot create UDP service
308 svc_register failed
309 couldn't create %s: %m
310 couldn't stat %s: %m
311 svc_run returned
312 Cannot fork: %m
313 svc_run returned
314 Can't mount %s: %m
315 Bad timeout value\n
316 automount: bad timeout switch\n
317 Can't update %s
318 lstat %s: %m
319 unmount %s: %m
320 exiting
321 Usage: automount\n%s%s%s%s%s%s%s%s%s%s%s
322 \t[-n]\t\t(no mounts)\n
323 \t[-m]\t\t(ignore NIS auto.master)\n
324 \t[-T]\t\t(trace nfs requests)\n
325 \t[-v]\t\t(verbose error msgs)\n
326 \t[-tl n]\t\t(mount duration)\n
327 \t[-tm n]\t\t(attempt interval)\n
328 \t[-tw n]\t\t(unmount interval)\n
329 \t[-M dir]\t(temporary mount dir)\n
330 \t[-D n=s]\t(define env variable)\n
331 \t[-f file]\t(get mntpnts from file)\n
332 \t[ dir map [-mntopts] ] ...\n
333 unmount failed - processes still accessing %s - will retry
$ ==============
$ auto_mount.c
$ 400-499
$ ==============
401 renew link for %s\n
402 It's on my host\n
403 already mounted %s:%s on %s (%s)\n
404 Couldn't stat created mountpoint %s: %m
405 Couldn't create mountpoint %s: %m
406 couldn't create mntpnt %s\n
407 WARNING: %s already mounted on
408 Couldn't stat: %s: %m
409 couldn't stat %s: %m
410 WARNING: %s: line %d: bad entry
411 %s:%s no longer mounted
412 %s:%s no longer mounted\n
413 Cannot stat %s: %m
414 Cannot stat %s: %m
415 %s:%s already mounted on %s
416 server not responding
417 Warning: cannot do local bind
418 unmount
419 ping %s 
420 OK
421 NO RESPONSE
422 nfs_cast: got %s\n
423 no response
424 trymany: servers not responding: %s
425 Couldn't stat %s: %m
426 %s: realpath failed: %m
427 %s:%s -> %s : dangerous symbolic link
428 host %s not responding
429 server not responding
430 Warning: cannot do local bind
431 %s: server not responding%s
432 can't mount %s: no permission
433 can't mount %s: %m
434 mount %s %s (%s)\n
435 Mount of %s on %s: %m
436 mount %s OK\n
437 remount %s %s (%s)\n
438 remount %s on %s: server not responding
439 remount FAILED: server not responding\n
440 remount: couldn't stat: %s: %m
441 remount of %s on %s: %m
442 remount FAILED 
443 remount: couldn't stat: %s: %m
444 remount OK\n
445 dupmntent: memory allocation failed
446 couldn't stat %s: %m
447 WARNING: %s: line %d: bad entry removed
448 truncate %s: %m
449 addmntent: %m
450 Bad numeric option '%s'
451 delay reading /etc/mnttab -  havechild %d\n 
452 fork a child process to add an entry to %s\n 
453 fail to add/delete entry to %s cannot lock the file\n 
454 fork a child process to delete an entry from %s\n 
$ ==============
$ auto_node.c
$ 500-599
$ ==============
501 dir %s must start with '/'
502 hierarchical mountpoints: %s and %s
503 WARNING: default option \"%s\" ignored for map %s
504 WARNING: %s:%s already mounted on %s
505 %s: Not found
506 %s: Not a directory
507 WARNING: %s not empty!
508 Cannot create directory %s: %m
509 dirinit: memory allocation failed: %m
510 Memory allocation failed: %m
511 Memory allocation failed: %m
512 do_timeouts: enter\n
513 do_timeouts: exit\n
$ ==============
$ auto_proc.c
$ 600-699
$ ==============
601 Memory allocation failed: %m
$ ==============
$ many_cast.c
$ 700-799
$ ==============
701 Cannot create socket for broadcast rpc: %m
702 Cannot send broadcast packet: %m
703 Many_cast select problem: %m
704 Cannot receive reply to many_cast: %m
$ ==============
$ nfs_cast.c
$ 800-899
$ ==============
801 Cannot create socket for nfs: %m
802 nfscast: Cannot send packet: %m
803 nfscast: select: %m
804 nfscast: cannot receive reply: %m
$ ==============
$ nfs_server.c
$ 900-999
$ ==============
901 svc_freeargs failed
$ ==============
$ Files with no message catalog accesses yet
$ ==============
$ auto_site.c
$ ==============
$ getwd.c
$ ==============
$ nfs_prot.c
$ ==============
$ nfs_trace.c
$ ==============
$ putenv.c
$ ==============
$ realpath.c
$ ==============
$ strdup.c
$ ==============
