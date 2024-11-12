/*
 * locore.S
 *   Original is "L.O. for PK (c)1999 K-Communications".
 */
	.text
	.code 32

	.globl _start
_start:

/*
 * header
 */
	.ascii	"SC"		/* ID */
	.byte	0x11		/* 7-4bit: 1, 3-0bit: ICON number */
	.byte	1		/* memory card block number */

	      /* 01234567890123456789012345678901  TITLE */
	.ascii	"てとりす　　　　　　　　　　　　"
	.ascii	"　　　　　　　　　　　　　　　"
	.byte	0,0

	.space	12

	.byte	1,0
	.ascii	"MCX0"
	.byte	1
	.byte	0
	.word	0

	.word	_progstart

	.space	16*2
	.space	16*16/2

IconAp:
	.word	0x00000000,0x00000000,0x00000000,0x00000000
	.word	0x00000000,0x00000000,0x1919ee3c,0x3dbe7fcf
	.word	0x7ffe60cc,0x67e660cc,0x07c667cc,0x0ffe61cc
	.word	0x399e60cc,0x61b660cc,0x67b664cc,0x7fe664cc
	.word	0x3ce227c4,0x00000000,0x00000000,0x00000000
	.word	0x00000000,0x00000000,0x00000000,0x00000000
	.word	0x00000000,0x00000000,0x00000000,0x00000000
	.word	0x00000000,0x00000000,0x00000000,0x00000000

	.byte	1,0
	.byte	1,0
	.word	IconAp

	.org	0x200
	.globl	SaveDataBuf
SaveDataBuf:
	.space	128

_progstart:
	b	main
	/* not reach */

/*
 * dummy
 */
	.globl	__gccmain
__gccmain:
	mov	pc, lr

/*
 * swi
 */
	/*
	 * set interrupt handler
	 * r0: 0=swi2 1=IRQ 2=FIQ 3=?
	 * r1: handler
	 */
	.globl	sys_handler
sys_handler:
	swi	1
	mov	pc, lr

	/*
	 * save
	 * r0: sector number
	 * r1: source
	 */
	.globl	sys_save
sys_save:
	swi	3
	mov	pc, lr

	/*
	 * set system clock
	 * r0: clock rate
	 */
	.globl	sys_clock
sys_clock:
	swi	4
	mov	pc, lr

	/*
	 * set exec argument
	 * r0: 1
	 * r1: 0(system program)
	 * r2: argument(r0)
	 */
	.globl	sys_execset
sys_execset:
	swi	8
	mov	pc, lr

	/*
	 * exec application
	 */
	.globl	sys_exec
sys_exec:
	swi	9
	/* not reached */

	/*
	 * get time
	 */
	.globl	sys_gettime
sys_gettime:
	swi	14
	mov	pc, lr

	/*
	 * set playstation communicate
	 * r0: 1=on
	 */
	.globl  sys_pscomm
sys_pscomm:
	swi     17
	mov	pc, lr

	/*
	 * get system status
	 */
	.globl	sys_status
sys_status:
	swi	19
	ldr	r0, [r0]
	mov	pc, lr

	/*
	 * get application number
	 */
	.globl	sys_appnum
sys_appnum:
	swi	22
	mov	pc, lr

/*
 * interrupt handler
 */
	/*
	 * IRQ
	 */
	.globl	IntIRQ
IntIRQ:
	stmdb	sp!,{r0-r3,lr}
	ldr	r0, offset_irq
	mov	lr,pc
	bx	r0
	ldmia	sp!,{r0-r3,pc}
offset_irq:
	.word	irq_handler

	/*
	 * FIQ
	 */
	.globl	IntFIQ
IntFIQ:
	stmdb	sp!,{r0-r3,lr}
	ldr	r8, offset_fiq
	mov	lr,pc
	bx	r8
	ldmia	sp!,{r0-r3,pc}
offset_fiq:
	.word	fiq_handler

	.end
