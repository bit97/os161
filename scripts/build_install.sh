OS161_VERS=SYSCALLS

function err {
  notify-send --urgency=critical --expire-time=1000 $1
}

cd kern/compile/"${OS161_VERS}" || exit 1
bmake depend  || err "bmake depend error"
bmake         || err "bmake error"
bmake install || err "bmake install error"