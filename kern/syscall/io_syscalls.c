#include <syscall.h>
#include <lib.h>

int
sys_write(int fd, userptr_t buf, size_t nbyte)
{
  unsigned i;
  const char* ch_buf = (const char*) buf;
  (void)fd;

  for (i = 0; i < nbyte; i++)
    putch(ch_buf[i]);

  return 0;
}