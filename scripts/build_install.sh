OS161_VERS=SYSCALLS

cd kern/compile/"${OS161_VERS}" || exit 1
bmake depend
bmake
bmake install