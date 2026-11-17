#!/bin/sh
# Linux: choose Run in Terminal in your file manager, or run this from a terminal.
cd -- "$(dirname -- "$0")/.." || exit 1

for keysprint_python in python3 python; do
    if command -v "$keysprint_python" >/dev/null 2>&1 &&
       "$keysprint_python" -c 'import sys; sys.exit(sys.version_info < (3, 10))' 2>/dev/null; then
        "$keysprint_python" launcher.py
        keysprint_status=$?
        if [ "$keysprint_status" -ne 0 ]; then
            printf '\nKeySprint could not start. Press Enter to close.'
            read -r keysprint_reply
        fi
        exit "$keysprint_status"
    fi
done

printf '%s\n' 'Python 3.10 or newer is required.' \
    'Install Python: https://www.python.org/downloads/' \
    'On Ubuntu/Debian, also install python3-venv if environment setup reports it missing.'
printf 'Press Enter to close.'
read -r keysprint_reply
exit 1
