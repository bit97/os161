#include <syscall.h>
#include <lib.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <kern/wait.h>
#include <opt-fork.h>
#include <kern/errno.h>

/**
 * Minimal support for Exit system call. Clean the thread and the process
 * structure associated. Store the return code in the proper field.
 * @param code      Exit status code
 */
void
sys__exit(int code)
{
  struct proc* proc = curthread->t_proc;
  struct thread* t = curthread;

  proc->p_exitcode = code;
  /*
   * Destroy the address space since it is no more needed
   * but preserve the needed fields for proper return code handling
   */
  if (proc->p_addrspace) proc_destroy_as(proc);

#if OPT_WAIT
  /*
   * Detach the thread from its process, since we cannot destroy a proc
   * structure with associated threads. This *MUST* come before proc_destroy()
   */
  proc_remthread(t);
  proc_signal(proc);
#else
  (void)t;
#endif /* OPT_WAIT */

  /*
   * We can now safely kill the thread (i.e. move it to the ZOMBIE state)
   */
  thread_exit();
}

#if OPT_WAIT
pid_t
sys_waitpid(pid_t pid, int *stat_loc, int options)
{
  struct proc *proc;
  int status;

  (void)options;

  /*
   * Retrieve process associated to given pid, if any
   */
  proc = proc_from_pid(pid);

  /*
   * Return error if there's no such process or if trying to wait for itself
   */
  if (!proc || curproc->p_pid == pid) {
    if (stat_loc) *stat_loc = __WEXITED;
    return -1;
  }

  status = proc_wait(proc);
  status |= __WEXITED;

  if (stat_loc) *stat_loc = status;

  return pid;
}

pid_t
sys_getpid(void)
{
  struct proc *proc = curproc;

  KASSERT(proc != NULL);

  return proc->p_pid;
}
#endif /* OPT_WAIT */

#if OPT_FORK

pid_t
sys_fork(struct trapframe *tf)
{
  struct proc *parent, *child;
  struct fork fork;
  int result;

	parent = curproc;

	/* Create a process for the new program to run in. */
	child = proc_create_runprogram(parent->p_name /* name */);
	if (child == NULL) {
		return ENOMEM;
	}

	child->parent = parent;
  fork.fork_sem = sem_create("fork_sem", 0);
  fork.fork_tf = tf;

  result = thread_fork(child->p_name /* thread name */,
                       child /* new process */,
                       dupprogram /* thread function */,
                       &fork /* thread arg */, 0 /* thread arg */);
  if (result) {
    kprintf("thread_fork failed: %s\n", strerror(result));
    proc_destroy(child);
    return result;
  }

  /*
   * Wait for child to duplicate the trapframe
   */
  P(fork.fork_sem);

  /*
   * In parent, return child's pid
   */
  return child->p_pid;
}

#endif /* OPT_FORK */
