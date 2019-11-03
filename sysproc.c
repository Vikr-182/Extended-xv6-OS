#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "pstat.h"

	int
sys_fork(void)
{
	return fork();
}

	int
sys_exit(void)
{
	exit();
	return 0;  // not reached
}

	int
sys_wait(void)
{
	return wait();
}

	int
sys_kill(void)
{
	int pid;

	if(argint(0, &pid) < 0)
		return -1;
	return kill(pid);
}

	int
sys_getpid(void)
{
	return myproc()->pid;
}

	int
sys_sbrk(void)
{
	int addr;
	int n;

	if(argint(0, &n) < 0)
		return -1;
	addr = myproc()->sz;
	if(growproc(n) < 0)
		return -1;
	return addr;
}

	int
sys_sleep(void)
{
	int n;
	uint ticks0;

	if(argint(0, &n) < 0)
		return -1;
	acquire(&tickslock);
	ticks0 = ticks;
	while(ticks - ticks0 < n){
		if(myproc()->killed){
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

// return how many clock tick interrupts have occurred
// since start.
	int
sys_uptime(void)
{
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

	int 
sys_hello(void)
{
	int n;
	if(argint(0,&n)<0)
	{
		cprintf("Please enter correct value\n");
		return -1;
	}
#ifndef RA
	cprintf("Hello World %d\n",n);
#else 
	cprintf("Ra %d\n",n);
#endif
	return 0;
}

	int 
sys_getpinfo(void)
{
	cprintf("Sr.No\tPID\tRunTime\tCurrentQueue\tQ1 Ticks\tQ2 Ticks\tQ3 Ticks\tQ4 Ticks\tQ5 Ticks\n");
	for(int g=0;g<64;g++)
	{
		struct pstat *st;
		getpinfo(&st,n);
		cprintf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",h,i->pid,i->runtime,i->current_queue,i->ticks[0],i->ticks[1],i->ticks[2],i->ticks[3],i->ticks[4]);
	}
	return 0;
}
