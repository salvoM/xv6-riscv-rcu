#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/defs.h"
#include "kernel/kmalloc.h"



static Header base;
static Header *freep;

void
knfree(void *ap)
{
  Header *bp, *p;
  //printf("[LOG KNFREE] knfree(%p)\n", ap);

  bp = (Header*)ap - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

static Header*
knmorecore(uint nu)
{
  char *p;
  Header *hp;
  // printf("[LOG KNMORECOREUNIT] knmorecore unit called\n");
  if(nu < 4096)
    nu = 4096;
  printf("[LOG PRIMA KALLOC]");
  p = (char*)kalloc();
  printf("[LOG KNMORECOREUNIT] kalloc returned %p\n", p);
  if(p == (char*)0)
    return 0;
  hp = (Header*)p;
  hp->s.size = nu;
  knfree((void*)(hp + 1));
  return freep;
}

void*
knmalloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
  //printf("[LOG KNMALLOC] knmalloc(%d)\n", nbytes);
  // printf("[LOG KNMALLOC] sizeof(file) = %d\n", sizeof( a ));
  //printf("[LOG KNMALLOC] nunits = %d, sizeof(Header) = %d\n", nunits, sizeof(Header));
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      //printf("[LOG KNMALLOC] return %p\n", (void*)(p+1));
      return (void*)(p + 1);
    }
    // printf("[LOG KNMALLOC] p = %p, freep = %p\n", p, freep);
    if(p == freep)
      if((p = knmorecore(nunits)) == 0)
      {
        //printf("[LOG KNMALLOC] return 0\n");
        return 0;
      }
  }
}
