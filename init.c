/*
 * init
 */
#include "tetris.h"
#include "locore.h"

typedef void (*func)();
func IntTable[14];
volatile int VsyncCount;
int volume;
int connected;

void check_connect();

void
init()
{
	int data;
	extern void IntIRQ(), IntFIQ();
	extern void timer0_handler(), timer2_handler();

	/*
	 * system clock
	 */
	sys_clock(5);

	/*
	 * sound volume
	 */
	data = sys_status();
	volume = (data >> 18) & 0x3;
	volume = volume * volume * 2;

	/*
	 * mask interrupt
	 */
	*(volatile unsigned int *)0xa00000c = 0x3fff;
	*(volatile unsigned int *)0xa000008 = 0x0a00;

	/*
	 * regist handler
	 */
	sys_handler(1, IntIRQ);
	sys_handler(2, IntFIQ);

	IntTable[7] = timer0_handler;
	IntTable[13] = timer2_handler;

	/*
	 * timer0
	 */
	*(volatile unsigned int *)0xa800008 &= ~0x4;	/* stop timer */
	*(volatile unsigned int *)0xa800000 = 15625;

	/* enable interrupt */
	*(volatile unsigned int *)0xa000010 = 0x2080;
	*(volatile unsigned int *)0xa000008 = 0x2080;

	*(volatile unsigned int *)0xa800008 |= 0x4;	/* start timer */

	/*
	 * LCD on 32hz
	 */
	data = *(volatile unsigned int *)0xd000000;
	data &= ~0x7f;
	data |= 0x68;
	*(volatile unsigned int *)0xd000000 = data;
}

void
sleep()
{

	if (connected)
		return;

	*(volatile unsigned int *)0xc800000 = 0x02;
	*(volatile unsigned int *)0xd000000 &= ~0x68;
	*(volatile unsigned int *)0xa00000c = 0x3fff;
	*(volatile unsigned int *)0xa000008 = (1 << 0);
	*(volatile unsigned int *)0xa800008 &= ~0x4;	/* stop timer */

	/*
	 * zzz...
	 */
	*(volatile unsigned int *)0xb000004 = 0x1;

	/*
	 * wake up
	 */
	*(volatile unsigned int *)0xd000000 |= 0x68;
	*(volatile unsigned int *)0xa00000c = 0x3fff;
	*(volatile unsigned int *)0xa000008 = 0x2080;
	*(volatile unsigned int *)0xa000010 = 0x2080;
	*(volatile unsigned int *)0xa800008 |= 0x4;	/* start timer */
}

void
app_exit()
{
	int num;

	*(volatile unsigned int *)0xd800000 = 0x02;
	*(volatile unsigned int *)0xd800004 = 0x62;
	*(volatile unsigned int *)0xd800010 = 0x00;
	*(volatile unsigned int *)0xc800000 = 0x02;
	*(volatile unsigned int *)0xa00000c = 0x3fff;
	*(volatile unsigned int *)0xa000008 = 0x0a00;

	sys_pscomm(0);		/* close psx comunicate */

	/* score save */
	sys_save((int)(SaveDataBuf - _start) / 128, &save_data);

	num = sys_appnum();
	sys_execset(1, 0, num | 0x30);
	sys_exec();
	/* NOT REACHED */
}

void
vsync_wait()
{
	register int v = VsyncCount;

	check_connect();

	while (v == VsyncCount)
		;
}

void
irq_handler()
{
	register int status, i;

	status = *(volatile unsigned int *)0xa000000;
	status &= *(volatile unsigned int *)0xa000008;
	status &= ~0x2040;	/* clear FIQ bits */

	for (i = 12; i >= 0; i--) {
		if ((status & (1 << i)) && IntTable[i]) {
			(IntTable[i])();
		}
	}

	*(volatile unsigned int *)0xa000010 = status;
}

void
fiq_handler()
{
	register int status;

	status = *(volatile unsigned int *)0xa000000;
	status &= 0x2000;	/* FIQ bit */

	if (IntTable[13])
		(IntTable[13])();

	*(volatile unsigned int *)0xa000010 = status;
}

void
timer0_handler()
{
	VsyncCount++;
}

void
timer2_handler()
{
}

void
check_connect()
{

	if (*(volatile int *)0xd80000c & 0x10) {
		/* connect */

		if (connected)
			return;

		connected++;

		*(volatile char *)0xd000000 |= 0x80;	/* rotate 180 degree */

		*(volatile unsigned int *)0xa800008 &= ~0x4; /* stop timer0 */
		*(volatile unsigned int *)0xa800028 &= ~0x4; /* stop timer2 */

		*(volatile unsigned int *)0xa800000 = 2000000/32;
		sys_pscomm(1);
		*(volatile unsigned int *)0xa800008 |= 0x4; /* start timer0 */
	} else {
		/* not connect */

		if (connected == 0)
			return;

		connected = 0;

		*(volatile char *)0xd000000 &= ~0x80;	/* reset rotate */

		*(volatile unsigned int *)0xa800008 &= ~0x4; /* stop timer0 */
		*(volatile unsigned int *)0xa800028 &= ~0x4; /* stop timer2 */

		*(volatile unsigned int *)0xa800000 = 500000/32;
		sys_clock(5);
		*(volatile unsigned int *)0xa800008 |= 0x4; /* start timer0 */
	}
}
