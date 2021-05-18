#include <syscall.h>
#include <lib.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>

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