#include <syscall.h>
#include <lib.h>

/**
 * Write system call. It redirects output to stdout
 * @param fd    File descriptor - ignored
 * @param buf   Location of buffer containing the message
 * @param nbyte Size of buffer
 * @return      Number of byte written
 */
ssize_t
sys_write(int fd, userptr_t buf, size_t nbyte)
{
  unsigned i;
  const char* ch_buf = (const char*) buf;
  (void)fd;

  for (i = 0; i < nbyte; i++)
    putch(ch_buf[i]);

  return nbyte;
}

/**
 * Read system call. It reads from stdin
 * @param fd    File descriptor - ignored
 * @param buf   Location of buffer to read to
 * @param nbyte Size of buffer
 * @return      Number of byte read
 */
ssize_t
sys_read(int fd, userptr_t buf, size_t nbyte)
{
  ssize_t n, u_nbyte = (ssize_t)nbyte;
  char* ch_buf = (char*) buf;
  char ch;
  (void)fd;

  n = 0;

  do {
    ch = (char)getch();
    if (ch == '\n') break;
    ch_buf[n++] = ch;
  } while (n < u_nbyte);

  return n;
}