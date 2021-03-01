OS161_VERS=DUMBVM

cd kern/compile/"${OS161_VERS}" || exit 1
ls
bmake depend
bmake
bmake install