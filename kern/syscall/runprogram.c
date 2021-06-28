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
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than runprogram() does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <cpu.h>
#include "../arch/mips/include/trapframe.h"

#include <opt-args.h>

#if OPT_ARGS
#include <copyinout.h>
#endif

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char **kargv, int kargc)
{
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr, *argv;
	userptr_t baseptr;
	int result, i;
	size_t arg_len, stack_len;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	/* We should be a new process. */
	KASSERT(proc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as == NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	proc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

#if OPT_ARGS
  /* include the NULL pointer as the last element of argv */
  argv = (vaddr_t*)kmalloc(sizeof (vaddr_t) * (kargc + 1));
	baseptr = (userptr_t)stackptr;

  /* Push the arguments themselves */
  for (i = 0; i < kargc; i++) {
    arg_len = strlen(kargv[i]) + 1;
    stack_len = ROUNDUP(arg_len, 4);
    baseptr -= stack_len;

    copyoutstr(kargv[i], baseptr, arg_len, &arg_len);
    argv[i] = (vaddr_t) baseptr;
  }
  /* NULL terminator */
  argv[i] = (vaddr_t) NULL;

  /* Push the pointer to the arguments */
  for (i = kargc; i >= 0; i--) {
    baseptr -= 4;
    copyout(&argv[i], baseptr, 4);
  }

#else
	(void)kargc;
	(void)kargv;
	(void)baseptr;
	(void)argv;
	(void)arg_len;
	(void)stack_len;
#endif

	/* Warp to user mode. */
	enter_new_process(kargc /*argc*/, (userptr_t) baseptr /*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
            (vaddr_t) baseptr, entrypoint);

	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}

#if OPT_FORK

void
cloneprogram(void *tf_ptr, unsigned long unused)
{
  struct trapframe *tf_heap, tf;
  struct proc *child = curproc;

  (void)unused;

  tf_heap = (struct trapframe*)tf_ptr;
  /*
   * We need a *local* copy of the tf
   */
  tf = *tf_heap;
  tf.tf_v0 = 0;       /* in child fork() returns 0  */
  tf.tf_a3 = 0;       /* signal no error            */
  /*
   * Now, advance the program counter, to avoid restarting
   * the syscall over and over again.
   */
  tf.tf_epc += 4;

  /* Switch to it and activate it. */
  proc_setas(child->p_addrspace);
  as_activate();

  enter_forked_process(&tf);
}

#endif /* OPT_FORK */
