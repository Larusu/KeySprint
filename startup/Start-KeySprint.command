#!/bin/sh
# Finder opens .command files in Terminal.
cd -- "$(dirname -- "$0")" || exit 1
exec /bin/sh ./Start-KeySprint.sh
