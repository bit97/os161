#include <syscall.h>
#include <lib.h>

#include <opt-sys_io.h>
#include <opt-file.h>

#if OPT_FILE
#include <vfs.h>
#include <vnode.h>
#include <uio.h>
#include <current.h>
#include <proc.h>
#include <kern/seek.h>

#define IO_WRITE 0U
#define IO_READ  1U

static
void
prepare_io(struct addrspace *as, struct iovec *iov,
                struct uio *u, off_t offset, const char* buf,
                size_t nbytes, unsigned short mode)
{
  iov->iov_ubase = (userptr_t) buf;
  iov->iov_len = nbytes;
  u->uio_iov = iov;
  u->uio_iovcnt = 1;
  u->uio_resid = nbytes;
  u->uio_offset = offset;
  u->uio_segflg = UIO_USERSPACE;
  u->uio_rw = mode == IO_WRITE ? UIO_WRITE : UIO_READ;
  u->uio_space = as;
}
#endif /* OPT_FILE */


static
ssize_t
console_write(const char* buf, size_t nbyte)
{
  size_t i;

  for (i = 0; i < nbyte; i++)
    putch(buf[i]);
  return (ssize_t) nbyte;
}

static
ssize_t
file_write(int fd, const char* buf, size_t nbyte)
{
  struct proc* p = curproc;
  struct iovec iov;
  struct uio u;
  struct openfile of;
  int result;

  of = p->openfiles[fd];
  if (!of.v) return -1;

  prepare_io(proc_getas(), &iov, &u, of.offset, buf, nbyte, IO_WRITE);

  result = VOP_WRITE(of.v, &u);
  if (result) {
    return result;
  }
  return (ssize_t)nbyte - u.uio_resid;
}

static
ssize_t
console_read(char* buf, size_t nbyte)
{
  ssize_t n = 0, u_nbyte = (ssize_t)nbyte;
  char ch;

  do {
    ch = (char)getch();
    if (ch == '\n') break;
    buf[n++] = ch;
  } while (n < u_nbyte);

  return n;
}

static
ssize_t
file_read(int fd, const char* buf, size_t nbyte)
{
  struct proc* p = curproc;
  struct iovec iov;
  struct uio u;
  struct openfile of;
  int result;

  of = p->openfiles[fd];
  if (!of.v) return -1;

  prepare_io(proc_getas(), &iov, &u, of.offset, buf, nbyte, IO_READ);

  result = VOP_READ(of.v, &u);
  if (result) {
    return result;
  }
  return (ssize_t)nbyte - u.uio_resid;
}

#if OPT_SYS_IO
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
  const char* ch_buf = (const char*) buf;

#if OPT_FILE
  switch(fd) {
    case STDIN_FILENO:
      return -1;
    case STDERR_FILENO:
    case STDOUT_FILENO:
      return console_write(ch_buf, nbyte);
    default:
      return file_write(fd, ch_buf, nbyte);
  }
#else
  (void)fd;
  return console_write(ch_buf, nbyte);
#endif /* OPT_FILE */
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
  char* ch_buf = (char*) buf;

#if OPT_FILE
  switch(fd) {
    case STDERR_FILENO:
    case STDOUT_FILENO:
      return -1;
    case STDIN_FILENO:
      return console_read(ch_buf, nbyte);
    default:
      return file_read(fd, ch_buf, nbyte);
  }
#else
  (void)fd;
  return console_write(ch_buf, nbyte);
#endif /* OPT_FILE */
}
#endif /* OPT_SYS_IO */

#if OPT_FILE
int
sys_open(const char *path, int oflag)
{
  int i;
  struct vnode* v;
  struct proc* p = curproc;

  if (vfs_open((char*)path, oflag, 0, &v) < 0) {
    return -1;
  }

  for (i = STDERR_FILENO + 1; i < OPEN_MAX; i++) {
    if (p->openfiles[i].v == NULL) {
      p->openfiles[i].v = v;
      return i;
    }
  }

  return -1;
}

int
sys_close(int fd)
{
  struct proc* p = curproc;

  if (fd <= STDERR_FILENO || fd > OPEN_MAX) return -1;
  if (!p->openfiles[fd].v)  return -1;

  vfs_close(p->openfiles[fd].v);
  p->openfiles[fd].v = NULL;

  return 0;
}

int
sys_remove(const char *path)
{
  return vfs_remove((char*)path);
}

off_t
sys_lseek(int fd, off_t offset, int whence)
{
  struct proc* p = curproc;
  struct openfile of;

  if (fd <= STDERR_FILENO || fd > OPEN_MAX) return -1;

  of = p->openfiles[fd];
  if (!of.v)  return -1;

  switch (whence) {
    case SEEK_SET:
      of.offset = offset;
      break;
    case SEEK_CUR:
      of.offset += offset;
      break;
    default:
      /* SEEK_END not implemented */
      break;
  }

  return 0;
}

#endif /* OPT_FILE */