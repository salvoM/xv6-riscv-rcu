#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "list_proc.h"

struct cpu cpus[NCPU];

// Per i kstacks
int bitmap[NPROC];


// struct proc proc[NPROC];
t_list process_list;                //! Must be initialized!!
struct spinlock rcu_writers_lock;   //! Must be initialized!!

#define next_node(p) \
        rcu_dereference_pointer((p)->next)

#define for_each_node(p) \
        for (p = process_list ; (p = next_node(p)) != 0 ; )

#define list_entry_for_process(ptr, type, member) \
        (type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member))


struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl) {
  // * Allocate when needed ??

  int n;

  for(n = 0; n < NPROC; n++){
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (n));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }

}

// initialize the proc table at boot time.
void
procinit(void)
{
  //*Move this logic when a process is created.
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  // initlock(&rcu_writers_lock, "rcu_writers_lock"); it is inside init_list
  init_list(&process_list, &rcu_writers_lock);

}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void) {
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid() {
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}


static struct proc*
allocproc(void)
{
  struct proc* tmp_proc_ptr;
  tmp_proc_ptr = (struct proc*)knmalloc((sizeof(struct proc)));
  
  
  tmp_proc_ptr->pid = allocpid();
  tmp_proc_ptr->state = USED;

  //Allocate kernel stack
  int n;
  int found=0;
  for(n=0;n<NPROC;n++){
    if(bitmap[n]==0){
      bitmap[n]=1;
      tmp_proc_ptr->nKStack=n;
      initlock(&(tmp_proc_ptr->lock), "proc");
      tmp_proc_ptr->kstack=KSTACK((int) (n));
      found=1;
      break;
    }
  }
  if(!found){
    panic("kernelstack overflow");
  }

  // Allocate a trapframe page.
  struct trapframe* tmp_trapframe_ptr = (struct trapframe*)kalloc();
  if(tmp_trapframe_ptr == 0){
    //Cannot alloc a trapframe
    knfree(tmp_proc_ptr);
    return 0;
  }
  tmp_proc_ptr->trapframe = tmp_trapframe_ptr;
  
  // An empty user page table.
  tmp_proc_ptr->pagetable = proc_pagetable(tmp_proc_ptr);
  if(tmp_proc_ptr->pagetable == 0){
      knfree(tmp_proc_ptr);
    return 0;
  }
  
  memset(&(tmp_proc_ptr->context), 0, sizeof(tmp_proc_ptr->context));
  tmp_proc_ptr->context.ra = (uint64)forkret;
  tmp_proc_ptr->context.sp = tmp_proc_ptr->kstack + PGSIZE;

  /* Here the process is ready to go, the node is ready to be added to the list*/

  /*  RCU add to list*/
  // list_add_rcu(&process_list,tmp_node_ptr,&rcu_writers_lock);
  /*  RCU add to list*/
  
  return tmp_proc_ptr; // ?
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{

  if(p->trapframe)
    kfree((void*)p->trapframe);
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  
  // KSTACKS
  bitmap[p->nKStack] = 0;
  knfree(p);
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  /* */

  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  // Fine modifiche al processo

  //Creo il nodo 
  t_node* tmp_node_ptr = (t_node*)knmalloc(sizeof(t_node));

  //Riempio il nodo
  tmp_node_ptr->process = *p;
  tmp_node_ptr->next = 0;

  /* Here the process is ready to go, the node is ready to be added to the list*/
  /*  RCU add to list*/
  list_add_rcu(&process_list,tmp_node_ptr,&rcu_writers_lock);
  /*  RCU add to list*/


  // release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  t_node* node_ptr_to_free;
  t_node* new_node_ptr = (t_node*)knmalloc(sizeof(t_node));

  struct proc* tmp_proc_ptr = myproc();
  new_node_ptr->process = *tmp_proc_ptr;
  sz = tmp_proc_ptr->sz;
  
  if(n > 0){
    if((sz = uvmalloc(new_node_ptr->process.pagetable, sz, sz + n)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(new_node_ptr->process.pagetable, sz, sz + n);
  }
  new_node_ptr->process.sz = sz;
  printf("[LOG LIST_UPDATE:RCU] Called from growproc");
  if(list_update_rcu(&process_list, new_node_ptr, tmp_proc_ptr, &rcu_writers_lock, &node_ptr_to_free) == 0){
    panic("proc disappeared");
  }
  /* Reclamation phase */
  synchronize_rcu(); // funziona? boh
  knfree(node_ptr_to_free);
  /* End Reclamation phase */

  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  
  // La sezione critica non c'è perchè myproc disabilita (e riabilita) gli interrupts
  // queste operazioni corrispondono a rcu read_lock/unlock
  struct proc *p = myproc();
  struct proc process = *p;
  
  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(process.pagetable, np->pagetable, process.sz) < 0){
    freeproc(np);
    return -1;
  }
  np->sz = process.sz;

  // copy saved user registers.
  *(np->trapframe) = *(process.trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(process.ofile[i])
      np->ofile[i] = filedup(process.ofile[i]);
  np->cwd = idup(process.cwd);

  safestrcpy(np->name, process.name, sizeof(process.name));

  pid = np->pid;



  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  np->state = RUNNABLE;
  // Fine modifiche al processo

  //Creo il nodo 
  t_node* tmp_node_ptr = (t_node*)knmalloc(sizeof(t_node));

  //Riempio il nodo
  tmp_node_ptr->process = *np;
  tmp_node_ptr->next = 0;

  /* Here the process is ready to go, the node is ready to be added to the list*/
  /*  RCU add to list*/
  list_add_rcu(&process_list,tmp_node_ptr,&rcu_writers_lock);
  /*  RCU add to list*/
  

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  t_node* tmp_node_ptr;
  t_node* ptr_node_to_free;
  
  tmp_node_ptr = list_entry_for_process(p, t_node, process);

  rcu_read_lock();
  for (tmp_node_ptr = process_list ; tmp_node_ptr != 0; tmp_node_ptr = tmp_node_ptr->next){
    
    
    if(tmp_node_ptr->process.parent == p){
    
      t_node* new_node_ptr = (t_node*)knmalloc(sizeof(t_node));
      
      new_node_ptr->process = tmp_node_ptr->process;
      rcu_read_unlock(); //lo sposteri qui
      
      new_node_ptr->process.parent = initproc;
      
      
      printf("[LOG LIST_UPDATE:RCU] Called from reparent\n");
      list_update_rcu(&process_list, new_node_ptr, p, &rcu_writers_lock, &ptr_node_to_free);
      
      synchronize_rcu(); // funziona? boh
      
      freeproc(&(ptr_node_to_free->process));
      knfree(ptr_node_to_free);
      
      wakeup(initproc); // Boh?
    }
    else rcu_read_unlock();
    rcu_read_lock();
  }

  rcu_read_unlock();

}

// Helper function that closes all the file descriptors of a given process
void closeFile(struct proc* p){
  t_node* node_ptr_to_free;
  t_node* new_node_ptr = (t_node*)knmalloc(sizeof(t_node));
  new_node_ptr->process=*p;
  for(int fd = 0; fd < NOFILE; fd++){
    if(new_node_ptr->process.ofile[fd]){
      struct file *f = new_node_ptr->process.ofile[fd];
      fileclose(f);
      //? non dovrebbe essere necessario eseguire quest'operazione poiché alla fine facciamo la free
      new_node_ptr->process.ofile[fd] = 0;
    }
  }
  printf("[LOG LIST_UPDATE:RCU] Called from closefile\n");
  if(list_update_rcu(&process_list, new_node_ptr, p, &rcu_writers_lock, &node_ptr_to_free) == 0){
        panic("[LOG closeFile] proc disappeared");
  }
  synchronize_rcu(); // funziona? boh
  freeproc(&(node_ptr_to_free->process));
  knfree(node_ptr_to_free);
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  closeFile(p);

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  t_node* node_ptr_to_free;
  t_node* new_node_ptr=(t_node*)knmalloc(sizeof(t_node));

  new_node_ptr->process         = *p;  
  new_node_ptr->process.xstate  = status;
  new_node_ptr->process.state   = ZOMBIE;
  
  printf("[LOG LIST_UPDATE:RCU] Called from exit\n");
  if(list_update_rcu(&process_list, new_node_ptr, p, &rcu_writers_lock, &node_ptr_to_free) == 0){
        panic("[LOG reparent] proc disappeared");
  }
  synchronize_rcu(); // funziona? boh
  freeproc(&(node_ptr_to_free->process));
  knfree(node_ptr_to_free);

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{

  int havekids, pid;
  struct proc *p = myproc();

  t_node* ptr_index_node;
  acquire(&wait_lock);  // Non mi piace che ci sia un lock qui, blocca la lettura della lista

  // Search through the list for children that are in ZOMBIE state
  // if any, copy data to the parent (xstate)
  // If there are children that are not in ZOMBIE, sleep!

  for(;;){
    havekids = 0;
    for_each_node(ptr_index_node){
      if(ptr_index_node->process.parent == p){
        havekids = 1;
        if(ptr_index_node->process.state == ZOMBIE){
          // We need to update the parent
          // and remove this child
          pid = ptr_index_node->process.pid;

          //update the parent
          t_node* ptr_new_node = (t_node*)knmalloc(sizeof(t_node));
          t_node* ptr_node_to_free;
          ptr_new_node->process = *p;
          ptr_new_node->next    = 0; 

          if(addr != 0 && copyout(p->pagetable, addr, (char *)&(ptr_index_node->process.xstate),
                                  sizeof(ptr_index_node->process.xstate)) < 0) {
            /* if something went wrong */
            knfree(ptr_new_node);
            release(&wait_lock);
            return -1;
          }
          // Update of the parent // Is it necessary?
          printf("[LOG LIST_UPDATE:RCU] Called from wait\n");
          list_update_rcu(&process_list, ptr_new_node, p, &rcu_writers_lock, &ptr_node_to_free);
          synchronize_rcu(); // funziona? boh
          freeproc(&(ptr_node_to_free->process));
          knfree(ptr_node_to_free);

          // Delete the zombie child from the list and reclaim it
          list_del_rcu(&process_list, ptr_index_node, &rcu_writers_lock);
          synchronize_rcu(); // funziona? boh
          freeproc(&(ptr_index_node->process));
          knfree(ptr_index_node);

          return pid;
        }
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || p->killed){
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  
  struct cpu *c = mycpu();
  
  c->proc = 0;
  printf("[LOG SCHEDULER] inside scheduler()\n");
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    t_node* tmp_node_ptr;

    // Is it ok with intr_on()?
    rcu_read_lock();

    for (tmp_node_ptr = process_list ; tmp_node_ptr != 0; tmp_node_ptr = tmp_node_ptr->next)
    {
      printf("[LOG SCHEDULER] process state = %d\n",tmp_node_ptr->process.state );
      if(tmp_node_ptr->process.state == RUNNABLE){

        // Update dello stato a running
        if(tmp_node_ptr->process.pid==1){
          acquire(&tmp_node_ptr->process.lock); 
          tmp_node_ptr->process.state = RUNNING;
          c->proc=&(tmp_node_ptr->process);
          swtch(&c->context, &(tmp_node_ptr->process.context));
          c->proc = 0;
          release(&tmp_node_ptr->process.lock);
          break;
        }
        t_node* new_node_ptr = (t_node*)knmalloc(sizeof(t_node));
        t_node* ptr_node_to_free;

        new_node_ptr->process = tmp_node_ptr->process;
        rcu_read_unlock(); // lo metterei qui
        
        new_node_ptr->process.state = RUNNING;
      

        printf("[LOG LIST_UPDATE:RCU] Called from scheduler\n");
        printf("[LOG LIST_UPDATE:RCU] (%p,%p,%p,%p,%p)\n",
        &process_list, new_node_ptr, &(tmp_node_ptr->process), &rcu_writers_lock, &ptr_node_to_free);
        
        list_update_rcu(&process_list, new_node_ptr, &(tmp_node_ptr->process), &rcu_writers_lock, &ptr_node_to_free);

        synchronize_rcu(); // funziona? boh
        freeproc(&(ptr_node_to_free->process));
        knfree(ptr_node_to_free);  
        
        // Scheduler operation
        c->proc = &(new_node_ptr->process);    

        swtch(&c->context, &(new_node_ptr->process.context));
        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
        


      }
      else 
      {
        rcu_read_unlock();
        release(&tmp_node_ptr->process.lock);
      }
  
      rcu_read_lock();
    }

    rcu_read_unlock();


  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  t_node* tmp_node_ptr;

  rcu_read_lock();
  for (tmp_node_ptr = process_list ; tmp_node_ptr != 0; tmp_node_ptr = tmp_node_ptr->next){

    if(&(tmp_node_ptr->process) != myproc() && tmp_node_ptr->process.state == SLEEPING && tmp_node_ptr->process.chan == chan){
      // update dello stato 
      t_node* new_node_ptr = (t_node*)knmalloc(sizeof(t_node));
      t_node* ptr_node_to_free;

      new_node_ptr->process = tmp_node_ptr->process;
      rcu_read_unlock(); //lo sposteri qui
      new_node_ptr->process.state = RUNNABLE;

      printf("[LOG LIST_UPDATE:RCU] Called from wakeup\n");
      list_update_rcu(&process_list, new_node_ptr, &(tmp_node_ptr->process), &rcu_writers_lock, &ptr_node_to_free);
      
      synchronize_rcu(); // funziona? boh
      freeproc(&(ptr_node_to_free->process));
      knfree(ptr_node_to_free);

    }
    else rcu_read_unlock();

    rcu_read_lock();
  }

  rcu_read_unlock();
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  t_node* tmp_node_ptr;

  rcu_read_lock();
  for (tmp_node_ptr = process_list ; tmp_node_ptr != 0; tmp_node_ptr = tmp_node_ptr->next){

    if(tmp_node_ptr->process.pid == pid){
      // update dello stato 

      t_node* new_node_ptr = (t_node*)knmalloc(sizeof(t_node));
      t_node* ptr_node_to_free;

      new_node_ptr->process = tmp_node_ptr->process;
      rcu_read_unlock(); //lo sposteri qui
      
      new_node_ptr->process.killed = 1;

      if(new_node_ptr->process.state == SLEEPING){
        //Se sta dormendo sveglialo
        new_node_ptr->process.state = RUNNABLE;
      }

      printf("[LOG LIST_UPDATE:RCU] Called from kill\n");
      list_update_rcu(&process_list, new_node_ptr, &(tmp_node_ptr->process), &rcu_writers_lock, &ptr_node_to_free);
      synchronize_rcu(); // funziona? boh
      freeproc(&(ptr_node_to_free->process));
      knfree(ptr_node_to_free);
      return 0;
    }
    else rcu_read_unlock(); //lo sposteri qui


    rcu_read_lock();
  }

  rcu_read_unlock();

  return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  
  char *state;

  t_node* tmp_node_ptr;

  rcu_read_lock();
  for (tmp_node_ptr = process_list ; tmp_node_ptr != 0; tmp_node_ptr = tmp_node_ptr->next){

    if(tmp_node_ptr->process.state >= 0 && tmp_node_ptr->process.state < NELEM(states) && states[tmp_node_ptr->process.state])
          state = states[tmp_node_ptr->process.state];
    else
          state = "???";
    printf("%d %s %s", tmp_node_ptr->process.pid, state, tmp_node_ptr->process.name);
    printf("\n");

    rcu_read_unlock();
    rcu_read_lock();
  }

  rcu_read_unlock();

}
