cd ../root || exit 1
# already launched sys161 -w kernel
mips-harvard-os161-gdb -ex 'connect' 2>/dev/null
