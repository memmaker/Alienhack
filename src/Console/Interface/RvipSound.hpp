// RVIP web port: game actions name a sound; the page (rvip-sound.js) plays it
// when Audio > Sound effects is on. No-op outside the web build.
#ifndef ALIENHACK_RVIP_SOUND_HPP
#define ALIENHACK_RVIP_SOUND_HPP
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define RVIP_SOUND(n) EM_ASM({ if (Module.rvipSound) Module.rvipSound(UTF8ToString($0)); }, n)
#else
#define RVIP_SOUND(n) ((void)0)
#endif
#endif
