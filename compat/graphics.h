/* Wraps SDL_bgi's <graphics.h>. Without this, SDL intercepts Ctrl+C and SIGTERM
   and the labs ignore them while waiting in getch(), so they cannot be stopped
   from the terminal. Runs before main(), ahead of SDL starting up. */
#ifndef COMPAT_GRAPHICS_H
#define COMPAT_GRAPHICS_H
#include <stdlib.h>
__attribute__((constructor)) static void compat_keep_default_signals(void) {
    setenv("SDL_NO_SIGNAL_HANDLERS", "1", 1);
}
#include_next <graphics.h>
#endif
