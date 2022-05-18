
build:
	docker build . -t xv6rv

connect:
	docker run --name container1 -ti -v `pwd`/..:/local --rm xv6rv:latest /bin/bash

gdb:
	# did you start the container with gdb session?
	docker exec -ti container1 /bin/bash

# to compile the kernel
#      cd /local && make TOOLPREFIX=/opt/riscv/toolchain/bin/riscv64-unknown-elf-

# to run qemu:
#      cd /local && make TOOLPREFIX=/opt/riscv/toolchain/bin/riscv64-unknown-elf- qemu

# for debugging
#      cd /local && make TOOLPREFIX=/opt/riscv/toolchain/bin/riscv64-unknown-elf- qemu-gdb
# in another window
#      /opt/riscv/toolchain/bin/riscv64-unknown-elf-gdb /local/kernel/kernel
#      (gdb) target remote localhost:25000



