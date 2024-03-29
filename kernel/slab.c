#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "slab.h"

void *kmem_cache_alloc(struct kmem_cache *cache)
{
  return kalloc();
}

void kmem_cache_free(struct kmem_cache *cache, void *obj)
{
  kfree(obj);
}