# MIDI Quick Reference Card

## Channel Assignments

| Channel | Mode | Behavior | Use Case |
|---------|------|----------|----------|
| **1** | Trigger | 10ms poke, decays naturally | Melodic phrases, percussion, arpeggios |
| **2** | Drive | Sustained 100ms pulses | Drones, bowed tones, energy buildup |

## Note-to-Node Mapping

```
With N nodes in network: Node ID = MIDI Note % N

8 nodes example:
C0  (0)  → Node 0       C1  (12) → Node 4
C#0 (1)  → Node 1       C#1 (13) → Node 5
D0  (2)  → Node 2       D1  (14) → Node 6
D#0 (3)  → Node 3       D#1 (15) → Node 7
E0  (4)  → Node 4       E1  (16) → Node 0  (wraps)
...
```

## Velocity Guidelines

| Range | Dynamic | Strength | Effect |
|-------|---------|----------|--------|
| 1-30 | ppp | 0.0-0.24 | Whisper-quiet, subtle |
| 31-60 | p-mp | 0.24-0.47 | Gentle, balanced |
| 61-90 | mf-f | 0.47-0.71 | Strong, present |
| 91-127 | ff-fff | 0.71-1.0 | Maximum energy, may saturate |

## Performance Templates

### 🔔 Template 1: "Melodic Bells"
```
Track 1 [Ch1]: Melody line (arpeggios, scales)
Track 2 [Ch2]: Bass pedal tone (C1 or C2)
Velocity: 70-100 (triggers), 40-60 (drone)
Result: Bell-like tones with harmonic foundation
```

### 🌊 Template 2: "Wave Machine"
```
Track 1 [Ch1]: Node 0, E(5,8) rhythm @ 120 BPM
Track 2 [Ch1]: Node 2, E(7,12) rhythm @ 120 BPM
Track 3 [Ch1]: Node 4, straight 8ths @ 120 BPM
Track 4 [Ch1]: Node 6, E(3,8) rhythm @ 120 BPM
Velocity: 60-80 (consistent)
Result: Polyrhythmic wave interference
```

### 🎵 Template 3: "Evolving Drone"
```
Track 1 [Ch2]: C2 (root)
Track 2 [Ch2]: G2 (fifth) - enter at 0:30
Track 3 [Ch2]: C3 (octave) - enter at 1:00
Track 4 [Ch1]: Sparse melodic triggers (decoration)
Automation: Gradually increase coupling strength
Result: Building harmonic complexity
```

### ⚡ Template 4: "Rhythmic Cascade"
```
All on Ch1, same note (e.g., C4):
Track 1: Whole notes (foundation)
Track 2: Quarter notes (pulse)
Track 3: 8th notes (motion)
Track 4: 16th notes (shimmer)
Velocity: Decrease with faster subdivisions
Result: Layered temporal textures
```

## Common Techniques

### Sequential Wave (Ring Topology)
```
C0 → C#0 → D0 → D#0 → E0 → F0 → F#0 → G0 (all Ch1)
Delay each by 1/16th note
Creates: Circular traveling wave
```

### Interference Pattern
```
Node 0 + Node 4 (opposite in ring):
Same rhythm, opposite phase (offset by 50%)
Creates: Standing wave, alternating buildup/cancellation
```

### Drone Swell
```
Ch2: Start vel=20 → automate to vel=100 over 30 seconds
Single note held throughout
Creates: Gradual energy buildup, network "heating up"
```

### Chaos Scatter
```
Random note generator, Ch1
Random nodes (C0-G0)
Random velocities (40-100)
Random timing (sparse to dense)
Creates: Unpredictable, emergent patterns
```

## Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| No sound | Wrong node assignment | Check note % num_nodes |
| Muddy/distorted | Too much energy buildup | Reduce velocity or coupling |
| Notes too short | High damping | Increase decay time (reduce γ) |
| No network effect | Coupling too low | Increase coupling strength |
| Too much resonance | Coupling too high | Reduce coupling or use Ch1 |
| Rhythms not locking | Inharmonic frequencies | Try different tempo/note combos |

## Parameter Ranges (For Future CC Mapping)

```
Coupling Strength:    0.0 (isolated) → 1.0 (locked)
Damping (γ):          0.1 (long) → 2.0 (short)
Excitation Duration:  1ms (click) → 20ms (smooth)
Mode Weights:         0.0 (silent) → 1.0 (full)
Output Gain:          0.0 (mute) → 1.0 (max)
```

## Network Topologies

```
RING (default):
  0—1—2—3—4—5—6—7—0

STAR:
      ┌─1  2─┐
      3       4
      │   0   │
      5       6
      └─7  8─┘

LATTICE:
  0—1—2
  │ │ │
  3—4—5
  │ │ │
  6—7—8

ALL-TO-ALL:
  Every node connected to every other
```

## Tips for Live Performance

1. **Start simple** - Single trigger pattern, add layers
2. **Use mutes** - Mute/unmute tracks for dynamic variation
3. **Velocity sweeps** - Automate velocity for expression
4. **Coupling modulation** - CC control for real-time network changes
5. **Know your decay** - Timing depends on damping settings
6. **Leave space** - Sparse patterns reveal network character
7. **Listen for resonance** - Network has sweet spots, find them

## Emergency Stops

**Silence the network:**
1. Stop all MIDI playback
2. Send MIDI Ch2 note-offs for all active drones
3. Network will decay naturally
4. Or: Send STOP message via hub controller

**Reset network:**
1. Stop all MIDI
2. Send RESET command (if implemented)
3. All modes cleared to zero

---

**Remember:** You're not playing a synth, you're **conducting energy through a network**. Think physics, not piano.
