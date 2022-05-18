//
// Support functions for system calls that involve file descriptors.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "stat.h"
#include "proc.h"


struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
  struct list *tail; //tail of the list of files
  struct list *head; //head of the list of files
} ftable;


void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
  printf("[LOG FILE] initializing the linked list\n");
  ftable.tail = (struct list *)knmalloc(sizeof(struct list));
  lst_init(ftable.tail);
  ftable.head = ftable.tail;
  printf("[LOG FILE] list initialized %p\n",ftable.tail);

}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  // for(f = ftable.file; f < ftable.file + NFILE; f++){
  //   if(f->ref == 0){
  //     f->ref = 1;
  // //     release(&ftable.lock);
  // //     return f;
  //   }
  // }
  // f = (struct file *)kmem_cache_alloc(&ftable.cache);
  f = (struct file *) knmalloc(sizeof(struct file));

  printf("[LOG FILE] allocating memory of the file pointer on the linkedilist\n");
  


  //adding the file on the linked list
  printf("[LOG FILE] pushing the file %p next %p\n",f,ftable.tail);

  lst_push(ftable.tail,f);
  printf("[LOG FILE] file %p pushed\n",f);

  // * inserimento in coda, logica andrebbe spostata in list.c
  // ftable.tail=f;

  f->ref = 1;
  release(&ftable.lock);
  return f;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  printf("[LOG FILEDUP] Starting dup \n");
  acquire(&ftable.lock);
  if(f->ref < 1){
	// printf("f->node = %p, f->ref = %d\n",f, f->ref);  
	panic("filedup");
  }
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;
  printf("[LOG FILECLOSE] fileclose(%p) called\n", f);
  // printf("[LOG FILECLOSE] fileclose %p <- %p -> %p \n", f->node->prev,f->node,f->node->next);

  acquire(&ftable.lock);
  if(f->ref < 1){
	// printf("[LOG FILECLOSE] PANIC fileclose %p <- %p -> %p \n", f->node->prev,f->node,f->node->next);
	panic("fileclose");
  }	
  if(--f->ref > 0){
	release(&ftable.lock);
	return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
//   printf("[LOG FILE] removing file %p from the linked list\n",f->node);
  // lst_remove(f->node);
  // printf("[LOG FILECLOSE] file %p removed\n", f->node);

  // printf("[LOG FILECLOSE] knfree(%p) \n", f);
  knfree((void*) f);
  release(&ftable.lock);

  if(ff.type == FD_PIPE){
	pipeclose(ff.pipe, ff.writable);
  } else if(ff.type == FD_INODE || ff.type == FD_DEVICE){
	begin_op();
	iput(ff.ip);
	end_op();
  }
}


// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
  struct stat st;
  
  if(f->type == FD_INODE || f->type == FD_DEVICE){
	ilock(f->ip);
	stati(f->ip, &st);
	iunlock(f->ip);
	if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
		return -1;	
	return 0;
  }
  return -1;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;

  if(f->readable == 0)
	return -1;

  if(f->type == FD_PIPE){
	r = piperead(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
	if(f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
	  return -1;
	r = devsw[f->major].read(1, addr, n);
  } else if(f->type == FD_INODE){
	ilock(f->ip);
	if((r = readi(f->ip, 1, addr, f->off, n)) > 0)
	  f->off += r;
	iunlock(f->ip);
  } else {
	panic("fileread");
  }

  return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64 addr, int n)
{
  int r, ret = 0;

  if(f->writable == 0)
	return -1;

  if(f->type == FD_PIPE){
	ret = pipewrite(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
	if(f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
	  return -1;
	ret = devsw[f->major].write(1, addr, n);
  } else if(f->type == FD_INODE){
	// write a few blocks at a time to avoid exceeding
	// the maximum log transaction size, including
	// i-node, indirect block, allocation blocks,
	// and 2 blocks of slop for non-aligned writes.
	// this really belongs lower down, since writei()
	// might be writing a device like the console.
	int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
	int i = 0;
	while(i < n){
	  int n1 = n - i;
	  if(n1 > max)
		n1 = max;

	  begin_op();
	  ilock(f->ip);
	  if ((r = writei(f->ip, 1, addr + i, f->off, n1)) > 0)
		f->off += r;
	  iunlock(f->ip);
	  end_op();

	  if(r != n1){
		// error from writei
		break;
	  }
	  i += r;
	}
	ret = (i == n ? n : -1);
  } else {
	panic("filewrite");
  }

  return ret;
}

