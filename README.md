# xv6-riscv-rcu
RCU implementation on xv6-riscv

#how to destroy your personal computer
#inside scripts dir

sudo make -f container.mk build

# connect
# How? black magic?

sudo make -f container.mk connect


#inside the container

cd /local && make TOOLPREFIX=/opt/riscv/toolchain/bin/riscv64-unknown-elf-



#last command

cd /local && make TOOLPREFIX=/opt/riscv/toolchain/bin/riscv64-unknown-elf- qemu

#############



#To run the last command, rename the file README.md to README, inside the local folder



cd /local/xv6-riscv && mv README.md README

#then run the command

cd /local && make TOOLPREFIX=/opt/riscv/toolchain/bin/riscv64-unknown-elf- qemu


#Exit from Qemu:

Ctrl+A, x
