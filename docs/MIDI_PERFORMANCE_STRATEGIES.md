# MIDI Performance Strategies for the Networked Modal Resonator

## The Instrument's Place in Your Setup

You have a sequencer (DAW or hardware) and this new networked instrument. Unlike traditional synthesizers that respond predictably to each note, this is a **living network** - a collection of coupled resonators that can ring, shimmer, and propagate energy across the network topology.

Think of it as:
- **Not a polyphonic synth** - it's a network of interconnected resonant bodies
- **Not just triggered** - it has memory, inertia, and complex coupling dynamics
- **A hybrid instrument** - part percussion, part drone, part delay network

---

## Current MIDI Implementation

### Channel 1: Triggers (Percussive Mode)
- **Behavior**: Short poke (10ms envelope)
- **Musical character**: Struck bells, gongs, metallic percussion
- **Energy**: Brief excitation that rings out naturally
- **Network effect**: Creates traveling waves across the topology

### Channel 2: Drive (Sustained Mode)
- **Behavior**: Continuous pokes every 100ms while note is held
- **Musical character**: Bowed strings, sustained energy injection
- **Energy**: Maintains amplitude, can build up resonance
- **Network effect**: Creates standing wave patterns, can push into self-oscillation

### Note Routing
```
MIDI Note → Node ID (modulo mapping)
C0 (note 0) → Node 0
C#0 (note 1) → Node 1
...
C1 (note 12) → Node 4 (if 8 nodes)
```

Each note frequency sets the excitation frequency at the target node.

---

## Strategy 1: Direct Intervention

### Playing Philosophy
Use MIDI notes as **direct perturbations** - you're poking specific resonators in the network with specific frequencies and intensities.

### Sequencer Setup

#### A. Melodic Triggers (Channel 1)
```
DAW Track 1: "Melodic Triggers" → MIDI Channel 1
- Create melodic sequences (arpeggios, scales, melodies)
- Each note strikes a node with that frequency
- Velocity controls excitation strength
- Notes ring out and decay naturally
- Network propagates energy to neighbors
```

**Creative Techniques:**
1. **Arpeggio cascades** - Rising/falling patterns create traveling waves
2. **Chord strikes** - Multiple nodes excited simultaneously, complex interference
3. **Rhythmic melodies** - Syncopated patterns that exploit decay times
4. **Velocity ramps** - Gradually increasing/decreasing hit strength

#### B. Drone Foundations (Channel 2)
```
DAW Track 2: "Sustained Tones" → MIDI Channel 2
- Hold long notes (whole notes, half notes)
- Creates sustained energy injection
- Builds up resonance over time
- Can drive the network into ringing
```

**Creative Techniques:**
1. **Pedal tones** - Single low note held while triggers play above
2. **Parallel fifths** - Two nodes sustained, creates harmonic coupling
3. **Gradual fade-ins** - Start with low velocity, automate CC to increase
4. **Cluster drones** - 3-4 notes held to create complex standing waves

### Example Patch: "Shimmer Bells"
```
Track 1 (Ch1 Triggers):
- Pattern: C4, E4, G4, B4 (16th notes, ascending)
- Velocity: 80-120 (randomized)
- Effect: Cascading bell-like tones

Track 2 (Ch2 Drive):
- Pattern: C2 (whole note, held)
- Velocity: 40 (subtle)
- Effect: Low drone provides "gravity" to the network
```

---

## Strategy 2: Rhythmic Excitation - "Kicking the Network"

### Playing Philosophy
Use **rhythmic patterns** to inject energy at specific rates, creating wave-like motion and resonant buildups. You're not playing melodies - you're creating excitation rhythms that the network transforms into complex textures.

### The Physics
When you excite the network rhythmically:
- **Periodic excitation** can reinforce resonances (like pushing a swing)
- **Syncopated patterns** create interference and beating
- **Polyrhythmic layers** across multiple nodes = wave collisions
- **Tempo relationships** to node frequencies = phase-locking or chaos

### Sequencer Setup

#### A. Pulse Trains (Channel 1)
```
DAW Track 3: "Node 0 Pulse" → MIDI Channel 1
- Send C0 (node 0) at regular intervals
- Example: 8th notes @ 120 BPM = 4 Hz pulse rate
- Velocity: Moderate (60-80)
- Effect: Rhythmic pumping of node 0
```

**Rhythm Patterns:**
1. **Straight pulses** - Quarter notes, 8th notes, 16th notes
2. **Euclidean rhythms** - E(5,8), E(7,16) - create interesting phase patterns
3. **Accent patterns** - Velocity variations (80, 40, 40, 80, ...)
4. **Polymeters** - 5/4 against 4/4 across different nodes

#### B. Multi-Node Rhythms
```
Track 4: "Node 2 Pulse" → MIDI Channel 1 (D0 = Node 2)
Track 5: "Node 5 Pulse" → MIDI Channel 1 (F0 = Node 5)

- Same tempo, different rhythms
- Creates interference patterns
- Energy travels between nodes
```

**Spatial Patterns:**
1. **Adjacent nodes** - Create localized waves
2. **Opposite nodes** (in ring topology) - Standing waves
3. **Sequential firing** - Node 0, 1, 2, 3... = traveling wave
4. **Random scatter** - Chaos and complexity

#### C. Tempo-Frequency Relationships

The magic happens when excitation rate relates to resonator frequencies:

```
Node resonant frequency: 100 Hz
Excitation at 120 BPM, 16th notes = 8 Hz

Ratio: 100/8 = 12.5
Result: Excitation is inharmonic - creates shimmer/beating

Excitation at 4 Hz (quarter notes @ 60 BPM):
Ratio: 100/4 = 25
Result: Closer to harmonic lock - more stable buildup
```

**Techniques:**
1. **Sub-harmonic driving** - Slow pulses (1-4 Hz) below resonance
2. **Near-resonance pumping** - Excite at 1/2, 1/3, 1/4 of resonant freq
3. **Beating patterns** - Two tempos slightly detuned
4. **Acceleration/deceleration** - Tempo automation

### Example Patch: "Wave Machine"
```
Setup: 8 nodes in ring topology

Track 1 (Ch1): Node 0 - E(5,8) @ 120 BPM, vel=70
Track 2 (Ch1): Node 2 - E(7,12) @ 120 BPM, vel=60
Track 3 (Ch1): Node 4 - Straight 8ths @ 120 BPM, vel=50
Track 4 (Ch1): Node 6 - E(3,8) @ 120 BPM, vel=80

Result: Complex traveling wave patterns, builds and decays in cycles
```

---

## Strategy 3: Hybrid Approaches

### A. Melody + Rhythm Foundation
```
Track 1 (Ch2 Drive): Low drone on Node 0 (C1, held)
Track 2 (Ch1 Trigger): Rhythmic pulse on Node 4 (opposite side, 8th notes)
Track 3 (Ch1 Trigger): Melodic sequence on Nodes 1-7 (improvised)

Effect: Drone provides energy base, pulse creates structure,
        melody adds harmonic content
```

### B. Evolving Textures
```
0:00-0:30 - Sparse triggers (Channel 1), low velocity
0:30-1:00 - Add slow drive note (Channel 2), single node
1:00-1:30 - Increase trigger density, add rhythmic patterns
1:30-2:00 - Add second drive note, fifth above
2:00-2:30 - Fade out triggers, let drones ring
2:30-3:00 - Remove drones, decay to silence
```

### C. Call and Response
```
Phrase A (4 bars): Melodic triggers only (Ch1) - sharp attacks
Phrase B (4 bars): Sustained drones only (Ch2) - smooth sustain
Phrase C (4 bars): Both together - interaction and complexity
```

---

## Advanced MIDI Techniques

### 1. MIDI CC Mapping (Future Enhancement)

Potential CC mappings for expressive control:

```
CC 1 (Mod Wheel):     Global coupling strength (0-1)
CC 7 (Volume):        Master output gain
CC 11 (Expression):   Drive intensity (affects Ch2 poke strength)
CC 74 (Brightness):   Mode weight distribution (favor high modes)
CC 71 (Resonance):    Damping coefficient (γ) - higher = shorter decay
CC 73 (Attack):       Excitation envelope duration (1-20ms)

Per-node CCs (16 channels):
CC 16-23:             Node 0-7 individual gains
CC 24-31:             Node 0-7 coupling weights
```

### 2. Velocity Mapping

Current: `velocity (0-127) → strength (0-1)`

**Musical uses:**
- **ppp (20-40)**: Subtle excitation, natural decay
- **mp (50-70)**: Medium energy, balanced
- **ff (100-127)**: Strong excitation, potential overload/saturation

**Technique**: Velocity automation for dynamic swells

### 3. Program Changes

Could trigger different network configurations:
```
PC 0: Ring topology, 4-mode resonators
PC 1: Star topology (all nodes → center hub)
PC 2: Grid topology (2D lattice)
PC 3: All-to-all coupling (dense network)
PC 4: Custom topology loaded from SD card
```

### 4. Aftertouch / Polyphonic Pressure

```
Channel Aftertouch:   Modulate drive intensity while note held
Poly Aftertouch:      Per-note expression (if supported)
```

---

## Network Topology Considerations

The network topology dramatically affects how energy propagates:

### Ring Topology (Default)
```
Node 0 ↔ Node 1 ↔ Node 2 ↔ ... ↔ Node 7 ↔ Node 0
```
- **Wave propagation**: Circular traveling waves
- **Strategy**: Sequential excitation creates smooth waves
- **Interference**: Opposite nodes create standing waves

### Star Topology
```
        Node 0 (hub)
       ↙ ↓ ↓ ↓ ↘
Node 1-2-3-4-5-6-7
```
- **Wave propagation**: Radial from/to center
- **Strategy**: Excite hub to broadcast to all nodes
- **Strategy**: Excite periphery, energy converges at hub

### Lattice/Grid
```
0 ↔ 1 ↔ 2
↕   ↕   ↕
3 ↔ 4 ↔ 5
↕   ↕   ↕
6 ↔ 7 ↔ 8
```
- **Wave propagation**: 2D wave fronts
- **Strategy**: Corner excitation → diagonal waves
- **Strategy**: Edge excitation → wave fronts

---

## Performance Workflows

### Workflow 1: Studio Composition
1. Set up DAW with multiple MIDI tracks
2. Track 1-2: Foundational drones (Ch2)
3. Track 3-8: Melodic/rhythmic triggers (Ch1)
4. Record automation for dynamics
5. Render stems, post-process

### Workflow 2: Live Performance (Hybrid)
1. Pre-sequenced rhythmic foundation (Ableton, etc.)
2. Live MIDI controller for melodic triggers
3. Expression pedal → coupling strength CC
4. Mod wheel → drive intensity

### Workflow 3: Generative/Algorithmic
1. Max/MSP or Pure Data patch
2. Algorithmic pattern generation
3. Probability-based triggering
4. Evolving polyrhythms

### Workflow 4: Hardware Sequencer
1. Elektron Octatrack / Digitakt
2. Pattern per node
3. Parameter locks for velocity variation
4. Live muting/unmuting of patterns

---

## Creative Exercises

### Exercise 1: Single Node Exploration
- Send MIDI to only Node 0 (C notes)
- Try different rhythmic patterns
- Observe how energy leaks to neighbors
- Find the "sweet spot" tempo that builds resonance

### Exercise 2: Wave Propagation
- Ring topology (8 nodes)
- Sequential triggering: C0 → C#0 → D0 → ... (all Ch1)
- Try different speeds: quarter notes, 8ths, 16ths
- Listen to the wave travel around the ring

### Exercise 3: Interference Patterns
- Two opposite nodes in ring (e.g., Node 0 and Node 4)
- Both triggered with same rhythm, Ch1
- Vary phase offset (delay one by 1/32 note)
- Hear constructive/destructive interference

### Exercise 4: Drone Building
- Ch2: Start with single low note
- Add second note (fifth above) after 30 seconds
- Add third note (octave) after 60 seconds
- Observe how the network "fills up" with energy

### Exercise 5: Chaos Mode
- All 8 nodes triggered randomly (Ch1)
- Velocities randomized (40-100)
- No rhythmic structure
- Let the network find its own patterns

---

## Sound Design Tips

### Timbre Shaping
The **4-mode structure** gives you timbral control:

```
Mode 0 (weight=1.0):   Fundamental tone
Mode 1 (weight=0.5):   First overtone (character)
Mode 2 (weight=0.3):   Phase modulation (shimmer)
Mode 3 (weight=0.2):   High frequency "air"
```

**MIDI Strategy**: Different note ranges = different mode emphasis
- Low notes (C0-C2): Favor Mode 0-1, fundamental weight
- Mid notes (C3-C5): Balanced modes, harmonic richness
- High notes (C6-C8): Favor Mode 2-3, airy/glassy textures

### Decay Time Control
**Damping (γ)** controls how long resonances last:

```
γ = 0.1:  Very long decay (10+ seconds) - ethereal
γ = 0.5:  Medium decay (2-3 seconds) - balanced
γ = 2.0:  Short decay (0.5 seconds) - percussive
```

**MIDI Strategy**:
- Long sequences → short decay (avoid buildup/mud)
- Sparse hits → long decay (let notes breathe)
- Dense rhythms → medium decay (rhythmic clarity)

### Coupling Strength
**coupling_strength** (0-1) controls how much energy transfers between nodes:

```
0.0:  Isolated nodes - no network effect
0.2:  Subtle coupling - gentle bleeding
0.5:  Moderate coupling - clear wave propagation
0.8:  Strong coupling - collective behavior
1.0:  Maximum coupling - network "locks up"
```

**MIDI Strategy**:
- Start performance with low coupling (0.2)
- Gradually increase via CC to build tension
- Drop to 0.0 for isolated "solo" moments

---

## Notation and Documentation

When writing down your patches/performances:

### Patch Sheet Format
```
PATCH: "Shimmer Cascade"
Date: 2026-01-06
Topology: Ring (8 nodes)
Config: 4-mode resonators, γ=0.5, coupling=0.3

MIDI Tracks:
T1 [Ch1]: C4-E4-G4 arp, 16ths @ 120 BPM, vel=80
T2 [Ch2]: C2 drone, whole notes, vel=40
T3 [Ch1]: Random triggers, nodes 0-7, sparse

Automation:
  0:30 - Coupling 0.3 → 0.6 (30sec ramp)
  1:00 - Stop T3
  1:30 - Fade T2 velocity 40 → 0
  2:00 - END
```

---

## Conclusion: The Instrument's Identity

This networked resonator is:

✨ **An excitable medium** - you inject energy, the network shapes it
🌊 **A wave propagator** - rhythms become spatial patterns
🔔 **A meta-instrument** - collection of coupled resonators
🎨 **A texture generator** - from sparse bells to dense drones
⚡ **Responsive to rhythm** - temporal patterns → sonic structures

**Your role as musician:**
Not to "play notes" but to **choreograph energy flow** through the network.

**Key insight:**
The most interesting sounds come from the **interaction** between your excitation patterns and the network's natural dynamics. You're composing the initial conditions; the network composes the evolution.

---

## Next Steps

1. **Experiment** with single-node patterns first
2. **Map the topology** - understand your network's shape
3. **Find resonances** - discover which rhythms build up energy
4. **Layer textures** - combine triggers and drives
5. **Automate** - use CCs and automation for evolution
6. **Record everything** - this instrument is unpredictable!

Happy exploring! 🎶
