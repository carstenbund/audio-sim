# Quick Start Keyboard Manual
## Play Your Networked Resonator in 5 Minutes

---

## 🎹 What You Need

- MIDI keyboard (or pad controller)
- This networked resonator instrument (ESP32 or simulator)
- **No special setup required** - just regular MIDI!

---

## ⚡ Quick Start: Two Ways to Play

### Method 1: Rhythmic Pumping (Channel 1) - **Easiest!**

**Creates:** Percussive builds, rhythmic textures, traveling waves

**Steps:**

1. **Set your keyboard to MIDI Channel 1**
   - (Check your keyboard manual for "MIDI channel" setting)

2. **Start a metronome at 100 BPM**

3. **Play 8th notes on Middle C (C4)**
   ```
   Count: "1-and-2-and-3-and-4-and"
   Play:   C4  C4  C4  C4  C4  C4  C4  C4
           ▄   ▄   ▄   ▄   ▄   ▄   ▄   ▄

   Technique: SHORT, staccato hits (like hi-hat)
   Velocity: Hit keys at medium strength (~70-80)
   ```

4. **Keep playing for 10 seconds** → Listen for buildup!

**What you'll hear:**
- 0-3 sec: Faint ringing starting
- 3-6 sec: Clear tone emerging
- 6-10 sec: Full, shimmering resonance
- Stop playing → 2-4 second decay

**That's it!** You've created resonant waves! 🌊

---

### Method 2: Continuous Waterfall (Channel 2)

**Creates:** Smooth drones, ambient textures, sustained resonance

**Steps:**

1. **Set your keyboard to MIDI Channel 2**

2. **Press and HOLD a low note (C2 or C3)**
   ```
   Press: C2
   Hold:  ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
          (for 20-30 seconds)

   Technique: Like sustain pedal - just hold!
   Velocity: Medium-soft (~50-60)
   ```

3. **Wait patiently** → Listen for slow buildup!

**What you'll hear:**
- 0-5 sec: Almost silent
- 5-10 sec: Low tone starting to emerge
- 10-20 sec: Rich, warm drone building
- 20-30 sec: Full harmonic texture
- Release → 5-8 second decay!

**Perfect for:** Ambient music, soundscapes, meditation

---

## 🎼 Essential Patterns

### Pattern 1: "Steady Pulse" ⭐ **START HERE**
```
Channel: 1
Tempo: 100-120 BPM
Pattern: 8th notes (every half-beat)
Note: C4 (Middle C)
Velocity: 70

Count:  1 & 2 & 3 & 4 & | 1 & 2 & 3 & 4 &
Play:   ▄ ▄ ▄ ▄ ▄ ▄ ▄ ▄ | ▄ ▄ ▄ ▄ ▄ ▄ ▄ ▄

Keep playing for 8-16 bars
Perfect for: Learning how the system responds
```

---

### Pattern 2: "Heartbeat"
```
Channel: 1
Tempo: 70 BPM
Pattern: Quarter notes (every beat)
Note: C4
Velocity: 75

Count:  1 . . . 2 . . . | 3 . . . 4 . . .
Play:   ▄       ▄       | ▄       ▄

Slower = more space between pokes
Perfect for: Gentle, meditative buildup
```

---

### Pattern 3: "Tremolo"
```
Channel: 1
Tempo: 120-140 BPM
Pattern: 16th notes (very fast!)
Note: C4
Velocity: 80

Count:  1e&a2e&a3e&a4e&a
Play:   ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄

Technique: Alternate fingers (index-middle-index-middle)
Perfect for: Fast buildup, dense textures
```

---

### Pattern 4: "Drone + Melody" (Both Channels)
```
LEFT HAND (Channel 2):
  Play: C2
  Hold: ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
  (Continuous foundation)

RIGHT HAND (Channel 1):
  Play: C4, E4, G4 (melody)
  Pattern: Sparse, musical phrases
  Example: C4 . . E4 . . G4 . . . . E4 C4 . .

Perfect for: Musical performance, ambient with melody
```

---

## 🎚️ Playing Techniques

### Velocity Control (How Hard You Hit Keys)

```
Very Soft (20-40):    Gentle whisper
  → Slow buildup, delicate
  → Use for: Ambient, quiet sections

Medium (50-70):       Balanced ⭐
  → Good buildup rate
  → Use for: Most playing, learning

Hard (80-100):        Strong
  → Fast buildup, can saturate
  → Use for: Rhythmic sections, climaxes

Very Hard (100-127):  Maximum
  → Very fast buildup, risk distortion
  → Use for: Peaks, special effects
```

**Tip:** Start medium (60-70), adjust from there!

---

### Note Duration (How Long to Hold)

**Channel 1 (Rhythmic):**
```
Short = Good:   ▄ ▄ ▄ ▄  (staccato)
Long = Also Good: ▬▬▬▬▬▬  (legato)

It doesn't matter! System only cares about when you PRESS.
Just keep steady rhythm.
```

**Channel 2 (Sustained):**
```
Short hold (2 sec):   ▬▬  → Brief waterfall
Medium hold (10 sec): ▬▬▬▬▬▬▬▬▬▬ → Moderate buildup
Long hold (30 sec):   ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬ → Full resonance

Duration MATTERS! Hold longer = more energy.
```

---

### Which Notes to Play (Node Selection)

**8-Node Network:**
```
C0  (very low C)  → Node 0
C#0               → Node 1
D0                → Node 2
D#0               → Node 3
E0                → Node 4
F0                → Node 5
F#0               → Node 6
G0                → Node 7
G#0               → Node 0 (wraps around)
...and so on

Higher octaves: C4, D4, E4 also work!
  C4 (middle C) → Node 4
  E4            → Node 0
  G4            → Node 3
```

**Tips:**
- **Single node:** Play same note repeatedly (e.g., C4)
- **Multiple nodes:** Play different notes (C4, E4, G4 = 3 nodes)
- **Wave propagation:** Play consecutive notes (C4, D4, E4, F4...)

---

## 🎯 Quick Recipes

### Recipe 1: "First Time" (2 minutes)
```
1. Channel 1, Middle C (C4)
2. Metronome: 100 BPM
3. Play 8th notes, medium velocity
4. Keep going for 10 seconds
5. Stop and listen to decay
6. Done! You've made resonant waves!
```

---

### Recipe 2: "Ambient Drone" (1 minute)
```
1. Channel 2, Low C (C2)
2. Hold key for 30 seconds, soft velocity
3. Listen to slow buildup
4. Release and enjoy decay
5. Try adding more notes (G2, C3) after 15 seconds
```

---

### Recipe 3: "Traveling Wave" (3 minutes)
```
1. Channel 1
2. Play ascending pattern: C4-D4-E4-F4-G4
3. 8th notes at 110 BPM
4. Repeat pattern for 20 seconds
5. Listen for energy traveling through network
6. Try reverse: G4-F4-E4-D4-C4
```

---

### Recipe 4: "Polyrhythm" (5 minutes)
```
Need: Two keyboards or one hand + sequencer

Track 1 (Ch1): C4, quarter notes, vel=70
Track 2 (Ch1): E4, dotted quarters, vel=65

Play together → Complex interference!
```

---

## ❓ Troubleshooting

### "I don't hear anything!"

**Check:**
- ✅ Is MIDI connected? (Test with other synth)
- ✅ Is system powered on and running?
- ✅ Is output volume up?
- ✅ Are you playing loud enough? (Try vel=80)
- ✅ Try Channel 2 with held note instead

---

### "Sound is too quiet / barely builds up"

**Fixes:**
- ⬆️ Increase velocity (try 80-90)
- ⬇️ Lower damping in config (gamma = 0.3 instead of 0.5)
- ⏱️ Wait longer (10-15 seconds minimum)
- 🎵 Try faster rhythm (16th notes instead of quarters)

---

### "Sound distorts / clips / harsh"

**Fixes:**
- ⬇️ Reduce velocity (try 50-60)
- ⬆️ Increase damping in config (gamma = 0.8)
- 🎵 Slower rhythm (quarters instead of 16ths)
- 🔧 Lower coupling strength (0.3 instead of 0.6)

---

### "Sound decays instantly"

**Fixes:**
- ⬇️ Reduce damping (gamma = 0.2-0.3)
- Check configuration is loaded correctly
- Make sure resonator is actually running

---

### "Chaotic / unpredictable oscillations"

**Fixes:**
- ⬇️ Reduce coupling strength (0.2-0.4)
- ⬆️ Increase damping (gamma = 0.6-0.8)
- ⬇️ Lower velocity
- This can be a feature! Experiment with it.

---

## 🎼 Musical Ideas

### Idea 1: Build and Release
```
0:00-0:30  Play steady 8ths, vel=60  (buildup)
0:30-0:45  Increase to vel=80        (climax)
0:45-1:00  Stop playing              (decay/release)
```

---

### Idea 2: Dynamics Through Velocity
```
Start: vel=30 (whisper)
Build: vel=30 → 90 over 30 seconds
Peak:  vel=90 for 10 seconds
Fade:  vel=90 → 20 over 20 seconds
End:   Stop, let decay
```

---

### Idea 3: Call and Response
```
A section (8 bars):  Channel 1, rhythmic triggers
B section (8 bars):  Channel 2, held drone
C section (8 bars):  Both together!
```

---

### Idea 4: Network Exploration
```
Play each node individually (C0, C#0, D0, etc.)
Listen to how each sounds
Find your favorite nodes
Create patterns using those nodes
```

---

## 📋 Settings Cheat Sheet

### Good Starting Configuration
```json
{
  "carrier_freq_hz": 440,
  "coupling_strength": 0.4,
  "modes": [
    {"gamma": 0.4, "weight": 1.0},
    {"gamma": 0.5, "weight": 0.5},
    {"gamma": 0.6, "weight": 0.3},
    {"gamma": 0.7, "weight": 0.2}
  ]
}
```

**This gives:**
- Moderate buildup (not too fast, not too slow)
- Good resonance without chaos
- Clear pitch
- Nice harmonic content

---

### For Slower, Ethereal Sounds
```
gamma: 0.2-0.3 (all modes)
coupling: 0.25-0.35
velocity: 40-60
```

---

### For Faster, Rhythmic Sounds
```
gamma: 0.6-0.8 (all modes)
coupling: 0.5-0.6
velocity: 70-90
```

---

## 🚀 Next Steps

**After you're comfortable:**

1. ✅ Try different tempos (60-160 BPM)
2. ✅ Experiment with different rhythms (triplets, swing)
3. ✅ Play chords (multiple notes at once)
4. ✅ Try all the patterns in this manual
5. ✅ Record long sessions, listen back
6. ✅ Read full performance strategies docs

---

## 📖 Quick Reference

| Want... | Use... | Pattern | Velocity |
|---------|--------|---------|----------|
| **Easy start** | Ch1 | 8ths @ 100 BPM | 70 |
| **Fast buildup** | Ch1 | 16ths @ 130 BPM | 80 |
| **Slow buildup** | Ch1 | Quarters @ 80 BPM | 60 |
| **Smooth drone** | Ch2 | Hold 20+ sec | 50 |
| **Rhythmic** | Ch1 | Any steady pattern | 70-80 |
| **Ambient** | Ch2 | Hold long notes | 40-60 |

---

## 💡 Remember

**Channel 1:**
- Short, rhythmic hits
- Like playing drums/percussion
- Focus on steady timing
- Note duration doesn't matter

**Channel 2:**
- Hold notes down
- Like playing organ/pad
- Duration DOES matter
- Continuous waterfall effect

**Both:**
- Velocity controls strength
- System adds 10ms envelope automatically
- You just play normal keyboard patterns!
- Be patient - buildup takes 5-20 seconds

---

## ✨ The Magic

You're not playing notes like a piano.

You're **injecting energy** into a **network of coupled resonators**.

Each key press creates a **10ms wave** that ripples through the system.

With the right **timing** and **velocity**, these waves **reinforce** and create **beautiful resonant buildups**.

**Experiment. Listen. Adjust. Enjoy!** 🌊🎵

---

**Ready? Go play! Start with Recipe 1, then explore from there.** 🎹
