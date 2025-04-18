#!/bin/sh

# call in the project root
# as ./scripts/screenshots.sh PLATFORM DAY HOUR MINUTE
mkdir -p design/store/screenshots/$1/
export SCREENSHOT=$(( 1740787230 + (((($2 - 1) * 24) + $3) * 60 + $4) * 60 ))
echo $SCREENSHOT
rebble wipe
rebble build
rebble install --emulator $1
rebble emu-app-config --emulator $1
sleep 5s
rebble screenshot design/store/screenshots/$1/$(date -Iseconds).png
rebble kill

