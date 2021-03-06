/* See COPYRIGHT for copyright information. */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>
#include <kern/ansi.h>

#define MONO_BASE	0x3B4
#define MONO_BUF	0xB0000
#define CGA_BASE	0x3D4
#define CGA_BUF		0xB8000

#define CRT_ROWS	25
#define CRT_COLS	80
#define CRT_SIZE	(CRT_ROWS * CRT_COLS)

void cons_init(void);
int cons_getc(void);

void kbd_intr(void); // irq 1
void serial_intr(void); // irq 4

void cga_clear_screen(void);
void cga_clear_line(void);
void cga_move_cursor(cursor_t);
uint16_t crt_attr;

#endif /* _CONSOLE_H_ */
