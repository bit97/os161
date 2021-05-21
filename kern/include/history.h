#ifndef _HISTORY_H_
#define _HISTORY_H_

#include <opt-history.h>

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

#define MAX_HIST    4
#define MAX_CMD_LEN 64

struct history;  /* Opaque. */

void            history_bootstrap(void);
void            history_shutdown(void);

void            history_write(char* data);
bool            history_up(char* data);
bool            history_down(char* data);

#endif /* _HISTORY_H_ */