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

### Stage 2 — Explore + stairs + no --More-- (done, cloud run)
- **Explore key `H`** (`Explore H` added to `keys.txt`; the only keyset; free). In-game help
  (`?`) row "Auto-explore"; stairs rows say "(or walk to >/<)".
- Code: `PlayingGame.cpp` `startAuto/stopAuto/autoStep/scanView` (+ members in `.hpp`).
  Known grid = `Zone::recordedTerrainAt` (!=0 known) and `recordedObjectAt` (!=0 = seen;
  vision is a 90° cone, so "seen" is the frontier test). Frontier = unvisited passable known
  cell next to an unseen cell, or a visible item cell; per-zone visited/skip sets.
  Doors: walking into a closed door opens it (game's own bump); its "You activate the
  door." message is ignored; a door that stays shut is skipped next time.
  Stops: visible alien ("In view: <name>."), new item in view (per zone, items in view at
  the key press don't stop), any new message, any key (dropped), step that didn't move
  ("Something is in the way.", cell skipped), zone change, "Nothing left to explore."
  or "...blocked doors or obstacles cut the way.".
- Hook: web `Console::readKey` returns synthetic `KeyCode(1,true)` after `emscripten_sleep(40)`
  while `extern "C" bool rvip_auto_on` is set (a queued real key clears it and is dropped);
  `exitToChild` (story/message boxes) clears it.
- **`>`/`<`** (FloorUp/FloorDown; the game draws up-stairs as `>`): off the stairs, walk to the
  nearest known unbroken stairs of that kind and stop ("You reach the stairs up. Press it
  again to climb."); stops only when more aliens are in view than at the start.
- **No --More--:** the game has none (message log panel; story/terminal boxes are one-off
  MessageBoxes needing Enter, kept). Nothing to add.
- **Bug fixed (stage 1 leftover):** `ConsoleView` slept `clock()` ticks as ms
  (CLOCKS_PER_SEC = 1e6 under Emscripten): every multi-frame action (a move) froze input ~20 s.
  Now `sleep(ticks*1000/CLOCKS_PER_SEC)` in `web/rl-shared.patch`; patch also adds
  `MessageDisplay::hasNewMessage()`.
- Header edits: `build.sh` only checks .cpp mtimes — touch includers (or `rm -rf web/obj`).
- Tested (Playwright headless): H runs across a floor with door opening and item/message
  stops; `>` from off-stairs walks, arrival stop, second `>` climbs to 1st Floor;
  "You don't know of any reachable stairs up." with none known.

### Open
- No deploy (cloud run). Mac check in the browser pane still to do (stage 1 + 2: watch explore painting).
- Explore key interrupt not tested headless (Asyncify timing); explore not run on deep/dark floors.
- Saves/mortem/keys go to MEMFS `/`, not IDBFS yet (5.10, later stage).
- Screen is one 80x40 grid; splitting into WM windows is stage 5.
