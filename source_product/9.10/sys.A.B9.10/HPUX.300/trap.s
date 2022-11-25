	comm	_cnt,188
	comm	_rate,188
	comm	_sum,188
	comm	_total,56
	comm	_nswap,4
	comm	_rootdev,4
	comm	_swapdev,4
	comm	_swapdev_vp,4
	comm	_argdev_vp,4
	comm	_nswdev,4
	comm	_mpid,4
	comm	_kmapwnt,1
	comm	_updlock,4
	comm	_rablock,4
	comm	_rasize,4
	comm	_physmembase,4
	comm	_p1pages,4
	comm	_highpages,4
	comm	_usrstack,4
	comm	_user_area,4
	comm	_float_area,4
	comm	_processor,4
	comm	_total_lockable_mem,4
	comm	_lockable_mem,4
	comm	_unlockable_mem,4
	comm	_float_present,4
	comm	_dragon_present,4
	comm	_forkstat,16
	comm	_boottime,8
	comm	_time,8
	comm	_tz,8
	comm	_lbolt,4
	comm	_avenrun,24
	comm	_sbolt,4
	comm	_ipc,16
	comm	_audit_state_flag,4
	comm	_audit_ever_on,4
	comm	_curr_file,1024
	comm	_audit_mode,4
	comm	_currlen,4
	comm	_curr_vp,4
	comm	_next_file,1024
	comm	_nextlen,4
	comm	_next_vp,4
	comm	_next_sz,4
	comm	_next_dz,4
	global	_default_fault_hook
_default_fault_hook:
	link.w	%a6,&0
	movq	&1,%d0
	unlk	%a6
	rts
	data
	lalign	4
	global	_fault_hook
_fault_hook:
	long	_default_fault_hook
	lalign	4
	global	_m68040_fp_emulation_error
_m68040_fp_emulation_error:
	long	0
	lalign	4
	global	_trap_type
_trap_type:
	long	L302
	long	L303
	long	L304
	long	L305
	long	L306
	long	L307
	long	L308
	long	L309
	long	L310
	long	L311
	long	L312
	long	L313
	lalign	4
	global	_err_format
_err_format:
	long	L315
	lalign	4
	global	_buserr_format
_buserr_format:
	long	L317
	lalign	4
	global	_log_syscall
_log_syscall:
	long	0
	lalign	4
	global	_test_printf
_test_printf:
	long	_printf
	text
	global	_trap
_trap:
	link.w	%a6,&-16
	mov.l	%a5,-(%sp)
	mov.l	%d7,-(%sp)
	clr.w	-6(%a6)
	mov.w	78(%a6),%d0
	and.w	&0xfff,%d0
	and.l	&0xffff,%d0
	mov.l	%d0,-16(%a6)
	mov.l	&_cnt+0x4,%a0
	addq.l	&1,(%a0)
	mov.l	_u,%a0
	mov.l	196(%a0),-4(%a6)
	mov.w	72(%a6),%d0
	and.w	&0x2000,%d0
	tst.w	%d0
	bne.b	L324
	mov.l	&_u+0x8e,%a0
	clr.w	(%a0)
	mov.l	&_u+0x8c,%a0
	clr.b	(%a0)
	bset	&4,-14(%a6)
	mov.l	%a6,%a0
	addq.l	&8,%a0
	mov.l	&_u+0x4,%a1
	mov.l	%a0,(%a1)
L324:
	mov.l	_u,%a5
	mov.l	-16(%a6),%d0
	cmp.l	%d0,&4192
	bgt.w	L410
	beq.w	L402
	cmp.l	%d0,&224
	bgt.w	L411
	beq.w	L335
	cmp.l	%d0,&196
	bgt.w	L412
	beq.w	L370
	cmp.l	%d0,&36
	beq.w	L399
	cmp.l	%d0,&52
	beq.w	L344
	cmp.l	%d0,&56
	beq.w	L344
	cmp.l	%d0,&96
	beq.w	L402
	cmp.l	%d0,&192
	beq.w	L370
L328:
	mov.l	&_u+0x516,%a0
	mov.b	(%a0),%d0
	and.b	&0x2,%d0
	tst.b	%d0
	beq.b	L329
	mov.l	&L331,-(%sp)
	bsr.l	_panic
	addq.w	&4,%sp
L329:
	mov.l	&_u+0x4de,%a0
	tst.l	(%a0)
	beq.b	L332
L9000002:
	mov.l	&_u+0x4de,%a0
	mov.l	(%a0),74(%a6)
	mov.l	&_u+0x516,%a0
	mov.b	(%a0),%d0
	and.l	&0xff,%d0
	bset	&0,%d0
	mov.b	%d0,(%a0)
	bra.w	L322
	lalign	8
L332:
	movq	&0,%d0
	mov.w	72(%a6),%d0
	mov.l	%d0,-(%sp)
	mov.l	74(%a6),-(%sp)
	mov.l	-16(%a6),-(%sp)
	mov.l	_err_format,-(%sp)
	bsr.l	_printf
	add.w	&16,%sp
	bclr	&4,-14(%a6)
	mov.l	-16(%a6),%d0
	asr.l	&2,%d0
	mov.l	%d0,-16(%a6)
	movq	&12,%d1
	cmp.l	%d1,%d0
	bls.b	L333
	mov.l	&_trap_type,%a0
	mov.l	(%a0,%d0.l*4),-(%sp)
	bsr.l	_panic
	addq.w	&4,%sp
L333:
	mov.l	&L334,-(%sp)
L9000001:
	bsr.l	_panic
L9000000:
	addq.w	&4,%sp
L326:
	mov.l	%d7,-(%sp)
	mov.l	%a5,-(%sp)
	bsr.l	_psignal
	addq.w	&8,%sp
L349:
	tst.b	14(%a5)
	bne.w	L423
	tst.l	16(%a5)
	beq.w	L422
	bsr.l	_issig
	mov.l	%d0,-12(%a6)
	bge.w	L426
	mov.l	&_ipc+0x4,%a0
	clr.l	(%a0)
	mov.l	&_ipc,-(%sp)
	bsr.l	_wakeup
	addq.w	&4,%sp
	mov.l	-12(%a6),%d0
	neg.l	%d0
	mov.l	%d0,-(%sp)
	bsr.l	_exit
	addq.w	&4,%sp
	tst.l	%d0
	bne.w	L423
	bra.w	L422
	lalign	8
L335:
	movq	&3,%d1
	cmp.l	%d1,_processor
	bne.b	L336
	mov.l	&L337,-(%sp)
	bra.w	L9000001
	lalign	8
L336:
	mov.l	&L339,-(%sp)
	bra.w	L9000001
	lalign	8
L340:
	movq	&3,%d1
	cmp.l	%d1,_processor
	bne.b	L341
	mov.l	_m68040_fp_emulation_error,%d7
	mov.l	&_u+0x27a,%a0
	clr.l	(%a0)
	bra.w	L326
	lalign	8
L341:
	mov.l	&L343,-(%sp)
	bra.w	L9000001
	lalign	8
L344:
	mov.l	&8,-(%sp)
	mov.l	%a5,-(%sp)
	bsr.l	_psignal
	addq.w	&8,%sp
	mov.l	&_u+0x130,-(%sp)
	bsr.l	_longjmp
	bra.w	L9000000
	lalign	8
L348:
	addq.l	&2,74(%a6)
	bra.w	L349
	lalign	8
L350:
	mov.l	&_u+0x27a,%a0
	clr.l	(%a0)
	movq	&4,%d7
	bra.w	L326
	lalign	8
L352:
	mov.l	-16(%a6),%d0
	bclr	&12,%d0
	asr.l	&2,%d0
	mov.l	&_u+0x27a,%a0
	mov.l	%d0,(%a0)
	movq	&4,%d7
	bra.w	L326
	lalign	8
L367:
	mov.l	&_u+0x27a,%a0
	clr.l	(%a0)
	movq	&6,%d7
	bra.w	L326
	lalign	8
L368:
	mov.l	&_u+0x27a,%a0
	clr.l	(%a0)
	movq	&3,%d7
	cmp.l	%d7,_processor
	bne.b	L369
	tst.l	_m68040_fp_emulation_error
	beq.b	L369
	mov.l	_m68040_fp_emulation_error,%d7
	clr.l	_m68040_fp_emulation_error
	bra.w	L326
	lalign	8
L369:
	movq	&7,%d7
	bra.w	L326
	lalign	8
L370:
	mov.l	_fpsr_save,%d0
	bclr	&31,%d0
	mov.l	&_u+0x27a,%a0
	mov.l	%d0,(%a0)
	mov.l	&_u+0x4de,%a0
	tst.l	(%a0)
	bne.b	L377
	mov.l	&L378,-(%sp)
	bra.w	L9000001
	lalign	8
L377:
	mov.l	&L380,-(%sp)
	bsr.l	_printf
	addq.w	&4,%sp
	bra.w	L9000002
	lalign	8
L381:
	bsr.l	_get_fpiar
	mov.l	&_u+0x52c,%a0
	mov.l	%d0,(%a0)
	movq	&3,%d7
	cmp.l	%d7,_processor
	beq.b	L389
	movq	&0,%d0
	mov.w	78(%a6),%d0
	and.l	&0xf000,%d0
	cmp.l	%d0,&36864
	bne.b	L389
	mov.w	&1,-6(%a6)
L389:
	mov.l	_fpsr_save,%d0
	bclr	&31,%d0
L9000003:
	mov.l	&_u+0x27a,%a0
	mov.l	%d0,(%a0)
	movq	&8,%d7
	bra.w	L326
	lalign	8
L392:
	movq	&10,%d7
	mov.l	%d7,-(%sp)
	pea	8(%a6)
	mov.l	_fault_hook,%a0
	jsr	(%a0)
	addq.w	&8,%sp
	bra.w	L326
	lalign	8
L393:
	mov.l	-16(%a6),%d0
	bclr	&12,%d0
	asr.l	&2,%d0
	bra.b	L9000003
	lalign	8
L391:
	mov.l	&_u+0x27a,%a0
	clr.l	(%a0)
	movq	&8,%d7
	bra.w	L326
	lalign	8
L395:
	mov.l	&_u+0x516,%a0
	mov.b	(%a0),%d0
	and.b	&0x4,%d0
	tst.b	%d0
	bne.b	L396
	movq	&0,%d0
	mov.w	72(%a6),%d0
	bclr	&15,%d0
	mov.w	%d0,72(%a6)
	bra.w	L349
	lalign	8
L396:
	mov.l	&_u+0x516,%a0
	mov.b	(%a0),%d0
	and.l	&0xfb,%d0
	mov.b	%d0,(%a0)
	movq	&0,%d0
	mov.w	72(%a6),%d0
	bclr	&15,%d0
	mov.w	%d0,72(%a6)
	bsr.l	_mm_trace_trap
	cmp.l	%d0,&1
	beq.w	L349
	movq	&5,%d7
	bra.w	L326
	lalign	8
L399:
	mov.l	&_u+0x516,%a0
	mov.b	(%a0),%d0
	and.b	&0x4,%d0
	tst.b	%d0
	beq.w	L322
	bsr.l	_mm_trace_trap
	cmp.l	%d0,&1
	bne.b	L401
L9000004:
	mov.l	&_u+0x516,%a0
	mov.b	(%a0),%d0
	and.l	&0xfb,%d0
	mov.b	%d0,(%a0)
	movq	&0,%d0
	mov.w	72(%a6),%d0
	bclr	&15,%d0
	mov.w	%d0,72(%a6)
	bra.w	L322
	lalign	8
L401:
	mov.l	&5,-(%sp)
	mov.l	%a5,-(%sp)
	bsr.l	_psignal
	addq.w	&8,%sp
	bra.b	L9000004
	lalign	8
L402:
	mov.l	&L404,-(%sp)
	bsr.l	_printf
	addq.w	&4,%sp
	bra.w	L322
	lalign	8
L406:
	mov.l	32(%a5),%d0
	and.l	&0x4000,%d0
	beq.b	L407
	mov.l	&_u+0x458,%a0
	tst.l	(%a0)
	beq.b	L407
	mov.l	&1,-(%sp)
	mov.l	&_u+0x44c,-(%sp)
	mov.l	74(%a6),-(%sp)
	bsr.l	_addupc
	add.w	&12,%sp
	bclr	&6,34(%a5)
L407:
	movq	&0,%d0
	mov.w	78(%a6),%d0
	and.l	&0xf000,%d0
	cmp.l	%d0,&36864
	bne.w	L349
	mov.w	&1,-6(%a6)
	bra.w	L349
	lalign	8
L412:
	cmp.l	%d0,&200
	beq.w	L370
	cmp.l	%d0,&204
	beq.w	L370
	cmp.l	%d0,&208
	beq.w	L370
	cmp.l	%d0,&212
	beq.w	L370
	cmp.l	%d0,&216
	beq.w	L370
	bra.w	L328
	lalign	8
L411:
	cmp.l	%d0,&4124
	bgt.b	L413
	beq.w	L352
	cmp.l	%d0,&256
	beq.w	L322
	cmp.l	%d0,&4108
	beq.w	L392
	cmp.l	%d0,&4112
	beq.w	L350
	cmp.l	%d0,&4116
	beq.w	L393
	cmp.l	%d0,&4120
	beq.w	L352
	bra.w	L328
	lalign	8
L413:
	cmp.l	%d0,&4136
	bgt.b	L414
	beq.w	L367
	cmp.l	%d0,&4128
	beq.w	L352
	cmp.l	%d0,&4132
	beq.w	L395
	bra.w	L328
	lalign	8
L414:
	cmp.l	%d0,&4140
	beq.w	L368
	cmp.l	%d0,&4148
	beq.w	L391
	cmp.l	%d0,&4152
	beq.w	L350
	bra.w	L328
	lalign	8
L410:
	cmp.l	%d0,&4272
	bgt.w	L415
	beq.w	L352
	cmp.l	%d0,&4248
	bgt.b	L416
	beq.w	L352
	cmp.l	%d0,&4228
	beq.w	L396
	cmp.l	%d0,&4232
	beq.w	L348
	cmp.l	%d0,&4236
	beq.w	L352
	cmp.l	%d0,&4240
	beq.w	L352
	cmp.l	%d0,&4244
	beq.w	L352
	bra.w	L328
	lalign	8
L416:
	cmp.l	%d0,&4252
	beq.w	L352
	cmp.l	%d0,&4256
	beq.w	L391
	cmp.l	%d0,&4260
	beq.w	L352
	cmp.l	%d0,&4264
	beq.w	L352
	cmp.l	%d0,&4268
	beq.w	L352
	bra.w	L328
	lalign	8
L415:
	cmp.l	%d0,&4296
	bgt.b	L417
	beq.w	L381
	cmp.l	%d0,&4276
	beq.w	L352
	cmp.l	%d0,&4280
	beq.w	L352
	cmp.l	%d0,&4284
	beq.w	L352
	cmp.l	%d0,&4288
	beq.w	L381
	cmp.l	%d0,&4292
	beq.w	L381
	bra.w	L328
	lalign	8
L417:
	cmp.l	%d0,&4308
	bgt.b	L418
	beq.w	L381
	cmp.l	%d0,&4300
	beq.w	L381
	cmp.l	%d0,&4304
	beq.w	L381
	bra.w	L328
	lalign	8
L418:
	cmp.l	%d0,&4312
	beq.w	L381
	cmp.l	%d0,&4320
	beq.w	L340
	cmp.l	%d0,&4352
	beq.w	L406
	bra.w	L328
	lalign	8
L426:
	tst.l	-12(%a6)
	beq.b	L422
L423:
	tst.w	-6(%a6)
	beq.b	L427
	mov.b	14(%a5),%d0
	extb.l	%d0
	subq.l	&1,%d0
	movq	&1,%d1
	lsl.l	%d0,%d1
	and.l	28(%a5),%d1
	beq.b	L427
	mov.b	14(%a5),%d0
	extb.l	%d0
	mov.l	%d0,-(%sp)
	mov.l	%a5,-(%sp)
	bsr.l	_psignal
	addq.w	&8,%sp
	clr.b	14(%a5)
	movq	&0,%d0
	mov.w	72(%a6),%d0
	bset	&15,%d0
	mov.w	%d0,72(%a6)
	bra.b	L422
	lalign	8
L427:
	bsr.l	_psig
L422:
	mov.b	8(%a5),9(%a5)
	bset	&1,33(%a5)
	tst.l	_runrun
	beq.b	L430
	bsr.l	_spl6
	mov.l	%d0,%d7
	mov.l	%a5,-(%sp)
	bsr.l	_setrq
	addq.w	&4,%sp
	mov.l	&_u+0x2fc,%a0
	addq.l	&1,(%a0)
	bsr.l	_swtch
	mov.l	%d7,-(%sp)
	bsr.l	_splx
	addq.w	&4,%sp
L430:
	mov.l	&_u+0x458,%a0
	tst.l	(%a0)
	beq.b	L435
	mov.l	_u,%a0
	mov.l	196(%a0),%d0
	sub.l	-4(%a6),%d0
	mov.l	%d0,-4(%a6)
	beq.b	L435
	mov.w	72(%a6),%d0
	and.w	&0x2000,%d0
	tst.w	%d0
	bne.b	L435
	mov.l	-4(%a6),-(%sp)
	mov.l	&_u+0x44c,-(%sp)
	mov.l	74(%a6),-(%sp)
	bsr.l	_addupc
	add.w	&12,%sp
L435:
	mov.b	9(%a5),_curpri
L322:
	mov.l	(%sp)+,%d7
	mov.l	(%sp)+,%a5
	unlk	%a6
	rts
	global	_nosys
_nosys:
	link.w	%a6,&0
	mov.l	%a5,-(%sp)
	mov.l	_u,%a5
	mov.l	&_u+0x19e,%a0
	cmp.l	(%a0),&1
	beq.b	L440
	mov.l	20(%a5),%d0
	and.l	&0x800,%d0
	beq.b	L439
L440:
	mov.l	&_u+0x82,%a0
	mov.w	&22,(%a0)
L439:
	mov.l	&12,-(%sp)
	mov.l	_u,-(%sp)
	bsr.l	_psignal
	addq.w	&8,%sp
	mov.l	(%sp)+,%a5
	unlk	%a6
	rts
	global	_nfs_nosys
_nfs_nosys:
	link.w	%a6,&0
	mov.l	&_u+0x82,%a0
	mov.w	&220,(%a0)
	unlk	%a6
	rts
	global	_syscall
_syscall:
	link.l	%a6,&LF5
	movm.l	&LS5,(%sp)
	mov.l	&_u,%a5
	mov.l	&_cnt+0x8,%a0
	add.l	&1,(%a0)
	mov.l	(%a5),%a0
	mov.l	196(%a0),-4(%a6)
	mov.l	%a6,%d0
	add.l	&8,%d0
	mov.l	%d0,%a0
	mov.l	%a0,4(%a5)
	clr.w	130(%a5)
	mov.l	8(%a6),%d7
	mov.l	68(%a6),%d0
	add.l	&4,%d0
	mov.l	%d0,%a0
	mov.l	%a0,%a2
	tst.l	%d7
	bne	L447
	mov.l	%a2,%a0
	add.w	&4,%a2
	mov.l	%a0,-(%sp)
	bsr.l	_fuword
	add	&4,%sp
	mov.l	%d0,%d7
L447:
	cmp.l	%d7,1308(%a5)
	bcs	L449
	mov.l	&0,%d7
L449:
	mov.l	%d7,%d0
	mov.l	%d0,%d1
	add.l	%d0,%d0
	add.l	%d1,%d0
	lsl.l	&2,%d0
	mov.l	1304(%a5),%d1
	add.l	%d0,%d1
	mov.l	%d1,%a0
	mov.l	%a0,%a3
	tst.l	(%a3)
	beq	L450
	mov.l	(%a3),-(%sp)
	pea	24(%a5)
	mov.l	%a2,-(%sp)
	bsr.l	_copyin4
	add	&12,%sp
L450:
	mov.w	%d7,142(%a5)
	mov.l	%a5,%d0
	add.l	&24,%d0
	mov.l	%d0,%a0
	mov.l	%a0,64(%a5)
	clr.l	132(%a5)
   mov.l   %a6,U_QSAVE_A6(%a5)             # save     A6 (label_t) 
	lea	-8(%sp),%a0			# adj SP for _longjmp  	
   mov.l   %a0,U_QSAVE_SP(%a5)             # save     SP (label_t) 
   mov.l   &longjmp_rtn,U_QSAVE_PC(%a5)    # save     PC (label_t) 
	mov.b	&1,140(%a5)
	tst.l	_log_syscall
	beq	L452
	mov.l	(%a5),%a0
	mov.l	32(%a0),%d0
	and.l	&0x20,%d0
	tst.l	%d0
	beq	L452
	mov.l	&1,-(%sp)
	mov.l	(%a5),%a0
	mov.l	56(%a0),-(%sp)
	mov.l	_log_syscall,%a0
	jsr	(%a0)
	add	&8,%sp
L452:
	mov.l	64(%a5),-(%sp)
	mov.l	4(%a3),%a0
	jsr	(%a0)
	add	&4,%sp
L453:
	mov.l	(%a5),%a4
	cmp.w	130(%a5),&134
	bne	L454
	clr.w	130(%a5)
L454:
	tst.l	_audit_ever_on
	beq	L455
	bsr.l	_save_aud_data
L455:
	tst.l	_log_syscall
	beq	L457
	mov.l	(%a5),%a0
	mov.l	32(%a0),%d0
	and.l	&0x20,%d0
	tst.l	%d0
	beq	L457
	tst.w	130(%a5)
	beq	L458
	mov.l	&3,-(%sp)
	mov.l	(%a5),%a0
	mov.l	56(%a0),-(%sp)
	mov.l	_log_syscall,%a0
	jsr	(%a0)
	add	&8,%sp
	bra	L459
L458:
	mov.l	&2,-(%sp)
	mov.l	(%a5),%a0
	mov.l	56(%a0),-(%sp)
	mov.l	_log_syscall,%a0
	jsr	(%a0)
	add	&8,%sp
L459:
L457:
	tst.b	14(%a4)
	bne	L461
	tst.l	16(%a4)
	beq	L463
	bsr.l	_issig
	mov.l	%d0,-8(%a6)
	bge	L464
	mov.l	&_ipc+0x4,%a0
	clr.l	(%a0)
	mov.l	&_ipc,-(%sp)
	bsr.l	_wakeup
	add	&4,%sp
	mov.l	-8(%a6),%d0
	neg.l	%d0
	mov.l	%d0,-(%sp)
	bsr.l	_exit
	add	&4,%sp
	tst.l	%d0
	bne	L462
	bra	L460
L464:
	tst.l	-8(%a6)
	bne	L462
	bra	L460
L463:
	bra	L460
L462:
L461:
	bsr.l	_psig
L460:
	tst.w	130(%a5)
	beq	L465
	mov.l	&0,%d0
	mov.w	130(%a5),%d0
	mov.l	%d0,8(%a6)
	mov.l	&0,%d0
	mov.w	72(%a6),%d0
	or.l	&0x1,%d0
	mov.w	%d0,72(%a6)
	bra	L466
L465:
	cmp.b	140(%a5),&1
	bne	L467
	mov.l	132(%a5),8(%a6)
	mov.l	136(%a5),12(%a6)
	mov.l	&0,%d0
	mov.w	72(%a6),%d0
	and.l	&0xfffffffe,%d0
	mov.w	%d0,72(%a6)
	bra	L468
L467:
	cmp.b	140(%a5),&3
	bne	L469
	sub.l	&2,74(%a6)
L469:
L468:
L466:
	mov.b	8(%a4),9(%a4)
	tst.l	_runrun
	beq	L470
	bsr.l	_spl6
	mov.l	%d0,%d7
	mov.l	%a4,-(%sp)
	bsr.l	_setrq
	add	&4,%sp
	add.l	&1,764(%a5)
	bsr.l	_swtch
	mov.l	%d7,-(%sp)
	bsr.l	_splx
	add	&4,%sp
L470:
	tst.l	1112(%a5)
	beq	L471
	mov.l	(%a5),%a0
	mov.l	196(%a0),%d0
	sub.l	-4(%a6),%d0
	mov.l	%d0,-4(%a6)
	tst.l	-4(%a6)
	beq	L472
	mov.w	72(%a6),%d0
	and.w	&0x2000,%d0
	tst.w	%d0
	bne	L472
	tst.l	-4(%a6)
	bge	L473
	cmp.w	142(%a5),&2
	beq	L474
	cmp.w	142(%a5),&66
	bne	L473
L474:
	mov.l	(%a5),%a0
	mov.l	196(%a0),-4(%a6)
L473:
	mov.l	-4(%a6),-(%sp)
	pea	1100(%a5)
	mov.l	74(%a6),-(%sp)
	bsr.l	_addupc
	add	&12,%sp
L472:
L471:
	mov.b	9(%a4),_curpri
	bra	L446
longjmp_rtn:						
L475:
	mov.l	&_u,%a5
	tst.w	130(%a5)
	bne	L476
	cmp.b	140(%a5),&1
	bne	L476
	mov.w	&4,130(%a5)
L476:
	bra	L453
L446:
	movm.l	(%sp),&LS5
	unlk	%a6
	rts
	set	LF5,-28
	set	LS5,15488
	global	_buserr_panic
_buserr_panic:
	link.w	%a6,&-8
	mov.l	8(%a6),%a0
	mov.w	64(%a0),%d0
	and.l	&0xffff,%d0
	lsr.l	&8,%d0
	and.l	&0xf,%d0
	mov.l	%d0,-8(%a6)
	ble.b	L482
	movq	&5,%d1
	cmp.l	%d1,%d0
	bge.b	L481
L482:
	mov.l	&L483,-4(%a6)
	bra.b	L484
	lalign	8
L481:
	mov.l	&L485,-4(%a6)
L484:
	mov.l	8(%a6),%a0
	mov.l	66(%a0),-(%sp)
	mov.l	8(%a6),%a0
	mov.w	70(%a0),%d0
	and.l	&0xf000,%d0
	mov.l	%d0,-(%sp)
	mov.l	-8(%a6),-(%sp)
	mov.l	-4(%a6),-(%sp)
	mov.l	&L487,-(%sp)
	mov.l	_panic_msg_len,-(%sp)
	mov.l	&_panic_msg,-(%sp)
	bsr.l	_sprintf
	add.w	&28,%sp
	mov.l	&_panic_msg,-(%sp)
	bsr.l	_panic
	addq.w	&4,%sp
	unlk	%a6
	rts
	global	_get_fault
_get_fault:
	link.w	%a6,&0
	mov.l	%a5,-(%sp)
	mov.l	%d7,-(%sp)
	mov.l	%d6,-(%sp)
	mov.l	%d5,-(%sp)
	mov.l	8(%a6),%a5
	movq	&0,%d5
	movq	&0,%d0
	mov.w	70(%a5),%d0
	and.l	&0xf000,%d0
	cmp.l	%d0,&32768
	beq.b	L492
	cmp.l	%d0,&40960
	beq.b	L494
	cmp.l	%d0,&45056
	beq.w	L503
	mov.l	%a5,-(%sp)
	bsr.l	_buserr_panic
	addq.w	&4,%sp
L490:
	mov.l	12(%a6),%a0
	mov.l	%d7,(%a0)
	mov.l	16(%a6),%a0
	mov.w	%d6,(%a0)
	mov.l	20(%a6),%a0
	mov.l	%d5,(%a0)
	mov.l	(%sp)+,%d5
	mov.l	(%sp)+,%d6
	mov.l	(%sp)+,%d7
	mov.l	(%sp)+,%a5
	unlk	%a6
	rts
L492:
	mov.l	&L493,-(%sp)
	bsr.l	_panic
	addq.w	&4,%sp
L494:
	mov.w	74(%a5),%d6
	and.l	&0xffff,%d6
	mov.w	%d6,%d0
	and.w	&0x100,%d0
	tst.w	%d0
	beq.b	L495
	mov.w	%d6,%d0
	and.w	&0x7,%d0
	and.l	&0xffff,%d0
	mov.l	%d0,%d5
	mov.l	80(%a5),%d7
	bra.b	L496
	lalign	8
L495:
	mov.w	%d6,%d0
	and.w	&0x4000,%d0
	tst.w	%d0
	beq.b	L497
	mov.l	66(%a5),%d7
	addq.l	&4,%d7
	mov.l	%d7,%d0
	bra.b	L496
	lalign	8
L497:
	movq	&0,%d0
	mov.w	%d6,%d0
	and.l	&0x8000,%d0
	beq.b	L499
	mov.l	66(%a5),%d7
	addq.l	&2,%d7
	mov.l	%d7,%d0
	bra.b	L496
	lalign	8
L499:
	mov.l	%a5,-(%sp)
	bsr.l	_buserr_panic
	addq.w	&4,%sp
L496:
	mov.w	%d6,%d0
	and.w	&0x1000,%d0
	tst.w	%d0
	beq.b	L501
	movq	&0,%d0
	mov.w	74(%a5),%d0
	bset	&14,%d0
	mov.w	%d0,74(%a5)
L501:
	mov.w	%d6,%d0
	and.w	&0x2000,%d0
	tst.w	%d0
	beq.w	L490
L9000005:
	movq	&0,%d0
	mov.w	74(%a5),%d0
	bset	&15,%d0
	mov.w	%d0,74(%a5)
	bra.w	L490
	lalign	8
L503:
	mov.w	74(%a5),%d6
	and.l	&0xffff,%d6
	mov.w	%d6,%d0
	and.w	&0x100,%d0
	tst.w	%d0
	beq.b	L504
	mov.w	%d6,%d0
	and.w	&0x7,%d0
	and.l	&0xffff,%d0
	mov.l	%d0,%d5
	mov.l	80(%a5),%d7
	bra.b	L505
	lalign	8
L504:
	mov.w	%d6,%d0
	and.w	&0x4000,%d0
	tst.w	%d0
	beq.b	L506
	mov.l	100(%a5),%d7
	bra.b	L505
	lalign	8
L506:
	movq	&0,%d0
	mov.w	%d6,%d0
	and.l	&0x8000,%d0
	beq.b	L508
	mov.l	100(%a5),%d7
	subq.l	&2,%d7
	mov.l	%d7,%d0
	bra.b	L505
	lalign	8
L508:
	mov.l	%a5,-(%sp)
	bsr.l	_buserr_panic
	addq.w	&4,%sp
L505:
	mov.w	%d6,%d0
	and.w	&0x1000,%d0
	tst.w	%d0
	beq.b	L510
	movq	&0,%d0
	mov.w	74(%a5),%d0
	bset	&14,%d0
	mov.w	%d0,74(%a5)
L510:
	mov.w	%d6,%d0
	and.w	&0x2000,%d0
	tst.w	%d0
	bne.w	L9000005
	bra.w	L490
	global	_get_writeback_status
_get_writeback_status:
	link.w	%a6,&-4
	movq	&3,%d0
	cmp.l	%d0,_processor
	beq.b	L517
	movq	&0,%d0
	bra.b	L514
	lalign	8
L9000007:
	movq	&1,%d0
	bra.b	L514
	lalign	8
L520:
	mov.l	12(%a6),-(%sp)
	mov.l	8(%a6),%a0
	mov.l	52(%a0),-(%sp)
	bsr.l	_tablewalk
	addq.w	&8,%sp
	mov.l	%d0,-4(%a6)
	beq.b	L523
	mov.l	%d0,%a0
	bftst	3(%a0){&6:&2}
	beq.b	L523
	mov.l	-4(%a6),%a0
	bftst	3(%a0){&5:&1}
	beq.b	L519
L523:
	movq	&2,%d0
L514:
	unlk	%a6
	rts
L517:
	cmp.l	8(%a6),&_kernvas
	beq.b	L9000007
	mov.l	&_u+0x4de,%a0
	cmp.l	(%a0),&_wb_error
	beq.b	L520
	cmp.l	(%a0),&_wbl_err
	beq.b	L520
L519:
	movq	&1,%d0
	bra.b	L514
	global	_do_buserr
_do_buserr:
	link.w	%a6,&-20
	mov.l	%a5,-(%sp)
	mov.l	%d7,-(%sp)
	mov.l	%d6,-(%sp)
	mov.l	%d5,-(%sp)
	mov.l	%d4,-(%sp)
	mov.l	8(%a6),%a5
	mov.l	_u,-4(%a6)
	movq	&0,%d4
	mov.l	_u,%a0
	mov.l	196(%a0),-8(%a6)
	mov.l	&_u+0x520,%a0
	mov.l	12(%a6),(%a0)
	mov.w	64(%a5),%d0
	and.w	&0x2000,%d0
	tst.w	%d0
	bne.b	L527
	mov.l	&_u+0x8e,%a0
	clr.w	(%a0)
	mov.l	&_u+0x8c,%a0
	clr.b	(%a0)
	mov.l	&_u+0x4,%a0
	mov.l	%a5,(%a0)
	mov.l	&4096,%d6
	bra.b	L528
	lalign	8
L527:
	movq	&0,%d6
L528:
	tst.l	32(%a6)
	bne.b	L529
	tst.l	28(%a6)
	beq.b	L529
	clr.l	16(%a6)
L529:
	tst.l	%d6
	bne.b	L530
	movq	&1,%d1
	cmp.l	%d1,32(%a6)
	bne.b	L531
	mov.l	&_u+0x4de,%a0
	tst.l	(%a0)
	bne.b	L532
	mov.l	12(%a6),-(%sp)
	mov.l	&L533,-(%sp)
	bsr.l	_printf
	addq.w	&8,%sp
	mov.l	&L534,-(%sp)
	bsr.l	_panic
	addq.w	&4,%sp
L532:
	mov.l	_u,%a0
	bra.b	L9000009
	lalign	8
L531:
	mov.l	&_kernvas,-16(%a6)
	bra.b	L536
	lalign	8
L530:
	mov.l	-4(%a6),%a0
L9000009:
	mov.l	112(%a0),-16(%a6)
L536:
	mov.l	12(%a6),-(%sp)
	mov.l	-16(%a6),-(%sp)
	mov.l	%a5,-(%sp)
	bsr.l	_mm_buserror
	add.w	&12,%sp
	cmp.l	%d0,&1
	beq.w	L539
	movq	&1,%d5
	cmp.l	%d5,32(%a6)
	bne.b	L540
	cmp.l	12(%a6),&-1048576
	bcs.b	L541
	cmp.l	12(%a6),&-917504
	bhi.b	L541
	movq	&1,%d0
	tst.l	_dragon_present
	beq.b	L540
	mov.l	&_u+0x68c,%a0
	cmp.w	(%a0),&-1
	bne.b	L540
	mov.l	&_u+0x44,-(%sp)
	bsr.l	_setjmp
	addq.w	&4,%sp
	tst.l	%d0
	bne.b	L547
	bsr.l	_dragon_buserror
L547:
	movq	&1,%d0
L525:
	mov.l	(%sp)+,%d4
	mov.l	(%sp)+,%d5
	mov.l	(%sp)+,%d6
	mov.l	(%sp)+,%d7
	mov.l	(%sp)+,%a5
	unlk	%a6
	rts
L541:
	movq	&0,%d0
L540:
	mov.l	&_u+0x82,%a0
	mov.w	(%a0),%d0
	and.l	&0xffff,%d0
	mov.l	%d0,%d5
	tst.l	28(%a6)
	beq.w	L549
	tst.l	20(%a6)
	bne.w	L549
	mov.l	12(%a6),-(%sp)
	mov.l	-16(%a6),-(%sp)
	mov.l	16(%a6),-(%sp)
	mov.l	-16(%a6),-(%sp)
	bsr.l	_vfault
	add.w	&16,%sp
	mov.l	%d0,%d7
	bne.w	L551
	tst.l	%d6
	bne.b	L552
L9000018:
	mov.l	12(%a6),-(%sp)
	mov.l	-16(%a6),-(%sp)
	bsr.l	_get_writeback_status
	addq.w	&8,%sp
	bra.b	L525
	lalign	8
L552:
	movq	&3,%d0
	cmp.l	%d0,_processor
	bne.b	L553
	mov.l	&10,-(%sp)
	mov.l	%a5,-(%sp)
	mov.l	_fault_hook,%a0
	jsr	(%a0)
	addq.w	&8,%sp
	tst.l	%d0
	bne.b	L553
L9000015:
	movq	&10,%d7
L554:
	mov.l	&_u+0x524,%a0
	mov.l	%d7,(%a0)
	tst.l	%d6
	bne.w	L565
	mov.l	&_u+0x4de,%a0
	tst.l	(%a0)
	beq.w	L566
	mov.l	(%a0),66(%a5)
	mov.l	&_u+0x516,%a0
	mov.b	(%a0),%d0
	and.l	&0xff,%d0
	bset	&0,%d0
	mov.b	%d0,(%a0)
	movq	&0,%d0
	bra.w	L525
	lalign	8
L553:
	movq	&1,%d4
	bra.w	L539
	lalign	8
L551:
	mov.l	%d7,-(%sp)
	mov.l	%a5,-(%sp)
	mov.l	_fault_hook,%a0
	jsr	(%a0)
	addq.w	&8,%sp
	bra.b	L554
	lalign	8
L549:
	tst.l	20(%a6)
	bne.b	L555
	movq	&10,%d7
	mov.l	&11,-(%sp)
	bra.w	L9000014
	lalign	8
L555:
	tst.l	_pmmu_exist
	bne.b	L558
	movq	&3,%d1
	cmp.l	%d1,_processor
	beq.b	L558
	mov.l	12(%a6),-(%sp)
	mov.l	-16(%a6),%a0
	mov.l	52(%a0),-(%sp)
	bsr.l	_tablewalk
	addq.w	&8,%sp
	mov.l	%d0,-20(%a6)
	beq.b	L557
	mov.l	%d0,%a0
	bftst	3(%a0){&6:&2}
	beq.b	L557
L558:
	mov.l	&10,-(%sp)
	mov.l	%a5,-(%sp)
	mov.l	_fault_hook,%a0
	jsr	(%a0)
	addq.w	&8,%sp
	tst.l	%d0
	beq.w	L9000015
	mov.l	12(%a6),-(%sp)
	mov.l	-16(%a6),-(%sp)
	mov.l	16(%a6),-(%sp)
	mov.l	-16(%a6),-(%sp)
	bsr.l	_pfault
	add.w	&16,%sp
	mov.l	%d0,%d7
	bra.b	L556
	lalign	8
L557:
	mov.l	12(%a6),-(%sp)
	mov.l	-16(%a6),-(%sp)
	mov.l	16(%a6),-(%sp)
	mov.l	-16(%a6),-(%sp)
	bsr.l	_vfault
	add.w	&16,%sp
	mov.l	%d0,%d7
	beq.b	L556
	mov.l	%d7,-(%sp)
L9000014:
	mov.l	%a5,-(%sp)
	mov.l	_fault_hook,%a0
	jsr	(%a0)
	addq.w	&8,%sp
L556:
	tst.l	%d7
	bne.w	L554
	tst.l	%d6
	beq.w	L9000018
	movq	&1,%d4
	bra.b	L539
	lalign	8
L566:
	cmp.l	-16(%a6),&_kernvas
	bne.b	L567
	mov.l	12(%a6),-(%sp)
	mov.l	&L568,-(%sp)
	bra.b	L9000017
	lalign	8
L567:
	mov.l	12(%a6),-(%sp)
	mov.l	&L570,-(%sp)
L9000017:
	bsr.l	_printf
	addq.w	&8,%sp
	mov.l	%a5,-(%sp)
	bsr.l	_buserr_panic
	addq.w	&4,%sp
L565:
	mov.l	%d7,-(%sp)
	mov.l	-4(%a6),-(%sp)
	bsr.l	_psignal
	addq.w	&8,%sp
L539:
	mov.l	-4(%a6),%a0
	mov.l	32(%a0),%d0
	and.l	&0x1000000,%d0
	beq.b	L571
	mov.l	12(%a6),-(%sp)
	mov.l	-16(%a6),-(%sp)
	mov.l	%a5,-(%sp)
	bsr.l	_umem_bus_return
	add.w	&12,%sp
L571:
	mov.l	&_u+0x82,%a0
	mov.w	%d5,(%a0)
	cmp.w	(%a0),&134
	bne.b	L573
	clr.w	(%a0)
L573:
	mov.l	-4(%a6),%a0
	tst.b	14(%a0)
	bne.b	L575
	tst.l	16(%a0)
	beq.w	L574
	bsr.l	_issig
	mov.l	%d0,-12(%a6)
	bge.b	L578
	mov.l	&_ipc+0x4,%a0
	clr.l	(%a0)
	mov.l	&_ipc,-(%sp)
	bsr.l	_wakeup
	addq.w	&4,%sp
	mov.l	-12(%a6),%d0
	neg.l	%d0
	mov.l	%d0,-(%sp)
	bsr.l	_exit
	addq.w	&4,%sp
	tst.l	%d0
	bne.b	L575
	bra.w	L574
	lalign	8
L578:
	tst.l	-12(%a6)
	beq.w	L574
L575:
	tst.l	%d4
	beq.b	L579
	mov.l	-4(%a6),%a0
	mov.b	14(%a0),%d0
	extb.l	%d0
	subq.l	&1,%d0
	movq	&1,%d1
	lsl.l	%d0,%d1
	mov.l	28(%a0),%d0
	and.l	%d1,%d0
	beq.b	L579
	mov.b	14(%a0),%d0
	extb.l	%d0
	mov.l	%d0,-(%sp)
	mov.l	-4(%a6),-(%sp)
	bsr.l	_psignal
	addq.w	&8,%sp
	mov.l	-4(%a6),%a0
	clr.b	14(%a0)
	movq	&0,%d0
	mov.w	64(%a5),%d0
	bset	&15,%d0
	mov.w	%d0,64(%a5)
	bra.b	L574
	lalign	8
L579:
	movq	&3,%d7
	cmp.l	%d7,_processor
	bne.b	L581
	mov.l	&_u+0x516,%a0
	mov.b	(%a0),%d0
	and.l	&0xff,%d0
	bset	&4,%d0
	mov.b	%d0,(%a0)
	mov.l	&124,-(%sp)
	mov.l	&_u+0x528,%a0
	mov.l	(%a0),-(%sp)
	mov.l	%a5,-(%sp)
	bsr.l	_bcopy
	add.w	&12,%sp
L581:
	bsr.l	_psig
L574:
	mov.l	-4(%a6),%a1
	mov.l	%a1,%a0
	mov.b	8(%a0),9(%a1)
	mov.l	-4(%a6),%a0
	bset	&1,33(%a0)
	tst.l	_runrun
	beq.b	L583
	bsr.l	_spl6
	mov.l	%d0,%d7
	mov.l	-4(%a6),-(%sp)
	bsr.l	_setrq
	addq.w	&4,%sp
	mov.l	&_u+0x2fc,%a0
	addq.l	&1,(%a0)
	bsr.l	_swtch
	mov.l	%d7,-(%sp)
	bsr.l	_splx
	addq.w	&4,%sp
L583:
	mov.l	&_u+0x458,%a0
	tst.l	(%a0)
	beq.b	L584
	mov.l	_u,%a0
	mov.l	196(%a0),%d0
	sub.l	-8(%a6),%d0
	mov.l	%d0,-8(%a6)
	beq.b	L584
	mov.w	64(%a5),%d0
	and.w	&0x2000,%d0
	tst.w	%d0
	bne.b	L584
	mov.l	-8(%a6),-(%sp)
	mov.l	&_u+0x44c,-(%sp)
	mov.l	66(%a5),-(%sp)
	bsr.l	_addupc
	add.w	&12,%sp
L584:
	mov.l	-4(%a6),%a0
	mov.b	9(%a0),_curpri
	bra.w	L547
	global	_pmmubuserror
_pmmubuserror:
	link.l	%a6,&LF10
	movm.l	&LS10,(%sp)
	clr.l	-16(%a6)
	pea	-4(%a6)
	pea	-10(%a6)
	pea	-8(%a6)
	pea	8(%a6)
	bsr.l	_get_fault
	add	&16,%sp
	mov.l	-8(%a6),_p_fault_addr
	mov.l	-4(%a6),_p_fault_fc
       mov.l   %a0,-(%sp) 
       mov.l   %a1,-(%sp) 
       mov.l   %d0,-(%sp) 
	mov.l	_p_fault_addr,%a0 
       mov.l   &_pmmu_sr,%a1 
       mov.l   _p_fault_fc,%d0 
       long    0xf0109e08 
       long    0xf0116200 
       mov.l   (%sp)+,%d0 
       mov.l   (%sp)+,%a1 
       mov.l   (%sp)+,%a0 
	mov.l	_pmmu_sr,%d7
	cmp.l	_processor,&1
	bne	L591
	mov.w	-10(%a6),%d0
	and.w	&0x100,%d0
	tst.w	%d0
	beq	L591
	mov.w	-10(%a6),%d0
	and.w	&0x80,%d0
	tst.w	%d0
	beq	L591
	mov.l	%d7,%d0
	and.l	&0x8c000000,%d0
	tst.l	%d0
	bne	L591
   mov.l   %a0,-(%sp) 
   mov.l   %d0,-(%sp) 
   mov.l   _p_fault_addr,%a0 
   mov.l   _p_fault_fc,%d0 
   long    0xf0102008
   mov.l   (%sp)+,%d0 
   mov.l   (%sp)+,%a0 
	bra	L590
L591:
	mov.w	-10(%a6),%d0
	and.w	&0xc0,%d0
	cmp.w	%d0,&64
	beq	L592
	mov.l	%d7,%d0
	and.l	&0x4000000,%d0
	tst.l	%d0
	bne	L592
	mov.l	%d7,%d0
	and.l	&0x8000000,%d0
	tst.l	%d0
	beq	L592
	mov.l	&1,-16(%a6)
L592:
	mov.l	-4(%a6),-(%sp)
	mov.l	%d7,%d0
	and.l	&0x4000000,%d0
	mov.l	%d0,-(%sp)
	mov.l	%d7,%d0
	and.l	&0x80000000,%d0
	mov.l	%d0,-(%sp)
	mov.l	-16(%a6),-(%sp)
	mov.w	-10(%a6),%d0
	and.w	&0xc0,%d0
	cmp.w	%d0,&64
	beq	L593
	mov.l	&1,%d0
	bra	L594
L593:
	mov.l	&0,%d0
L594:
	and.l	&0xffff,%d0
	mov.l	%d0,-(%sp)
	mov.l	-8(%a6),-(%sp)
	pea	8(%a6)
	bsr.l	_do_buserr
	add	&28,%sp
L590:
	movm.l	(%sp),&LS10
	unlk	%a6
	rts
	set	LF10,-20
	set	LS10,128
	global	_buserror
_buserror:
	link.w	%a6,&-12
	mov.l	%d7,-(%sp)
	mov.l	&541016078,%a0
	mov.w	(%a0),%d7
	and.l	&0xffff,%d7
	movq	&0,%d0
	mov.w	%d7,%d0
	and.w	&0x1ff7,%d0
	mov.w	%d0,(%a0)
	pea	-12(%a6)
	pea	-6(%a6)
	pea	-4(%a6)
	pea	8(%a6)
	bsr.l	_get_fault
	add.w	&16,%sp
	mov.l	-12(%a6),-(%sp)
	movq	&0,%d0
	mov.w	%d7,%d0
	and.l	&0xc000,%d0
	mov.l	%d0,-(%sp)
	mov.w	%d7,%d0
	and.w	&0x8,%d0
	and.l	&0xffff,%d0
	mov.l	%d0,-(%sp)
	mov.w	%d7,%d0
	and.w	&0x2000,%d0
	and.l	&0xffff,%d0
	mov.l	%d0,-(%sp)
	mov.w	-6(%a6),%d0
	and.w	&0x40,%d0
	tst.w	%d0
	bne.b	L597
	movq	&1,%d0
	bra.b	L598
	lalign	8
L597:
	movq	&0,%d0
L598:
	and.l	&0xffff,%d0
	mov.l	%d0,-(%sp)
	mov.l	-4(%a6),-(%sp)
	pea	8(%a6)
	bsr.l	_do_buserr
	add.w	&28,%sp
	mov.l	(%sp)+,%d7
	unlk	%a6
	rts
	global	_writeback_handler
_writeback_handler:
	link.w	%a6,&-16
	mov.l	%d7,-(%sp)
	mov.l	%d2,-(%sp)
	mov.l	8(%a6),%a0
	mov.w	76(%a0),%d7
	and.l	&0xffff,%d7
	clr.l	-4(%a6)
	mov.b	83(%a0),%d0
	and.l	&0xff,%d0
	and.w	&0x80,%d0
	tst.w	%d0
	beq.w	L601
	mov.b	83(%a0),%d0
	and.b	&0x18,%d0
	and.l	&0xff,%d0
	lsr.l	&3,%d0
	cmp.l	%d0,&1
	bne.b	L602
	mov.l	8(%a6),%a0
	mov.b	83(%a0),%d0
	and.b	&0x7,%d0
	and.l	&0xff,%d0
	mov.l	%d0,-(%sp)
	mov.l	8(%a6),%a0
	pea	108(%a0)
	mov.l	8(%a6),%a0
	mov.l	104(%a0),%d0
	and.b	&0xf0,%d0
	mov.l	%d0,-(%sp)
	bsr.l	_writeback_line
	add.w	&12,%sp
	tst.l	%d0
	bne.w	L9000025
	bra.w	L601
	lalign	8
L602:
	mov.l	8(%a6),%a0
	mov.b	83(%a0),%d0
	and.b	&0x60,%d0
	and.l	&0xff,%d0
	lsr.l	&5,%d0
	mov.l	%d0,-8(%a6)
	mov.l	8(%a6),%a0
	mov.l	104(%a0),%d0
	and.l	&0x3,%d0
	mov.l	%d0,-12(%a6)
	mov.l	-8(%a6),%d0
	cmp.l	%d0,&3
	bhi.w	L617
	mov.l	(L619,%za0,%d0.l*4),%a0
	jmp	(%a0)
L620:
	lalign	4
L619:
	long	L609
	long	L610
	long	L613
	long	L616
L607:
	mov.l	8(%a6),%a0
	mov.b	83(%a0),%d0
	and.b	&0x7,%d0
	and.l	&0xff,%d0
	mov.l	%d0,-(%sp)
	mov.l	8(%a6),%a0
	mov.b	83(%a0),%d0
	and.b	&0x60,%d0
	and.l	&0xff,%d0
	lsr.l	&5,%d0
	mov.l	%d0,-(%sp)
	mov.l	-16(%a6),-(%sp)
	mov.l	8(%a6),%a0
	mov.l	104(%a0),-(%sp)
	bsr.l	_writeback
	add.w	&16,%sp
	tst.l	%d0
	beq.b	L601
L9000025:
	mov.l	8(%a6),-(%sp)
	bsr.l	_writeback_signal
	addq.w	&4,%sp
L601:
	mov.w	%d7,%d0
	and.w	&0x18,%d0
	and.l	&0xffff,%d0
	lsr.l	&3,%d0
	tst.l	%d0
	bne.b	L623
	mov.w	%d7,%d0
	and.w	&0x7,%d0
	tst.w	%d0
	bne.b	L623
	mov.l	8(%a6),%a0
	mov.b	83(%a0),%d0
	and.l	&0xff,%d0
	and.w	&0x80,%d0
	tst.w	%d0
	beq.b	L624
	mov.l	&L625,-(%sp)
	bsr.l	_panic
	addq.w	&4,%sp
L624:
	movq	&1,%d7
	mov.l	%d7,-4(%a6)
	mov.l	&5,-(%sp)
	mov.l	8(%a6),%a0
	pea	108(%a0)
	mov.l	8(%a6),%a0
	mov.l	84(%a0),%d0
	and.b	&0xf0,%d0
	mov.l	%d0,-(%sp)
	bsr.l	_writeback_line
	add.w	&12,%sp
	tst.l	%d0
	beq.b	L623
	mov.l	8(%a6),-(%sp)
	bsr.l	_writeback_signal
	addq.w	&4,%sp
L623:
	mov.l	8(%a6),%a0
	mov.b	81(%a0),%d0
	and.l	&0xff,%d0
	and.w	&0x80,%d0
	tst.w	%d0
	beq.w	L627
	mov.b	81(%a0),%d0
	and.b	&0x18,%d0
	and.l	&0xff,%d0
	lsr.l	&3,%d0
	cmp.l	%d0,&1
	beq.b	L627
	mov.l	8(%a6),%a0
	mov.b	81(%a0),%d0
	and.b	&0x7,%d0
	and.l	&0xff,%d0
	mov.l	%d0,-(%sp)
	mov.l	8(%a6),%a0
	mov.b	81(%a0),%d0
	and.b	&0x60,%d0
	and.l	&0xff,%d0
	lsr.l	&5,%d0
	mov.l	%d0,-(%sp)
	mov.l	8(%a6),%a0
	mov.l	100(%a0),-(%sp)
	mov.l	8(%a6),%a0
	mov.l	96(%a0),-(%sp)
	bsr.l	_writeback
	add.w	&16,%sp
	tst.l	%d0
	beq.b	L627
	mov.l	8(%a6),-(%sp)
	bsr.l	_writeback_signal
	addq.w	&4,%sp
L627:
	mov.l	8(%a6),%a0
	mov.b	81(%a0),%d0
	and.l	&0xff,%d0
	and.w	&0x80,%d0
	tst.w	%d0
	beq.w	L629
	mov.b	81(%a0),%d0
	and.b	&0x18,%d0
	and.l	&0xff,%d0
	lsr.l	&3,%d0
	cmp.l	%d0,&1
	bne.b	L629
	mov.l	8(%a6),%a0
	mov.b	83(%a0),%d0
	and.l	&0xff,%d0
	and.w	&0x80,%d0
	tst.w	%d0
	bne.b	L629
	tst.l	-4(%a6)
	bne.b	L629
	mov.b	81(%a0),%d0
	and.b	&0x7,%d0
	and.l	&0xff,%d0
	mov.l	%d0,-(%sp)
	mov.l	8(%a6),%a0
	pea	108(%a0)
	mov.l	8(%a6),%a0
	mov.l	96(%a0),%d0
	and.b	&0xf0,%d0
	mov.l	%d0,-(%sp)
	bsr.l	_writeback_line
	add.w	&12,%sp
	tst.l	%d0
	beq.b	L629
	mov.l	8(%a6),-(%sp)
	bsr.l	_writeback_signal
	addq.w	&4,%sp
L629:
	mov.l	8(%a6),%a0
	mov.b	79(%a0),%d0
	and.l	&0xff,%d0
	and.w	&0x80,%d0
	tst.w	%d0
	beq.b	L600
	mov.b	79(%a0),%d0
	and.b	&0x7,%d0
	and.l	&0xff,%d0
	mov.l	%d0,-(%sp)
	mov.l	8(%a6),%a0
	mov.b	79(%a0),%d0
	and.b	&0x60,%d0
	and.l	&0xff,%d0
	lsr.l	&5,%d0
	mov.l	%d0,-(%sp)
	mov.l	8(%a6),%a0
	mov.l	92(%a0),-(%sp)
	mov.l	8(%a6),%a0
	mov.l	88(%a0),-(%sp)
	bsr.l	_writeback
	add.w	&16,%sp
	tst.l	%d0
	beq.b	L600
	mov.l	8(%a6),-(%sp)
	bsr.l	_writeback_signal
	addq.w	&4,%sp
L600:
	mov.l	(%sp)+,%d2
	mov.l	(%sp)+,%d7
	unlk	%a6
	rts
L609:
	mov.l	8(%a6),%a0
	mov.l	108(%a0),%d0
	movq	&3,%d1
L9000026:
	sub.l	-12(%a6),%d1
	lsl.l	&3,%d1
	asr.l	%d1,%d0
	mov.l	%d0,-16(%a6)
	bra.w	L607
	lalign	8
L610:
	movq	&3,%d1
	cmp.l	%d1,-12(%a6)
	bne.b	L611
	mov.l	8(%a6),%a0
	mov.l	108(%a0),%d0
	movq	&24,%d1
	asr.l	%d1,%d0
	mov.l	8(%a6),%a0
	mov.l	108(%a0),%d1
	lsl.l	&8,%d1
	or.l	%d0,%d1
	mov.l	%d1,-16(%a6)
	bra.w	L607
	lalign	8
L611:
	mov.l	8(%a6),%a0
	mov.l	108(%a0),%d0
	movq	&2,%d1
	bra.b	L9000026
	lalign	8
L613:
	tst.l	-12(%a6)
	beq.b	L614
	mov.l	8(%a6),%a0
	mov.l	108(%a0),%d0
	mov.l	-12(%a6),%d1
	lsl.l	&3,%d1
	lsl.l	%d1,%d0
	mov.l	-12(%a6),%d1
	lsl.l	&3,%d1
	movq	&32,%d2
	sub.l	%d1,%d2
	mov.l	108(%a0),%d1
	asr.l	%d2,%d1
	or.l	%d1,%d0
	mov.l	%d0,-16(%a6)
	bra.w	L607
	lalign	8
L614:
	mov.l	8(%a6),%a0
	mov.l	108(%a0),-16(%a6)
	bra.w	L607
	lalign	8
L616:
	mov.l	8(%a6),%a0
	mov.l	108(%a0),-16(%a6)
	bra.w	L607
	lalign	8
L617:
	mov.l	&L618,-(%sp)
	bsr.l	_panic
	addq.w	&4,%sp
	bra.w	L607
	global	_writeback_signal
_writeback_signal:
	link.w	%a6,&-20
	mov.l	%a5,-(%sp)
	mov.l	8(%a6),%a5
	mov.l	_u,-4(%a6)
	mov.l	&_u+0x524,%a0
	mov.l	(%a0),-20(%a6)
	mov.l	_user_area,%a0
	add.w	&646,%a0
	mov.l	%a0,%d0
	cmp.l	%d0,66(%a5)
	beq.w	L633
	mov.l	_u,%a0
	mov.l	196(%a0),-8(%a6)
	mov.l	&_u+0x8e,%a0
	clr.w	(%a0)
	mov.l	&_u+0x8c,%a0
	clr.b	(%a0)
	mov.l	&_u+0x4,%a0
	mov.l	%a5,(%a0)
	mov.l	-20(%a6),-(%sp)
	mov.l	-4(%a6),-(%sp)
	bsr.l	_psignal
	addq.w	&8,%sp
	mov.l	-4(%a6),%a0
	tst.b	14(%a0)
	bne.b	L636
	tst.l	16(%a0)
	beq.w	L635
	bsr.l	_issig
	mov.l	%d0,-16(%a6)
	bge.b	L639
	mov.l	&_ipc+0x4,%a0
	clr.l	(%a0)
	mov.l	&_ipc,-(%sp)
	bsr.l	_wakeup
	addq.w	&4,%sp
	mov.l	-16(%a6),%d0
	neg.l	%d0
	mov.l	%d0,-(%sp)
	bsr.l	_exit
	addq.w	&4,%sp
	tst.l	%d0
	bne.b	L636
	bra.b	L635
	lalign	8
L639:
	tst.l	-16(%a6)
	beq.b	L635
L636:
	mov.l	&_u+0x516,%a0
	mov.b	(%a0),%d0
	and.l	&0xff,%d0
	bset	&4,%d0
	mov.b	%d0,(%a0)
	mov.l	&124,-(%sp)
	mov.l	&_u+0x528,%a0
	mov.l	(%a0),-(%sp)
	mov.l	%a5,-(%sp)
	bsr.l	_bcopy
	add.w	&12,%sp
	bsr.l	_psig
L635:
	mov.l	-4(%a6),%a1
	mov.l	%a1,%a0
	mov.b	8(%a0),9(%a1)
	mov.l	-4(%a6),%a0
	bset	&1,33(%a0)
	tst.l	_runrun
	beq.b	L640
	bsr.l	_spl6
	mov.l	%d0,-12(%a6)
	mov.l	-4(%a6),-(%sp)
	bsr.l	_setrq
	addq.w	&4,%sp
	mov.l	&_u+0x2fc,%a0
	addq.l	&1,(%a0)
	bsr.l	_swtch
	mov.l	-12(%a6),-(%sp)
	bsr.l	_splx
	addq.w	&4,%sp
L640:
	mov.l	&_u+0x458,%a0
	tst.l	(%a0)
	beq.b	L641
	mov.l	_u,%a0
	mov.l	196(%a0),%d0
	sub.l	-8(%a6),%d0
	mov.l	%d0,-8(%a6)
	beq.b	L641
	mov.w	64(%a5),%d0
	and.w	&0x2000,%d0
	tst.w	%d0
	bne.b	L641
	mov.l	-8(%a6),-(%sp)
	mov.l	&_u+0x44c,-(%sp)
	mov.l	66(%a5),-(%sp)
	bsr.l	_addupc
	add.w	&12,%sp
L641:
	mov.l	-4(%a6),%a0
	mov.b	9(%a0),_curpri
L633:
	mov.l	(%sp)+,%a5
	unlk	%a6
	rts
	global	_mc68040_access_error
_mc68040_access_error:
	link.w	%a6,&-12
	mov.l	%a5,-(%sp)
	mov.l	%d7,-(%sp)
	mov.l	%d6,-(%sp)
	mov.l	%a6,%a5
	addq.l	&8,%a5
	mov.l	%a5,%a0
	mov.w	76(%a5),%d7
	and.l	&0xffff,%d7
	clr.l	-8(%a6)
	clr.l	-12(%a6)
	mov.w	%d7,%d0
	and.w	&0x200,%d0
	tst.w	%d0
	beq.b	L645
	movq	&0,%d0
	mov.w	%d7,%d0
	bclr	&8,%d0
	mov.w	%d0,%d7
	and.l	&0xffff,%d7
L645:
	mov.w	%d7,%d0
	and.w	&0x400,%d0
	tst.w	%d0
	beq.b	L646
	mov.w	%d7,%d0
	and.w	&0x800,%d0
	tst.w	%d0
	beq.b	L646
	mov.l	84(%a5),%d0
	add.l	&4095,%d0
	and.w	&0xf000,%d0
	mov.l	%d0,-4(%a6)
	bra.b	L647
	lalign	8
L646:
	mov.l	84(%a5),-4(%a6)
L647:
	mov.w	%d7,%d0
	and.w	&0x400,%d0
	tst.w	%d0
	beq.b	L648
	mov.w	%d7,%d0
	and.w	&0x100,%d0
	and.l	&0xffff,%d0
	mov.l	%d0,-(%sp)
	mov.w	%d7,%d0
	and.w	&0x7,%d0
	and.l	&0xffff,%d0
	mov.l	%d0,-(%sp)
	mov.l	-4(%a6),-(%sp)
	bsr.l	_mc68040_ptest
	add.w	&12,%sp
	mov.l	%d0,%d6
	bra.b	L650
	lalign	8
L648:
	mov.l	&2049,%d6
L650:
	mov.l	%d6,%d0
	and.l	&0x1,%d0
	beq.b	L651
	mov.l	%d6,%d0
	and.l	&0x4,%d0
	beq.b	L651
	mov.w	%d7,%d0
	and.w	&0x100,%d0
	tst.w	%d0
	bne.b	L651
	movq	&1,%d1
	mov.l	%d1,-8(%a6)
L651:
	mov.w	%d7,%d0
	and.w	&0x7,%d0
	and.l	&0xffff,%d0
	mov.l	%d0,-(%sp)
	btst	&0,%d6
	bne.b	L653
	movq	&1,%d0
	bra.b	L654
	lalign	8
L653:
	movq	&0,%d0
L654:
	mov.l	%d0,-(%sp)
	mov.l	%d6,%d0
	and.l	&0x800,%d0
	mov.l	%d0,-(%sp)
	mov.l	-8(%a6),-(%sp)
	mov.w	%d7,%d0
	and.w	&0x100,%d0
	tst.w	%d0
	bne.b	L655
	movq	&1,%d0
	bra.b	L656
	lalign	8
L655:
	movq	&0,%d0
L656:
	mov.l	%d0,-(%sp)
	mov.l	-4(%a6),-(%sp)
	pea	8(%a6)
	bsr.l	_do_buserr
	add.w	&28,%sp
	mov.l	%d0,-12(%a6)
	beq.b	L644
	movq	&2,%d7
	cmp.l	%d7,%d0
	bne.b	L658
	bsr.l	_has_procmemreserved
	tst.l	%d0
	bne.b	L658
	clr.l	-(%sp)
	mov.l	&1,-(%sp)
	mov.l	&10,-(%sp)
	bsr.l	_procmemreserve
	add.w	&12,%sp
L658:
	pea	8(%a6)
	bsr.l	_writeback_handler
	addq.w	&4,%sp
	movq	&2,%d7
	cmp.l	%d7,-12(%a6)
	bne.b	L644
	bsr.l	_has_procmemreserved
	tst.l	%d0
	beq.b	L644
	bsr.l	_procmemunreserve
L644:
	mov.l	(%sp)+,%d6
	mov.l	(%sp)+,%d7
	mov.l	(%sp)+,%a5
	unlk	%a6
	rts
	bss
	lalign	4
_p_fault_fc:
	space	4*1
	lalign	4
_p_fault_addr:
	space	4*1
	lalign	4
_pmmu_sr:
	space	4*1
	data
L302:
	byte	0
L303:
	byte	0
L304:
	byte	66,117,115,32,101,114,114,111,114,0
L305:
	byte	65,100,100,114,101,115,115,32,101,114,114,111,114,0
L306:
	byte	73,108,108,101,103,97,108,32,105,110,115,116,114,117,99,116
	byte	105,111,110,0
L307:
	byte	90,101,114,111,32,100,105,118,105,100,101,0
L308:
	byte	67,72,75,32,105,110,115,116,114,117,99,116,105,111,110,0
L309:
	byte	84,82,65,80,86,32,105,110,115,116,114,117,99,116,105,111
	byte	110,0
L310:
	byte	80,114,105,118,105,108,101,103,101,32,118,105,111,108,97,116
	byte	105,111,110,0
L311:
	byte	84,114,97,99,101,0
L312:
	byte	76,105,110,101,32,49,48,49,48,32,101,109,117,108,97,116
	byte	111,114,0
L313:
	byte	76,105,110,101,32,49,49,49,49,32,101,109,117,108,97,116
	byte	111,114,0
L315:
	byte	116,114,97,112,32,116,121,112,101,32,37,120,44,32,112,99
	byte	32,61,32,37,120,44,32,112,115,32,61,32,37,120,10,0
L317:
	byte	98,117,115,101,114,114,58,32,112,99,32,61,32,37,120,44
	byte	32,112,115,32,61,32,37,120,10,0
L331:
	byte	80,82,79,66,69,95,77,65,83,75,0
L334:
	byte	116,114,97,112,0
L337:
	byte	84,95,85,78,73,77,80,95,68,65,84,65,32,105,110,32
	byte	115,117,112,101,114,118,105,115,111,114,121,32,115,116,97,116
	byte	101,0
L339:
	byte	84,95,85,78,73,77,80,95,68,65,84,65,32,105,110,32
	byte	115,117,112,101,114,118,105,115,111,114,121,32,115,116,97,116
	byte	101,32,111,110,32,110,111,110,45,54,56,48,52,48,0
L343:
	byte	84,95,85,78,73,77,80,95,68,65,84,65,43,85,83,69
	byte	82,32,111,110,32,110,111,110,45,54,56,48,52,48,0
L378:
	byte	102,112,32,101,114,114,111,114,32,105,110,32,115,117,112,101
	byte	114,118,105,115,111,114,121,32,115,116,97,116,101,58,32,112
	byte	114,111,98,101,32,110,111,116,32,115,101,116,0
L380:
	byte	108,111,110,103,106,117,109,112,105,110,103,32,111,110,32,102
	byte	112,32,101,114,114,111,114,0
L404:
	byte	83,112,117,114,105,111,117,115,32,105,110,116,101,114,114,117
	byte	112,116,10,0
L483:
	byte	105,110,32,115,121,115,99,97,108,108,32,111,114,32,107,101
	byte	114,110,101,108,32,112,114,111,99,101,115,115,0
L485:
	byte	104,97,110,100,108,105,110,103,32,100,101,118,105,99,101,32
	byte	105,110,116,101,114,114,117,112,116,0
L487:
	byte	98,117,115,32,101,114,114,111,114,32,119,104,105,108,101,32
	byte	37,115,32,40,73,76,61,37,100,44,32,116,121,112,101,61
	byte	48,120,37,120,44,32,80,67,61,48,120,37,120,41,0
L493:
	byte	54,56,48,49,48,32,116,114,97,112,0
L533:
	byte	66,97,100,32,107,101,114,110,101,108,32,114,101,102,101,114
	byte	101,110,99,101,32,116,111,32,117,115,101,114,32,97,100,100
	byte	114,101,115,115,32,48,120,37,120,10,0
L534:
	byte	117,115,101,114,32,97,100,100,114,0
L568:
	byte	75,101,114,110,101,108,32,98,117,115,32,101,114,114,111,114
	byte	32,111,110,32,97,100,100,114,101,115,115,32,48,120,37,120
	byte	10,0
L570:
	byte	75,101,114,110,101,108,32,114,101,102,101,114,101,110,99,101
	byte	32,116,111,32,98,97,100,32,117,115,101,114,32,97,100,100
	byte	114,32,48,120,37,120,10,0
L618:
	byte	119,98,115,49,58,32,117,110,101,120,112,101,99,116,101,100
	byte	32,115,105,122,101,0
L625:
	byte	112,104,121,115,105,99,97,108,32,112,117,115,104,32,97,99
	byte	99,101,115,115,32,101,114,114,111,114,32,119,105,116,104,32
	byte	119,98,115,49,32,118,97,108,105,100,0
	version	2
