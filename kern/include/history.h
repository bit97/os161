/*
 * Adds support for fixed sized circular buffers
 *
 * Functions:
 *      history_init      - allocate an empty circular buffer of given size
 *      history_write     - adds an element into the circular buffer. It
 *                          potentially overwrites an old element on full buffer
 *      history_up        - get previous (older) element of history. Save the index
 *      history_down      - get next (newer) element of history. Save the index
 *      history_destroy   - release acquired resources
 */

#define MAXHISTORY    64

struct history;  /* Opaque. */

struct history *history_create(unsigned size);
void            history_write(struct history *, char* data);
bool            history_up(struct history*, char* data);
bool            history_down(struct history*, char* data);
void            history_destroy(struct history *);
