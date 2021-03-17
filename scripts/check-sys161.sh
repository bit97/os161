if [ "$(pgrep --count sys161)" -eq 0 ]; then
  notify-send --urgency=critical --expire-time=1000 "no sys161 istance running.."
  exit 1
fi

exit 0