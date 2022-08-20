import os
PROJECT_HOME_DIR = '/home/sm/uni/AOS/xv6-riscv-rcu'
KERNELDIR = 'kernel' # change if i go to scripts
kernel_files = ['kmalloc.c', 'list_proc.c', 'proc.c', 'sleeplock.c']

def is_printf(line):
    return 'printf' in line and ');' in line

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
            print(s)
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
        for b, lead_space in find_printf_blocks(content):
            printfs        = ''.join(b)
            printfs_debug  = ' ' * lead_space + s 
            printfs_debug += ''.join(b)
            printfs_debug += ' ' * lead_space + e
            original_code  = ''.join(content)
            mod_code       = original_code.replace(printfs, printfs_debug)

    with open(filepath, 'w') as f:
        f.write(mod_code)

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

