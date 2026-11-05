#!/usr/bin/env bash
# Build and run the graphics labs on Linux (SDL_bgi).
#   ./run.sh              build everything, then run each program in turn
#   ./run.sh lab3_que1    build and run one program (type its input yourself)
#   ./run.sh --build      build everything, run nothing
# Press a key in the graphics window to close a program and move to the next.
cd "$(dirname "$(readlink -f "$0")")" || exit 1

CFLAGS=(-Wall -Wextra -g3 -Icompat -I/usr/local/include)
LDFLAGS=(-L/usr/local/lib64 -Wl,-rpath,/usr/local/lib64 -lSDL_bgi -lSDL2 -lm)
mkdir -p output

build() { # build <path to .c>
    gcc "${CFLAGS[@]}" "$1" -o "output/$(basename "$1" .c)" "${LDFLAGS[@]}"
}

# Sample input for the programs that read from the keyboard, taken from the
# notes in each source file. Used only when running everything.
sample_input() {
    case "$1" in
        lab3_que1) printf '100 100\n400 300\n' ;;
        lab3_que2) printf '3\n100 100\n200 100\n150 50\n100 50\n2 2\n45\n' ;;
        lab4_que1) printf '350 250\n100\n' ;;
        lab4_que2) printf '100 200 400 300\n' ;;
    esac
}

case "${1:-}" in
    --build|"")
        failed=0
        for src in *.c; do
            echo "Building $src"
            build "$src" || failed=1
        done
        [ "$failed" -eq 0 ] || { echo "Some files failed to build." >&2; exit 1; }
        [ "${1:-}" = "--build" ] && exit 0
        for src in *.c; do
            name=${src%.c}
            echo; echo "=== $name  (press a key in the window to continue) ==="
            input=$(sample_input "$name")
            if [ -n "$input" ]; then
                echo "$input" | "./output/$name"
            else
                "./output/$name"
            fi
        done
        ;;
    *)
        name=${1%.c}
        [ -f "$name.c" ] || { echo "No such program: $name.c" >&2; exit 1; }
        build "$name.c" && "./output/$name"
        ;;
esac
