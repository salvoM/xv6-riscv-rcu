import os
PROJECT_HOME_DIR = '/home/sm/uni/AOS/xv6-riscv-rcu'
KERNELDIR = 'kernel' # change if i go to scripts
kernel_files = ['sched.c','kmalloc.c', 'list_proc.c', 'proc.c', 'sleeplock.c']

def is_printf(line):
    return 'print' in line and ');' in line

def is_comment(line):
    return '//' in line

def count_leading_space(line):
    return len(line) - len(line.lstrip())

def find_printf_blocks(content):
    blocks = []
    indent = []
    b = []
    prev_line = ''
    for line in content:
        if is_printf(line):
            b.append(line)
            s = count_leading_space(line)
        else:
            if len(b) != 0:
                blocks.append((b, s))
                b = []
                s = 0
    return blocks

macro_name = 'DEBUG'
s = f'#ifdef {macro_name}\n'
e = '#endif\n'

for name in kernel_files:
    filepath = os.path.join(PROJECT_HOME_DIR, KERNELDIR, name)
    with open(filepath, 'r') as f:
        content = f.readlines()
        code  = ''.join(content)
        for b, lead_space in find_printf_blocks(content):
            printfs        = ''.join(b)
            # print(printfs, lead_space)
            printfs_debug  = ' ' * lead_space + s 
            printfs_debug += ''.join(b)
            printfs_debug += ' ' * lead_space + e
            code           = code.replace(printfs, printfs_debug)
            # print(f"original: {code}\n mod {}")

    with open(filepath, 'w') as f:
        f.write(code)

print("[*] Source code modified\n")

line    =  'CFLAGS += -mcmodel=medany\n'
my_flag = f'CFLAGS += -D {macro_name}\n'
filepath = os.path.join(PROJECT_HOME_DIR, 'Makefile')
with open(filepath, 'r') as f:
    content = f.readlines()
    i = content.index(line)
    content.insert(i+1, my_flag)
with open(filepath, 'w') as f:
    f.write(''.join(content))

print("[*] Makefile modified\n")
print("[*] Done\n")

