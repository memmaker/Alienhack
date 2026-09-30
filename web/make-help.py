#!/usr/bin/env python3
"""Build dist/help.html, the page's Help guide (RVIP stage 6).
Self-contained (cloud run): key rows parsed from the game's own help screen
(src/Console/Interface/HelpScreen.cpp: label + control name) with keys from keys.txt;
guide text written for the web port; the full ah_readme.txt in a <details>."""
import html, os, re, sys
W = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(W)
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(W, 'dist', 'help.html')
e = html.escape

KEYNAME = {'Enter': 'Enter', 'Esc': 'Esc', 'Space': 'Space', 'Up': '↑ / 8', 'Down': '↓ / 2',
           'Left': '← / 4', 'Right': '→ / 6', 'UpAndLeft': 'Home / 7', 'UpAndRight': 'PgUp / 9',
           'DownAndLeft': 'End / 1', 'DownAndRight': 'PgDn / 3'}
keys = {}
for l in open(os.path.join(ROOT, 'keys.txt')):
    p = l.split()
    if len(p) == 2: keys.setdefault(p[0], []).append(KEYNAME.get(p[1], p[1]))
src = open(os.path.join(ROOT, 'src/Console/Interface/HelpScreen.cpp')).read()
rows = re.findall(r'^\tconsole\.drawText\( \d+, \d+, "([^"]+)", text_col \);\n\tdrawControls\( console, controls, "(\w+)"', src, re.M)
rows.append(('Inventory window / item menu (web port)', 'Inventory'))
assert len(rows) > 30, len(rows)
for lab, fn in rows: assert fn in keys, fn
table = '\n'.join('<tr><td><kbd>%s</kbd></td><td>%s</td></tr>' % (' , '.join(e(k) for k in keys[fn]), e(lab)) for lab, fn in rows)
readme = open(os.path.join(ROOT, 'ah_readme.txt'), encoding='latin-1').read()

page = f'''<h1>AlienHack</h1>
<p><b>AlienHack</b> (v0.9.1 beta) by <b>Sock Puppet</b> is a tactical, combat-focused roguelike loosely based
on <i>AliensRL</i> by Kornel Kisielewicz. You are a lone soldier escaping a labyrinthine colony complex
infested with fast, deadly aliens. You see only what is in front of you, actions take different amounts of
time, and your survival depends on guns, grenades, armour and positioning. Source code is CC0 (public domain).
This browser version is an RVIP port; its sound effects are synthesized for it (public domain).</p>

<div style="border:1px solid var(--line,#888);border-radius:6px;padding:6px 12px;margin:10px 0">
<b>Essential keys</b><br>
<kbd>?</kbd> game help screen · <kbd>H</kbd> auto-explore · <kbd>Enter</kbd> command menu ·
<kbd>I</kbd> inventory / item menu · <kbd>&gt;</kbd> / <kbd>&lt;</kbd> walk to the stairs, press again to climb ·
<kbd>f</kbd> fire · <kbd>o</kbd> reload · <kbd>l</kbd> look / map · <kbd>S</kbd> save and quit · <kbd>Esc</kbd> cancel
</div>
<details><summary>All keys (from the game's help screen)</summary>
<table>{table}</table>
<p>Movement: arrow keys or the number pad (7 9 1 3 or Home PgUp End PgDn for diagonals).
<kbd>z</kbd>/<kbd>x</kbd> before a direction strafe/turn for one move; <kbd>Z</kbd>/<kbd>X</kbd> lock it until <kbd>Esc</kbd>.
Note: the game draws stairs up as <code>&gt;</code> and stairs down as <code>&lt;</code>.</p>
</details>

<h2>Saving</h2>
<p>The game keeps its save in your browser (IndexedDB), not on a server. <kbd>S</kbd> saves and returns to the
title screen; choose <i>Load</i> there to go on. The port also saves by itself when you start or load a game and
after every floor change, so closing the tab loses at most the current floor. When your soldier dies the save is
deleted (unless you saved with <kbd>S</kbd>). <b>File ▾ → Export</b> downloads your save; <b>Import</b> puts one back
(<code>save.txt</code> = long game, <code>quick.txt</code> = quick game). Clearing site data deletes saves.</p>

<h2>New player guide</h2>
<ol>
<li>At the title pick a <i>Quick game</i> for a short run or a <i>Long game</i> for the full escape. Read the
briefing with <kbd>Enter</kbd>. Spending perk points (<kbd>$</kbd>) early helps: <i>Keen Hearing</i> is the
readme's pick if the game feels too hard.</li>
<li>Your field of view is a cone in front of you. Moving turns you; <kbd>z</kbd> strafes (move without turning),
<kbd>x</kbd> turns on the spot. Check corners and look behind you.</li>
<li>Press <kbd>H</kbd> to explore: it walks to unseen places, opens doors, and stops when an alien comes into view,
when new items appear, on a new message, or on any key.</li>
<li>Pick things up with <kbd>g</kbd>; on a weapon it also offers to take its ammo.
Wear armour at all times; its condition is the cyan bar, ammo the green one.</li>
<li>Fire with <kbd>f</kbd> (choose a target, <kbd>Enter</kbd>). Reload (<kbd>o</kbd>) only when safe; switching from
sidearm to primary is slow.</li>
<li>Find the stairs and climb out. <kbd>&gt;</kbd> walks to the nearest known stairs up; press it again on them to
climb. Info terminals (<kbd>Space</kbd> next to one) map the whole floor.</li>
<li><kbd>Enter</kbd> opens a menu of every command, <kbd>I</kbd> lists your gear with actions.</li>
</ol>

<h2>Tips</h2>
<ul>
<li>Don't let aliens get close: they are "deadly at close range" (ah_readme). Doors buy time, but aliens can ram them (the game's <code>BreakDoorAction</code>: "doorRammed").</li>
<li>Guns make noise, explosions more (ah_readme advice 8, 9). Aliens use ventilation shafts: "An alien crawls out of a ventilation shaft!" (game message).</li>
<li>Stun grenades are safest dropped at your feet when something is too close; never throw a grenade at a wall.</li>
<li>Too much acid on your armour: take it off. Medkits take a long time and lower your maximum HP when hurt.</li>
<li>Don't shoot from too far away; avoid single shots at small or fast aliens. Take ammo from weapons you find.</li>
</ul>

<h2>About this version</h2>
<p>Based on AlienHack 0.9.1 beta, <a href="https://github.com/SockPuppet8/Alienhack/tree/b843d42">SockPuppet8/Alienhack @ b843d42</a>
(with <a href="https://github.com/SockPuppet8/RL-Shared/tree/c42d3889d58672a5198ee2066c5cb1af509889f1">SockPuppet8/RL-Shared @ c42d388</a>).
Browser port: <a href="https://github.com/memmaker/Alienhack">memmaker/Alienhack</a>.</p>

<h2>Playing in the browser</h2>
<ul>
<li><b>Help</b> opens this guide (<kbd>Esc</kbd> closes it; the game gets no keys while it is open).</li>
<li><b>Windows ▾</b> switches between the split view (Map, Status, Inventory, Visible, Messages) and the
original 80×40 screen. <b>Font</b> changes the text font; the Map window has its own font list.</li>
<li><b>Audio ▾ → Sound effects</b> turns on sounds for shots, hits, doors, aliens, explosions, stairs and more.
Sound is off by default. The game has no music.</li>
<li>Number-pad keys work with Num Lock on or off. Keys are shown for the default key set (<code>keys.txt</code>).</li>
</ul>

<details><summary>The original readme (ah_readme.txt): controls, combat manual, advice</summary>
<pre style="white-space:pre-wrap">{e(readme)}</pre></details>
'''
os.makedirs(os.path.dirname(out), exist_ok=True)
open(out, 'w').write(page)
print('help:', len(rows), 'key rows ->', out)
