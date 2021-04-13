
/*
 * Simple program to test I/O system call
 */

#include <stdio.h>
#include <unistd.h>

#define BUF_SIZE 10

int
main(void)
{
	char intro[] = "io_syscalls test\nAttempt to read from STDIN\n> ";
	char buf[BUF_SIZE];

	printf("%s", intro);

	read(STDIN_FILENO, buf, BUF_SIZE);

	printf("Read message: %s", buf);

	return 0;
}
