/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
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

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *sem_create(const char *name, unsigned initial_count)
{
  struct semaphore *sem;

  sem = kmalloc(sizeof(*sem));
  if (sem == NULL) {
    return NULL;
  }

  sem->sem_name = kstrdup(name);
  if (sem->sem_name == NULL) {
    kfree(sem);
    return NULL;
  }

  sem->sem_wchan = wchan_create(sem->sem_name);
  if (sem->sem_wchan == NULL) {
    kfree(sem->sem_name);
    kfree(sem);
    return NULL;
  }

  spinlock_init(&sem->sem_lock);
  sem->sem_count = initial_count;

  return sem;
}

void sem_destroy(struct semaphore *sem)
{
  KASSERT(sem != NULL);

  /* wchan_cleanup will assert if anyone's waiting on it */
  spinlock_cleanup(&sem->sem_lock);
  wchan_destroy(sem->sem_wchan);
  kfree(sem->sem_name);
  kfree(sem);
}

void P(struct semaphore *sem)
{
  KASSERT(sem != NULL);

  /*
   * May not block in an interrupt handler.
   *
   * For robustness, always check, even if we can actually
   * complete the P without blocking.
   */
  KASSERT(curthread->t_in_interrupt == false);

  /* Use the semaphore spinlock to protect the wchan as well. */
  spinlock_acquire(&sem->sem_lock);
  while (sem->sem_count == 0) {
    /*
     *
     * Note that we don't maintain strict FIFO ordering of
     * threads going through the semaphore; that is, we
     * might "get" it on the first try even if other
     * threads are waiting. Apparently according to some
     * textbooks semaphores must for some reason have
     * strict ordering. Too bad. :-)
     *
     * Exercise: how would you implement strict FIFO
     * ordering?
     */
    wchan_sleep(sem->sem_wchan, &sem->sem_lock);
  }
  KASSERT(sem->sem_count > 0);
  sem->sem_count--;
  spinlock_release(&sem->sem_lock);
}

void V(struct semaphore *sem)
{
  KASSERT(sem != NULL);

  spinlock_acquire(&sem->sem_lock);

  sem->sem_count++;
  KASSERT(sem->sem_count > 0);
  wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

  spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *lock_create(const char *name)
{
  struct lock *lock;

  lock = kmalloc(sizeof(*lock));
  if (lock == NULL) {
    return NULL;
  }

  lock->lk_name = kstrdup(name);
  if (lock->lk_name == NULL) {
    kfree(lock);
    return NULL;
  }

  HANGMAN_LOCKABLEINIT(&lock->lk_hangman, lock->lk_name);

#if OPT_LOCK
  lock->lk_wchan = wchan_create(lock->lk_name);
  spinlock_init(&lock->lk_splk);
  lock->lk_holder = NULL;

  KASSERT(lock->lk_wchan != NULL);
#elif OPT_LOCK_SEM
  lock->lk_sem = sem_create(name, 1);
  lock->lk_holder = NULL;

  KASSERT(lock->lk_sem != NULL);
#endif

  return lock;
}

void lock_destroy(struct lock *lock)
{
  KASSERT(lock != NULL);

#if OPT_LOCK
  spinlock_cleanup(&lock->lk_splk);
  wchan_destroy(lock->lk_wchan);
#elif OPT_LOCK_SEM
  KASSERT(lock->lk_holder != NULL);
  sem_destroy(lock->lk_sem);
#endif

  kfree(lock->lk_name);
  kfree(lock);
}

void lock_acquire(struct lock *lock)
{
  /* Call this (atomically) before waiting for a lock */
  HANGMAN_WAIT(&curthread->t_hangman, &lock->lk_hangman);

#if OPT_LOCK
  spinlock_acquire(&lock->lk_splk);

  while (lock->lk_holder != NULL) {
    wchan_sleep(lock->lk_wchan, &lock->lk_splk);
  }
  lock->lk_holder = curthread;

  spinlock_release(&lock->lk_splk);
#elif OPT_LOCK_SEM
  P(lock->lk_sem);
  lock->lk_holder = curthread;
#endif

  /* Call this (atomically) once the lock is acquired */
  HANGMAN_ACQUIRE(&curthread->t_hangman, &lock->lk_hangman);
}

void lock_release(struct lock *lock)
{
  /* Call this (atomically) when the lock is released */
  HANGMAN_RELEASE(&curthread->t_hangman, &lock->lk_hangman);

#if OPT_LOCK
  spinlock_acquire(&lock->lk_splk);

  KASSERT(lock_do_i_hold(lock));
  lock->lk_holder = NULL;
  wchan_wakeone(lock->lk_wchan, &lock->lk_splk);

  spinlock_release(&lock->lk_splk);
#elif OPT_LOCK_SEM
  KASSERT(lock_do_i_hold(lock));

  lock->lk_holder = NULL;
  V(lock->lk_sem);
#endif
}

bool lock_do_i_hold(struct lock *lock)
{
#if (OPT_LOCK || OPT_LOCK_SEM)
  /*
   * We're protected by the semaphore/spinlock, no need to extra protect this
   * section
   */
  return lock->lk_holder == curthread;
#else
  return true; // dummy until code gets written
#endif
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *cv_create(const char *name)
{
  struct cv *cv;

  cv = kmalloc(sizeof(*cv));
  if (cv == NULL) {
    return NULL;
  }

  cv->cv_name = kstrdup(name);
  if (cv->cv_name == NULL) {
    kfree(cv);
    return NULL;
  }

#if OPT_CV
  cv->cv_wchan = wchan_create(cv->cv_name);
  spinlock_init(&cv->cv_splk);

  KASSERT(cv->cv_wchan != NULL);
  KASSERT(&cv->cv_splk != NULL);
#endif

  return cv;
}

void cv_destroy(struct cv *cv)
{
  KASSERT(cv != NULL);

#if OPT_CV
  wchan_destroy(cv->cv_wchan);
  spinlock_cleanup(&cv->cv_splk);
#endif

  kfree(cv->cv_name);
  kfree(cv);
}

void cv_wait(struct cv *cv, struct lock *lock)
{
#if OPT_CV
  KASSERT(cv != NULL);

  spinlock_acquire(&cv->cv_splk);
  KASSERT(lock_do_i_hold(lock));
  lock_release(lock);
  wchan_sleep(cv->cv_wchan, &cv->cv_splk);
  spinlock_release(&cv->cv_splk);
  lock_acquire(lock);
#else
  // Write this
  (void) cv;    // suppress warning until code gets written
  (void) lock;  // suppress warning until code gets written
#endif
}

void cv_signal(struct cv *cv, struct lock *lock)
{
#if OPT_CV
  KASSERT(cv != NULL);

  spinlock_acquire(&cv->cv_splk);
  KASSERT(lock_do_i_hold(lock));
  wchan_wakeone(cv->cv_wchan, &cv->cv_splk);
  spinlock_release(&cv->cv_splk);
#else
  // Write this
  (void) cv;    // suppress warning until code gets written
  (void) lock;  // suppress warning until code gets written
#endif
}

void cv_broadcast(struct cv *cv, struct lock *lock)
{
#if OPT_CV
  KASSERT(cv != NULL);

  spinlock_acquire(&cv->cv_splk);
  KASSERT(lock_do_i_hold(lock));
  wchan_wakeall(cv->cv_wchan, &cv->cv_splk);
  spinlock_release(&cv->cv_splk);
#else
  // Write this
  (void) cv;    // suppress warning until code gets written
  (void) lock;  // suppress warning until code gets written
#endif
}
