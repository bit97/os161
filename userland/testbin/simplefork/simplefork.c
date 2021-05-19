#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define ECHILD 16

static
void
fork_test(void)
{
  pid_t pid;
  int status;

  printf("\n\n >>>> fork_test() <<<<<\n\n");

  printf("In the parent BEFORE fork()\n");

  pid = fork();

  printf("fork() returned %d, should print twice\n", pid);

  switch(pid) {
    case -1:
      printf("Error in fork()\n");
      return;
    case 0:
      printf("In child AFTER fork()\n");
      _exit(0);
    default:
      printf("In parent AFTER fork(), waiting child with pid = %d\n", pid);
      waitpid(pid, &status, 0);
      printf("In parent AFTER fork(), child has exited with status = %d\n", status);
      break;
  }
}

static
void
waitpid_test(void)
{
  int status, res, i;
  pid_t pid;

  printf("\n\n >>>> waitpid_test() <<<<<\n\n");

  /* Error: no child */
  res = waitpid(10, &status, 0);
  if (res < 0) {
    res = errno;
    if (res == ECHILD) {
      printf("Error in waitpid(): ECHILD\n");
    }
  }

  pid = getpid();
  printf("I have pid: %d\n", pid);

  /* Error: self waiting */
  res = waitpid(pid, &status, 0);
  if (res < 0) {
    res = errno;
    if (res == ECHILD) {
      printf("Error in waitpid(): ECHILD\n");
    }
  }

  for (i = 0; i < 20; i++) {
    pid = fork();

    if (pid) {
      // parent
      res = waitpid(pid, &status, 0);
      if (res < 0) {
        res = errno;
        if (res == ECHILD) {
          printf("Error in waitpid(): ECHILD\n");
        }
      } else {
        printf("Correctly waited for child with pid: %d\n", pid);
      }
    } else {
      // child
      printf("exiting from child with pid: %d\n", getpid());
      _exit(0);
    }
  }
}

int
main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  fork_test();
  waitpid_test();

	return 0;
}
