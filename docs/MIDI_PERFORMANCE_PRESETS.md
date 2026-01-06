# Performance Presets for MIDI Control

These presets are optimized for different musical approaches. Each preset defines network topology, resonator parameters, and suggested MIDI strategies.

---

## Preset 1: "Shimmer Bells" 🔔

**Character:** Bright, percussive, bell-like tones with long decay
**Best for:** Melodic sequences, arpeggios, ambient textures

### Configuration
```json
{
  "name": "Shimmer Bells",
  "topology": "ring",
  "num_nodes": 8,
  "coupling_strength": 0.25,

  "resonator": {
    "personality": "resonator",
    "carrier_freq_hz": 440.0,
    "audio_gain": 0.8,

    "modes": [
      {"omega_ratio": 1.0,  "gamma": 0.2, "weight": 1.0},   // Fundamental
      {"omega_ratio": 2.7,  "gamma": 0.3, "weight": 0.6},   // Inharmonic 1
      {"omega_ratio": 5.4,  "gamma": 0.4, "weight": 0.3},   // Inharmonic 2
      {"omega_ratio": 8.9,  "gamma": 0.5, "weight": 0.15}   // High shimmer
    ]
  }
}
```

### MIDI Strategy
```
Primary: Channel 1 (triggers)
- Melodic sequences in C major pentatonic
- Velocities: 70-100 (bright but not harsh)
- Rhythm: 8th notes to quarter notes @ 90-120 BPM

Secondary: Channel 2 (optional drone)
- Single low note (C1 or C2)
- Velocity: 30-50 (subtle foundation)
- Duration: Whole notes or longer

Node mapping:
- Use consecutive notes for smooth wave propagation
- Example: C4, D4, E4, G4, A4 → nodes in sequence
```

### Performance Tips
- **Sparse is better**: Let each note ring and decay
- **Ascending patterns**: Create nice traveling waves in ring
- **Low coupling**: Keeps bells clear and separate
- **Long decay**: Notes blend into shimmering textures

---

## Preset 2: "Wave Machine" 🌊

**Character:** Polyrhythmic, hypnotic, phase-shifting textures
**Best for:** Minimal techno, ambient, generative music

### Configuration
```json
{
  "name": "Wave Machine",
  "topology": "ring",
  "num_nodes": 8,
  "coupling_strength": 0.6,

  "resonator": {
    "personality": "resonator",
    "carrier_freq_hz": 220.0,
    "audio_gain": 0.7,

    "modes": [
      {"omega_ratio": 1.0,  "gamma": 0.4, "weight": 1.0},   // Strong fundamental
      {"omega_ratio": 2.0,  "gamma": 0.5, "weight": 0.4},   // Octave
      {"omega_ratio": 3.0,  "gamma": 0.6, "weight": 0.2},   // Fifth
      {"omega_ratio": 4.0,  "gamma": 0.7, "weight": 0.1}    // Second octave
    ]
  }
}
```

### MIDI Strategy
```
All on Channel 1 (triggers):

Track 1: Node 0 - Euclidean(5,8) @ 120 BPM, vel=70
Track 2: Node 2 - Euclidean(7,12) @ 120 BPM, vel=60
Track 3: Node 4 - Euclidean(3,8) @ 120 BPM, vel=80
Track 4: Node 6 - Straight 8ths @ 120 BPM, vel=50

All tracks: Same note (e.g., C3) but different nodes
```

### Performance Tips
- **Polyrhythms**: Use Euclidean or odd-meter patterns
- **Same pitch**: Rhythmic complexity, not harmonic
- **Medium coupling**: Energy travels, creates interference
- **Medium decay**: Balance between pulse and ring
- **Let it run**: Patterns evolve over minutes

---

## Preset 3: "Bowed Strings" 🎻

**Character:** Sustained, evolving, drone-like
**Best for:** Ambient, soundscapes, textural pads

### Configuration
```json
{
  "name": "Bowed Strings",
  "topology": "ring",
  "num_nodes": 6,
  "coupling_strength": 0.45,

  "resonator": {
    "personality": "self_oscillator",  // Negative damping
    "carrier_freq_hz": 110.0,
    "audio_gain": 0.6,

    "modes": [
      {"omega_ratio": 1.0,  "gamma": -0.05, "weight": 1.0},  // Slight gain
      {"omega_ratio": 2.0,  "gamma": 0.1,   "weight": 0.5},  // Harmonic
      {"omega_ratio": 3.0,  "gamma": 0.2,   "weight": 0.3},  // Third harmonic
      {"omega_ratio": 1.5,  "gamma": 0.3,   "weight": 0.2}   // Inharmonic color
    ]
  }
}
```

### MIDI Strategy
```
Primary: Channel 2 (sustained drive)

Track 1: C2 (root) - hold throughout
Track 2: G2 (fifth) - enter at 0:30, vel increase
Track 3: C3 (octave) - enter at 1:00
Track 4 (Ch1): Sparse triggers for "bowing" articulation

Velocity automation:
- Start: 20-30 (gentle)
- Build: 30 → 70 over 1 minute
- Sustain: 70 (stable)
- Fade: 70 → 0 over 30 seconds
```

### Performance Tips
- **Slow evolution**: Minutes-long changes
- **Velocity = bow pressure**: Automate for expression
- **Self-oscillation**: Network sustains itself
- **Add triggers sparingly**: Like re-bowing
- **Harmonic layers**: Stack intervals slowly

---

## Preset 4: "Metallic Percussion" 🥁

**Character:** Sharp, metallic, gong-like strikes
**Best for:** Rhythmic patterns, percussion sequences

### Configuration
```json
{
  "name": "Metallic Percussion",
  "topology": "star",  // Hub-and-spoke
  "num_nodes": 9,      // 1 hub + 8 periphery
  "coupling_strength": 0.15,

  "resonator": {
    "personality": "resonator",
    "carrier_freq_hz": 880.0,
    "audio_gain": 0.9,

    "modes": [
      {"omega_ratio": 1.0,   "gamma": 0.8, "weight": 1.0},   // Fast decay
      {"omega_ratio": 3.14,  "gamma": 0.9, "weight": 0.7},   // Inharmonic
      {"omega_ratio": 7.83,  "gamma": 1.0, "weight": 0.4},   // High clang
      {"omega_ratio": 13.2,  "gamma": 1.2, "weight": 0.2}    // Sizzle
    ]
  }
}
```

### MIDI Strategy
```
All on Channel 1 (triggers):

Hub node (Node 0): Kick drum pattern (C0)
Periphery nodes: Hi-hat, snare, toms (C#0-G0)

Example drum pattern:
Track 1 (C0 - Hub):     |X...|X...|X...|X...|  (quarter notes)
Track 2 (D0 - Node 2):  |..X.|..X.|..X.|..X.|  (snare on 2-4)
Track 3 (F0 - Node 5):  |X.X.|X.X.|X.X.|X.X.|  (hi-hats)
Track 4 (E0 - Node 4):  |....|..X.|....|.X..|  (tom fills)

Velocities: 80-127 (punchy, varied for dynamics)
```

### Performance Tips
- **Short decay**: Tight, rhythmic
- **High frequencies**: Metallic character
- **Low coupling**: Keep hits separate
- **Star topology**: Hub = bass/kick, periphery = highs
- **Velocity accents**: Create groove

---

## Preset 5: "Resonant Chaos" 💫

**Character:** Unpredictable, complex, evolving textures
**Best for:** Experimental, generative, aleatoric music

### Configuration
```json
{
  "name": "Resonant Chaos",
  "topology": "all_to_all",  // Dense connections
  "num_nodes": 6,            // Smaller for manageable complexity
  "coupling_strength": 0.75,

  "resonator": {
    "personality": "resonator",
    "carrier_freq_hz": 330.0,
    "audio_gain": 0.5,

    "modes": [
      {"omega_ratio": 1.0,   "gamma": 0.2, "weight": 1.0},
      {"omega_ratio": 1.414, "gamma": 0.3, "weight": 0.7},   // Sqrt(2)
      {"omega_ratio": 2.718, "gamma": 0.4, "weight": 0.5},   // e
      {"omega_ratio": 3.141, "gamma": 0.5, "weight": 0.3}    // π
    ]
  }
}
```

### MIDI Strategy
```
Use random/generative sequencing:

Option A: Probability-based triggers (Ch1)
- Each node: 25% chance per 16th note
- Random velocities: 40-90
- No fixed pattern

Option B: Slow random walk (Ch2)
- Each node gets random sustained notes
- Duration: 2-8 seconds
- Random pitch offsets: ±200 cents

Option C: Chaos injection (Ch1)
- Burst of random triggers every 10-20 seconds
- Let network settle between bursts
```

### Performance Tips
- **High coupling**: Maximum interaction
- **All-to-all topology**: Everything affects everything
- **Random input**: Emergent patterns
- **Record long takes**: Chaos evolves over time
- **Lower gain**: Prevent overload

---

## Preset 6: "Harmonic Lattice" 🎹

**Character:** Pitched, harmonic, chord-based
**Best for:** Harmonic progressions, chord sequences

### Configuration
```json
{
  "name": "Harmonic Lattice",
  "topology": "lattice_3x3",
  "num_nodes": 9,
  "coupling_strength": 0.35,

  "resonator": {
    "personality": "resonator",
    "carrier_freq_hz": 261.63,  // C4
    "audio_gain": 0.7,

    "modes": [
      {"omega_ratio": 1.0, "gamma": 0.3, "weight": 1.0},   // Fundamental
      {"omega_ratio": 2.0, "gamma": 0.4, "weight": 0.5},   // Octave
      {"omega_ratio": 3.0, "gamma": 0.5, "weight": 0.3},   // Fifth
      {"omega_ratio": 4.0, "gamma": 0.6, "weight": 0.2}    // Second octave
    ]
  }
}
```

### MIDI Strategy
```
Play chords across the lattice:

C major chord:
  C0 (node 0) + E0 (node 4) + G0 (node 7)

F major chord:
  F0 (node 5) + A0 (node 9) + C1 (node 12 % 9 = 3)

G major chord:
  G0 (node 7) + B0 (node 11 % 9 = 2) + D1 (node 14 % 9 = 5)

Progression: C → F → G → C
Duration: 4 beats per chord @ 90 BPM
Channel: Mix of Ch1 (attacks) and Ch2 (sustain)
```

### Performance Tips
- **Lattice topology**: 2D wave propagation
- **Chord voicings**: Spatial distribution matters
- **Mix channels**: Ch1 for attacks, Ch2 for sustain
- **Medium coupling**: Chords blend but stay clear
- **Progressions**: Standard harmony works!

---

## Choosing the Right Preset

| If you want... | Use this preset |
|----------------|-----------------|
| Melodic sequences | Shimmer Bells |
| Hypnotic rhythms | Wave Machine |
| Ambient drones | Bowed Strings |
| Percussive hits | Metallic Percussion |
| Experimental textures | Resonant Chaos |
| Harmonic progressions | Harmonic Lattice |

---

## Customization Tips

### Adjusting Decay Time
```
Longer decay:  Reduce γ (gamma) → 0.1 to 0.3
Shorter decay: Increase γ → 0.8 to 2.0
```

### Adjusting Network Effect
```
More isolated:  coupling_strength → 0.1 to 0.2
More connected: coupling_strength → 0.6 to 0.8
```

### Adjusting Timbre
```
More fundamental:  Increase mode 0 weight to 1.0
More harmonics:    Increase mode 1-2 weights
More inharmonic:   Use irrational omega_ratios (π, e, √2)
More air/shimmer:  Increase mode 3 weight
```

### Adjusting Pitch Range
```
Bass frequencies:    carrier_freq_hz = 55 to 110 Hz
Mid frequencies:     carrier_freq_hz = 220 to 440 Hz
High frequencies:    carrier_freq_hz = 880 to 1760 Hz
```

---

## Loading Presets

### On ESP32 (Future Implementation)
```cpp
// Load from SD card
hub_load_preset("/presets/shimmer_bells.json");

// Or select via MIDI Program Change
// PC 0 = Shimmer Bells
// PC 1 = Wave Machine
// etc.
```

### On Python Simulator
```python
from src.config_loader import load_preset

config = load_preset("presets/shimmer_bells.json")
network = ModalNetwork(config)
network.start()
```

---

## Creating Your Own Presets

1. **Start with a base**: Copy existing preset
2. **Adjust topology**: Match your musical intent
3. **Tune resonators**: Set decay, coupling, modes
4. **Test with MIDI**: Try different patterns
5. **Iterate**: Fine-tune based on sound
6. **Document**: Write down MIDI strategies that work
7. **Save**: JSON file for reproducibility

**Template:**
```json
{
  "name": "My Preset",
  "topology": "ring|star|lattice|all_to_all",
  "num_nodes": 4-16,
  "coupling_strength": 0.0-1.0,

  "resonator": {
    "personality": "resonator|self_oscillator",
    "carrier_freq_hz": 55-1760,
    "audio_gain": 0.0-1.0,

    "modes": [
      {"omega_ratio": X, "gamma": Y, "weight": Z},
      // ... up to 4 modes
    ]
  }
}
```

---

Happy performing! 🎵
