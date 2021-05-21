#ifndef _HISTORY_H_
#define _HISTORY_H_

#include <opt-history.h>

/*
 * Adds support for fixed sized circular buffers
 *
 * Functions:
 *      history_bootstrap - allocate an history element and read history from
 *                          disk file
 *      history_shutdown  - flush history to the disk file and frees the memory
 *      history_up        - get previous (older) element of history. Save the index
 *      history_down      - get next (newer) element of history. Save the index
 *      history_write     - put an element into the history
 */

struct history;  /* Opaque. */

void            history_bootstrap(void);
void            history_shutdown(void);

void            history_write(char* data);
bool            history_up(char* data);
bool            history_down(char* data);

#endif /* _HISTORY_H_ */