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

void synchronize_rcu(int cpu_id,struct spinlock* write_lock){
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

    acquire(write_lock);
    int finished = 0;
    int target_value_achieved[NCPU];

    struct context cs[NCPU];

    for (int i = 0; i < NCPU; i++)
    {
        if(i == cpu_id) continue;

        if(__atomic_load_n(&(cpus[i].idle), __ATOMIC_RELAXED) == 1)
            target_value_achieved[i] = 1;
        else
        {
            target_value_achieved[i] = 0;
            cs[i] = cpus[i].context;
        }
    }

    /* End bootstrapping phase*/
    
    while(!finished){
        finished = 1;

        for (int i = 0; i < NCPU; i++)
        {
            if(i == cpu_id) continue;

            if(target_value_achieved[i] == 1) continue;

            if( __atomic_load_n(&(cpus[i].idle), __ATOMIC_RELAXED) == 1 ||
                !context_eq(cs[i], cpus[i].context)
              ) // If the CPU is idle, or changed context
                target_value_achieved[i] = 1;
            else
                finished = 0;
        }
    }
    release(write_lock);
}
int context_eq(struct context c1, struct context c2)
{
    return 
    c1.ra == c2.ra   &&
    c1.sp == c2.sp   &&
    c1.s0 == c2.s0   &&
    c1.s1 == c2.s1   &&
    c1.s2 == c2.s2   &&
    c1.s3 == c2.s3   &&
    c1.s4 == c2.s4   &&
    c1.s5 == c2.s5   &&
    c1.s6 == c2.s6   &&
    c1.s7 == c2.s7   &&
    c1.s8 == c2.s8   &&
    c1.s9 == c2.s9   &&
    c1.s10 == c2.s10 &&
    c1.s11 == c2.s11
    ;
}

void rcu_assign_pointer(t_list* list_ptr_dst, t_node* node_ptr_src){
   
    __sync_synchronize();
    //__atomic_load(*list_ptr_dst,node_ptr_src,__ATOMIC_RELAXED);
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
    
    rcu_read_lock();
    tmp_node_ptr = rcu_dereference_pointer(*list_ptr);
    node_ptr->next = tmp_node_ptr; // do we need this?
    rcu_read_unlock();
    
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

    if((current_node_ptr->process->pid) == proc_ptr->pid){
        // Found
        new_node_ptr->next = current_node_ptr->next;
        
        *ptr_to_free = current_node_ptr;

        rcu_assign_pointer(list_ptr, new_node_ptr); 
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
        if((current_node_ptr->process->pid) == proc_ptr->pid){
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

    if((current_node_ptr->process->pid) == proc_ptr->pid){
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
        if((current_node_ptr->process->pid) == proc_ptr->pid){
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

void print_list(t_list list){
    int count = 0;
    printf("[PROCESS LIST *Start] \n");
    while(!is_empty(list)){
        printf("%d)\t", count);
        print_proc((list->process));
        count++;
        list = list->next;
    }
    printf("[PROCESS LIST *End] \n");

}

