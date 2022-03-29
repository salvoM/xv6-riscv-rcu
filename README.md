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
