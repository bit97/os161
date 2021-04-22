#include <lib.h>
#include <types.h>
#include <history.h>

struct history {
  char **data;
  unsigned size;
  unsigned count;
  unsigned index;
};

struct history *
history_create(unsigned size)
{
  struct history *h;

  h = kmalloc(sizeof(struct history));

  h->size   = size;
  h->count  = 0;
  h->index  = 0;
  h->data   = kmalloc(size * sizeof(char**));

  KASSERT(h != NULL);
  KASSERT(h->data != NULL);

  return h;
}

void
history_write(struct history *h, char* data)
{
  unsigned i;

  if (h->count < h->size) {
    /* Same size as cmd buffer in kgets */
    h->data[h->count] = kmalloc(64 * sizeof(char));
    strcpy(h->data[h->count++], data);
  } else {
    /* Shift history up */
    kfree(h->data[0]);
    for (i = 1; i < h->size; i++) {
      h->data[i-1] =  h->data[i];
    }
    h->data[h->size - 1] = kmalloc(64 * sizeof(char));
    strcpy(h->data[h->size - 1], data);
  }
  h->index = h->count;
}

bool
history_up(struct history* h, char* data)
{
  if (h->index == 0)
    return false;

  strcpy(data, h->data[--h->index]);
  return true;
}

bool
history_down(struct history* h, char* data)
{
  if (h->count == 0 || h->index == h->count - 1)
    return false;

  strcpy(data, h->data[++h->index]);
  return true;
}


void
history_destroy(struct history *h)
{
  unsigned i;

  for (i = 0; i < h->size; i++)
    kfree(h->data[i]);
  kfree(h->data);
  kfree(h);
}