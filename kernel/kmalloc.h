#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/defs.h"
typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Header;
void knfree(void *ap);
static Header* knmorecore(uint nu);
void* knmalloc(uint nbytes);