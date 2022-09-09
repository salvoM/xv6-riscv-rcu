struct kmem_cache {
  uint size;
  uint align;
  /* other fields */
};

void *kmem_cache_alloc(struct kmem_cache *cache);
void kmem_cache_free(struct kmem_cache *cache, void *obj);