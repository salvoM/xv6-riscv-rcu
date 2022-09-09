# xv6-riscv-rcu
Authors: Nicolas Gallo, Salvo Maccarrone

RCU implementation on xv6-riscv


## Build the container 

```sh
cd /scripts
sudo make -f container.mk build
```
## Connect to the container
```sh 
sudo make -f container.mk connect
#How? black magic?
```


## compile kernel
```sh
cd /local && make TOOLPREFIX=/opt/riscv/toolchain/bin/riscv64-unknown-elf-
```


## Start Qemu

```sh
cd /local && make TOOLPREFIX=/opt/riscv/toolchain/bin/riscv64-unknown-elf- qemu
```

### Exit from Qemu:
Ctrl+A, x

## How to add a syscall
1. Add entry in usys.pl 
```pl
entry("syscallname")
```
2. Define it in user.h
3. Add its code number to syscall.h
4. Add it to syscall.c, both the signature and the entry in the array
5. Implement it in the appropriate file, e.g. sys_nfree() is implemented in kalloc.c

## How to debug
for debugging
1. In a window start a container 
    ```sh
    cd scripts/ 
    sudo make -f container.mk connect
    ```
    And run qemu-gdb
    ```sh
    cd /local && make TOOLPREFIX=/opt/riscv/toolchain/bin/riscv64-unknown-elf- qemu-gdb
    ```
2. In another window connect to the same instance of docker
get the name of the session
    ```sh
    sudo docker ps
    ```
3. Connect to interactively to it using:
    ```sh
    sudo docker exec -ti <instance_name> bash
    ```
    Inside the container run gdb with:
    ```sh
    /opt/riscv/toolchain/bin/riscv64-unknown-elf-gdb /local/kernel/kernel
    ```      
    
    Inside gdb run this command
    ```sh
    (gdb) target remote localhost:25000
    ```

## How to execute with only 1 CPU
In the main folder of xv6 edit the Makefile:
 
```make
ifndef CPUS
CPUS := 3
endif
```
to the intuitive 
```make
ifndef CPUS
CPUS := 1
endif
```