#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fcntl.h"
#include "kernel/memlayout.h"
#include "user/user.h"

#define SIZE 50

void test0()
{
	// int i, j;
	int fd;
	char string[SIZE];
	int res;
	printf("[READ TEST] START\n");
	// printf("Opening README.md");
	if ((fd = open("echo", O_RDONLY)) < 0)
	{
		// the open() failed; exit with -1
		printf("[-] Open failed\n");
		printf("[READ TEST] FAILED\n");
		exit(-1);
	}
	else
	{	
		// File opened 
		printf("[+] File opened\n");
		res = read(fd, string, SIZE - 1);
		if (res != 0)
		{
			string[SIZE - 1] = 0;
			// printf("%s", string);
			printf("[+] Read %d bytes from file descriptor %d\n", res, fd);
		}
		else
		{
			printf("[-] Cannot read from the opened file\n");
			printf("[READ TEST] FAILED\n\n\n");
			return;
		}
		close(fd);
	}
	printf("[READ TEST] SUCCESS\n\n\n");
}

void test1()
{
	/*
	Il
	*/
	// int pid, xstatus, n0, n;
	int fd, res = 0;
	const char test_string[] = "I solemnly swear that I have no good intentions\n";
	char buffer[SIZE];
	const char filename[] = "temp1";
	printf("[WRITE TEST] START\n");

	if ((fd = open(filename, O_RDWR | O_CREATE)) < 0)
	{
		printf("[-] Open failed\n");
		printf("[-] Cannot open %s\n", filename);
		printf("[WRITE TEST] FAILED\n");
	}
	else
	{
		// File opened
		printf("[+] File opened\n");
		if (write(fd, test_string, sizeof(test_string)) == 0)
		{
			printf("[-] Write failed\n");
			printf("[WRITE TEST] FAILED\n");
			close(fd);
			exit(-1);
		}
		else
		{
			printf("[+] Write OK\n");
			printf("[+] Trying to read the content of the file...\n");
			
			/*
			You should not need to close and reopen the file,
			but otherwise it does not work 
			*/
			close(fd);
			fd = open(filename, O_RDONLY);
			res = read(fd, buffer, sizeof(test_string));
			// printf("la stringa è %s\n", buffer);
			if (res == 0)
			{
				printf("[-] Read failed\n");
				printf("[WRITE TEST] FAILED\n");
			}
			else{
				if (res == sizeof(test_string))
					printf("[WRITE TEST] SUCCESS\n");
				else
					printf("[WRITE TEST] FAILED\n");
			}
		}
		close(fd);
	}
}

int main(int argc, char *argv[])
{
	test0();
	test1();
	exit(0);
}
