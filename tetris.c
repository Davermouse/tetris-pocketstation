/*	$NetBSD: tetris.c,v 1.5 1998/09/13 15:27:30 hubertf Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek and Darren F. Provine.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)tetris.c	8.1 (Berkeley) 5/31/93
 */

/*
 * Tetris (or however it is spelled).
 */

#include <sys/types.h>

#include "locore.h"
#include "init.h"
#include "rand.h"
#include "tetris.h"
#include "shapes.h"

static void setup_board();
static void scr_write();
static int elide();
static void cont_exit();
static int title();
static void gameover();
static int wait_key();

cell board[B_SIZE];
int score, level, line;
struct save_data save_data;

static void __memcpy( char * const a, const char * const b, int len )
{
    for( int i = 0; i < len; i++ )
    {
        a[i] = b[i];
    }
}

static void __memset( char * const a, char v, int len )
{
    for( int i = 0; i < len; i++ )
    {
        a[i] = v;
    }
}

/*
 * Set up the initial board.  The bottom display row is completely set,
 * along with another (hidden) row underneath that.  Also, the left and
 * right edges are set.
 */
static void
clear_board()
{
	__memset((void *)board, 0, B_SIZE);
}

static void
setup_board()
{
	int i, j;
	cell *p;

	p = board;
	for (j = 0; j < B_ROWS; j++) {
		if ((j == ROW_FIRST - 2) || (j == ROW_LAST + 1)) {
			for (i = 0; i < B_COLS; i++)
				*p++ = (i >= COL_FIRST - 1) &&
				       (i <= COL_LAST + 1);
		} else if ((j >= ROW_FIRST - 1) && (j <= ROW_LAST)) {
			for (i = 0; i < B_COLS; i++)
				*p++ = (i == COL_FIRST - 1) ||
				       (i == COL_LAST + 1);
		} else {
			for (i = 0; i < B_COLS; i++)
				*p++ = 0;
		}
	}
}

/*
 * Elide any full active rows.
 */
static int
elide()
{
	int i, j, base, ln;
	cell *p;

	ln = 0;
	for (i = ROW_FIRST + 1; i <= ROW_LAST; i++) {
		base = i * B_COLS + COL_FIRST;
		p = &board[base];
		for (j = COL_LENGTH; *p++ != 0;) {
			if (--j <= 0) {
				/* this row is to be elided */
				int k;

				ln++;
				__memset(&board[base], 0, COL_LENGTH);

				vsync_wait();
				scr_write();
				for (k = 0; k < 2; k++)
					vsync_wait();

				for (k = i; k >= ROW_FIRST; k--) {
					__memcpy(&board[k * B_COLS + COL_FIRST],
					   &board[(k - 1) * B_COLS + COL_FIRST],
					   COL_LENGTH);
				}
				break;
			}
		}
	}
	return (ln);
}

/*
 * Write screen board.
 */
static void
scr_write()
{
	int i, j, k;
	int d;
	volatile int *lcd;
	cell *p;

	lcd = (volatile int *)0xd000100;
	p = board;

	for (i = 0; i < B_ROWS; i++) {
		d = 0;
		for (j = 0; j < B_COLS; j++)
			d |= (*p++ == 1) << j;
		*lcd++ = d;
	}
}

int
padread()
{
	int pad;

	pad = *(volatile int *)0xa000004;
	pad |= *(volatile char *)0x203;
	*(volatile char *)0x203 = 0;
	
	return (pad);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	static int pos, l, ln;
	static int fallcount, stopflag, stopcount;
	static int ostatus, status, bcount, rcount, lcount;
	static int next_block;
	static struct shape *curshape, *nextshape;

	/*
	 * for bianero
	 */
	*(volatile int *)0x200 = 0;
	*(volatile char *)0x200 = 'B';
	*(volatile char *)0x201 = 'N';

	init();
	srand(sys_gettime());
	__memcpy((char*)&save_data, (char*)SaveDataBuf, sizeof (struct save_data));

	for (;;) {
		level = title();

		setup_board();
		pos = ROW_FIRST * B_COLS + COL_FIRST +  COL_LENGTH / 2;
		next_block = rand() % 7;
		curshape = &shapes[next_block];

		next_block = rand() % 7;
		nextshape = &shapes[next_block];
		place(nextshape, 8 * B_COLS + 25, 1);

		ln = line = 0;
		score = 0;
		bcount = rcount = lcount = 0;

		fallcount = 32 / level;
		stopflag = stopcount = 0;
		ostatus = padread();

		for (;;) {
			place(curshape, pos, 1);
			printnumber(27, 0, score);
			printnumber(COL_FIRST - 3, ROW_FIRST + 1, level);

			vsync_wait();

			status = padread();
			if (status & 1)
				bcount++;
			else
				bcount = 0;
			if (bcount >= 32 * 3)
				cont_exit();

			scr_write();
			place(curshape, pos, 0);

			if (--fallcount < 0) {
				/*
				 * Timeout.  Move down if possible.
				 */
				if (fits_in(curshape, pos + B_COLS)) {
					pos += B_COLS;
					fallcount = 32 / level;
					goto next;
				}

				if (stopflag == 0) {
					stopflag++;
					fallcount = 32 / level;
					stopcount = 10;
					goto next;
				} else {
					if (--stopcount > 0)
						goto next;
				}
				stopflag = 0;

				/*
				 * Put up the current shape `permanently',
				 * bump score, and elide any full rows.
				 */
				place(curshape, pos, 1);
				l = elide();
				score += (l * l * 10 * level);
				line += l;
				ln += l;
				if (ln >= 10) {
					level++;
					ln -= 10;
				}

				/*
				 * Choose a new shape.  If it does not fit,
				 * the game is over.
				 */
				place(nextshape, 8 * B_COLS + 25, 0);
				curshape = &shapes[next_block];

				next_block = rand() % 7;
				nextshape = &shapes[next_block];
				place(nextshape, 8 * B_COLS + 25, 1);

				pos = ROW_FIRST * B_COLS
					+ COL_FIRST + COL_LENGTH / 2;
				if (!fits_in(curshape, pos)) {
					place(curshape, pos, 1);
					break;
				}
				goto next;
			}

next:
			if (status & (1 << 1)) {
				rcount++;
				if (rcount > 5) {
					ostatus &= ~(1 << 1);
				}
			} else {
				rcount = 0;
			}
			if (status & (1 << 2)) {
				lcount++;
				if (lcount > 5) {
					ostatus &= ~(1 << 2);
				}
			} else {
				lcount = 0;
			}

			l = ostatus;
			ostatus = status;
			status &= ~(l & 7);

			if (status & (1 << 2)) {
				/* move left */
				if (fits_in(curshape, pos - 1))
					pos--;
			}
			if (status & (1 << 0)) {
				/* turn */
				struct shape *new = &shapes[curshape->rot];

				if (fits_in(new, pos))
					curshape = new;
			}
			if (status & (1 << 1)) {
				/* move right */
				if (fits_in(curshape, pos + 1))
					pos++;
			}
			if (status & (1 << 3)) {
				fallcount = 0;
				stopcount = 0;
				score++;
			}
		}

		/*
		 * gameover
		 */
		if (score >= save_data.highscore) {
			save_data.highscore = score;
			save_data.highlevel = level;
			save_data.highline = line;
		}

		gameover();
	}
}

/*
 * Title
 */
static int
title()
{
	int i, j;
	int s;
	int ostatus, status, rcount, lcount;
	static int lv;
	static int bmp_title[] = {
		0x00000000, 0x00002baa, 0x000028aa, 0x00003aae,
		0x00002aaa, 0x00002baa, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000800,
		0x00000800, 0x00014800, 0x00014800, 0x0000b800,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};

	clear_board();

	s = 0;
	for (j = 0; j < sizeof (bmp_title) / sizeof (int); j++)
		for (i = 0; i < 32; i++)
			board[s++] = ((bmp_title[j] & (1 << i)) != 0);

	printnumber(31, 7, save_data.highscore);
	printnumber(31, 13, save_data.highline);
	printnumber(13, 13, save_data.highlevel);

	lv = 1;
	ostatus = 0;
	rcount = lcount = 0;
	while (wait_key()) {
		scr_write();
		status = padread();

		if (status & ((1 << 1) | (1 << 3))) {
			rcount++;
			if (rcount > 10) {
				ostatus &= ~((1 << 1) | (1 << 3));
			}
		} else {
			rcount = 0;
		}
		if (status & ((1 << 2) | (1 << 4))) {
			lcount++;
			if (lcount > 10) {
				ostatus &= ~((1 << 2) | (1 << 4));
			}
		} else {
			lcount = 0;
		}

		s = ostatus;
		ostatus = status;
		status &= ~(s & 0x1f);

		if (status & ((1 << 2) | (1 << 4))) {
			if (--lv < 1)
				lv = 1;
		}
		if (status & ((1 << 1) | (1 << 3))) {
			if (++lv > 32)
				lv = 32;
		}

		printnumber(23, 23, 0);
		printnumber(27, 23, lv);
	}

	return (lv);
}

/*
 * Gameover
 */
static void
gameover()
{
	int i;
	volatile int *p;
	static int bmp_gameover[] = {
		0x00000000, 0xeeae7577, 0xa2aa1751, 0x6eaa7775,
		0xa2aa1555, 0xae4e7557, 0x00000000
	};

	vsync_wait();
	p = (volatile int *)(0xd000100 + 16 * sizeof (int));
	for (i = 0; i < sizeof (bmp_gameover) / sizeof (int); i++)
		*p++ = bmp_gameover[i];

	while (wait_key())
		;
}

/*
 * wait key
 */
static int
wait_key()
{
	static int bcount, sleep_count;
	int status;

	vsync_wait();
	status = padread() & 0x1f;

	if (status) {
		sleep_count = 0;
	} else {
		if (++sleep_count > 32 * 30) {
			sleep_count = 0;
			sleep();
		}
	}

	if (status & (1 << 0)) {
		while (status) {
			vsync_wait();
			status = padread() & 0x1f;

			if (status & (1 << 0))
				bcount++;
			else
				bcount = 0;
			if (bcount >= 32 * 3) {
				cont_exit();
				bcount = 0;
				status = 0;
			}
		}
		return 0;
	}
	return 1;
}

/*
 * Exit?
 */
static void
cont_exit()
{
	int timeout;
	int i;
	int *p;
	int status, ostatus;
	int select;
#define	SELECT_CONTINUE	0
#define	SELECT_EXIT	1
	static int bmp_continue[] = {
		0x00000000, 0x7575dddc, 0x15549544, 0x75549544,
		0x15549544, 0x775495dc, 0x00000000
	};
	static int bmp_exit[] = {
		0x00000000, 0x00755c00, 0x00254400, 0x00249c00,
		0x00254400, 0x00255c00, 0x00000000
	};

	select = SELECT_CONTINUE;
	ostatus = padread() & 0x19;

	for (timeout = 32 * 60; timeout > 0; timeout--) {
		vsync_wait();

		timeout = 32 * 60;
		p = (int *)0xd000100;
		for (i = 0; i < 32; i++) {
			if (i >= 8 &&
			    i < 8 + sizeof (bmp_continue) / sizeof (int))
				*p++ = select == SELECT_CONTINUE ?
				    ~bmp_continue[i - 8] : bmp_continue[i - 8];
			else if (i >= 17 &&
			    i < 17 + sizeof (bmp_exit) / sizeof (int))
				*p++ = select == SELECT_EXIT ?
				    ~bmp_exit[i - 17] : bmp_exit[i - 17];
			else 
				*p++ = 0;
		}

		status = padread() & 0x19;
		if (ostatus != status)
			ostatus = status;
		else
			continue;

		if (status & (1 << 3))
			select = SELECT_EXIT;
		if (status & (1 << 4))
			select = SELECT_CONTINUE;
		if (status & (1 << 0))
			if (select == SELECT_CONTINUE) {
				break;
			} else {
				while (padread() & 0x1f)
					;
				app_exit();
			}
	}
}
