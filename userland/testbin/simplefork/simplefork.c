#include <unistd.h>
#include <stdio.h>

int
main(int argc, char *argv[])
{
	pid_t pid;
	int status;

  (void)argc;
  (void)argv;
  (void)status;

	printf("In the parent BEFORE fork()\n");

	pid = fork();

  printf("fork() returned %d, should print twice\n", pid);

	switch(pid) {
	  case -1:
      printf("Error in fork()\n");
      return -1;
    case 0:
      printf("In child AFTER fork()\n");
      break;
    default:
      printf("In parent AFTER fork(), waiting child with pid = %d\n", pid);
      waitpid(pid, &status, 0);
      printf("In parent AFTER fork(), child has exited with status = %d\n", status);
      break;
	}
	return pid;
}
