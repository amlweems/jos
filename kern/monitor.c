// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display backtrace of all stack frames", mon_backtrace },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	uint16_t i;
	uint32_t *ebp = (uint32_t *)read_ebp();
	uint32_t eip = (uint32_t)mon_backtrace;
	struct Eipdebuginfo info;
	cprintf("Stack backtrace:\n");
	while (ebp != NULL) {
		if (debuginfo_eip(eip, &info) < 0) {
			cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n",
				ebp, ebp[1], ebp[2], ebp[3], ebp[4], ebp[5], ebp[6]);
		} else {
			cprintf("  ebp %08x  eip %08x  args", ebp, ebp[1]);
			if (info.eip_fn_narg == 0) {
				cprintf(" %08x", 0);
			} else {
				for (i = 0; i < info.eip_fn_narg; i++) {
					cprintf(" %08x", ebp[2+i]);
				}
			}
			cprintf("\n\t%s:%d: %.*s+%d\n", info.eip_file, info.eip_line,
				info.eip_fn_namelen, info.eip_fn_name, eip - info.eip_fn_addr);
		}

		eip = ebp[1];
		ebp = (uint32_t *) ebp[0];
	}
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the \x1b[34mJOS kernel monitor\x1b[0m!\n");
	cprintf("Type '\x1b[35mhelp\x1b[0m' for a list of commands.\n");


	while (1) {
		buf = readline("\x1b[33mK> \x1b[0m");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
