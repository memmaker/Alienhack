#!/usr/bin/env python3
"""Synthesize AlienHack's sound effects (no samples: made here, public domain).
Writes <out>/<name>.wav, 22.05 kHz mono 16-bit. Names match RVIP_SOUND() calls."""
import math, random, struct, sys, wave, os
R = 22050
out = sys.argv[1] if len(sys.argv) > 1 else 'sound'
os.makedirs(out, exist_ok=True)
rnd = random.Random(7)

def env(i, n, a=0.005, rel=None):
    t = i / R; d = n / R
    e = min(1, t / a) if a else 1
    return e * (1 - i / n) ** (rel or 1.5)

def tone(d, f0, f1=None, wave_='sin', noise=0.0, rel=1.5, vol=0.8, vib=0):
    n = int(d * R); f1 = f1 or f0; ph = 0; s = []
    for i in range(n):
        f = f0 + (f1 - f0) * i / n + vib * math.sin(i / R * 40)
        ph += 2 * math.pi * f / R
        v = math.sin(ph) if wave_ == 'sin' else (1 if math.sin(ph) > 0 else -1) if wave_ == 'sq' else 2 * (ph / (2 * math.pi) % 1) - 1
        v = v * (1 - noise) + (rnd.random() * 2 - 1) * noise
        s.append(v * env(i, n, rel=rel) * vol)
    return s

def noise(d, lp=0.3, rel=2, vol=0.8, lp1=None):
    n = int(d * R); y = 0; s = []
    for i in range(n):
        k = lp + ((lp1 if lp1 is not None else lp) - lp) * i / n
        y += k * ((rnd.random() * 2 - 1) - y)
        s.append(y * env(i, n, rel=rel) * vol / max(k, 0.15) ** 0.5)
    return s

def cat(*p): return [x for q in p for x in q]
def mix(a, b):
    n = max(len(a), len(b)); return [(a[i] if i < len(a) else 0) + (b[i] if i < len(b) else 0) for i in range(n)]
def gap(d): return [0.0] * int(d * R)

S = {
    'fire': mix(noise(0.18, 0.6, 3, 0.9), tone(0.12, 180, 60, 'sq', 0.3, 3, 0.5)),
    'click': tone(0.03, 1800, 900, 'sq', 0.5, 4, 0.4),
    'pickup': cat(tone(0.06, 660, None, 'sq', 0, 1, 0.25), tone(0.08, 990, None, 'sq', 0, 1.5, 0.25)),
    'reload': cat(noise(0.05, 0.8, 3, 0.6), gap(0.06), tone(0.04, 1200, 700, 'sq', 0.4, 3, 0.5)),
    'wear': noise(0.25, 0.08, 1, 0.6),
    'wield': cat(tone(0.03, 1400, 900, 'sq', 0.4, 3, 0.4), gap(0.04), tone(0.03, 1000, 700, 'sq', 0.4, 3, 0.4)),
    'drop': tone(0.12, 220, 110, 'sin', 0.3, 3, 0.6),
    'spray': noise(0.5, 0.9, 1, 0.4),
    'heal': cat(tone(0.1, 523), tone(0.1, 659), tone(0.18, 784)),
    'dodge': noise(0.15, 0.1, 1, 0.5, 0.6),
    'hit': mix(noise(0.1, 0.4, 3, 0.8), tone(0.1, 150, 80, 'sin', 0, 3, 0.6)),
    'hurt': mix(tone(0.2, 300, 120, 'saw', 0.2, 2, 0.6), noise(0.1, 0.3, 3, 0.5)),
    'armour': tone(0.2, 900, 850, 'sin', 0.1, 3, 0.5),
    'death': tone(1.2, 400, 60, 'saw', 0.1, 1.2, 0.6, vib=8),
    'hugger': cat(noise(0.1, 0.5, 1, 0.7), tone(0.4, 900, 300, 'saw', 0.3, 1.5, 0.6, vib=30)),
    'alert': cat(tone(0.08, 1400, None, 'sin', 0, 1, 0.5), gap(0.05), tone(0.08, 1400, None, 'sin', 0, 1, 0.5)),
    'beep': cat(tone(0.06, 1000, None, 'sq', 0, 1, 0.3), tone(0.06, 1500, None, 'sq', 0, 1, 0.3), tone(0.1, 2000, None, 'sq', 0, 2, 0.3)),
    'arm': cat(tone(0.05, 2000, None, 'sq', 0, 1, 0.3), gap(0.1), tone(0.05, 2000, None, 'sq', 0, 1, 0.3), gap(0.1), tone(0.05, 2400, None, 'sq', 0, 1, 0.3)),
    'explode': mix(noise(1.2, 0.2, 2.5, 1.0, 0.03), tone(0.6, 80, 30, 'sin', 0, 2, 0.8)),
    'door': mix(noise(0.35, 0.05, 1, 0.5), tone(0.35, 120, 160, 'saw', 0.3, 1, 0.3)),
    'bang': mix(noise(0.25, 0.3, 3, 0.9), tone(0.25, 90, 50, 'sq', 0.2, 3, 0.5)),
    'hiss': noise(0.4, 0.7, 1.2, 0.3),
    'kill': cat(tone(0.25, 700, 200, 'saw', 0.3, 2, 0.6, vib=20), noise(0.15, 0.5, 2, 0.5)),
    'screech': tone(0.9, 1200, 700, 'saw', 0.35, 1.2, 0.6, vib=60),
    'spit': mix(noise(0.2, 0.6, 2, 0.6, 0.2), tone(0.1, 500, 200, 'sin', 0.3, 2, 0.4)),
    'break': mix(noise(0.3, 0.9, 2.5, 0.8), tone(0.3, 1500, 400, 'sq', 0.5, 2, 0.3)),
    'acid': noise(0.6, 0.9, 1, 0.35, 0.5),
    'stairs': cat(*[mix(noise(0.07, 0.2, 3, 0.6), gap(0.07)) + gap(0.06) for _ in range(4)]),
}
for k, s in S.items():
    m = max(1e-9, max(abs(x) for x in s)); g = 0.7 / m if m > 0.7 else 1
    with wave.open(os.path.join(out, k + '.wav'), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(R)
        w.writeframes(b''.join(struct.pack('<h', int(max(-1, min(1, x * g)) * 32767)) for x in s))
print(len(S), 'sounds ->', out)
