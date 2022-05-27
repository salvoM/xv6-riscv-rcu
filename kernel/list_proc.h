/*
    Work in progress
*/
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"


typedef struct s_node{
    struct proc process;
    struct s_node* next;
}t_node;

typedef struct t_node* t_list;


void rcu_read_lock();

void rcu_read_unlock();

void synchronize_rcu();

void rcu_assign_pointer(t_list* list_ptr_dst, t_node* node_ptr_src);

t_node* rcu_dereference_pointer(t_node* node_ptr);



void list_add_rcu(t_list* list_ptr, t_node* node_ptr, struct spinlock* writers_lock_ptr);

void list_del_rcu(t_list* list_ptr, t_node* node_ptr, struct spinlock* writers_lock_ptr);

int list_update_rcu(t_list* list_ptr, t_node* new_node_ptr, struct proc* proc_ptr, struct spinlock* writers_lock_ptr, t_node** ptr_to_free );

int list_del_from_proc_rcu(t_list* list_ptr, struct proc* proc_ptr, struct spinlock* writers_lock_ptr, t_node** ptr_to_free);

void init_list(t_list* list_ptr, struct spinlock* writers_lock_ptr);

void init_writers_lock(struct spinlock* writers_lock_ptr);

int is_empty(t_list list);

