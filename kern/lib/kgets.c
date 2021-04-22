/*
 * Copyright (c) 2001
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <types.h>
#include <lib.h>
#include <opt-history.h>

#if OPT_HISTORY

#include <history.h>

static struct history *cmd_history = NULL;

static void cmd_history_init(void)
{
  cmd_history = history_create(MAXHISTORY);
  KASSERT(cmd_history != NULL);
}

#endif /* OPT_HISTORY */

/*
 * Do a backspace in typed input.
 * We overwrite the current character with a space in case we're on
 * a terminal where backspace is nondestructive.
 */
static void backsp(void)
{
  putch('\b');
  putch(' ');
  putch('\b');
}

/*
 * Read a string off the console. Support a few of the more useful
 * common control characters. Do not include the terminating newline
 * in the buffer passed back.
 */
void kgets(char *buf, size_t maxlen)
{
  size_t pos = 0;
  int ch;
#if OPT_HISTORY
  bool maybe_arrow = false, found;
  char ptr[64];

  if (cmd_history == NULL)
    cmd_history_init();
#endif

  while (1) {
    ch = getch();
    if (ch == '\n' || ch == '\r') {
      putch('\n');
      break;
    }

    /* Only allow the normal 7-bit ascii */
    if (ch >= 32 && ch < 127 && pos < maxlen - 1) {
#if OPT_HISTORY
      if (maybe_arrow) {
        found = false;
        switch (ch) {
          case 91:
            break;
          case 65:
            /* Arrow UP */
            found = history_up(cmd_history, ptr);
            maybe_arrow = false;
            break;
          case 66:
            /* Arrow DOWN */
            found = history_down(cmd_history, ptr);
            maybe_arrow = false;
            break;
          default:
            maybe_arrow = false;
            break;
        }
        if (found) {
          found = false;
          while (pos > 0) {
            backsp();
            pos--;
          }
          strcpy(buf, ptr);
          while (buf[pos]) {
            putch(buf[pos]);
            pos++;
          }
        } else {
          if (ch == 66) {
            /* Moved down after the newest history command.. clear the output */
            while (pos > 0) {
              backsp();
              pos--;
            }
          }
        }
      } else {
        putch(ch);
        buf[pos++] = ch;
      }
#else
      putch(ch);
      buf[pos++] = ch;
#endif
    } else if ((ch == '\b' || ch == 127) && pos > 0) {
      /* backspace */
      backsp();
      pos--;
    } else if (ch == 3) {
      /* ^C - return empty string */
      putch('^');
      putch('C');
      putch('\n');
      pos = 0;
      break;
    } else if (ch == 18) {
      /* ^R - reprint input */
      buf[pos] = 0;
      kprintf("^R\n%s", buf);
    } else if (ch == 21) {
      /* ^U - erase line */
      while (pos > 0) {
        backsp();
        pos--;
      }
    } else if (ch == 23) {
      /* ^W - erase word */
      while (pos > 0 && buf[pos - 1] == ' ') {
        backsp();
        pos--;
      }
      while (pos > 0 && buf[pos - 1] != ' ') {
        backsp();
        pos--;
      }
    }
#if OPT_HISTORY
    else if (ch == 27) {
      maybe_arrow = true;
    }
#endif
    else {
      beep();
    }
  }

  buf[pos] = 0;
#if OPT_HISTORY
  history_write(cmd_history, buf);
#endif
}
