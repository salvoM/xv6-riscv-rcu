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
#include "list_proc.h"



struct spinlock wx_lock; // Guarantees mutual exclusion between writers

void rcu_read_lock(){
    //No block or sleep in read critical section
    push_off();
}

void rcu_read_unlock(){
    // End of no-block/sleep zone
    pop_off();
}

void synchronize_rcu(){
    // Tries to run on each CPU to force a context switch

    /*
    Thus, RCU Classic's synchronize_rcu() can conceptually be as simple as the following:
    1 for_each_online_cpu(cpu)
    2   run_on(cpu);
    Here, run_on() switches the current thread to the specified CPU,
    which forces a context switch on that CPU. The for_each_online_cpu()
    loop therefore forces a context switch on each CPU,
    thereby guaranteeing that all prior RCU read-side critical sections have completed,
    as required. 
    */

    int cpu_0 = 1;
    int cpu_1 = 1;
    int cpu_2 = 1;
    int id = -1;
    while(1){
        push_off();
        id = cpuid();
        pop_off();
        if( cpu_0 && id == 0){
            cpu_0--;
        }
        if( cpu_1 && id == 1){
            cpu_1--;
        }
        if( cpu_2 && id == 2){
            cpu_2--;
        }
        __sync_synchronize();
        if( cpu_0 == 0 && cpu_1 == 0 && cpu_2 == 0){
            return;
        }
    }
}

void rcu_assign_pointer(t_list* list_ptr_dst, t_node* node_ptr_src){
    //rcu_assign_pointer(gobal_ptr, ptr);
    // Provo a fare solo con le barriers
    /*
    #define rcu_assign_pointer(p, v) \
    ({ \
            smp_store_release(&(p), (v)); \
    })
    */
    __sync_synchronize();
    *list_ptr_dst = node_ptr_src;
}

t_node* rcu_dereference_pointer(t_node* node_ptr){
    
    /*
        * reads (copy) a RCU-protected pointer to a local variable
        * into a RCU read-side critical section. The pointer can later be safely
        * dereferenced within the critical section.
        *
        * This ensures that the pointer copy is invariant thorough the whole critical
        * section
    */

    /*
        rcu_read_lock();
        p = rcu_dereference(head.next);
        rcu_read_unlock();
        x = p->address; // BUG!!! 
        rcu_read_lock();
        y = p->data;    // BUG!!! 
        rcu_read_unlock();
    */
    __sync_synchronize();
    return node_ptr;

}

#define next_task(p) \
        rcu_dereference_pointer((p)->next)

#define for_each_process(p) \
        for (p = *process_list ; (p = next_task(p)) != NULL ; )

void list_add_rcu(t_list* list_ptr, t_node* node_ptr, struct spinlock* writers_lock_ptr){
    // Writer
    t_node* tmp_node_ptr;

    //Lock out other writers
    acquire(writers_lock_ptr);
    
    // rcu_read_lock();
    tmp_node_ptr = rcu_dereference_pointer(*list_ptr);
    node_ptr->next = tmp_node_ptr; // do we need this?
    // rcu_read_unlock();
    
    rcu_assign_pointer(list_ptr, node_ptr);
    
    release(writers_lock_ptr);

}

void list_del_rcu(t_list* list_ptr, t_node* node_ptr, struct spinlock* writers_lock_ptr){
    // Writer
    // Possible actions relative to the lifecycle of the process must be taken before calling the delete of the list
    // KFreeing must be performed after the call to list_del_rcu
    int found = 0;
    if(*list_ptr == 0){
        panic("list_del_rcu");
    }

    acquire(writers_lock_ptr);

    // Dereferencing like this? W/o read_lock is it ok?
    t_node* tmp_node_ptr = rcu_dereference_pointer(*list_ptr);

    if(tmp_node_ptr == node_ptr){
        // Changing the head of the list
        // *list_ptr = tmp_node_ptr->next;
        rcu_assign_pointer(list_ptr, tmp_node_ptr->next); // Dovrebbe essere atomica
        release(writers_lock_ptr);
        return;
    }
    t_node* prev_node_ptr = tmp_node_ptr;
    tmp_node_ptr = tmp_node_ptr->next;


    while(tmp_node_ptr != 0 && found == 0){
        // doppi puntatori per eliminare
        if(tmp_node_ptr == node_ptr){
            //Found node to delete
            prev_node_ptr->next = tmp_node_ptr->next;
            found = 1;
        }
        else{
            tmp_node_ptr = tmp_node_ptr->next;
            prev_node_ptr = prev_node_ptr->next;
        }
        
    }
    release(writers_lock_ptr);

}

int list_update_rcu(t_list* list_ptr, t_node* new_node_ptr, struct proc* proc_ptr, struct spinlock* writers_lock_ptr, t_node** ptr_to_free ){
    int found = 0;
    acquire(writers_lock_ptr);
    rcu_read_lock();
    t_node* current_node_ptr = rcu_dereference_pointer(*list_ptr);

    if(&(current_node_ptr->process) == proc_ptr){
        // Found
        new_node_ptr->next = current_node_ptr->next;
        
        *ptr_to_free = current_node_ptr;
        
        rcu_assign_pointer(list_ptr, current_node_ptr->next); // va fatta atomicamente
        rcu_read_unlock();
        release(writers_lock_ptr);
        return 1;
    }
    if(current_node_ptr == 0){
        printf("SIAMO PROPRIO SCEMI");
    }
    t_node* prev_node_ptr = current_node_ptr;
    current_node_ptr = current_node_ptr->next;


    while(current_node_ptr != 0 && found == 0){
        // doppi puntatori per fare update
        if(&(current_node_ptr->process) == proc_ptr){
            //Found node relative to the process

            new_node_ptr->next = current_node_ptr->next; // non atomico
            prev_node_ptr->next = new_node_ptr; // rcu_assign_pointer()
            *ptr_to_free = current_node_ptr;
            found = 1;
        }
        else{
            current_node_ptr = current_node_ptr->next;
            prev_node_ptr = prev_node_ptr->next;
        }
    }

    rcu_read_unlock();
    release(writers_lock_ptr);
    return found;
}

int list_del_from_proc_rcu(t_list* list_ptr, struct proc* proc_ptr, struct spinlock* writers_lock_ptr, t_node** ptr_to_free){
    int found = 0;
    acquire(writers_lock_ptr);
    rcu_read_lock();
    t_node* current_node_ptr = rcu_dereference_pointer(*list_ptr);

    if(&(current_node_ptr->process) == proc_ptr){
        // Found        
        *ptr_to_free = current_node_ptr;
        
        rcu_assign_pointer(list_ptr, current_node_ptr->next); // va fatta atomicamente
        rcu_read_unlock();
        release(writers_lock_ptr);
        return 1;
    }

    t_node* prev_node_ptr = current_node_ptr;
    current_node_ptr = current_node_ptr->next;


    while(current_node_ptr != 0 && found == 0){
        // doppi puntatori per fare update
        if(&(current_node_ptr->process) == proc_ptr){
            //Found node relative to the process

            
            prev_node_ptr->next = current_node_ptr->next; // rcu_assign_pointer() va fatto atomicamente
            *ptr_to_free = current_node_ptr;
            found = 1;
        }
        else{
            current_node_ptr = current_node_ptr->next;
            prev_node_ptr = prev_node_ptr->next;
        }
    }

    rcu_read_unlock();
    release(writers_lock_ptr);
    return found;
}

void init_list(t_list* list_ptr, struct spinlock* writers_lock_ptr){
    *list_ptr = 0;
    init_writers_lock(writers_lock_ptr);
}

void init_writers_lock(struct spinlock* writers_lock_ptr){
    initlock(writers_lock_ptr, "RCU_writers lock");
}

int is_empty(t_list list){
    if(list == 0)
        return 1; // True
    
    return 0;
}

// void insert_at_head(struct proc p, t_list* list_ptr){        
    //    Creating the node
    //     t_node* node_ptr = (t_node*)knmalloc(sizeof(t_node));
    //     if(node_ptr == 0)
    //         panic("insert_at_head - knmalloc");

    //    Filling the node
    //     node_ptr->process = p;
    //     node_ptr->next = *list_ptr;
        
    //      Update the head of the list
    //     *list_ptr = node_ptr;
// }
