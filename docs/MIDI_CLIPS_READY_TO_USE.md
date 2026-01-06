# Ready-to-Use MIDI Clips for Resonant Excitation

**Quick Start:** Copy these exact MIDI patterns into your DAW. Each clip includes precise timing, velocities, and expected sonic results.

All clips assume **8-node ring topology** unless noted.

---

## Clip 1: "Gentle Resonance Builder"
**Goal:** Slow, beautiful buildup. Perfect for ambient textures.

### Configuration Needed
```
Damping (gamma): 0.3 (all modes)
Coupling: 0.25
Carrier freq: 440 Hz
```

### MIDI Clip
```
Track: "Gentle Builder"
Channel: 1 (triggers)
Length: 8 bars
Tempo: 100 BPM
Time Signature: 4/4

Bar 1-8:
Pattern: Straight 8th notes
Note: C4 (MIDI 60) → Node 4
Velocity: 60 (constant)

Actual clip (each X = one 8th note):
|1.&.2.&.3.&.4.&.|1.&.2.&.3.&.4.&.|1.&.2.&.3.&.4.&.|1.&.2.&.3.&.4.&.|
|C4  C4  C4  C4  |C4  C4  C4  C4  |C4  C4  C4  C4  |C4  C4  C4  C4  |
|60  60  60  60  |60  60  60  60  |60  60  60  60  |60  60  60  60  |

Bars 5-8: Same pattern continues
```

### Expected Result
- **0-2s:** Barely audible, mode 0 starting to ring
- **2-4s:** Clear 440 Hz tone emerging, amplitude ~30%
- **4-6s:** Rich harmonics appear, amplitude ~60%
- **6-8s:** Full, shimmering tone, amplitude ~80%
- **After stop:** ~3 second decay

### DAW Notes
- **Ableton:** Create MIDI clip, draw notes on C4, set all velocities to 60
- **Logic:** Use Step Sequencer, 8th note grid, C4, velocity 60
- **FL Studio:** Piano roll, paint C4 notes every 8th, set velocity

---

## Clip 2: "Polyrhythmic Wave"
**Goal:** Complex interference patterns, traveling waves

### Configuration Needed
```
Damping (gamma): 0.5
Coupling: 0.5
Carrier freq: 220 Hz
Topology: Ring (8 nodes)
```

### MIDI Clips (3 tracks)

#### Track 1: Node 0
```
Channel: 1
Length: 4 bars (loop)
Tempo: 120 BPM

Pattern: Euclidean(5,16)
Note: C0 (MIDI 12) → Node 0
Velocity: 75

16th note grid:
|X..X...X..X...X..|................|................|................|
Bar 1 only, then repeats
```

#### Track 2: Node 4
```
Channel: 1
Length: 4 bars (loop)
Tempo: 120 BPM

Pattern: Euclidean(7,16)
Note: E0 (MIDI 16) → Node 4
Velocity: 70

16th note grid:
|X.X..X.X..X.X..X.|................|................|................|
Bar 1 only, then repeats
```

#### Track 3: Node 2
```
Channel: 1
Length: 4 bars
Tempo: 120 BPM

Pattern: Straight 8th notes
Note: D0 (MIDI 14) → Node 2
Velocity: 65

8th note grid:
|X.X.X.X.X.X.X.X.|X.X.X.X.X.X.X.X.|X.X.X.X.X.X.X.X.|X.X.X.X.X.X.X.X.|
All 4 bars
```

### Expected Result
- **Wave propagation** visible around ring
- **Interference patterns** at nodes 1, 3, 5, 6, 7
- **Beating** every ~3.5 seconds (LCM of rhythms)
- **Dense texture** but clear pitch (220 Hz fundamental)

### Euclidean Pattern Generation
**If your DAW doesn't have Euclidean sequencer:**

E(5,16) as 16th notes:
```
Position: 1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
Note:     X  .  .  X  .  .  .  X  .  .  X  .  .  .  X  .
```

E(7,16) as 16th notes:
```
Position: 1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
Note:     X  .  X  .  .  X  .  X  .  .  X  .  X  .  .  X
```

---

## Clip 3: "Waterfall Swell"
**Goal:** Continuous drive with gradual intensity increase

### Configuration Needed
```
Damping (gamma): 0.25 (very low, high Q)
Coupling: 0.35
Carrier freq: 110 Hz
```

### MIDI Clip
```
Track: "Waterfall"
Channel: 2 (sustained drive)
Length: 16 bars (64 seconds @ 60 BPM)
Tempo: 60 BPM

Note: C2 (MIDI 36) → Node 4
Duration: 16 whole notes (held continuously)

Velocity automation (linear ramp):
Bar 1-4:   20 → 35
Bar 5-8:   35 → 50
Bar 9-12:  50 → 65
Bar 13-16: 65 → 80

Timeline:
0:00-0:16  vel=20  ▁▁
0:16-0:32  vel=28  ▂▂
0:32-0:48  vel=35  ▃▃
0:48-1:04  vel=43  ▃▃
1:04 ...   (continue to vel=80)
```

### Expected Result
- **0-15s:** Barely audible, mode 0 slowly accumulating
- **15-30s:** Clear low tone (110 Hz), amplitude building
- **30-45s:** Harmonics emerging, rich texture
- **45-60s:** Full, warm tone, approaching saturation
- **After release:** 6-8 second decay!

### DAW Implementation
1. Create 16-bar MIDI clip
2. Draw one long note (C2, 16 bars)
3. Create velocity automation lane
4. Draw linear ramp: bar 1 = 20, bar 16 = 80

---

## Clip 4: "Rhythmic Cascade"
**Goal:** Melodic arpeggio with wave propagation

### Configuration Needed
```
Damping (gamma): 0.4
Coupling: 0.3
Carrier freq: 440 Hz
```

### MIDI Clip
```
Track: "Cascade"
Channel: 1
Length: 2 bars (loop)
Tempo: 110 BPM

Pattern: 16th note arpeggio (ascending then descending)
Notes → Nodes:
C4 (60)  → Node 4
E4 (64)  → Node 0
G4 (67)  → Node 3
B4 (71)  → Node 7
C5 (72)  → Node 0
B4 (71)  → Node 7
G4 (67)  → Node 3
E4 (64)  → Node 0

Velocity pattern (accent on first and top):
90 80 80 85 95 85 80 80

16th note timing:
Bar 1:
|C4E4G4B4C5B4G4E4|................|
|90 80 80 85 95 85 80 80 (only first 8 16ths)

Bar 2:
|C4E4G4B4C5B4G4E4|................|
(repeat)
```

### Expected Result
- **Wave travels** around ring as notes progress
- **Shimmer** from different nodes being excited
- **Harmonic richness** from multiple frequencies
- **"Liquid" quality** from coupling

---

## Clip 5: "Standing Wave"
**Goal:** Interference pattern, alternating buildup/cancellation

### Configuration Needed
```
Damping (gamma): 0.6
Coupling: 0.6
Carrier freq: 220 Hz
Topology: Ring (8 nodes)
```

### MIDI Clips (2 tracks, opposite nodes)

#### Track 1: Node 0
```
Channel: 1
Tempo: 120 BPM
Length: 8 bars

Pattern: Straight 8th notes
Note: C0 (MIDI 12) → Node 0
Velocity: 75

All 8 bars:
|X.X.X.X.X.X.X.X.|X.X.X.X.X.X.X.X.|... (continue)
```

#### Track 2: Node 4 (opposite side of ring)
```
Channel: 1
Tempo: 120 BPM
Length: 8 bars

Pattern: Straight 8th notes, OFFSET by 1/16 note
Note: E0 (MIDI 16) → Node 4
Velocity: 75

All 8 bars (note the offset):
|.X.X.X.X.X.X.X.X|X.X.X.X.X.X.X.X.| ... (continue)
   ↑ Starts on "and" of 1
```

### Expected Result
- **Standing wave** forms between Node 0 and Node 4
- **Nodes 2 and 6** (midpoints) have maximum amplitude variation
- **Alternating buildup** at these midpoints
- **Stationary pattern** (doesn't travel)

### Variation: Perfect Opposition
Change Track 2 offset to exactly 1/8 note (half period):
- Creates pure destructive/constructive interference
- Even more dramatic amplitude swings

---

## Clip 6: "Drone Foundation + Melody"
**Goal:** Sustained base + melodic triggers

### Configuration Needed
```
Damping (gamma): 0.35
Coupling: 0.4
Carrier freq: 220 Hz
```

### MIDI Clips (2 tracks)

#### Track 1: Drone (Ch2)
```
Channel: 2 (sustained drive)
Tempo: Any
Length: 32 bars

Notes (chord voicing):
C2 (36)  → Node 4
G2 (43)  → Node 3
C3 (48)  → Node 0

All held for full 32 bars
Velocity: 40, 35, 30 (descending, bass louder)

Timeline:
|████████████████████████████████████████████████████|
 C2                                (32 bars)
 ████████████████████████████████████████████████████
 G2                                (32 bars)
 ████████████████████████████████████████████████████
 C3                                (32 bars)
```

#### Track 2: Melody (Ch1)
```
Channel: 1 (triggers)
Tempo: 100 BPM
Length: 4 bars (loop)

Pattern: Sparse melodic phrase
Bar 1: C4 (60) on beat 1, E4 (64) on beat 3
Bar 2: G4 (67) on beat 2, Rest
Bar 3: E4 (64) on beat 1, D4 (62) on beat 3
Bar 4: C4 (60) on beat 1, Rest

Velocity: 70 (constant)

Piano roll:
Bar 1: |C4.....E4......|
Bar 2: |...G4..........|
Bar 3: |E4.....D4......|
Bar 4: |C4.............|
```

### Expected Result
- **Foundation:** Rich, warm drone (C power chord)
- **Melody:** Bell-like tones over drone
- **Interaction:** Melodic notes excite drone frequencies
- **Texture:** Full, lush, ambient

---

## Clip 7: "Chaos Burst"
**Goal:** Random excitation, emergent patterns

### Configuration Needed
```
Damping (gamma): 0.4
Coupling: 0.7
Carrier freq: 330 Hz
```

### MIDI Clip
```
Track: "Chaos"
Channel: 1
Tempo: Free time (or 120 BPM with extreme randomization)
Length: 10 seconds

Pattern: Random notes, random timing, random velocities
Node targets: All nodes (0-7)
Notes: C0-G0 (12-19)
Density: ~3-5 notes per second

Example burst (one 10-second phrase):
Time:   0.0s  0.3s  0.7s  1.1s  1.3s  1.8s  2.2s  2.6s  3.1s  3.4s ...
Note:   C0    F0    D0    G0    C0    E0    C0    F#0   D0    A0  ...
Node:   0     5     2     7     0     4     0     6     2     9%8 ...
Vel:    72    85    60    90    55    78    95    65    80    70  ...

Then 20-30 seconds of silence to let network settle.

Repeat.
```

### Expected Result
- **Initial:** Chaotic, unpredictable
- **Evolution:** Network finds patterns, modes ring
- **Settling:** After burst, emergent oscillations
- **Surprise:** Unexpected harmonic relationships

### DAW Implementation
1. Use random MIDI generator plugin (many DAWs have this)
2. Or: Draw notes randomly in piano roll
3. Set note range: C0-G0
4. Set velocity range: 55-95
5. Create bursts of 15-20 notes
6. Leave gaps between bursts

---

## Clip 8: "Tempo Acceleration"
**Goal:** Increasing excitation rate, building intensity

### Configuration Needed
```
Damping (gamma): 0.5
Coupling: 0.45
Carrier freq: 440 Hz
```

### MIDI Clip
```
Track: "Accelerando"
Channel: 1
Variable tempo (automation required)
Length: 16 bars @ starting tempo

Note: C4 (60) → Node 4 (constant)
Velocity: 70 (constant)
Pattern: Straight quarter notes (but tempo increases!)

Tempo automation:
Bar 1-4:   60 BPM  (quarter note = 1000ms)
Bar 5-8:   80 BPM  (quarter note = 750ms)
Bar 9-12:  100 BPM (quarter note = 600ms)
Bar 13-16: 120 BPM (quarter note = 500ms)

Result: Excitation rate accelerates from 1 Hz → 2 Hz
```

### Expected Result
- **Slow start:** Gentle buildup (1 Hz excitation)
- **Gradual acceleration:** More energy injection
- **Faster tempo:** Denser excitation, more buildup
- **Climax:** Full resonance at 120 BPM

### DAW Implementation
1. Create MIDI clip with quarter notes
2. Enable tempo automation
3. Draw tempo curve: 60 → 120 BPM over 16 bars
4. Or: Use tempo changes every 4 bars (stepped)

---

## Clip 9: "Grid Excitation" (Lattice Topology)
**Goal:** 2D wave propagation (requires lattice topology)

### Configuration Needed
```
Topology: Lattice 3x3 (9 nodes)
Damping (gamma): 0.5
Coupling: 0.5
Carrier freq: 220 Hz

Layout:
  0 — 1 — 2
  |   |   |
  3 — 4 — 5
  |   |   |
  6 — 7 — 8
```

### MIDI Clip
```
Track: "Grid"
Channel: 1
Tempo: 110 BPM
Length: 8 bars

Phase 1 (Bars 1-2): Corner excitation
Bar 1-2: C0 (node 0), D0 (node 2), F#0 (node 6), G#0 (node 8)
Pattern: Quarter notes
Velocity: 80

Phase 2 (Bars 3-4): Edge excitation
Bar 3-4: C#0 (node 1), D#0 (node 3), F0 (node 5), G0 (node 7)
Pattern: Quarter notes
Velocity: 75

Phase 3 (Bars 5-6): Center excitation
Bar 5-6: E0 (node 4) only
Pattern: 8th notes
Velocity: 85

Phase 4 (Bars 7-8): All nodes
Bar 7-8: All 9 notes (C0-G#0)
Pattern: 16th notes (dense)
Velocity: 70
```

### Expected Result
- **Phase 1:** Diagonal waves toward center
- **Phase 2:** Orthogonal waves from edges
- **Phase 3:** Radial waves from center
- **Phase 4:** Complex 2D interference, full lattice activation

---

## Clip 10: "Velocity Ramp Test"
**Goal:** Find saturation threshold

### Configuration Needed
```
Damping (gamma): 0.4
Coupling: 0.3
Carrier freq: 440 Hz
```

### MIDI Clip
```
Track: "Velocity Test"
Channel: 1
Tempo: 120 BPM
Length: 16 bars

Pattern: Straight 8th notes
Note: C4 (60) → Node 4
Velocity: Automated ramp

Velocity automation (4-bar sections):
Bar 1-4:   vel = 20  (very quiet)
Bar 5-8:   vel = 40  (quiet)
Bar 9-12:  vel = 70  (medium)
Bar 13-16: vel = 100 (loud)

Listen for saturation/distortion threshold
```

### Expected Result
- **Bars 1-4:** Barely audible, gentle buildup
- **Bars 5-8:** Clear tone, moderate buildup
- **Bars 9-12:** Strong resonance, full sound
- **Bars 13-16:** Possible saturation/distortion

**Use this to find optimal velocity range for your config!**

---

## Template: General Purpose Resonator Trigger

**Copy this into your DAW as a starting point:**

```
Track Name: "Resonator Trigger"
Channel: 1 (triggers)
Tempo: 120 BPM
Length: 4 bars (loop)

Pattern: Straight 8th notes
Note: C4 (MIDI 60)
Velocity: 70

|C4 C4 C4 C4 C4 C4 C4 C4|C4 C4 C4 C4 C4 C4 C4 C4|...
|70 70 70 70 70 70 70 70|70 70 70 70 70 70 70 70|...

Adjust:
- Note: Choose node target (C0-G0 for 8 nodes)
- Velocity: 50-90 range
- Tempo: 90-140 BPM typical
- Pattern: Quarter/8th/16th notes
```

---

## Tips for Using These Clips

### 1. Start Simple
- Use Clip 1 first
- Get familiar with buildup behavior
- Adjust gamma if needed

### 2. Layer Gradually
- Start with one clip
- Add second clip after 8-16 bars
- Observe interaction

### 3. Record Long Takes
- Let clips run for minutes
- Network evolves slowly
- Interesting patterns emerge over time

### 4. Automate
- Use velocity automation
- Use tempo automation
- Use pattern muting (on/off)

### 5. Listen for Thresholds
- Find the velocity that causes saturation
- Find the tempo that creates best buildup
- Find the coupling that creates waves

---

## Quick Copy-Paste Values

### Node Mapping (8 nodes)
```
C0  (12) → Node 0
C#0 (13) → Node 1
D0  (14) → Node 2
D#0 (15) → Node 3
E0  (16) → Node 4
F0  (17) → Node 5
F#0 (18) → Node 6
G0  (19) → Node 7
```

### Velocity Ranges
```
ppp: 15-25
pp:  26-40
p:   41-55
mp:  56-70
mf:  71-85
f:   86-100
ff:  101-115
fff: 116-127
```

### Tempo Ranges
```
Slow buildup:   60-80 BPM
Medium buildup: 90-110 BPM
Fast buildup:   120-140 BPM
Very fast:      150+ BPM
```

---

**Load these clips, hit play, and watch the network come alive!** 🌊🎵
