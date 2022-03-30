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