# Practical MIDI Keyboard Guide: What You Actually Play

**Key Clarification:** The 10ms envelope is **applied automatically by the system**, not something you control from your keyboard!

---

## How It Actually Works

### What Happens Internally (ESP32/System)

```
Your MIDI Keyboard/DAW          →  ESP32/System              →  Resonator
─────────────────────────────────────────────────────────────────────────

Channel 1 (Triggers):
  Note-On (any duration)        →  10ms excitation envelope  →  Poke!
  Note-Off                      →  IGNORED (doesn't matter)  →  (nothing)

Channel 2 (Drive):
  Note-On                       →  Start poke loop           →  Poke!
  (hold key)                    →  Every 100ms: send poke    →  Poke! Poke! Poke!...
  Note-Off                      →  Stop poke loop            →  (stops)
```

### What You Control

**Channel 1 (Triggers):**
- ✅ **Timing between notes** (quarter, 8th, 16th notes)
- ✅ **Velocity** (how hard you hit the key)
- ✅ **Which notes** (which nodes to excite)
- ❌ **NOT the envelope duration** (always 10ms internally)

**Channel 2 (Drive):**
- ✅ **How long you hold the key** (duration of waterfall)
- ✅ **Velocity** (strength of continuous excitation)
- ❌ **NOT the poke rate** (always 10 Hz = every 100ms internally)

---

## Practical Keyboard Patterns

### Pattern 1: "Staccato Pumping" (Channel 1)

**What to play:**
```
Play short, detached notes (staccato)
Duration doesn't matter - could be 50ms or 200ms
What matters: TIMING between note-ons

Example: 8th notes @ 120 BPM
Press C4, release, wait 250ms, press C4, release, wait 250ms...

From keyboard:
|C4   C4   C4   C4   C4   C4   C4   C4  |
 ▄    ▄    ▄    ▄    ▄    ▄    ▄    ▄
 Short press each time, steady rhythm

From resonator's perspective:
|poke poke poke poke poke poke poke poke|
 10ms 10ms 10ms 10ms (each is 10ms internally)
```

**How it feels to play:**
- Like playing a hi-hat pattern
- Short, rhythmic stabs
- Keep steady tempo
- Don't worry about how long you hold each note

**Keyboard technique:**
- Quick release after each press
- Or let notes overlap slightly - doesn't matter for Ch1!
- Focus on steady rhythm, not duration

---

### Pattern 2: "Machine Gun" (Channel 1, Fast)

**What to play:**
```
16th notes @ 130 BPM
Very fast, percussive strikes

Period: ~115ms between note-ons

From keyboard:
|C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4|
 ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
 Rapid fire!

Result: Dense excitation, fast buildup
```

**Keyboard technique:**
- Use finger tremolo (alternating fingers)
- Or use drum pad/MIDI controller
- Or sequence it in DAW (easier!)

---

### Pattern 3: "Sustained Waterfall" (Channel 2)

**What to play:**
```
Hold a single key down continuously

From keyboard:
|C2━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━|
 ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
 Hold for 10-30 seconds

Internally (automatic):
t=0ms:    poke (10ms envelope)
t=100ms:  poke (10ms envelope)
t=200ms:  poke (10ms envelope)
t=300ms:  poke (10ms envelope)
...
(10 Hz poke train)

Result: Continuous waterfall, 10 Hz excitation rate
```

**Keyboard technique:**
- Press and HOLD
- Use sustain pedal if easier
- Vary velocity for different waterfall strength
- Release when you want the waterfall to stop

---

## Real-World Keyboard Playing

### Using a MIDI Keyboard (Live Performance)

#### Setup 1: Single Note Pumping
```
Channel: 1 (set on keyboard or track)
Choose one key: Middle C (C4)

Play pattern (quarter notes):
Press  Wait   Press  Wait   Press  Wait   Press  Wait
 ▄     pause   ▄     pause   ▄     pause   ▄     pause

Count: 1... 2... 3... 4... 1... 2... 3... 4...

Each press triggers a 10ms poke internally
```

**Metronome-like approach:**
- Set metronome to 100 BPM
- Press key on each beat (quarter notes)
- Or press on "1 and 2 and 3 and 4 and" (8th notes)
- System creates 10ms pokes automatically

#### Setup 2: Chord Pumping
```
Press chord: C-E-G (3 notes)
Release
Wait (8th note)
Press chord again
Release
...

This excites 3 nodes simultaneously
Each gets a 10ms poke on each chord hit
```

#### Setup 3: Arpeggio Wave
```
Play ascending arpeggio (16th notes):
C4 - E4 - G4 - B4 - C4 - E4 - G4 - B4 - ...
 ▄   ▄   ▄   ▄   ▄   ▄   ▄   ▄

Each note triggers a different node
Wave propagates through network
```

### Using a Drum Pad Controller

**Great option for Channel 1 triggering!**

```
Map pads to different MIDI notes:
Pad 1 → C0 (Node 0)
Pad 2 → C#0 (Node 1)
Pad 3 → D0 (Node 2)
Pad 4 → D#0 (Node 3)
...

Play rhythmic patterns like drums:
Pad 1: X . X . X . X . (kick pattern)
Pad 3: . . X . . . X . (snare pattern)
Pad 5: X X X X X X X X (hi-hat pattern)

Each hit = 10ms poke internally
```

**Advantage:** Very expressive velocity control, natural rhythm

---

## DAW Sequencer Patterns (Easier!)

### Why DAW is Better for Precise Patterns

**DAW advantages:**
1. ✅ Perfect timing (computer-precise)
2. ✅ Easy to create polyrhythms
3. ✅ Velocity automation
4. ✅ Loop/repeat without fatigue
5. ✅ Edit and experiment

**From keyboard:** Hard to play perfect 16th notes for minutes
**From DAW:** Easy! Just draw the pattern once, loop.

### DAW Pattern: "Steady Pump"

**In your DAW piano roll:**

```
Draw notes at 8th note intervals:
Grid: 8th notes
Note: C4
Velocity: 70
Duration: 16th note (or any short duration - doesn't matter!)

|●   ●   ●   ●   |●   ●   ●   ●   |
 C4  C4  C4  C4   C4  C4  C4  C4

The short duration (16th) is just visual
System only cares about note-on timing
```

**Key insight:** In DAW, you can make notes as short as you want (1/64, 1/128). The system still applies 10ms envelope. The visual length doesn't matter for Channel 1!

### DAW Pattern: "Held Drone"

**For Channel 2:**

```
Draw ONE long note:
Duration: 4 bars (or more)
Note: C2
Velocity: 50

|━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━|
 C2 (held for entire duration)

Internally: Pokes every 100ms automatically
You just hold the note, system does the rest
```

---

## What About Note Duration? Does It Matter?

### Channel 1 (Triggers): NO!

```
Scenario A (very short notes):
|●●●●|  (32nd notes, released immediately)

Scenario B (longer notes, overlap):
|▬▬▬▬|  (8th notes, held until next note)

SAME RESULT! Both create 10ms pokes at same timing.
```

**Why:** Channel 1 only listens to note-ON events. Note-OFF is ignored.

### Channel 2 (Drive): YES!

```
Scenario A (short note):
|●  |  (release after 250ms)
→ 2-3 pokes sent (250ms / 100ms = 2.5 pokes)

Scenario B (held note):
|▬▬▬▬| (held for 4 seconds)
→ ~40 pokes sent (4000ms / 100ms = 40 pokes)
```

**Why:** Channel 2 sends pokes continuously while note is held.

---

## Multiples of 10ms: Can We Use Them?

**Your question:** Can we use multiples like 20ms, 30ms, 100ms envelopes?

**Answer:** The system currently uses fixed 10ms (Channel 1) and 100ms spacing (Channel 2). But yes, you can approximate different effective durations:

### Effective "Longer Poke" Simulation

**Method 1: Velocity Ramp (Channel 1)**
```
Instead of one note at vel=100,
Send 3 quick notes with ramped velocity:

t=0ms:   Note-on, vel=70  → 10ms poke
t=15ms:  Note-on, vel=90  → 10ms poke
t=30ms:  Note-on, vel=70  → 10ms poke

Effective: ~45ms of excitation with attack/decay shape
```

**Method 2: Cluster Notes (Channel 1)**
```
Send multiple notes very close together:
t=0ms:   C4, vel=80
t=5ms:   C4, vel=80
t=10ms:  C4, vel=80

Effective: ~25ms excitation burst
(if system can handle timing this tight)
```

**Method 3: Use Channel 2 for Longer**
```
Want ~500ms effective poke?
Use Channel 2, hold for 500ms
→ 5 pokes sent (every 100ms)
→ Longer effective excitation
```

---

## Keyboard Pattern Examples (What It Feels Like)

### Example 1: "Heartbeat Pump"
```
Tempo: 70 BPM
Pattern: Quarter notes (like a slow heartbeat)

Count:  1 . . . 2 . . . 3 . . . 4 . . .
Press:  C4      C4      C4      C4
        ▄       ▄       ▄       ▄

Feels like: Gentle, pulsing heartbeat
Buildup: Slow (10-15 seconds)
```

### Example 2: "Galloping Rhythm"
```
Tempo: 120 BPM
Pattern: 8th notes with accents

Count:  1 & 2 & 3 & 4 &
Press:  C4  C4  C4  C4  C4  C4  C4  C4
Vel:    80  60  80  60  80  60  80  60
        ▄   ▄   ▄   ▄   ▄   ▄   ▄   ▄

Feels like: Galloping horse, steady rhythm
Buildup: Medium (5-8 seconds)
```

### Example 3: "Tremolo"
```
Tempo: 140 BPM
Pattern: 16th notes (rapid alternation)

Count:  1 e & a 2 e & a 3 e & a 4 e & a
Press:  C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4C4
        ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄

Feels like: Finger tremolo, very fast
Buildup: Fast (2-4 seconds)
```

### Example 4: "Sustained Drone" (Ch2)
```
Pattern: Hold one key

Press C2, hold for 20 seconds, release
         ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

Feels like: Sustain pedal, organ note
Internal: 200 pokes sent (20s / 0.1s = 200)
Buildup: Very slow, smooth (15-20 seconds)
```

---

## Practical Tips

### 1. Start with Quarter Notes
```
Easiest to play accurately
Count: 1 - 2 - 3 - 4 - 1 - 2 - 3 - 4
Press on each number
Natural, musical feel
```

### 2. Use Metronome
```
Set to your target tempo
Play on clicks (or subdivisions)
Builds muscle memory
Helps maintain steady timing
```

### 3. Sequencer is Your Friend
```
For complex patterns: Use DAW
For live feel: Use keyboard
For polyrhythms: Definitely DAW!
```

### 4. Don't Worry About Duration
```
For Channel 1: Just hit the key, let go
System handles the 10ms envelope
Focus on rhythm timing, not how long you hold
```

### 5. For Sustained Effects, Use Channel 2
```
Want continuous excitation? Channel 2!
Want rhythmic hits? Channel 1!
```

---

## Summary: What Pattern Resembles What You Described?

**Your question:** "What keyboard pattern resembles 10ms pokes?"

**Answer:**

### For Channel 1 (Triggers):
**Any staccato (short) rhythm:**
- Quarter notes = gentle pulse (like slow heartbeat)
- 8th notes = steady pump (like walking pace) ← **Most common**
- 16th notes = rapid fire (like running, tremolo)

**Play technique:**
- Short, detached notes (staccato)
- Like playing a hi-hat or percussion
- Duration doesn't matter - just steady rhythm

### For Channel 2 (Drive):
**Sustained notes:**
- Hold key down = continuous waterfall
- Like playing organ or pad sound
- System sends 10ms pokes every 100ms automatically

---

## Quick Recipe: Try This NOW

**Channel 1, Keyboard:**
1. Set your keyboard to MIDI Channel 1
2. Choose Middle C (C4)
3. Set metronome to 100 BPM
4. Play 8th notes: "1-and-2-and-3-and-4-and"
5. Short, staccato hits
6. Listen for buildup over 5-10 seconds

**Channel 2, Keyboard:**
1. Set your keyboard to MIDI Channel 2
2. Choose low C (C2)
3. Press and HOLD for 20 seconds
4. Don't worry about rhythm - just hold!
5. Listen for slow, smooth buildup

That's it! The system handles the 10ms envelopes and 100ms timing internally. You just play rhythms! 🎹
