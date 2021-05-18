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
  proc->p_exitcode = code;
  if (proc->p_addrspace) proc_destroy_as(proc);
  proc_signal(proc);
  thread_exit();
}