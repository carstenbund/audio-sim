"""
Audio Sonification of Modal Attractors

Real-time sonification of standing-wave memory states through spatial
mode order parameters. This script demonstrates audible coherence,
attractor switching, and recovery from perturbation.

Key Concept:
- Output 0 (220 Hz): q=0 mode (all-in-phase pattern)
- Output 1 (330 Hz): q=π mode (alternating-phase pattern)
- Amplitude tracks order parameter magnitude
- You HEAR the attractor state directly

Requirements:
- macOS or Linux with audio output
- Multiple audio outputs (2+ channels)
- sounddevice package

Fallback:
- Generates WAV files when no audio device available
"""

import numpy as np
import threading
import time
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from src.network import ModalNetwork, NetworkParams
from src.drive import make_drive
from src.triggers import TriggerSong

# Try to import sounddevice, fallback to WAV generation
try:
    import sounddevice as sd
    HAS_AUDIO = True
except (ImportError, OSError) as e:
    HAS_AUDIO = False
    print(f"Warning: sounddevice not available ({e}). Will generate WAV files instead.")

# Try to import scipy for WAV writing
try:
    from scipy.io import wavfile
    HAS_SCIPY = True
except ImportError:
    HAS_SCIPY = False


# ==========================
# GLOBAL PARAMETERS
# ==========================
SIMULATION_DT = 1e-3          # 1 kHz simulation rate
AUDIO_FS = 48000              # 48 kHz audio sample rate

# ==========================
# VOICE ROUTING CONFIGURATION
# ==========================
# 5 voices tuned to a harmonic chord (A major with perfect fifths)
# Voice 0: A3 = 220.00 Hz (root)
# Voice 1: E4 = 329.63 Hz (perfect fifth)
# Voice 2: A4 = 440.00 Hz (octave)
# Voice 3: C#5 = 554.37 Hz (major third above A4)
# Voice 4: E5 = 659.26 Hz (fifth above A4)
VOICE_PITCHES = [220.00, 329.63, 440.00, 554.37, 659.26]
NUM_VOICES = len(VOICE_PITCHES)

# Voice routing presets: which q-mode feeds each voice
# Preset A: emphasis on low q-modes
ROUTING_PRESET_A = [0, 4, 2, 3, 1]  # voice_i gets amplitude from S_all[q_i]
# Preset B: rotated mapping
ROUTING_PRESET_B = [0, 2, 4, 1, 3]
# Preset C: spread across spectrum
ROUTING_PRESET_C = [0, 1, 2, 3, 4]

# Amplitude smoothing for click-free transitions (one-pole filter alpha)
AMP_SMOOTH_ALPHA = 0.1  # 0.05-0.2 recommended
MAX_AMPLITUDE = 0.5     # Headroom for safety (lower since we have 5 voices)
DEVICE_INDEX = None     # Auto-detect, or set manually

# Experiment timeline
SETTLE_TIME = 4.0             # Let first attractor settle
PERTURB_TIME = 30.0           # Apply perturbation
SWITCH_TIME = 70.0            # Switch to different drive pattern
TOTAL_TIME = 120.0            # Total duration

# Bar-based routing switching
BAR_DURATION = 4.0            # Duration of each "bar" in seconds
ROUTING_SCHEDULE = {
    0: ROUTING_PRESET_A,      # 0-24s: Preset A
    6: ROUTING_PRESET_B,      # 24-48s: Preset B (bar 6)
    12: ROUTING_PRESET_C,     # 48-72s: Preset C (bar 12)
    18: ROUTING_PRESET_A,     # 72-96s: Back to A (bar 18)
    24: ROUTING_PRESET_B,     # 96-120s: Back to B (bar 24)
}

# ==========================
# SHARED STATE
# ==========================
class SharedState:
    """Thread-safe shared state between simulation and audio."""

    def __init__(self, N: int = 8):
        self.lock = threading.Lock()
        self.order_params = {'q0': 0.0, 'qpi': 0.0}
        self.S_all = np.zeros(N, dtype=np.float32)  # Full q-spectrum
        self.running = True
        self.phase = np.zeros(NUM_VOICES)  # Carrier phase tracking for 5 voices

        # Voice routing and amplitude smoothing
        self.voice_routing = ROUTING_PRESET_A.copy()  # Current routing
        self.amp_smoothed = np.zeros(NUM_VOICES, dtype=np.float32)  # Smoothed amplitudes

        # For WAV generation
        self.audio_buffer = []

    def update_order_params(self, q0: float, qpi: float, S_all: np.ndarray):
        with self.lock:
            self.order_params = {'q0': q0, 'qpi': qpi}
            self.S_all = S_all.copy()

    def get_order_params(self) -> dict:
        with self.lock:
            return self.order_params.copy()

    def get_S_all(self) -> np.ndarray:
        with self.lock:
            return self.S_all.copy()

    def set_voice_routing(self, routing: list):
        """Set voice routing at bar boundaries."""
        with self.lock:
            self.voice_routing = routing.copy()

    def get_voice_routing(self) -> list:
        with self.lock:
            return self.voice_routing.copy()

    def stop(self):
        with self.lock:
            self.running = False

    def is_running(self) -> bool:
        with self.lock:
            return self.running


# ==========================
# SIMULATION THREAD
# ==========================
def simulation_loop(state: SharedState, params: NetworkParams, verbose: bool = True):
    """
    Real-time simulation loop computing order parameters.

    Timeline:
    - 0-4s: Drive nodes [0,1] (ADJACENT attractor)
    - 6s: Apply perturbation
    - 10s: Switch to nodes [0,4] (OPPOSITE attractor)
    - Voice routing switches every 6 bars (24s)
    """
    net = ModalNetwork(params, seed=42)
    t = 0.0
    step_count = 0

    # Event tracking
    perturbed = False
    switched = False
    current_bar = -1

    # Trigger song setup
    song = TriggerSong(bpm=90.0, duration=TOTAL_TIME)
    events = song.events()
    event_idx = 0

    if verbose:
        print("\n=== Simulation Timeline ===")
        print(f"0.0-{SETTLE_TIME}s: ADJACENT attractor (nodes [0,1])")
        print(f"{PERTURB_TIME}s: Perturbation applied")
        print(f"{SWITCH_TIME}s: Switch to OPPOSITE attractor (nodes [0,4])")
        print(f"{TOTAL_TIME}s: End")
        print(f"\n=== Trigger Song Movements ===")
        print(f"0-24s: INTRO - Sparse phase kicks rotating around ring")
        print(f"24-60s: DEVELOPMENT - Mixed triggers (impulse + phase + noise)")
        print(f"60-96s: COMPLEXITY - Dense heterodyne probes + syncopation")
        print(f"96-120s: RESOLUTION - Simple impulses calming down\n")

    while state.is_running() and t < TOTAL_TIME:
        # Check for bar boundary and update routing if scheduled
        bar_idx = int(t / BAR_DURATION)
        if bar_idx != current_bar:
            current_bar = bar_idx
            if bar_idx in ROUTING_SCHEDULE:
                new_routing = ROUTING_SCHEDULE[bar_idx]
                state.set_voice_routing(new_routing)
                if verbose:
                    print(f"[{t:.2f}s] BAR {bar_idx}: Voice routing → {new_routing}")

        # Determine drive pattern
        if t < SWITCH_TIME:
            target_nodes = [0, 1]  # ADJACENT pattern favors q=0
        else:
            target_nodes = [0, 4]  # OPPOSITE pattern favors q=π
            if not switched and verbose:
                print(f"[{t:.2f}s] SWITCH: Driving nodes {target_nodes}")
                switched = True

        drive = make_drive(t, target_nodes, params.N)
        net.step(drive)

        # === Participatory trigger song (state perturbations, not drive) ===
        while event_idx < len(events) and t >= events[event_idx].t_on:
            ev = events[event_idx]
            if ev.kind == "noise":
                net.perturb_nodes(ev.strength, ev.target_nodes, mode=ev.mode, kind="noise", seed=123)
            elif ev.kind == "impulse":
                net.perturb_nodes(ev.strength, ev.target_nodes, mode=ev.mode, kind="impulse", phase=ev.phase)
            elif ev.kind == "phase_kick":
                net.phase_kick(ev.delta_phi, ev.target_nodes, mode=ev.mode or 0)
            elif ev.kind == "heterodyne":
                net.heterodyne_probe(ev.target_nodes, ev.mode_a, ev.mode_b, ev.out_mode, strength=ev.strength)
            event_idx += 1

        # Apply perturbation
        if t >= PERTURB_TIME and not perturbed:
            if verbose:
                print(f"[{t:.2f}s] PERTURBATION: strength=0.5")
            net.perturb(0.5)
            perturbed = True

        # Compute order parameters
        q0 = net.order_parameter_q0(mode=0)
        qpi = net.order_parameter_qpi(mode=0)
        S_all = net.order_parameters_all(mode=0)
        state.update_order_params(q0, qpi, S_all)

        # Progress output
        if verbose and step_count % 1000 == 0:
            print(f"[{t:.2f}s] q0={q0:.3f}, qπ={qpi:.3f}")

        t += params.dt
        step_count += 1
        time.sleep(params.dt)  # Real-time pacing

    if verbose:
        print(f"\n[{t:.2f}s] Simulation complete")
    state.stop()


# ==========================
# AUDIO CALLBACK
# ==========================
def soft_saturate(x: np.ndarray, gain: float = 2.0) -> np.ndarray:
    """
    Soft saturation using tanh for pleasant clipping.

    Args:
        x: Input signal
        gain: Pre-saturation gain (higher = more compression)

    Returns:
        Softly saturated signal
    """
    return np.tanh(gain * x) / gain


def make_audio_callback(state: SharedState):
    """
    Create 5-voice polyphonic audio callback with harmonic tuning.

    Each voice:
    - Amplitude: S_all[voice_routing[i]] (smoothed)
    - Pitch: VOICE_PITCHES[i] (fixed harmonic tuning)
    - Output: soft saturated sine wave

    Voice routing maps q-modes to output channels:
    - voice_routing[i] = q-mode index that feeds voice i
    """

    def audio_callback(outdata, frames, time_info, status):
        if status and status.input_overflow:
            print(f"Audio status: {status}")

        # Time vector for this block
        t = np.arange(frames) / AUDIO_FS

        # Get current q-spectrum and routing
        S_all = state.get_S_all()
        routing = state.get_voice_routing()

        # Generate each voice
        for i in range(NUM_VOICES):
            # Get raw amplitude from routed q-mode
            q_idx = routing[i]
            amp_raw = S_all[q_idx] if q_idx < len(S_all) else 0.0

            # Clip to safe range
            amp_raw = np.clip(amp_raw, 0.0, MAX_AMPLITUDE * 2)  # Allow headroom for smoothing

            # One-pole lowpass filter for smooth amplitude transitions
            with state.lock:
                state.amp_smoothed[i] += AMP_SMOOTH_ALPHA * (amp_raw - state.amp_smoothed[i])
                amp_smooth = state.amp_smoothed[i]

            # Clip smoothed amplitude
            amp_smooth = np.clip(amp_smooth, 0.0, MAX_AMPLITUDE)

            # Generate carrier
            carrier = amp_smooth * np.sin(2 * np.pi * VOICE_PITCHES[i] * t + state.phase[i])

            # Soft saturation for pleasant sound
            outdata[:, i] = soft_saturate(carrier, gain=1.5)

            # Update phase
            state.phase[i] += 2 * np.pi * VOICE_PITCHES[i] * frames / AUDIO_FS
            state.phase[i] = state.phase[i] % (2 * np.pi)  # Wrap phase

        # Store for WAV generation if needed
        if not HAS_AUDIO:
            state.audio_buffer.append(outdata.copy())

    return audio_callback


# ==========================
# WAV GENERATION FALLBACK
# ==========================
def generate_wav_simulation(state: SharedState, params: NetworkParams,
                           output_path: str = "audio_sonification.wav"):
    """
    Generate WAV file when no audio device available.
    Runs simulation and renders audio offline.
    """
    if not HAS_SCIPY:
        print("Error: scipy required for WAV generation. Install with: pip install scipy")
        return

    print("\n=== Generating WAV file (no audio device detected) ===")

    net = ModalNetwork(params, seed=42)

    # Pre-allocate audio buffer for 5 voices
    total_samples = int(TOTAL_TIME * AUDIO_FS)
    audio_data = np.zeros((total_samples, NUM_VOICES), dtype=np.float32)

    # Simulation to audio sample ratio
    sim_to_audio_ratio = int(AUDIO_FS * params.dt)

    phases = np.zeros(NUM_VOICES)
    amp_smoothed = np.zeros(NUM_VOICES, dtype=np.float32)
    sim_step = 0
    audio_idx = 0
    current_bar = -1

    print("Simulating...")

    perturbed = False
    switched = False

    # Trigger song setup
    song = TriggerSong(bpm=90.0, duration=TOTAL_TIME)
    events = song.events()
    event_idx = 0

    print(f"Generating {len(events)} trigger events over {TOTAL_TIME}s\n")

    for sim_step in range(int(TOTAL_TIME / params.dt)):
        t = sim_step * params.dt

        # Check for bar boundary and update routing if scheduled
        bar_idx = int(t / BAR_DURATION)
        if bar_idx != current_bar:
            current_bar = bar_idx
            if bar_idx in ROUTING_SCHEDULE:
                new_routing = ROUTING_SCHEDULE[bar_idx]
                state.set_voice_routing(new_routing)
                print(f"[{t:.2f}s] BAR {bar_idx}: Voice routing → {new_routing}")

        # Drive pattern
        if t < SWITCH_TIME:
            target_nodes = [0, 1]
        else:
            target_nodes = [0, 4]
            if not switched:
                print(f"[{t:.2f}s] Switch to nodes {target_nodes}")
                switched = True

        drive = make_drive(t, target_nodes, params.N)
        net.step(drive)

        # === Participatory trigger song (state perturbations, not drive) ===
        while event_idx < len(events) and t >= events[event_idx].t_on:
            ev = events[event_idx]
            if ev.kind == "noise":
                net.perturb_nodes(ev.strength, ev.target_nodes, mode=ev.mode, kind="noise", seed=123)
            elif ev.kind == "impulse":
                net.perturb_nodes(ev.strength, ev.target_nodes, mode=ev.mode, kind="impulse", phase=ev.phase)
            elif ev.kind == "phase_kick":
                net.phase_kick(ev.delta_phi, ev.target_nodes, mode=ev.mode or 0)
            elif ev.kind == "heterodyne":
                net.heterodyne_probe(ev.target_nodes, ev.mode_a, ev.mode_b, ev.out_mode, strength=ev.strength)
            event_idx += 1

        # Perturbation
        if t >= PERTURB_TIME and not perturbed:
            print(f"[{t:.2f}s] Perturbation applied")
            net.perturb(0.5)
            perturbed = True

        # Get order parameters
        q0 = net.order_parameter_q0(mode=0)
        qpi = net.order_parameter_qpi(mode=0)
        S_all = net.order_parameters_all(mode=0)

        # Get current routing
        routing = state.get_voice_routing()

        # Generate audio samples for this simulation step
        for _ in range(sim_to_audio_ratio):
            if audio_idx >= total_samples:
                break

            # Generate each voice
            for i in range(NUM_VOICES):
                # Get raw amplitude from routed q-mode
                q_idx = routing[i]
                amp_raw = S_all[q_idx] if q_idx < len(S_all) else 0.0
                amp_raw = np.clip(amp_raw, 0.0, MAX_AMPLITUDE * 2)

                # One-pole lowpass filter
                amp_smoothed[i] += AMP_SMOOTH_ALPHA * (amp_raw - amp_smoothed[i])
                amp_smooth = np.clip(amp_smoothed[i], 0.0, MAX_AMPLITUDE)

                # Generate carrier
                carrier = amp_smooth * np.sin(phases[i])

                # Soft saturation
                audio_data[audio_idx, i] = soft_saturate(np.array([carrier]), gain=1.5)[0]

                # Update phase
                phases[i] += 2 * np.pi * VOICE_PITCHES[i] / AUDIO_FS

            audio_idx += 1

        if sim_step % 1000 == 0:
            print(f"[{t:.2f}s] q0={q0:.3f}, qπ={qpi:.3f}")

    # Write WAV file
    audio_data_int = (audio_data * 32767).astype(np.int16)
    wavfile.write(output_path, AUDIO_FS, audio_data_int)

    print(f"\n✓ Generated: {output_path}")
    print(f"  Duration: {TOTAL_TIME}s")
    print(f"  Sample rate: {AUDIO_FS} Hz")
    print(f"  Channels: {NUM_VOICES}")
    for i, pitch in enumerate(VOICE_PITCHES):
        print(f"    Voice {i}: {pitch:.2f} Hz")


# ==========================
# REAL-TIME AUDIO
# ==========================
def run_realtime_audio(params: NetworkParams):
    """Run real-time audio output with live simulation."""

    print("\n=== Real-Time Audio Sonification ===")
    print(f"Sample rate: {AUDIO_FS} Hz")
    print(f"Channels: {NUM_VOICES}")
    print("\nVoice Tuning (A major chord):")
    for i, pitch in enumerate(VOICE_PITCHES):
        print(f"  Voice {i}: {pitch:.2f} Hz")
    print(f"\nDuration: {TOTAL_TIME}s")
    print("\nListening guide:")
    print("  - 0-24s: Gentle phase kick inquiries rotating around ring")
    print("           Routing Preset A - emphasis on low q-modes")
    print("  - 24-48s: Routing Preset B - rotated mapping")
    print("  - At ~30s: Strong perturbation - hear system wobble and recover")
    print("  - 48-72s: Routing Preset C - spread across spectrum")
    print("  - At ~70s: Attractor switch - voices shift between modes")
    print("  - 72-96s: Back to Preset A")
    print("  - 96-120s: Back to Preset B - system settles")
    print("\nYou'll hear:")
    print("  - Polyphonic harmony from 5 tuned voices")
    print("  - Different q-modes 'singing' through the chord")
    print("  - Melodic movement from routing changes\n")

    # Check available devices
    try:
        devices = sd.query_devices()
        print("Available audio devices:")
        print(devices)
        print()
    except Exception as e:
        print(f"Could not query audio devices: {e}")

    state = SharedState(N=params.N)

    # Start simulation thread
    sim_thread = threading.Thread(
        target=simulation_loop,
        args=(state, params, True),
        daemon=True
    )
    sim_thread.start()

    # Start audio stream
    try:
        callback = make_audio_callback(state)

        with sd.OutputStream(
            samplerate=AUDIO_FS,
            channels=NUM_VOICES,
            callback=callback,
            device=DEVICE_INDEX,
            dtype='float32',
        ):
            print("▶ Audio stream started. Listening...\n")

            # Wait for simulation to complete
            sim_thread.join(timeout=TOTAL_TIME + 2.0)

            # Brief tail for final state
            time.sleep(1.0)

        print("\n✓ Audio stream complete")

    except Exception as e:
        print(f"\n✗ Audio error: {e}")
        print("Falling back to WAV generation...")
        state.stop()
        generate_wav_simulation(state, params)


# ==========================
# MAIN
# ==========================
def main():
    """Main entry point."""

    print("=" * 70)
    print("MODAL ATTRACTOR AUDIO SONIFICATION")
    print("=" * 70)
    print("\nThis experiment sonifies standing-wave memory through spatial modes.")
    print("You will HEAR attractors lock, break, and recover in real-time.")

    # Setup parameters
    params = NetworkParams(
        N=8,
        K=2,
        dt=SIMULATION_DT,
        coupling=1.5,
        omega=np.array([20.0, 31.4]),
        gamma=np.array([0.6, 0.6]),
    )

    if HAS_AUDIO:
        # Check if audio device is actually available
        try:
            sd.query_devices()
            run_realtime_audio(params)
        except Exception as e:
            print(f"\nNo audio device available: {e}")
            print("Falling back to WAV generation...\n")
            state = SharedState(N=params.N)
            generate_wav_simulation(state, params)
    else:
        # No sounddevice, generate WAV
        state = SharedState(N=params.N)
        generate_wav_simulation(state, params)

    print("\n" + "=" * 70)
    print("INTERPRETATION")
    print("=" * 70)
    print(f"""
What you heard (or will hear in the WAV):

=== THE POLYPHONIC VOICE SYSTEM ===

You just experienced {NUM_VOICES} voices tuned to an A major chord:
  Voice 0: A3  (220.00 Hz) - Root
  Voice 1: E4  (329.63 Hz) - Perfect fifth
  Voice 2: A4  (440.00 Hz) - Octave
  Voice 3: C#5 (554.37 Hz) - Major third above A4
  Voice 4: E5  (659.26 Hz) - Fifth above A4

Each voice's AMPLITUDE is controlled by a spatial q-mode (S_q).
The voice routing maps q-modes to pitches, creating melody through routing.

=== TIMELINE ===

1. INTRO (0-24s) - Routing Preset A:
   - Sparse phase kicks rotate around the ring
   - Preset A emphasizes low q-modes: {ROUTING_PRESET_A}
   - Listen for which voices are active (which q-modes dominate)

2. DEVELOPMENT (24-48s) - Routing Preset B:
   - Routing switches to: {ROUTING_PRESET_B}
   - Mixed triggers (impulse + phase + noise)
   - MELODIC CHANGE from routing, not pitch shift

3. PERTURBATION (~30s):
   - Strong perturbation applied
   - All voices wobble - hear distributed recovery
   - Amplitude smoothing prevents harsh clicks

4. COMPLEXITY (48-72s) - Routing Preset C:
   - Routing spreads across spectrum: {ROUTING_PRESET_C}
   - Dense heterodyne probes create nonlinear interactions
   - Most complex section - mode competition audible

5. ATTRACTOR SWITCH (70s):
   - Drive pattern changes from ADJACENT [0,1] to OPPOSITE [0,4]
   - Hear voices shift as q-spectrum redistributes
   - Different spatial patterns become dominant

6. RECAPITULATION (72-96s) - Back to Preset A:
   - Return to original routing
   - System settles in new attractor basin

7. RESOLUTION (96-120s) - Preset B:
   - Simple impulses calm the system
   - Final routing rotation

=== WHAT THIS DEMONSTRATES ===

This is AUDIBLE PROOF of:

1. POLYPHONY: Each spatial q-mode is a "voice" that can be heard
   - Not a metaphor - S_q is a real physical observable
   - Harmony emerges from coherence structure

2. MELODY BY ROUTING: Musical movement from computation
   - Fixed pitches (harmonic tuning)
   - Routing changes create melodic contour
   - This is "melody native to the wavefield"

3. AMPLITUDE SMOOTHING: Click-free transitions
   - One-pole lowpass filter (α={AMP_SMOOTH_ALPHA})
   - Soft saturation for pleasant sound
   - Musical production quality matters!

4. HUMAN-READABLE COMPUTATION:
   - Same features (S_q spectrum) you'd use for a classifier
   - But now AUDIBLE to a human operator
   - You can HEAR what the system is computing

5. PARTICIPATORY INQUIRY:
   - Triggers are "questions" posed to the system
   - Amplitude changes are "answers"
   - The routing is "how you listen"

This is NOT sonification-as-decoration. This is the system's computational
state rendered as organized sound—a substrate where hearing IS reading.
""")

    print("=" * 70)


if __name__ == "__main__":
    main()
