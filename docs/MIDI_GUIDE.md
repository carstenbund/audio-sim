# MIDI Input Guide

Real-time MIDI control for the Modal Network Synthesizer.

## Overview

The modal network can be played as a real-time instrument using MIDI input. Different MIDI channels control different aspects of the system:

- **Channel 1**: Trigger notes (brief events)
- **Channel 2**: Drive notes (sustained standing waves)

## Installation

```bash
pip install mido python-rtmidi
```

## Quick Start

1. **List available MIDI ports:**
   ```bash
   python experiments/audio_sonification.py --list-midi
   ```

2. **Start MIDI mode:**
   ```bash
   python experiments/audio_sonification.py --midi
   ```

3. **Specify a MIDI port:**
   ```bash
   python experiments/audio_sonification.py --midi --port "Your MIDI Device"
   ```

## How It Works

### Note Mapping

Each MIDI note is mapped to one of the 8 network nodes using modulo:
```
note % 8 = node
```

Examples:
- MIDI note 60 (C4) → node 4
- MIDI note 61 (C#4) → node 5
- MIDI note 62 (D4) → node 6
- ...
- MIDI note 68 (G#4) → node 4 (wraps around)

### Channel Behaviors

#### Channel 1: Trigger Notes
- **Purpose**: Play notes on the network
- **Duration**: 100ms fixed duration
- **Effect**: Sets node frequency and velocity
- **Use case**: Play melodies, rhythms

When you play a note on channel 1:
1. The note's frequency is assigned to a node
2. The velocity gates the amplitude
3. The note automatically turns off after 100ms
4. The network's state shapes the sound

#### Channel 2: Drive Notes
- **Purpose**: Sustain standing waves
- **Duration**: Held as long as note is pressed
- **Effect**: Drives network at that frequency
- **Use case**: Bass notes, drones, resonance

When you play a note on channel 2:
1. The network is driven at that frequency
2. Creates a sustained standing wave
3. Continues until note is released
4. Can layer multiple notes

### Audio Output

The audio output is 8-channel (one per node):
- **Pitch**: From MIDI note frequency
- **Amplitude**: From network state |a[j,0]|
- **Velocity**: Gates sound on/off
- **Result**: Network dynamics shape each note

## Usage Examples

### Example 1: Simple Melody
1. Set your MIDI keyboard to channel 1
2. Play notes - each triggers a brief event
3. Hear the network respond with organic amplitude

### Example 2: Sustained Drone
1. Set your MIDI keyboard to channel 2
2. Hold a chord
3. The network maintains standing waves at those frequencies
4. Add channel 1 notes for melodic phrases over the drone

### Example 3: Two-Hand Performance
- **Left hand** (channel 2): Hold bass notes for sustained drive
- **Right hand** (channel 1): Play melody that triggers on the network

## Technical Details

### Timing
- Simulation runs at 1 kHz (1ms timestep)
- Audio output at 48 kHz
- MIDI polling at 1ms intervals
- Ultra-low latency

### Network Parameters
```python
N=8            # 8 nodes (8 audio channels)
coupling=1.5   # Network coupling strength
omega=[20.0, 31.4]  # Mode frequencies
gamma=[0.6, 0.6]    # Damping
```

### Amplitude Control
- Network state provides base amplitude
- Normalized to prevent any node dominating
- Smoothed with one-pole filter (α=0.12)
- Multiplied by MIDI velocity
- Clipped to MAX_AMPLITUDE=0.7

## Troubleshooting

### No MIDI Ports
```
No MIDI input ports available!
```
**Solution**:
- Connect a MIDI device or virtual MIDI bus
- On macOS: Use IAC Driver
- On Linux: Use `a2jmidid` or `virmidi`

### PortAudio Error
```
sounddevice not available (PortAudio library not found)
```
**Solution**:
- Install PortAudio system library
- On macOS: `brew install portaudio`
- On Linux: `sudo apt-get install portaudio19-dev`

### No Sound
- Check MIDI channel (1 or 2)
- Verify velocity > 0
- Check audio output device
- Ensure 8-channel output is supported

## Advanced: Virtual MIDI Setup

### macOS
1. Open Audio MIDI Setup
2. Go to Window → Show MIDI Studio
3. Double-click "IAC Driver"
4. Enable "Device is online"
5. Use "IAC Driver Bus 1" as port

### Linux
```bash
# Load virtual MIDI kernel module
sudo modprobe snd-virmidi

# Or use a2jmidid for JACK
a2jmidid -e &
```

### Windows
Install [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html)

## CSV Mode (Default)

To use the original CSV score playback:
```bash
python experiments/audio_sonification.py
```

Edit `score.csv` to change the notes.
