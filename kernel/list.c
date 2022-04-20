#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

// double-linked, circular list. double-linked makes remove
// fast. circular simplifies code, because don't have to check for
// empty list in insert and remove.



void
lst_init(struct list *lst)
{ 
  // printf("[LOG LIST] allocating mem for pointers \n");
  // lst->next = (struct list *)knmalloc(sizeof(struct list));
  // lst->prev = (struct list *)knmalloc(sizeof(struct list));

  lst->next = lst;
  lst->prev = lst;
}

int
lst_empty(struct list *lst) {
  return lst->next == lst;
}

void
lst_remove(struct list *e) {
  e->prev->next = e->next;
  e->next->prev = e->prev;
  knfree((void*) e);
}

void*
lst_pop(struct list *lst) {
  if(lst->next == lst)
    panic("lst_pop");
  struct list *p = lst->next;
  lst_remove(p);
  return (void *)p;
}

void
lst_push(struct list *lst, struct list *e)
{
  // // struct list *e = (struct list *) 
  // printf("[LOG LIST - PUSH] changing the list pointers %d %d\n",lst,e);
  // //e->next = (struct list *)knmalloc(sizeof(struct list));
  // printf("[LOG LIST - PUSH] e->next allocated at %d \n",e->next);

  e->next = lst->next;
  printf("[LOG LIST -PUSH] next pointer changed\n");
  e->prev = lst;
  printf("[LOG LIST - PUSH] prev pointer chnaged \n");

  lst->next->prev = e;
  lst->next = e;
}

void
lst_print(struct list *lst)
{
  for (struct list *p = lst->next; p != lst; p = p->next) {
    printf(" %p", p);
  }
  printf("\n");
}
