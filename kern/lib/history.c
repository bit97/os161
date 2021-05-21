#include <lib.h>
#include <types.h>
#include <history.h>
#include <vnode.h>

struct history {
  struct vnode*   h_v;          /* Reference to the history file     */
  unsigned        h_size;       /* Number of items in history        */
  unsigned        h_pos;        /* Current pointer in history buffer */
  unsigned        h_past;       /* Position of cursor based on user history navigation */
  char**          h_cmds;       /* Actual commands                   */
};

static struct history *cmd_history = NULL;

static
struct history *
history_create(unsigned size)
{
  struct history *h;

  h = kmalloc(sizeof(struct history));
  
  h->h_v      = NULL; // Todo
  // Todo read file  
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
  unsigned i;

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
}

void
history_shutdown(void)
{
  history_destroy();
}

void
history_write(char* data)
{
  struct history *h = cmd_history;
  unsigned i;

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
  unsigned pos;

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
  unsigned pos;

  /* Reached bottom of the history */
  if (h->h_past == 1)         h->h_past = 0;
  if (h->h_past == 0)         return false;

  h->h_past--;

  if (h->h_past <= h->h_pos)  pos = h->h_pos - h->h_past;
  else                        pos = MAX_HIST - (h->h_past - h->h_pos);

  strcpy(data, h->h_cmds[pos]);
  return true;
}

