OS161_VERS=THREADS

cd kern/compile/"${OS161_VERS}" || exit 1
bmake depend
bmake
bmake install