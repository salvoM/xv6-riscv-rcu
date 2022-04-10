#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/defs.h"
#include "kernel/kmalloc.h"

struct obj{
    int *array;
    char j [128];
    int val;
};

void test(){
    int numero,*array,i;
    numero=100;

    array= (int*) knmalloc(sizeof(int)*numero);
    for(i=0;i<numero;i++){
        array[i]=i;
        printf("array[i] = %d\n",array[i]);
    }
    knfree(array);

}