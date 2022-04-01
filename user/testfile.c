#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fcntl.h"
#include "kernel/memlayout.h"
#include "user/user.h"

#define SIZE 50

void
test0() {
  int i, j;
  int fd;
  char string[SIZE];
  int res;
  printf("[TEST] reading file descriptor: start\n");
  // printf("Opening README.md");
  if ((fd = open("echo", O_RDONLY)) < 0) {
    // the open() failed; exit with -1
    printf("open failed\n");
    printf("[TEST] reading file descriptor: FAILED\n");
    exit(-1);
  }
  else{

    if(res = read( fd, string, SIZE-1) != 0){
      string[SIZE-1] = "\0";
      printf("%s", string);
      printf("Read %d bytes from %d\n", res, fd);
    }
    else{
      printf("Cannot read from %d", fd);
      printf("[TEST] reading file descriptor: FAILED\n");
    }
  }
  printf("[TEST] reading file descriptor: OK\n");
}

void test1()
{
  int pid, xstatus, n0, n;
  int fd;
  const char test_string[] = "This is a test string";
  char buffer[SIZE];

  printf("[TEST] writing file descriptor: start\n");
  
  if ((fd = open("echo", O_RDWR)) < 0) {
    printf("open failed\n");
    printf("[TEST] writing file descriptor: FAILED\n");
  }
  else{
    if( write(fd, test_string, sizeof(test_string)) != 0){
      printf("write failed\n");
      printf("[TEST] writing file descriptor: FAILED\n");
      exit(-1);
    }
    else{
      printf("Write: OK");
      printf("Trying to read the content of the file...\n");
      if( read(fd, buffer, sizeof(test_string)) == 0){
        printf("Read failed\n");
        printf("[TEST] writing file descriptor: FAILED\n");
      }
      printf("[TEST] writing file descriptor: OK\n");
    }
  }
}

int
main(int argc, char *argv[])
{
  test0();
  test1();
  exit(0);
}
