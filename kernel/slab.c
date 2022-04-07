#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "slab.h"
// #include "kmalloc.h"

void *kmem_cache_alloc(struct kmem_cache *cache)
{
  return knmalloc(1024);
  // return kalloc();
}

void kmem_cache_free(struct kmem_cache *cache, void *obj)
{
  knfree(obj);
  // kfree(obj);
}