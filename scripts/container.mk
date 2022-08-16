
build:
	docker build . -t xv6rv

connect:
	docker run -ti --name xv6_rcu -v `pwd`/..:/local --rm xv6rv_mod /bin/bash
debug:
	docker exec -ti xv6_rcu /bin/bash

# to compile the kernel
#      cd /local && make TOOLPREFIX=/opt/riscv/toolchain/bin/riscv64-unknown-elf-

# to run qemu:
#      cd /local && make TOOLPREFIX=/opt/riscv/toolchain/bin/riscv64-unknown-elf- qemu

# for debugging
#      cd /local && make TOOLPREFIX=/opt/riscv/toolchain/bin/riscv64-unknown-elf- qemu-gdb
# in another window
#      /opt/riscv/toolchain/bin/riscv64-unknown-elf-gdb /local/kernel/kernel
#      (gdb) target remote localhost:25000



