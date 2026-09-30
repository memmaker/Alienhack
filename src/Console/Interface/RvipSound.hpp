// RVIP web port: game actions name a sound; the page (rvip-sound.js) plays it
// when Audio > Sound effects is on. No-op outside the web build.
#ifndef ALIENHACK_RVIP_SOUND_HPP
#define ALIENHACK_RVIP_SOUND_HPP
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define RVIP_SOUND(n) EM_ASM({ if (Module.rvipSound) Module.rvipSound(UTF8ToString($0)); }, n)
// Same, but at most once per ms real time from this call site (events that fire per alien move).
#define RVIP_SOUND_GAP(n, ms) do { static double t_ = -1e9; double now_ = emscripten_get_now(); \
	if (now_ - t_ >= (ms)) { t_ = now_; RVIP_SOUND(n); } } while (0)
#else
#define RVIP_SOUND(n) ((void)0)
#define RVIP_SOUND_GAP(n, ms) ((void)0)
#endif
#endif
