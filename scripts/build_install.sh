OS161_VERS=HELLO

cd kern/compile/"${OS161_VERS}" || exit 1
bmake depend
bmake
bmake install