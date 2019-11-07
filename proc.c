#include <stddef.h>
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"

int rp = 0;
struct {
	struct spinlock lock;
	struct proc proc[NPROC*NPROC];
//	struct pstat pstat_var[NPROC];
} ptable;

struct bets
{
    int a[5];
};

struct bets alpha;
static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

struct proc *mlfq[NPROC*NPROC][5];
struct pstat pstat_var;
int cnt[5]= {-1,-1,-1,-1,-1};

int clock_time[5] = {1,2,4,8,16};

	void
pinit(void)
{
	initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
	return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
	struct cpu*
mycpu(void)
{
	int apicid, i;

	if(readeflags()&FL_IF)
		panic("mycpu called with interrupts enabled\n");

	apicid = lapicid();
	// APIC IDs are not guaranteed to be contiguous. Maybe we should have
	// a reverse map, or reserve a register to store &cpus[i].
	for (i = 0; i < ncpu; ++i) {
		if (cpus[i].apicid == apicid)
			return &cpus[i];
	}
	panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
	struct cpu *c;
	struct proc *p;
	pushcli();
	c = mycpu();
	p = c->proc;
	popcli();
	return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
	static struct proc*
allocproc(void)
{
	struct proc *p;
	char *sp;

	acquire(&ptable.lock);

	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if(p->state == UNUSED)
			goto found;

	release(&ptable.lock);
	return 0;

found:
	//	! MLFQ SCHEDULING CODE NON-CONFLICTING WITH ORIGINAL
	p->state = EMBRYO;
	p->pid = nextpid++;
    if(p->pid > 1)// insert into queue only if not 0 or 1 process
    {
        mlfq[++cnt[0]][0] = p;
        reverse[p->pid] = cnt[0];
    }
    cprintf("%dna\n",cnt[0]);
	p->priority = 10;		// Inital random value
	p->ctime = ticks;		// Creation time
	p->rtime = 0;			// Total time
	p->etime = 0;			// End time is 0
	p->iotime = 0;			// IO time is 0
	p->clicks = 0;
    pstat_var.queue_num[p->pid] = 0;
	pstat_var.inuse[p->pid] = 1;
    pstat_var.priority[p->pid] = p->priority;
    cprintf("Making %d %d\n",p->pid,rp);
    pstat_var.pid[p->pid] = p->pid;

    for(int i=0;i<5;i++)
    {
         pstat_var.ticks[p->pid][i] = 0;
    }
	release(&ptable.lock);

	// Allocate kernel stack.
	if((p->kstack = kalloc()) == 0){
		p->state = UNUSED;
		return 0;
	}
	sp = p->kstack + KSTACKSIZE;

	// Leave room for trap frame.
	sp -= sizeof *p->tf;
	p->tf = (struct trapframe*)sp;

	// Set up new context to start executing at forkret,
	// which returns to trapret.
	sp -= 4;
	*(uint*)sp = (uint)trapret;

	sp -= sizeof *p->context;
	p->context = (struct context*)sp;
	memset(p->context, 0, sizeof *p->context);
	p->context->eip = (uint)forkret;

	return p;
}

//PAGEBREAK: 32
// Set up first user process.
	void
userinit(void)
{
	struct proc *p;
	extern char _binary_initcode_start[], _binary_initcode_size[];

	p = allocproc();

    for(int g=0;g<NPROC;g++)
    {
        pstat_var.pid[g] = -1;
        pstat_var.inuse[g] = -1;
        pstat_var.priority[g] = -1;
        pstat_var.runtime[g] = -1;
        pstat_var.num_run[g] = -1;
        pstat_var.queue_num[g] = -1;
        for(int b=0;b<5;b++)
        {
            pstat_var.ticks[g][b] = -1;
        }
    }
    initproc = p;
    if((p->pgdir = setupkvm()) == 0)
        panic("userinit: out of memory?");
    inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
    p->sz = PGSIZE;
    memset(p->tf, 0, sizeof(*p->tf));
    p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
    p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
    p->tf->es = p->tf->ds;
    p->tf->ss = p->tf->ds;
    p->tf->eflags = FL_IF;
    p->tf->esp = PGSIZE;
    p->ctime = ticks;
    p->tf->eip = 0;  // beginning of initcode.S

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");

    // this assignment to p->state lets other cores
    // run this process. the acquire forces the above
    // writes to be visible, and the lock is also needed
    // because the assignment might not be atomic.
    acquire(&ptable.lock);

    p->state = RUNNABLE;

    release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
    int
growproc(int n)
{
    uint sz;
    struct proc *curproc = myproc();

    sz = curproc->sz;
    if(n > 0){
        if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    } else if(n < 0){
        if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    }
    curproc->sz = sz;
    switchuvm(curproc);
    return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
    int
fork(void)
{
    int i, pid;
    struct proc *np;
    struct proc *curproc = myproc();

    // Allocate process.
    if((np = allocproc()) == 0){
        return -1;
    }

    // Copy process state from proc.
    if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0)
    {
        kfree(np->kstack);
        np->kstack = 0;
        np->state = UNUSED;
        return -1;
    }
    np->sz = curproc->sz;
    np->parent = curproc;
    *np->tf = *curproc->tf;

    // Clear %eax so that fork returns 0 in the child.
    np->tf->eax = 0;

    for(i = 0; i < NOFILE; i++)
        if(curproc->ofile[i])
            np->ofile[i] = filedup(curproc->ofile[i]);
    np->cwd = idup(curproc->cwd);

    safestrcpy(np->name, curproc->name, sizeof(curproc->name));

    pid = np->pid;

    acquire(&ptable.lock);

    np->state = RUNNABLE;

	release(&ptable.lock);

	return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
	void
exit(void)
{
	struct proc *curproc = myproc();
	struct proc *p;
	int fd;

	if(curproc == initproc)
		panic("init exiting");

	// Close all open files.
	for(fd = 0; fd < NOFILE; fd++){
		if(curproc->ofile[fd]){
			fileclose(curproc->ofile[fd]);
			curproc->ofile[fd] = 0;
		}
	}

	begin_op();
	iput(curproc->cwd);
	end_op();
	curproc->cwd = 0;

	acquire(&ptable.lock);

	// Parent might be sleeping in wait().
	wakeup1(curproc->parent);

	// Pass abandoned children to init.
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(p->parent == curproc){
			p->parent = initproc;
			if(p->state == ZOMBIE)
				wakeup1(initproc);
		}
	}

	// Jump into the scheduler, never to return.
	curproc->state = ZOMBIE;
    curproc->etime = ticks;
	sched();
	panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
	int
wait(void)
{
	struct proc *p;
	int havekids, pid;
	struct proc *curproc = myproc();

	acquire(&ptable.lock);
	for(;;){
		// Scan through table looking for exited children.
		havekids = 0;
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			if(p->parent != curproc)
				continue;
			havekids = 1;
			if(p->state == ZOMBIE){
				// Found one.
				pid = p->pid;
				kfree(p->kstack);
				p->kstack = 0;
				freevm(p->pgdir);
				p->pid = 0;
				p->parent = 0;
				p->name[0] = 0;
				p->killed = 0;
				p->state = UNUSED;
				release(&ptable.lock);
				return pid;
			}
		}

		// No point waiting if we don't have any children.
		if(!havekids || curproc->killed){
			release(&ptable.lock);
			return -1;
		}

		// Wait for children to exit.  (See wakeup1 call in proc_exit.)
		sleep(curproc, &ptable.lock);  //DOC: wait-sleep
	}
}

	int 
waitx(int *wtime,int *rtime )
{
	struct proc *p;
	int havekids, pid;
	struct proc *curproc = myproc();

	acquire(&ptable.lock);
	for(;;){
		// Scan through table looking for exited children.
		havekids = 0;
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			if(p->parent != curproc)
				continue;
			havekids = 1;
			if(p->state == ZOMBIE)
            {
				// Found one.
				*wtime= p->etime - p->ctime - p->rtime - p->iotime;
				*rtime=p->rtime;
				pid = p->pid;
				kfree(p->kstack);
				p->kstack = 0;
				freevm(p->pgdir);
				p->pid = 0;
				p->parent = 0;
				p->name[0] = 0;
				p->killed = 0;
				p->state = UNUSED;
				release(&ptable.lock);
				return pid;
			}
		}

		// No point waiting if we don't have any children.
		if(!havekids || curproc->killed){
			release(&ptable.lock);
			return -1;
		}

		// Wait for children to exit.  (See wakeup1 call in proc_exit.)
		sleep(curproc, &ptable.lock);  //DOC: wait-sleep
	}
}

void update_table()
{
#ifdef MLFQ
	acquire(&ptable.lock);
    struct proc *u = 0;
    for(u=ptable.proc;u<&ptable.proc[NPROC];u++)
    {
        if(u->state == RUNNING)
        {
            u->rtime++;
            int ind = reverse[u->pid];              // index where it is present in it's queue.
            int qu = pstat_var.queue_num[u->pid];   // current queue number of the process.
            u->rtime++;                             // increase running time
            pstat_var.runtime[u->pid]++;            // increase runtime
            
            if(pstat_var.runtime[u->pid] == clock_time[qu]) // if it exceeds the quantum , demote it 
            {
                if(qu != 4)
                {
                    cnt[qu+1]++;
                    mlfq[cnt[qu+1]][qu+1] = u;      // kept it down
                    reverse[u->pid] = cnt[qu+1];    // to store where it went
                    for(int g=ind;g<cnt[qu];qu++)    
                    {
                        mlfq[g][qu] = mlfq[g+1][qu];// shift left all processes
                        if(mlfq[g][qu])
                        {
                            reverse[mlfq[g][qu]->pid] = g;
                        }
                    }
                    mlfq[cnt[qu]][qu] = 0;
                    cnt[qu]--;
                    pstat_var.queue_num[u->pid] = qu+1;// update queue number after sending it down.
                }
            }
            continue;
        }
        else if(u->state == SLEEPING )
        {
            u->iotime++;
            continue;
        }
    }
    release(&ptable.lock);
#endif
#ifdef RR
    acquire(&ptable.lock);
    struct proc *p;
    for(p = ptable.proc; p < &ptable.proc[NPROC] ; p++)
    {
        if(p->pid > 1 && p!=0 && p->state == RUNNING)
        {
            p->rtime++;
        }
        else if(p->pid > 1 && p!= 0 && p->state == SLEEPING)
        {
            p->iotime++;
        }
    }
    release(&ptable.lock);
#endif
#ifdef FCFS
    acquire(&ptable.lock);
    struct proc *p; 
    for(p = ptable.proc; p < &ptable.proc[NROC] ; p++)
    {   
        if(p->state == RUNNING)
        {
            p->rtime++;
        }
        else if(p->state == SLEEPING)
        {
            p->iotime++;
        }
    }   
    release(&ptable.lock);
#endif
#ifdef PBS 
    acquire(&ptable.lock);
    struct proc *p ;
    struct proc *minP = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
        if(p->state == SLEEPING)
        {
            p->iotime++;
            continue;
        }
        else if(p->state == RUNNING)
        {
            p->rtime++;
            // choose the next higher priority process
            for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
            {
                if(minP == 0 || p->priority < minP->priority)
                {
                    if(p->pid > 1)
                    {
                        minP = p;
                    }
                }
            }
            continue;
        }
        
    }
    release(&ptable.lock);
#endif
}

void display()
{
    for(int g=0;g<5;g++)
    {
        cprintf(",%d,| %d|\t",g,cnt[g]);
        for(int b=0;b<cnt[g];b++){cprintf("|%d|",mlfq[b][g]->pid);}
    }
    cprintf("\n\n");
}


//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
    void
scheduler(void)
{
    struct proc *p;
    struct cpu *c = mycpu();
    c->proc = 0;
    //int last_scheduled = -1;
    for(;;)
    {
        // Enable interrupts on this processor.
        sti();

        // Loop over process table looking for process to run.
        acquire(&ptable.lock);
#ifdef RR
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        {
            if(p->state != RUNNABLE)
                continue;
            {
                // Switch to chosen process.  It is the process's job
                // to release ptable.lock and then reacquire it
                // before jumping back to us.
                c->proc = p;
                switchuvm(p);
                p->state = RUNNING;
                pstat_var.num_run[p->pid]++;
                //cprintf("This babe started running %d\n",p->pid);

                swtch(&(c->scheduler), p->context);
                switchkvm();

                // Process is done running for now.
                // It should have changed its p->state before coming back.
                c->proc = 0;
            }


        }
#endif

#ifdef MLFQ
        //	!	ADDED MLFQ 	///////////////////////////////////////////////////////////////////
        struct proc *mP = 0;
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        {
            if(p->state != RUNNABLE)
                continue;
            if(mP == 0)
            {
                mP = p;
            }
            else
            {
                int qp = pstat_var.queue_num[p->pid];
                int qm = pstat_var.queue_num[mP->pid];

                if(qp < qm || (qp==qm && p->ctime < mP->ctime))
                {
                    mP = p;
                }
            }
            // execute the selected process increase the ticks of all that in waiting queue this will happen in trap.c
            if(mP !=0 && mP->state==RUNNABLE)
			{
				p = mP;
			}
//            cprintf("Sel %d %d\n",mP->pid,pstat_var.queue_num[mP->pid]);
			//	!	REMAINING CODE ////////////////////////////////////////////////////////////////////
			if(p != 0)
			{
				// Switch to chosen process.  It is the process's job
				// to release ptable.lock and then reacquire it
				// before jumping back to us.
				c->proc = p;
				switchuvm(p);
				p->state = RUNNING;
                pstat_var.num_run[p->pid]++;
				//cprintf("This babe started running %d\n",p->pid);

				swtch(&(c->scheduler), p->context);
				switchkvm();

				// Process is done running for now.
				// It should have changed its p->state before coming back.
				c->proc = 0;
                p->clicks++; 
			}
        }
#endif
#ifdef PBS
        struct proc* highP = 0;
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        {
            if(p->state != RUNNABLE)
            {
                continue;
            }
            struct proc *p1 = 0;
            highP = p;
            for(p1 = ptable.proc; p1 < &ptable.proc[NPROC]; p1++)
            {
                if( ( (p1->state == RUNNABLE) && highP -> priority > p1->priority ) )    
                {
                    highP = p1;
                }    
            }
            p = highP;
            {
                // Switch to chosen process.  It is the process's job
				// to release ptable.lock and then reacquire it
				// before jumping back to us.
                p->clicks++; 
				c->proc = p;
				switchuvm(p);
				p->state = RUNNING;
				pstat_var.num_run[p->pid]++;
				//cprintf("This babe started running %d\n",p->pid);

				swtch(&(c->scheduler), p->context);
				switchkvm();

				// Process is done running for now.
				// It should have changed its p->state before coming back.
				c->proc = 0;
            }
        }
#endif
#ifdef FCFS
        // ! FCFS           //////////////////////////////////////////////////////////////////	
		struct proc *minP = 0;
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		{
			if(p->state != RUNNABLE)
				continue;
            struct proc *p1 = 0;
            minP = p;
            for(p1 = ptable.proc; p1 < &ptable.proc[NPROC]; p1++) {
                if(p1->state != RUNNABLE) 
                {
                    continue;
                }
                if(minP->ctime > p1->ctime) {
                    minP = p1; 
                }
            }   
            p = minP;

            /*
            if(p->pid > 1)
			{
				if(minP!=0)
				{
					if(p->ctime < minP->ctime)
					{
						minP = p;
					}	
                    else
					{
						// do nothing
					}
				}
				else
				{
					minP = p;
				}
			}
			if(minP !=0 && minP->state==RUNNABLE)
			{
				p = minP;
			}
            */
			//	!	REMAINING CODE ////////////////////////////////////////////////////////////////////
			if(p != 0)
			{
				// Switch to chosen process.  It is the process's job
				// to release ptable.lock and then reacquire it
				// before jumping back to us.
                p->clicks++;
				c->proc = p;
				switchuvm(p);
				p->state = RUNNING;
                pstat_var.num_run[p->pid]++;
				//cprintf("This babe started running %d\n",p->pid);

				swtch(&(c->scheduler), p->context);
				switchkvm();

				// Process is done running for now.
				// It should have changed its p->state before coming back.
				c->proc = 0;
			}
		}
#endif
		release(&ptable.lock);
	}
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
	void
sched(void)
{
	int intena;
	struct proc *p = myproc();

	if(!holding(&ptable.lock))
		panic("sched ptable.lock");
	if(mycpu()->ncli != 1)
		panic("sched locks");
	if(p->state == RUNNING)
		panic("sched running");
	if(readeflags()&FL_IF)
		panic("sched interruptible");
	intena = mycpu()->intena;
	swtch(&p->context, mycpu()->scheduler);
	mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
	void
yield(void)
{
	acquire(&ptable.lock);  //DOC: yieldlock
	myproc()->state = RUNNABLE;
	sched();
	release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
	void
forkret(void)
{
	static int first = 1;
	// Still holding ptable.lock from scheduler.
	release(&ptable.lock);

	if (first) {
		// Some initialization functions must be run in the context
		// of a regular process (e.g., they call sleep), and thus cannot
		// be run from main().
		first = 0;
		iinit(ROOTDEV);
		initlog(ROOTDEV);
	}

	// Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
	void
sleep(void *chan, struct spinlock *lk)
{
	struct proc *p = myproc();

	if(p == 0)
		panic("sleep");

	if(lk == 0)
		panic("sleep without lk");

	// Must acquire ptable.lock in order to
	// change p->state and then call sched.
	// Once we hold ptable.lock, we can be
	// guaranteed that we won't miss any wakeup
	// (wakeup runs with ptable.lock locked),
	// so it's okay to release lk.
	if(lk != &ptable.lock){  //DOC: sleeplock0
		acquire(&ptable.lock);  //DOC: sleeplock1
		release(lk);
	}
	// Go to sleep.
	p->chan = chan;
	p->state = SLEEPING;

	sched();

	// Tidy up.
	p->chan = 0;

	// Reacquire original lock.
	if(lk != &ptable.lock){  //DOC: sleeplock2
		release(&ptable.lock);
		acquire(lk);
	}
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
	static void
wakeup1(void *chan)
{
	struct proc *p;

	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if(p->state == SLEEPING && p->chan == chan)
			p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
	void
wakeup(void *chan)
{
	acquire(&ptable.lock);
	wakeup1(chan);
	release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
	int
kill(int pid)
{
	struct proc *p;

	acquire(&ptable.lock);
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(p->pid == pid){
			p->killed = 1;
			// Wake process from sleep if necessary.
			if(p->state == SLEEPING)
				p->state = RUNNABLE;
			release(&ptable.lock);
			return 0;
		}
	}
	release(&ptable.lock);
	return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
	void
procdump(void)
{
	static char *states[] = {
		[UNUSED]    "unused",
		[EMBRYO]    "embryo",
		[SLEEPING]  "sleep ",
		[RUNNABLE]  "runble",
		[RUNNING]   "run   ",
		[ZOMBIE]    "zombie"
	};
	int i;
	struct proc *p;
	char *state;
	uint pc[10];

	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(p->state == UNUSED)
			continue;
		if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
			state = states[p->state];
		else
			state = "???";
		cprintf("%d %s %s", p->pid, state, p->name);
		if(p->state == SLEEPING){
			getcallerpcs((uint*)p->context->ebp+2, pc);
			for(i=0; i<10 && pc[i] != 0; i++)
				cprintf(" %p", pc[i]);
		}
		cprintf("\n");
	}
}

	int
getpinfo(struct pstat *st)
{
    for(int g=0;g<NPROC;g++)
    {
        st->pid[g] = pstat_var.pid[g];
        st->inuse[g] = pstat_var.inuse[g];
        st->priority[g] = pstat_var.priority[g];
        st->runtime[g] = pstat_var.runtime[g];
        st->num_run[g] = pstat_var.num_run[g];
        st->queue_num[g] = pstat_var.queue_num[g];
        for(int b=0;b<5;b++)
        {
            st->ticks[g][b] = pstat_var.ticks[g][b];
        }
        cprintf("Anna %d-> %d\n",g,st->pid[g]);
    }
    return 0;
}

int set_priority(int priority,int pid)
{
    struct proc *p = 0;
    if(pid == 0)
    {
        pid = myproc()->pid;
    }
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
        if(p->pid == pid)
        {
            release(&ptable.lock);
            int old = p->priority;
            p->priority = priority;
            if( p ->priority < old)
            {
                yield();
            }
            return old;
        }
    }
    release(&ptable.lock);
    return 0;
}
