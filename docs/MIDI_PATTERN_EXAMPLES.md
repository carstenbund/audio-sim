# MIDI Pattern Examples (Visual Notation)

ASCII representations of MIDI patterns for the networked resonator. Each pattern shows timing, notes, and expected sonic result.

---

## Pattern 1: "Cascading Arpeggios"

**Style:** Shimmer Bells preset
**Channel:** 1 (triggers)
**Tempo:** 120 BPM

```
Time:  |1...2...3...4...|1...2...3...4...|

Track: Arpeggio (Ch1)
Notes: C4  E4  G4  B4  C5  B4  G4  E4
Nodes: 4   0   3   7   4   7   3   0
Vel:   80  85  90  95  100 95  90  85

Grid:  |X..X..X..X.....|X..X..X..X.....|
       ▔▔  ▔▔  ▔▔  ▔▔   ▔▔  ▔▔  ▔▔  ▔▔
       16th note spacing, ascending then descending

Result: Bell-like cascade, wave travels around ring
```

**DAW Setup:**
- Single MIDI clip, 2 bars
- Loop for continuous shimmer
- Vary velocity slightly for humanization
- Add 2-3% random timing (if DAW supports)

---

## Pattern 2: "Polyrhythmic Pulse"

**Style:** Wave Machine preset
**Channel:** 1 (triggers)
**Tempo:** 120 BPM

```
Time:  |1...2...3...4...|1...2...3...4...|

Track 1: Node 0 (C0), Euclidean(5,16)
Grid:  |X..X...X..X...X|........repeat..|
       ◼  ◼   ◼  ◼   ◼

Track 2: Node 2 (D0), Euclidean(7,16)
Grid:  |X.X..X.X..X.X..X|.......repeat..|
       ◼ ◼  ◼ ◼  ◼ ◼  ◼

Track 3: Node 4 (E0), Straight 8ths
Grid:  |X.X.X.X.X.X.X.X.|X.X.X.X.X.X.X.X.|
       ◼ ◼ ◼ ◼ ◼ ◼ ◼ ◼  ◼ ◼ ◼ ◼ ◼ ◼ ◼ ◼

Track 4: Node 6 (F#0), Euclidean(3,8)
Grid:  |X..X..X..........|repeat..........|
       ◼  ◼  ◼

All velocities: 70 (consistent)

Result: Complex interference patterns, evolving phase
```

**Notes:**
- Each track is same pitch (e.g., C3) but different node
- Polyrhythm creates shifting patterns
- Let run for 2-4 minutes to hear full evolution
- Patterns align every LCM(5,7,8,3) = 840 16th notes

---

## Pattern 3: "Drone Build"

**Style:** Bowed Strings preset
**Channels:** 2 (sustained drive) + 1 (articulation)
**Tempo:** 60 BPM

```
Time:  |0:00      |0:30      |1:00      |1:30      |2:00      |

Track 1 (Ch2): C2 (root)
Notes: [████████████████████████████████████████████████████]
Vel:   20─────────────40────────────60──────────70────────────0
       ▁▁▁▁▁▁▁▁▂▂▂▂▂▂▃▃▃▃▃▃▃▃▄▄▄▄▄▄▄▄▄▄▄▄▄▄▃▃▃▃▂▂▂▂▁▁▁▁
       (Gradual swell and fade)

Track 2 (Ch2): G2 (fifth)
Notes:          [███████████████████████████████████████]
Vel:            30───────────50──────────65──────────────0
                (Enters at 0:30, builds with Track 1)

Track 3 (Ch2): C3 (octave)
Notes:                      [████████████████████████]
Vel:                        40───────────70──────────0
                            (Enters at 1:00, completes harmony)

Track 4 (Ch1): Sparse triggers (C4, E4, G4)
Notes:     X     X      X       X       X     X
Vel:       50    60     70      80      70    60
           (Random "bowing" articulations)

Result: Evolving harmonic drone with textural bowing
```

**Automation:**
- Track 1-3: Velocity automation curves
- Optional: Coupling strength 0.3 → 0.6 over full duration

---

## Pattern 4: "Wave Propagation"

**Style:** Shimmer Bells preset
**Channel:** 1 (triggers)
**Tempo:** 100 BPM
**Topology:** Ring (8 nodes)

```
Time:  |1...2...3...4...|1...2...3...4...|

Sequential triggering (traveling wave):

Step 1:  |X...............|................|  Node 0 (C0)
Step 2:  |..X.............|................|  Node 1 (C#0)
Step 3:  |....X...........|................|  Node 2 (D0)
Step 4:  |......X.........|................|  Node 3 (D#0)
Step 5:  |........X.......|................|  Node 4 (E0)
Step 6:  |..........X.....|................|  Node 5 (F0)
Step 7:  |............X...|................|  Node 6 (F#0)
Step 8:  |..............X.|................|  Node 7 (G0)

Combined:
        |X.X.X.X.X.X.X.X.|................|
         ▔ ▔ ▔ ▔ ▔ ▔ ▔ ▔
         (Wave travels clockwise around ring)

All velocities: 80
Spacing: 8th notes (creates smooth wave)

Result: Visible/audible wave propagation, circular motion
```

**Variations:**
- **Faster:** 16th notes (denser wave)
- **Slower:** Quarter notes (sparse wave)
- **Reverse:** G0 → F#0 → ... → C0 (counter-clockwise)
- **Bidirectional:** Two waves from opposite sides

---

## Pattern 5: "Standing Wave"

**Style:** Wave Machine preset
**Channel:** 1 (triggers)
**Tempo:** 120 BPM
**Topology:** Ring (8 nodes)

```
Time:  |1...2...3...4...|1...2...3...4...|

Node 0 (opposite side):
Grid:  |X.X.X.X.X.X.X.X.|X.X.X.X.X.X.X.X.|
       ◼ ◼ ◼ ◼ ◼ ◼ ◼ ◼  ◼ ◼ ◼ ◼ ◼ ◼ ◼ ◼
       (8th notes, constant)

Node 4 (opposite side):
Grid:  |.X.X.X.X.X.X.X.X|.X.X.X.X.X.X.X.X|
       ◼ ◼ ◼ ◼ ◼ ◼ ◼ ◼  ◼ ◼ ◼ ◼ ◼ ◼ ◼ ◼
       (8th notes, offset by 1/16 note)

Both: Same note (C3), velocity 70

Result: Interference pattern, alternating buildup/cancellation
        at nodes 2 and 6 (midpoints)
```

**Physics:**
- Two waves traveling in opposite directions
- Constructive interference at some nodes
- Destructive interference at others
- Stationary pattern (doesn't travel)

---

## Pattern 6: "Rhythmic Texture"

**Style:** Metallic Percussion preset
**Channel:** 1 (triggers)
**Tempo:** 128 BPM

```
Time:  |1.&.2.&.3.&.4.&.|1.&.2.&.3.&.4.&.|

Kick (Node 0, C0):
Grid:  |X.......X.......|X.......X.X.....|
Vel:   100     100      100     100 80

Snare (Node 2, D0):
Grid:  |....X.......X...|....X.......X...|
Vel:        90       90      90       90

Hi-hat (Node 5, F0):
Grid:  |X.X.X.X.X.X.X.X.|X.X.X.X.X.X.X.X.|
Vel:   80 60 80 60 80 60 80 60 (alternating)

Tom 1 (Node 4, E0):
Grid:  |..............X.|X...............|
Vel:               110  90

Tom 2 (Node 6, F#0):
Grid:  |............X...|................|
Vel:             100

Result: Drum-like pattern with metallic timbres
```

**Pattern breakdown:**
- Kick: 4-on-floor with syncopation
- Snare: Backbeat (2 and 4)
- Hi-hat: Constant 8ths, accent on beats
- Toms: Fill on beat 4

---

## Pattern 7: "Harmonic Progression"

**Style:** Harmonic Lattice preset
**Channels:** 1 (attack) + 2 (sustain)
**Tempo:** 90 BPM

```
Chord progression: Cmaj → Fmaj → G7 → Cmaj

Bar 1: C major (C-E-G)
Track 1 (Ch1): |X...............|  C4 (vel 90)
Track 2 (Ch1): |.X..............|  E4 (vel 85)
Track 3 (Ch1): |..X.............|  G4 (vel 80)
Track 4 (Ch2): |████████████████|  C3-E3-G3 (vel 50, sustained)

Bar 2: F major (F-A-C)
Track 1 (Ch1): |X...............|  F4 (vel 90)
Track 2 (Ch1): |.X..............|  A4 (vel 85)
Track 3 (Ch1): |..X.............|  C5 (vel 80)
Track 4 (Ch2): |████████████████|  F3-A3-C4 (vel 50, sustained)

Bar 3: G7 (G-B-D-F)
Track 1 (Ch1): |X...............|  G4 (vel 90)
Track 2 (Ch1): |.X..............|  B4 (vel 85)
Track 3 (Ch1): |..X.............|  D5 (vel 80)
Track 4 (Ch1): |...X............|  F5 (vel 75)
Track 5 (Ch2): |████████████████|  G3-B3-D4 (vel 50, sustained)

Bar 4: C major (return)
(Same as Bar 1)

Result: Traditional harmony with networked resonator timbre
```

**Note:**
- Ch1 provides chord attack/articulation
- Ch2 provides sustained harmonic foundation
- Stagger Ch1 notes slightly for arpeggio effect
- Network coupling blends notes into chord

---

## Pattern 8: "Chaos Injection"

**Style:** Resonant Chaos preset
**Channel:** 1 (triggers)
**Tempo:** Free time

```
Event-based (no grid):

Phase 1 (0:00-0:10): BURST
Random notes: C0, D#0, F#0, A0, C1, E1, G#1
Random timing: 0.0s, 0.15s, 0.23s, 0.41s, 0.52s, ...
Random velocity: 60, 85, 72, 91, 68, ...
Density: ~20 notes in 10 seconds

        ◼ ◼  ◼   ◼ ◼◼  ◼    ◼  ◼ ◼    ◼◼  ◼   ◼
        |--------10 seconds-----------|

Phase 2 (0:10-0:40): SILENCE
Network settles, energy decays and redistributes

        |--------30 seconds-----------|
        (Network "thinks")

Phase 3 (0:40-0:50): BURST
(Repeat Phase 1 pattern)

Result: Bursts of chaos, followed by emergent patterns
        as network self-organizes
```

**Implementation:**
- Use probability-based sequencer (Max/MSP, PureData)
- Or: Record random MIDI in DAW, edit for density
- Or: Use generative MIDI plugin (e.g., Riffer, MOTU Randomizer)

---

## Pattern 9: "Slow Morphing"

**Style:** Bowed Strings preset
**Channel:** 2 (sustained drive)
**Tempo:** N/A (very slow)

```
Timeline (4 minutes):

0:00-1:00: Single note (C2)
           Velocity: 20 → 60 (gradual increase)
           |▁▁▁▁▂▂▂▂▃▃▃▃▄▄▄▄|

1:00-2:00: Add fifth (G2)
           C2: vel 60 (stable)
           G2: vel 0 → 50 (fade in)
           |▄▄▄▄▄▄▄▄| C2
           |▁▁▂▂▃▃▄▄| G2

2:00-3:00: Add octave (C3)
           C2: vel 60 (stable)
           G2: vel 50 (stable)
           C3: vel 0 → 70 (fade in)
           |▄▄▄▄▄▄▄▄| C2
           |▄▄▄▄▄▄▄▄| G2
           |▁▂▃▄▄▄▅▅| C3

3:00-4:00: Fade all
           All: vel → 0 (gradual decrease)
           |▄▄▄▃▃▃▂▂▁▁| C2
           |▄▄▃▃▃▂▂▁▁| G2
           |▅▄▄▃▃▂▂▁▁| C3

Result: Slow harmonic build and release, meditative
```

**Automation curves:**
- Use smooth automation (no steps)
- Consider coupling strength automation too
- Total duration: 4-8 minutes typical

---

## Pattern 10: "Grid Excitation"

**Style:** Harmonic Lattice preset (3×3 grid)
**Channel:** 1 (triggers)
**Tempo:** 110 BPM

```
Lattice topology:
  0 — 1 — 2
  |   |   |
  3 — 4 — 5
  |   |   |
  6 — 7 — 8

Excitation pattern (4 bars):

Bar 1: Corners
|X...............|................|  Node 0 (C0)
|....X...........|................|  Node 2 (D0)
|........X.......|................|  Node 6 (F#0)
|............X...|................|  Node 8 (G#0)
Result: Diagonal waves toward center

Bar 2: Edges
|X...............|................|  Node 1 (C#0)
|....X...........|................|  Node 3 (D#0)
|........X.......|................|  Node 5 (F0)
|............X...|................|  Node 7 (G0)
Result: Orthogonal waves toward center

Bar 3: Center
|X...............|................|  Node 4 (E0)
|....X...........|................|  Node 4 (E0)
|........X.......|................|  Node 4 (E0)
|............X...|................|  Node 4 (E0)
Result: Energy radiates outward to periphery

Bar 4: All
|X.X.X.X.X.X.X.X.|X.X.X.X.X.X.X.X.|  All nodes
Result: Complex 2D interference, full lattice activation

Velocity: 75 throughout
```

---

## Tips for Creating Your Own Patterns

### 1. Start Simple
- One node, one rhythm
- Observe how network responds
- Add complexity gradually

### 2. Use Repetition
- Short loops (1-4 bars)
- Let network find patterns
- Variation emerges naturally

### 3. Contrast Density
- Sparse vs dense sections
- Silence is powerful
- Network needs time to settle

### 4. Explore Velocities
- Low vel (20-50): Subtle, delicate
- Mid vel (60-80): Balanced
- High vel (90-127): Aggressive, saturated

### 5. Layer Channels
- Ch1 for articulation
- Ch2 for foundation
- Together: Complex textures

### 6. Think Spatially
- Note → Node mapping
- Topology matters
- Wave propagation patterns

### 7. Time Scales
- Fast (16ths): Shimmer, texture
- Medium (8ths, quarters): Rhythm, pulse
- Slow (whole notes+): Drones, evolution

---

## Pattern Templates (for DAW)

Save these as MIDI clip templates in your DAW:

1. **Euclidean Pulse**: E(5,8), E(7,12), E(3,8)
2. **Sequential Wave**: C0→C#0→D0→... (8th notes)
3. **Drone Triad**: C2+G2+C3 (whole notes, Ch2)
4. **Arpeggio Cascade**: Ascending/descending scale
5. **Random Scatter**: Probability-based triggers
6. **Drum Pattern**: Kick/snare/hat equivalent
7. **Chord Progression**: I-IV-V-I with Ch1+Ch2
8. **Interference**: Two nodes, offset phases

Load template → Adjust tempo → Experiment!

---

## Notation Key

```
X = Note trigger
◼ = Emphasized note
█ = Sustained note (Ch2)
▁▂▃▄▅ = Velocity/automation levels
|...| = Time divisions (beats)
→ = Automation direction
~ = Randomization
```

---

Happy pattern-making! 🎵🎹
