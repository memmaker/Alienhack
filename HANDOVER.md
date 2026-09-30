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

### Stage 3 — Enter menu + inventory (done, cloud run)
- New `src/Console/Interface/RvipMenus.{hpp,cpp}`: `CommandMenu` (Enter = keys.txt `OK`, was
  unused in play) lists every command in the help screen's groups (Actions / Weapons and items /
  Other), keys from the current keys.txt (`getFunctions()`), no movement/OK/Back rows; box sized to
  content (title counts), scrolls if taller than 40 rows. A command's own key runs it directly.
- **Running commands:** menus set `AlienHack::rvip_cmd` and queue `rvip_next_key = 0x102` (CMD key,
  ext char 2) — web `Console::readKey` returns `rvip_next_key` first. `PlayingGame::interpretInput`
  is now a wrapper around `interpretKey`; all `m_key_map->isFunction(input, ..)` became `isFn()`
  which matches `rvip_cmd` for the CMD key. Extra cmds: `Drop:<fn>` (SelectDropItem),
  `Examine:<fn>` (message), `Inventory?` (reopen unless `scanView` sees an alien), suffix `|reopen`.
  `exitToChild` cancels the reopen when the action opened a prompt (target select etc.).
- **Inventory `I`** (new `Inventory I` in keys.txt; `i`/`e` are Inc/Demolition): only carried
  items; each item's letter is its own game key (a armour, s/p weapons, r/t/i/k grenades, m, n, e).
  letter = main action (weapons: Ready, held one Fire; grenades Throw; medkit/neut Use; demo Place;
  armour Examine), Shift+letter drop, Ctrl+letter examine, Enter/Space = item menu (+ Reload for
  the held weapon). Any other key closes and runs as a normal command (forwarded via rvip_next_key).
- **Drop `d`** opens the same list as "Drop what?" (letters drop as before). Get dialog already had
  a cursor (kept). No `[ENTER] or` hints exist in the game.
- **Keypad:** page sends numpad keys as `0x200|char` (by `e.code`), Ctrl+letter as `0x400|letter`
  (Ctrl+W/T/N left to the browser). readKey: while `rvip_numpad_raw` (set by the menus) keypad keys
  come as ext chars ('8','2','5','+','-','*','0','.',4,6); otherwise 8/2/4/6/7/9/1/3 = directions,
  5 = wait. Ctrl+letter = ext `0xA0+index`. Menus: 8/2 move, 9/3 page, 5/Enter choose, 6 confirm /
  4 back, + main, - drop, * examine, 0 or . close.
- Tested (Playwright headless): menu layout, choose by arrows+numpad 5 and by key (`l`), inventory
  cursor, item menu drop + reopen, numpad * examine, Ctrl+a, drop prompt `t`, forwarded `f` → target.

### Stage 5 — Web page and windows (done, cloud run; not deployed)
- Page `web/index.html` (copied to `web/dist/` by `build.sh`, which now writes
  `web/dist/alienhack.{js,wasm,data}`; data preloaded at `/ahdata`). Loads `../rvip-wm.js`,
  `../rvip-app.js` (shared copies live in the `rvip` repo `web/`; no rvip-tools repo reachable).
  Top bar: Help · File ▾ | Windows ▾ · Font · Audio ▾ (no Tiles), hints H / I / Enter / ?.
- Windows: Map (canvas, the game's 40x33 world view; the game centres it on the player),
  Status (zone name + HUD), Inventory, Visible, Messages; "Whole screen" = one-window mode
  (the native 80x40 screen on a canvas, aspect kept). Map font select on its title bar.
- Routing in `web/Console-web.cpp`: cells drawn inside `RvipBase` (PlayingGame / LookMode /
  TargetSelect ::draw) are the main screen: Map = x2..41 y1..33, Status = x45..77 y1..19,
  prompt line = look/target info rows 35..38; cells drawn over it (dialogs, menus) = pop-up
  (their bounding box); screens without a main screen (title, story, help, char) = pop-up of
  all non-blank cells. Text panes use `\x01<fg><bg>` colour marks ('a'+Console::Colour),
  palette sent from C (`rvip_css`). Messages via `MessageDisplay::addString` hook
  (rl-shared.patch; same-turn messages join one line). Inventory/Visible built from game
  data in `RvipMenus.cpp rvipSidePanes()`; glyphs via `draw.cpp rvipObjectGlyph/rvipPickupGlyph`.
- Saves: IDBFS at `RvipApp.dir` (`/alienhack` on the site), cwd = that folder, keys.txt and
  ah_readme.txt symlinked from `/ahdata` each start. `saveGame` is atomic (`.tmp` + rename) and
  syncs. Autosave (the game deletes its save on load): at the first idle command prompt after
  start/load and after every floor change; deleted when the run ends unless the player saved (S).
  Layout/fonts/audio flags in `web-layout.json` there. S and death return to the game's own
  title (no reload); `Module.rvipEnd` (main returning) syncs and reloads as fallback.
- Tested (Playwright headless): title → story → map, all windows filled, look mode prompt line,
  Enter menu pop-up, one-window mode, autosave file appears, reload → Load → character back,
  S save; shared smoke.cjs (bar, menus, A+ only one window, kept over reload, IDB `/alienhack`),
  resize.cjs (no negative sizes), idbtest.cjs (no localStorage): no errors.
- Open: no deploy / `web/deploy.sh` yet (cloud); death → new game not tested in the browser;
  real look in the pane (Mac) still needed; sound checkboxes stored only (stage 6).

### Stage 6 — Docs and sound (done, cloud run; not deployed)
- **Help:** `web/make-help.py` (self-contained, cloud) → `dist/help.html` (run by `build.sh`). Key table
  parsed from `HelpScreen.cpp` (label + control name, 39 rows + Inventory) with keys from `keys.txt`
  (asserts >30 rows, every control in keys.txt); essentials box, Saving (IDB, autosave, Export/Import),
  new-player guide, tips (from ah_readme), "Playing in the browser", full ah_readme.txt in `<details>`.
  Credits: Sock Puppet, based on AliensRL (Kornel Kisielewicz); licence CC0 (`license.txt`).
  No Mac Docs entry yet (no Docs folder in cloud): generate one from make-help.py on the Mac.
- **Sound search:** web search (archive.org, roguetemple, GitHub, pastebin readme): upstream README says the
  game has no sound effects; no music, no fan packs found. So: effects synthesized at build time by
  `web/mksounds.py` (pure Python, public domain, 28 wavs → `dist/sound/`); **no music, Music toggle removed**.
- **Hooks (game actions, never message text):** `src/Console/Interface/RvipSound.hpp` `RVIP_SOUND(name)`
  (EM_ASM → `Module.rvipSound`, no-op natively) at the start of 36 `GameEvents.cpp` event handlers
  (fire, click, pickup, reload, wear, wield, drop, spray, heal, dodge, hit, hurt, armour, death, hugger,
  alert, beep, arm, explode, door, bang, hiss, kill, screech, spit, break, acid) + `stairs` in
  PlayingGame FloorUp/FloorDown climb. Page: `Module.rvipSound` plays via `../rvip-sound.js` only when
  Audio ▾ → Sound effects (`L.sfx`, web-layout.json) is on; off by default.
- Tested (Playwright headless, real clicks): sfx off by default and no sound requests while off; after a
  real click on the checkbox explore's door opens play `door` (`sound/door.wav` requested); Help opens with
  the guide, Esc closes. Other events only checked by name ⇄ wav (all names have a wav).
- Open: no deploy; Mac pane look + listening check of the synthesized sounds (volume/taste); `alert`
  (alien seen) and `hiss` (alien noise nearby) may be too frequent in play — tune after listening.


### Stage 7 — Publish (done, cloud run; not deployed)
- Version line: "Based on AlienHack 0.9.1 beta · SockPuppet8/Alienhack @ b843d42" (card, Help "About this
  version" in make-help.py, README top). RL-Shared @ c42d388 named in Help + README.
- Year 2018 (first public release, roguetemple topic 5669 / GitHub initial commit 2018-10-19); AliensRL 2007
  (Kornel Kisielewicz, 7DRL). Tree: new top-level `<li class="insp">` AliensRL (span, not web-published) with
  AlienHack as `<li class="insp">` child (own C++ code, "loosely based on AliensRL" per ah_readme).
- roguelikes (branch claude/dazzling-brahmagupta-nqq41h): card (no Info button, no shrine yet), tree,
  `img/alienhack.png` (Playwright shot after explore, crop 384x160 of map), years.json `alienhack` + `aliensrl`,
  "47 classic roguelikes"; `order.py` OK. og block written into `web/index.html` (og.py's game loop, inline).
- Open (Mac): merge both branches to main; game repo split per 5.15 (`rvip`-free already, but README compare
  link says `main`); add `web/deploy.sh` with guard; build + `deploy.sh` both repos; run og.py for the index
  og image; check `curl -s https://ruzzoli.de/roguelikes/alienhack/ | grep og:image`; Mac pane check.
### Mac check (block 2)
- Pane (local server of web/dist + shared JS) and headless: title, Quick game, story, map; explore H across
  floors (door opens, item/alien stops), a key press stops it; `>` walks to the known stairs and climbs
  (Ground → 1st floor); Enter menu; inventory I; death → mission report → title → new game: all pass.
- Fixed: side windows (Inventory, Visible, Messages, Status) kept the last game on the title: `Title::draw`
  calls `rvipClearPanes()` (RvipMenus.cpp), which sends each pane empty.
- Fixed: one-window mode was a canvas (W0 rule 6): the game sends the 80x40 screen as a text pane
  (`rvip_pane("screen")`, colour marks), shown in `<pre id="screen">`, font fitted to the window.
- Added mouse: a click on a pop-up line sends `0x800|line`, on the whole screen `0x1000|row`; readKey turns
  it into ext key 3 + `rvip_click_row`; Enter menu runs the row, inventory opens the item menu (drop prompt:
  drops), item menu runs the action.
- smoke.cjs, idbtest.cjs, resize.cjs: pass, no errors.
- Dark/deep floors (follow-up, temporary patch: every floor dark + no player damage, reverted): explore H
  walks on dark Ground/1st/2nd floors (Block A), stops at items and aliens, a key press stops it; `>` + H
  climbed to the 2nd floor. No code change needed. Note: the pane's `type "H"`/`shift+h` does not reach the
  page as `H` (a real keydown with key `H` does): test bots push 72 to `Module.rvipKeys`.
- One-window `<pre>` is centred in its window (padding set in `drawScreen`).
- Open: deploy by orchestrator.
### Open
- No deploy (cloud run). Mac check in the browser pane still to do (stage 1 + 2: watch explore painting).
- Explore key interrupt not tested headless (Asyncify timing); explore not run on deep/dark floors.
- Saves/mortem/keys go to MEMFS `/`, not IDBFS yet (5.10, later stage).
- Screen is one 80x40 grid; splitting into WM windows is stage 5.
- Stage 3: no mouse yet (page is a `<pre>`; add click → cursor/choose with the stage-5 page).
  Mac pane check of menus + keypad still to do.

### Stage 4 — Tiles (done, cloud run): **text only**
- Game ships no graphics (no image files in the tree; pure Win32-console ASCII, no tile
  support in RL-Shared). Theme is sci-fi (Aliens: marines, xenomorphs, facehuggers, pulse
  rifles, motion tracker, colony/ship terrain). Part 2 forbids a fantasy fallback set
  (DawnLike/Oryx/NetHack/Shockbolt), and no licensed sci-fi set covers ~95% of AlienHack's
  aliens/weapons/terrain in one sheet (mixing sets not allowed). Decision: text only, like
  ZAPM. No Tiles button; stage 5 builds the page without a tile switch.
- Tile search (2026-09-30, agent's decision, not the user's), candidates:
  - OGA "Sci-Fi RogueLike Pixel Art" (CC0/OGA-BY 3.0, 16x16): few aliens/robots/guns, no armours/pickups set; ~35%.
  - OGA Redshrike "Basic 32x32 sci-fi tiles" + sci-fi enemies (CC-BY/OGA-BY 3.0): terrain + a few enemies only; ~30%.
  - OGA "Top sci-fi CGA tileset": 7 robots/characters, doors, lasers; ~20%.
  - itch.io MOMONGA "Sci-Fi Dungeon Crawler Roguelike Tileset": page 403, licence unclear (not CC); not usable.
  - Kenney Sci-fi RTS / Roguelike Characters (CC0): RTS units or fantasy modular people; ~10%.
  None near 95%, mixing forbidden: text only stays.
- Text mode may use a period font (Oldschool PC Font Resource, CC BY-SA 4.0, credit) — stage 5.
- Open: no deploy in this cloud run.

### Stage 8 — Shrine (done, cloud run; not deployed)
- `roguelikes/shrine/alienhack.html` (zapm template) + `shrine/alienhack/manual.txt` (ah_readme.txt, CC0).
  Stats from code: 15 alien types (`Alien.hpp`), 11 weapons, 8 armours, 18 pickups; ~26k lines/154 files.
  No wizard/debug mode in code. No screenshots section (none taken in cloud). No web research done
  beyond stage-7 sources (Rogue Temple topic 5669, GitHub).
- Links: card Info button, tree ✦, game page `#bar h1` link (web/index.html).
- Open: deploy both repos + check the three links live; 375 px scrollWidth check; screenshots; AliensRL
  history (Kornel Kisielewicz, ChaosForge) could get more trivia.

### Stage 9 — Graveyard / beacon (done, cloud run; not deployed)
- Hook = the three places the game writes the mortem (`PlayingGame::notifyAHGameModelAdvance`):
  death (first "You are dead" frame, before the key wait), world explosion (death, no killer),
  escape (`m_good_ending` → `ev=win`, the bad ending "Escaped, but obliterated in the reactor
  explosion" → `ev=death`). `rvip_report_run(ev)` + `EM_JS rvip_beacon` in `PlayerCharacter.cpp`
  (values captured in `writeOutcome`, killer set in `takeDamage`/`huggerAttack` from
  `Alien::getSelectName(false)`; reset after each report). Via `RvipWM.report`, fetch fallback.
- Fields: `g=alienhack`, `ev`, `name` (NameEntry name), `killer` (alien type, deaths by an alien
  only; acid/fire/explosion/"dying blow" omit it), `depth` (`OverWorld::level` of the floor the
  run ended on; 0 on the ground-floor starting levels). Not sent: `score` (the game has no
  score/high-score list), `turns` (real-time game clock, no turn counter), `lvl` (no char level).
  No quit: the only way out is Save (save-and-quit sends nothing).
- Killer art: `roguelikes/killers/make.py alienhack()` → 15 PNGs (glyph + colour from `draw.cpp`,
  palette RVIP_CSS). Generated in the cloud with DejaVu Sans Mono Bold (Menlo missing): rerun
  `python3 make.py alienhack` on the Mac for Menlo.
- Tested (headless Playwright, `/roguelikes/beacon` routed, temp name-keyed patch `die*`/`win*`
  reverted): 503 → one URL with id/at in the IndexedDB outbox; `RvipWM.flush()` with 204 → sent,
  outbox empty. death: `ev=death&name=dieTester&killer=Death%20Spitter&depth=0`; win:
  `ev=win&name=winner%20x&depth=0`.
- Open: deploy both repos; real death in the user's browser → graveyard.html; Mac killer-art rerun.

### Mac: deploy (2026-09-30)
- `web/deploy.sh` (zapm guard) added; built with `~/tools/emsdk/emsdk_env.sh`; game + roguelikes deployed.
  og.py run (index now "48 classic roguelikes", shrine og block from the card text). Live md5 of
  index.html/js/wasm/data/help.html = web/dist; og:image, card Info, tree ✦, `h1` → shrine all live.
- Still open: Mac pane look, 375 px, screenshots, Menlo killer art, real death → graveyard.

### Mac: stage-6 follow-up (sound levels, alert/hiss, Docs)
- Sounds measured (Python wave): 0.03–1.2 s, no clipping, no silence; RMS spread was 14.8 dB (heal −12,
  click −27 dBFS). mksounds.py now levels all to −18 dBFS RMS, peak ≤ −3 dBFS (spread 4.9 dB; stairs −23, peaky steps).
- alert/hiss (pane, explore + `>`, rvipSound wrapped): hiss came in bursts (3 in 1.7 s; fires per alien move);
  alert only when an alien interrupts a long action (0–1 per 2 min). `RVIP_SOUND_GAP` (RvipSound.hpp): alert ≥5 s,
  hiss ≥4 s apart. Not listened to by ear.
- Docs entry: `~/Desktop/Games/Roguelikes/Docs/alienhack.html` (GAMES dict in build-docs.py + guide/Saving in
  guides.py, generated once from make-help.py; make-help.py stays self-contained).

**RVIP complete** (stages 1–9) except: deploys (stages 5–9) and the Mac checks (pane look,
375 px, screenshots, Menlo killer art).
