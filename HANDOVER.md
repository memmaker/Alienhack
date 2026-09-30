# AlienHack web port — handover

## RVIP progress

### Stage 1 — Get + build (done, cloud run)
- Base: `memmaker/Alienhack` master @ b843d42 (fork of SockPuppet8/Alienhack, v0.9.1 beta,
  upstream history kept = pristine). Only branch besides the work branch: `master`.
  Work branch: `claude/dazzling-brahmagupta-nqq41h`.
- Dependency RL-Shared (SockPuppet8/RL-Shared @ c42d3889d58672a5198ee2066c5cb1af509889f1) is
  **not committed**: `web/build.sh` clones it into `web/deps/` (gitignored) and applies
  `web/rl-shared.patch` (MSVC-isms: std::exception(msg), `at(at())`, temp-to-ref lineCast
  overloads, const setLocation, forward decl World, clock_t max).
  Boost: Emscripten `boost_headers` port (1.83) + Boost.Serialization / Boost.Filesystem
  sources cloned from boostorg @ boost-1.83.0 and compiled in.
- Case O: Windows console game. `RL-Shared/Console/Console.cpp` (Win32 console) is replaced by
  `web/Console-web.cpp`: 80x40 grid of char/fg/bg handed to JS (`Module.rvipScreen`),
  `readKey` waits via ASYNCIFY on `Module.rvipKeys` (PC scan codes | 0x100 for arrows/Home/PgUp..).
- Game source edits: `int main`, `<xutility>`→`<utility>`, `Overworld.hpp` case fix.
- Build: `source emsdk_env.sh; sh web/build.sh` → `web/alienhack.{js,wasm,data}` (keys.txt,
  ah_readme.txt preloaded). C++17, `-fexceptions`. `SAN=1 OPT=-O1` for ASan build (obj-asan/).
- Stage-1 page `web/index.html`: single `<pre>` of the 80x40 console (no WM yet).
- Tested (Playwright/Chromium): title → Long/Quick game → story → map, moves, fire/look/char
  screen, get. ASan build ran the same game start + save (S): no reports. ASan objs removed.
  No `signature mismatch`/`conflicting signatures` (build has no `-w`).

### Open
- No deploy (cloud run). Mac check in the browser pane still to do.
- Saves/mortem/keys go to MEMFS `/`, not IDBFS yet (5.10, later stage).
- Screen is one 80x40 grid; splitting into WM windows is stage 5.
