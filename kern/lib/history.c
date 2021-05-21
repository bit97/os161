#include <lib.h>
#include <types.h>
#include <history.h>
#include <vnode.h>
#include <vfs.h>
#include <uio.h>
#include <kern/fcntl.h>
#include <kern/errno.h>

#define MAX_HIST    64
#define MAX_CMD_LEN 64
#define MAGIC_HIST_0  0xBAADBAAD
#define MAGIC_HIST_1  0xCAFECAFE

struct history {
  struct vnode*   h_v;          /* Reference to the history file     */
  uint16_t        h_size;       /* Number of items in history        */
  uint16_t        h_pos;        /* Current pointer in history buffer */
  uint16_t        h_past;       /* Position of cursor based on user history navigation */
  char**          h_cmds;       /* Actual commands                   */
};

static struct history *cmd_history  = NULL;
static char cmd_history_filename[]  = ".history";

/*
 * Load the history file content from disk
 */
static
int
history_load(void)
{
  struct history *h = cmd_history;
  struct iovec iov;
  struct uio ku;
  int result;
  off_t off;
  uint16_t i;
  uint32_t magic[2];

  /* Open the file. */
  result = vfs_open(cmd_history_filename, O_RDWR | O_CREAT, 0, &h->h_v);
  if (result) {
    panic("Could not open .history file\n");
  }

  uio_kinit(&iov, &ku, &magic, 2*sizeof(uint32_t), 0, UIO_READ);
  result = VOP_READ(h->h_v, &ku);
  if (result) {
    return result;
  }

  if (ku.uio_resid != 0) {
    /* short read; problem with file format? */
    kprintf(".history file format corrupted?\n");
    return EFTYPE;
  }

  if (magic[0] != MAGIC_HIST_0 || magic[1] != MAGIC_HIST_1) {
    /* Not a correct history file... skip reading */
    return EFTYPE;
  }

  uio_kinit(&iov, &ku, &h->h_size, 2*sizeof(uint16_t), 2*sizeof(uint32_t), UIO_READ);
  result = VOP_READ(h->h_v, &ku);
  if (result) {
    return result;
  }

  if (ku.uio_resid != 0) {
    /* short read; problem with file format? */
    kprintf(".history file format corrupted?\n");
    return EFTYPE;
  }

  if (h->h_size > 0) {
    off = 2*sizeof(uint32_t) + 2*sizeof(uint16_t);
    /* Read cmd history */
    for (i = 0; i < h->h_size; i++, off += sizeof(char)*MAX_CMD_LEN) {
      h->h_cmds[i] = (char*)kmalloc(sizeof(char)*MAX_CMD_LEN);
      uio_kinit(&iov, &ku, h->h_cmds[i], sizeof(char)*MAX_CMD_LEN, off, UIO_READ);
      result = VOP_READ(h->h_v, &ku);
      if (result) {
        return result;
      }
    }
  }

  h->h_pos = h->h_size == MAX_HIST ? h->h_pos : h->h_size;

  return 0;
}


/*
 * Flush the in memory history into the file on disk
 */
static
int
history_flush(void)
{
  struct history *h = cmd_history;
  struct iovec iov;
  struct uio ku;
  int result;
  off_t off;
  uint16_t i;
  uint32_t magic[2] = { MAGIC_HIST_0, MAGIC_HIST_1 };

  uio_kinit(&iov, &ku, &magic, 2*sizeof(uint32_t), 0, UIO_WRITE);
  result = VOP_WRITE(h->h_v, &ku);
  if (result) {
    return result;
  }
  if (ku.uio_resid != 0) {
    /* short read; problem with file format? */
    kprintf(".history file format corrupted?\n");
    return EFTYPE;
  }

  uio_kinit(&iov, &ku, &h->h_size, 2*sizeof(uint16_t), 2*sizeof(uint32_t), UIO_WRITE);
  result = VOP_WRITE(h->h_v, &ku);
  if (result) {
    return result;
  }
  if (ku.uio_resid != 0) {
    /* short read; problem with file format? */
    kprintf(".history file format corrupted?\n");
    return EFTYPE;
  }

  if (h->h_size > 0) {
    off = 2*sizeof(uint32_t) + 2*sizeof(uint16_t);
    /* Write cmd history */
    for (i = 0; i < h->h_size; i++, off += sizeof(char)*MAX_CMD_LEN) {
      uio_kinit(&iov, &ku, h->h_cmds[i], sizeof(char)*MAX_CMD_LEN, off, UIO_WRITE);
      result = VOP_WRITE(h->h_v, &ku);
      if (result) {
        return result;
      }
    }
  }

  /* Done with the file now. */
  vfs_close(h->h_v);

  return 0;
}

static
struct history *
history_create(uint16_t size)
{
  struct history *h;

  h = kmalloc(sizeof(struct history));

  h->h_v      = NULL;
  h->h_size   = 0;
  h->h_pos    = 0;
  h->h_past   = 0;
  h->h_cmds   = (char**)kmalloc(sizeof(char*)*size);

  KASSERT(h != NULL);

  return h;
}

static
void
history_destroy()
{
  uint16_t i;

  for (i = 0; i < cmd_history->h_size; i++)
    kfree(cmd_history->h_cmds[i]);
  kfree(cmd_history->h_cmds);
  kfree(cmd_history);
}

void
history_bootstrap(void)
{
  cmd_history = history_create(MAX_HIST);
  KASSERT(cmd_history != NULL);
  history_load();
}

void
history_shutdown(void)
{
  history_flush();
  history_destroy();
}

void
history_write(char* data)
{
  struct history *h = cmd_history;
  uint16_t i;

  if (strlen(data) == 0)  return;

  i = h->h_pos;

  /* Allocate a new buffer for supplied cmd, at position pos */
  if (h->h_size < MAX_HIST) {
    h->h_cmds[i] = (char*) kmalloc(sizeof(char)*MAX_CMD_LEN);
    h->h_size++;

    KASSERT(h->h_cmds[i] != NULL);
  }

  /* Write in cmd buffer */
  strcpy(h->h_cmds[i], data);

  h->h_pos  = (i + 1) % MAX_HIST;
  h->h_past = 0;
}

bool
history_up(char* data)
{
  struct history *h = cmd_history;
  uint16_t pos;

  /* Reached top of the history */
  if (h->h_past >= h->h_size) return false;

  h->h_past++;

  if (h->h_past <= h->h_pos)  pos = h->h_pos - h->h_past;
  else                        pos = MAX_HIST - (h->h_past - h->h_pos);

  strcpy(data, h->h_cmds[pos]);
  return true;
}

bool
history_down(char* data)
{
  struct history *h = cmd_history;
  uint16_t pos;

  /* Reached bottom of the history */
  if (h->h_past == 1)         h->h_past = 0;
  if (h->h_past == 0)         return false;

  h->h_past--;

  if (h->h_past <= h->h_pos)  pos = h->h_pos - h->h_past;
  else                        pos = MAX_HIST - (h->h_past - h->h_pos);

  strcpy(data, h->h_cmds[pos]);
  return true;
}

