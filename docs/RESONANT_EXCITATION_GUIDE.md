# Resonant Excitation Guide: Kicking the Network Into Motion

**Metaphor:** Multiple ponds with waterfalls. Each waterfall creates periodic waves. When timed correctly, waves reinforce and build into large resonances.

**Goal:** Find the timing, velocity, and envelope parameters that create maximum resonant buildup across the networked oscillators.

---

## 1. The Physics of Resonance

### Modal Dynamics

Each node has 4 complex modes evolving as:

```
ȧ_k = (-γ_k + iω_k)a_k + u_k(t)

where:
  a_k      = complex amplitude of mode k
  ω_k      = angular frequency (rad/s)
  γ_k      = damping coefficient (1/s)
  u_k(t)   = excitation (poke) input
```

**Key insight:** When you "poke" periodically at frequency f_poke:
- If f_poke ≈ f_mode = ω_k/(2π), you get **resonance** (amplitude builds)
- If f_poke far from f_mode, energy doesn't accumulate efficiently
- Damping γ_k determines how "sharp" the resonance is

### Resonance Condition

**Optimal condition:** Excitation period matches mode period (or submultiples)

```
T_excite = n × T_mode    (where n = 1, 2, 3, ...)

Or in terms of frequency:
f_excite = f_mode / n

Examples:
  Mode freq: 100 Hz  →  Excite at 100, 50, 33.3, 25 Hz, ...
  Mode freq: 440 Hz  →  Excite at 440, 220, 146.7, 110 Hz, ...
```

**Why submultiples work:** Every nth poke arrives in phase with the oscillation, reinforcing it.

---

## 2. Current System Parameters

### From Code Analysis

```c
// Excitation envelope (modal_node.h:70)
envelope_duration = 1-20 ms  (configurable)

// MIDI implementation (hub_controller.h:275-276)
Channel 1: 10ms envelope (short trigger)
Channel 2: Pokes sent every 100ms = 10 Hz repetition rate

// Control rate (modal_node.h:31)
Control timestep: 2ms (500 Hz)
```

### Available Control Parameters

#### Per-Mode Parameters
```c
omega       // Angular frequency (rad/s) - sets resonant frequency
gamma       // Damping (1/s) - controls decay rate and Q-factor
weight      // Audio contribution [0,1]
```

#### Per-Poke Parameters (MIDI controllable)
```c
strength         // Excitation amplitude [0,1] ← MIDI velocity
phase_hint       // Phase alignment (radians) ← Future use
mode_weights[4]  // Which modes to excite ← Future use
```

#### Network Parameters
```c
coupling_strength  // How much energy transfers between nodes [0,1]
```

---

## 3. Calculating Resonant Timings

### Example: 440 Hz Resonator (A4)

**Mode 0 Setup:**
```
omega_0 = 2π × 440 = 2764 rad/s
gamma_0 = 0.3 (light damping)
f_mode = 440 Hz
T_mode = 1/440 = 2.27 ms
```

**Resonant Excitation Rates:**

| Harmonic | Excite Freq | Period | MIDI Timing @ 120 BPM | Notes |
|----------|-------------|--------|----------------------|-------|
| **1:1** | 440 Hz | 2.27 ms | N/A (too fast for MIDI) | Audio rate |
| **1:2** | 220 Hz | 4.55 ms | N/A | Still too fast |
| **1:4** | 110 Hz | 9.09 ms | ~64th notes | Borderline |
| **1:8** | 55 Hz | 18.2 ms | ~32nd notes @ 180 BPM | Practical |
| **1:16** | 27.5 Hz | 36.4 ms | 32nd notes @ 120 BPM | Good |
| **1:32** | 13.75 Hz | 72.7 ms | 16th notes @ 120 BPM | **Excellent** |
| **1:64** | 6.88 Hz | 145 ms | 8th notes @ 120 BPM | Very good |
| **1:128** | 3.44 Hz | 291 ms | Quarter notes @ 120 BPM | Slow buildup |

**Sweet spot:** 16th to 8th notes at typical tempos (100-140 BPM)

### Why 16th Notes Work Well

```
16th notes @ 120 BPM:
  Beat period: 60/120 = 0.5 s
  16th period: 0.5/4 = 0.125 s = 125 ms
  Frequency: 8 Hz

For 440 Hz mode:
  Ratio: 440/8 = 55 (1/55 subharmonic)

This is far from resonance, but the poke SPECTRUM contains energy at 440 Hz!
```

**Key insight:** A short poke (10ms duration) is a **broadband impulse**. Its frequency spectrum contains energy at the mode frequency, even if the repetition rate is much lower.

---

## 4. Concrete MIDI Strategies

### Strategy A: Broadband Pulse Trains (Channel 1)

**Concept:** Short 10ms pokes act as impulses. The mode filters out its resonant frequency component and builds up.

#### Setup 1: "Gentle Rain" (Slow Buildup)
```
MIDI Channel: 1 (triggers)
Pattern: Straight quarter notes
Tempo: 90 BPM
Period: 667 ms (1.5 Hz)
Velocity: 50-60

Result: Gentle, gradual buildup
        Each poke excites all modes equally
        Modes ring according to their gamma (damping)
```

**Concrete MIDI Clip (1 bar):**
```
Time:  |1.......2.......3.......4.......|
Notes: X       X       X       X
       C3      C3      C3      C3
Vel:   55      55      55      55
```

#### Setup 2: "Rhythmic Pump" (Moderate Buildup)
```
MIDI Channel: 1 (triggers)
Pattern: 8th notes
Tempo: 120 BPM
Period: 250 ms (4 Hz)
Velocity: 70-80

Result: Faster energy injection
        Modes build up more quickly
        Rhythmic character audible
```

**Concrete MIDI Clip (1 bar):**
```
Time:  |1...2...3...4...|
Notes: X.X.X.X.X.X.X.X.
       C3 (all notes)
Vel:   75 70 75 70 75 70 75 70
       (slight accent on beats)
```

#### Setup 3: "Machine Gun" (Fast Buildup)
```
MIDI Channel: 1 (triggers)
Pattern: 16th notes
Tempo: 140 BPM
Period: ~107 ms (9.3 Hz)
Velocity: 80-90

Result: Rapid energy buildup
        Can saturate quickly
        Dense texture
```

**Concrete MIDI Clip (1 bar):**
```
Time:  |1.&.2.&.3.&.4.&.|
Notes: XXXXXXXXXXXXXXXX (16 notes)
       C3 (all)
Vel:   85 80 80 75 85 80 80 75 (repeating pattern)
```

---

### Strategy B: Sustained Drive (Channel 2)

**Concept:** Hold MIDI note → pokes sent every 100ms (10 Hz) automatically.

#### Setup 4: "Waterfall" (Continuous Drive)
```
MIDI Channel: 2 (drive)
Pattern: Whole note (held)
Tempo: Any
Poke rate: 10 Hz (fixed, from code)
Velocity: 40-60

Result: Continuous energy injection at 10 Hz
        Like a waterfall constantly adding energy
        Builds up to steady-state amplitude
```

**MIDI Implementation:**
```
Note: C3 (hold for 4 seconds)
Velocity: 50
Duration: Whole note

Internally generates pokes every 100ms:
t=0ms    → poke (strength=0.39)
t=100ms  → poke (strength=0.39)
t=200ms  → poke (strength=0.39)
...

vel=50 → strength = 50/127 = 0.39
```

#### Setup 5: "Velocity Swell" (Building Intensity)
```
MIDI Channel: 2 (drive)
Pattern: Whole note with velocity automation
Velocity: 20 → 100 (over 8 seconds)

Result: Gradually increasing waterfall
        Energy buildup accelerates over time
        Can push into nonlinearity
```

**MIDI Automation:**
```
Time:  0s     2s     4s     6s     8s
Vel:   20  →  40  →  60  →  80  →  100
       ▁▁▁▁▂▂▂▂▃▃▃▃▄▄▄▄▅▅▅▅
```

---

### Strategy C: Hybrid Approach (Ch1 + Ch2)

#### Setup 6: "Resonant Foundation + Rhythmic Kicks"
```
Track 1 (Ch2): Sustained drive on Node 0
  Note: C2 (low frequency)
  Velocity: 35 (gentle waterfall)
  Duration: Hold continuously

Track 2 (Ch1): Rhythmic triggers on Node 4 (opposite in ring)
  Pattern: 8th notes
  Tempo: 110 BPM
  Velocity: 70

Result: Node 0 has steady-state oscillation
        Node 4 gets periodic kicks
        Energy travels between them via coupling
        Creates beating/interference patterns
```

---

## 5. Parameter Tuning for Resonance

### Damping (gamma) - Controls Q-Factor

**Q-factor** = Quality factor = ω/(2γ) (dimensionless)

Higher Q → Sharper resonance, slower decay, easier to excite

```
Example: Mode at 440 Hz (ω = 2764 rad/s)

gamma = 0.1  →  Q = 2764/(2×0.1) = 13,820  (VERY high Q)
  - Decay time: ~10 seconds
  - Very sharp resonance
  - Small excitations build up significantly
  - Risk: Can ring excessively

gamma = 0.5  →  Q = 2764/(2×0.5) = 2,764  (High Q)
  - Decay time: ~2 seconds
  - Good resonance
  - Balanced buildup
  - Recommended for musical use

gamma = 2.0  →  Q = 2764/(2×2.0) = 691  (Medium Q)
  - Decay time: ~0.5 seconds
  - Moderate resonance
  - Percussive character
  - Good for rhythmic patterns

gamma = 5.0  →  Q = 2764/(2×5.0) = 276  (Low Q)
  - Decay time: ~0.2 seconds
  - Weak resonance
  - Very percussive
  - Hard to build up energy
```

**Rule of thumb:**
- **For slow buildup/drones:** gamma = 0.2-0.5 (high Q)
- **For balanced response:** gamma = 0.5-1.0 (medium-high Q)
- **For percussion/rhythm:** gamma = 1.0-3.0 (low Q)

### Coupling Strength - Energy Transfer Rate

Controls how fast energy spreads through network:

```
coupling = 0.0   →  Isolated nodes (no network effect)
coupling = 0.2   →  Weak coupling (local neighborhoods)
coupling = 0.5   →  Moderate coupling (wave propagation visible)
coupling = 0.8   →  Strong coupling (collective oscillation)
coupling = 1.0   →  Maximum coupling (network acts as one)
```

**For resonant buildup:**
- **Start low (0.2-0.3)**: Let individual nodes build up first
- **Gradually increase**: Energy spreads to neighbors
- **Watch for saturation**: High coupling + high energy = runaway

### Excitation Strength (MIDI Velocity)

Maps velocity → poke strength:

```
velocity_to_strength(vel) = vel / 127.0

vel = 1    →  strength = 0.008  (barely audible)
vel = 32   →  strength = 0.25   (quiet)
vel = 64   →  strength = 0.50   (medium)
vel = 96   →  strength = 0.76   (loud)
vel = 127  →  strength = 1.0    (maximum)
```

**Resonance consideration:**
Even small velocities (20-40) can build up significantly if:
- Damping is low (high Q)
- Excitation rate matches resonance
- Coupling allows energy accumulation

**Start conservative:** vel = 50-70, increase if needed.

---

## 6. What to Watch For / Tune

### Indicators of Good Resonance

✅ **Gradual amplitude increase** over 2-10 seconds
✅ **Sustain after stopping excitation** (indicates energy storage)
✅ **Clear pitch** (mode frequency audible)
✅ **Wave propagation** (energy moves through network)
✅ **Interference patterns** (beating between nodes)

### Warning Signs

⚠️ **Saturation/distortion:** Amplitude too high
   - **Fix:** Reduce velocity or damping

⚠️ **No buildup:** Energy doesn't accumulate
   - **Fix:** Reduce damping (increase Q), increase coupling

⚠️ **Chaotic/unstable:** Unpredictable oscillations
   - **Fix:** Reduce coupling, increase damping

⚠️ **Mud/no pitch:** Too much inharmonic content
   - **Fix:** Increase damping on higher modes, reduce mode weights

⚠️ **Instant decay:** Notes don't ring
   - **Fix:** Reduce damping (lower gamma)

---

## 7. Concrete Examples with Full Parameter Sets

### Example 1: "Shimmering Bell Resonance"

**Goal:** Slow, beautiful resonant buildup with long sustain

**Node Configuration:**
```json
{
  "carrier_freq_hz": 440.0,
  "coupling_strength": 0.25,
  "audio_gain": 0.7,

  "modes": [
    {"omega_ratio": 1.0, "gamma": 0.3, "weight": 1.0},   // 440 Hz, Q=4610
    {"omega_ratio": 2.7, "gamma": 0.4, "weight": 0.5},   // 1188 Hz
    {"omega_ratio": 5.4, "gamma": 0.5, "weight": 0.3},   // 2376 Hz
    {"omega_ratio": 8.9, "gamma": 0.6, "weight": 0.15}   // 3916 Hz
  ]
}
```

**MIDI Pattern (Channel 1):**
```
Pattern: 8th notes
Tempo: 100 BPM
Note: C4 (MIDI 60)
Velocity: 60

Timeline:
0-4s:   8th notes (vel 60) - Initial buildup
4-8s:   Continue - Amplitude increases
8-10s:  Stop excitation - Observe decay (should ring ~3 seconds)

Timing: 8th @ 100 BPM = 300ms period = 3.33 Hz
```

**Expected Result:**
- Slow buildup over 4-6 seconds
- Reaches ~60% of saturation
- Clear 440 Hz pitch with shimmering overtones
- After stopping, decays gracefully over 3-4 seconds

**Why it works:**
- Low damping (γ=0.3) → High Q → Easy to excite
- Moderate coupling (0.25) → Local resonance, gentle spread
- 10ms poke @ 3.33 Hz → Broadband excitation catches 440 Hz component

---

### Example 2: "Rhythmic Wave Pump"

**Goal:** Fast buildup with rhythmic character, wave propagation

**Node Configuration:**
```json
{
  "topology": "ring",
  "num_nodes": 8,
  "carrier_freq_hz": 220.0,
  "coupling_strength": 0.5,
  "audio_gain": 0.6,

  "modes": [
    {"omega_ratio": 1.0, "gamma": 0.6, "weight": 1.0},   // 220 Hz, Q=1153
    {"omega_ratio": 2.0, "gamma": 0.7, "weight": 0.4},   // 440 Hz
    {"omega_ratio": 3.0, "gamma": 0.8, "weight": 0.2},   // 660 Hz
    {"omega_ratio": 4.0, "gamma": 1.0, "weight": 0.1}    // 880 Hz
  ]
}
```

**MIDI Pattern (Channel 1, Multi-Node):**
```
Track 1: Node 0 (C0) - Euclidean(5,16) @ 130 BPM, vel=75
Track 2: Node 4 (E0) - Euclidean(7,16) @ 130 BPM, vel=70
Track 3: Node 2 (D0) - Straight 8ths @ 130 BPM, vel=65

All same pitch (C3), different nodes
```

**Expected Result:**
- Buildup in 2-3 seconds
- Traveling waves visible around ring
- Interference creates complex amplitude patterns
- Rhythmic "pumping" audible
- Decay in ~1.5 seconds after stopping

**Why it works:**
- Moderate damping (γ=0.6) → Medium Q → Balanced
- Higher coupling (0.5) → Clear wave propagation
- Polyrhythms → Energy injected from multiple locations
- Different phases → Constructive/destructive interference

---

### Example 3: "Continuous Waterfall"

**Goal:** Push network into sustained oscillation with Ch2 drive

**Node Configuration:**
```json
{
  "carrier_freq_hz": 110.0,
  "coupling_strength": 0.4,
  "audio_gain": 0.5,

  "modes": [
    {"omega_ratio": 1.0, "gamma": 0.2, "weight": 1.0},   // 110 Hz, VERY high Q
    {"omega_ratio": 2.0, "gamma": 0.3, "weight": 0.5},   // 220 Hz
    {"omega_ratio": 3.0, "gamma": 0.4, "weight": 0.3},   // 330 Hz
    {"omega_ratio": 4.0, "gamma": 0.5, "weight": 0.2}    // 440 Hz
  ]
}
```

**MIDI Pattern (Channel 2):**
```
Note: C2 (MIDI 36) - Hold for 30 seconds
Velocity: Start at 30, automate to 70 over 20 seconds

Internal poke rate: 10 Hz (every 100ms)

Timeline:
0-10s:   vel=30 → 50  (gradual buildup)
10-20s:  vel=50 → 70  (approaching saturation)
20-30s:  vel=70       (sustained, steady-state)
30-40s:  Release     (observe decay, ~5 seconds)
```

**Expected Result:**
- Very slow, smooth buildup (10-15 seconds to full)
- Reaches steady-state oscillation
- Rich harmonic content
- After release, long decay (~5-7 seconds)

**Why it works:**
- Very low damping (γ=0.2) → Extremely high Q
- Channel 2 provides continuous 10 Hz excitation
- Low frequencies (110 Hz) easier to sustain
- Gradual velocity increase prevents overshoot

---

## 8. Experimental Procedure

### Finding the Resonance Sweet Spot

**Step 1: Single Node, Single Mode**
```
1. Set up one node, one mode (mode 0 only)
2. Choose frequency: 220 Hz (omega = 1382 rad/s)
3. Set damping: gamma = 0.5 (moderate Q)
4. Set coupling: 0.0 (isolated)
5. Send 8th notes @ 120 BPM, vel=60 (Ch1)
6. Listen for buildup over 5-10 seconds
7. Stop and observe decay
```

**Adjust:**
- If no buildup → Reduce gamma to 0.3
- If too much buildup → Increase gamma to 0.8
- If too fast → Reduce velocity to 40
- If too slow → Increase velocity to 80

**Step 2: Add Coupling**
```
1. Keep node settings from Step 1
2. Set coupling to 0.3
3. Excite Node 0 only
4. Observe energy spreading to Node 1, 7 (neighbors)
5. Increase coupling to 0.6
6. Observe faster spread, interference
```

**Step 3: Multi-Node Excitation**
```
1. Excite Node 0 and Node 4 simultaneously (opposite)
2. Same rhythm, different phase (offset by 1/8 note)
3. Observe standing wave pattern
4. Try different phase offsets
5. Listen for beating/interference
```

**Step 4: Channel 2 Drive**
```
1. Switch to Channel 2
2. Hold single note (C2)
3. Start vel=20, increase slowly
4. Find threshold where oscillation sustains
5. Note the critical velocity (varies with gamma)
```

---

## 9. Quick Reference: Timing Chart

### Recommended MIDI Timings vs. Resonator Frequency

| Resonator Freq | Tempo | Pattern | Period | Poke Rate | Buildup Time |
|----------------|-------|---------|--------|-----------|--------------|
| 55 Hz (A1) | 60 BPM | Quarters | 1000ms | 1 Hz | 10-15s |
| 110 Hz (A2) | 90 BPM | 8ths | 333ms | 3 Hz | 8-12s |
| 220 Hz (A3) | 120 BPM | 8ths | 250ms | 4 Hz | 5-8s |
| 440 Hz (A4) | 120 BPM | 16ths | 125ms | 8 Hz | 3-6s |
| 880 Hz (A5) | 140 BPM | 16ths | 107ms | 9.3 Hz | 2-4s |

**Note:** These are starting points. Actual buildup time depends on gamma, coupling, and velocity.

---

## 10. Summary: The Waterfall Recipe

**To create resonant waves in your network of ponds:**

### Choose Your Waterfall Type

**Option A: Rhythmic Waterfalls (Ch1)**
- Pattern: 8th or 16th notes
- Velocity: 60-80
- Envelope: 10ms (default)
- Effect: Rhythmic pumping, gradual buildup

**Option B: Continuous Waterfall (Ch2)**
- Hold note (any duration)
- Velocity: 40-70
- Poke rate: 10 Hz (automatic)
- Effect: Steady energy injection, smooth buildup

**Option C: Hybrid**
- Ch2 foundation (vel 30-50)
- Ch1 rhythmic accents (vel 70-90)
- Effect: Best of both worlds

### Tune the Ponds

**For slow, beautiful resonance:**
- gamma = 0.2-0.4 (high Q)
- coupling = 0.2-0.3 (gentle spread)
- velocity = 50-70 (moderate)

**For fast, rhythmic buildup:**
- gamma = 0.5-0.8 (medium Q)
- coupling = 0.4-0.6 (active spread)
- velocity = 70-90 (strong)

**For percussion/attacks:**
- gamma = 1.0-2.0 (low Q)
- coupling = 0.1-0.3 (local)
- velocity = 80-127 (punchy)

### Watch the Waves

✅ Amplitude growing? → Good resonance
✅ Spreading through network? → Good coupling
✅ Clear pitch? → Good mode frequencies
✅ Sustaining after stop? → Good Q-factor

⚠️ Distorting? → Lower velocity or increase gamma
⚠️ Not building? → Lower gamma or increase velocity
⚠️ Chaotic? → Reduce coupling

---

**The magic happens when excitation rhythm, damping, and coupling align to create self-reinforcing waves across your network of oscillators.** 🌊

Experiment, listen, adjust. The system will teach you its resonances!
