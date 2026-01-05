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
CARRIER_FREQS = [220.0, 330.0]  # Hz for q=0 and q=π
MAX_AMPLITUDE = 0.7           # Headroom for safety
DEVICE_INDEX = None           # Auto-detect, or set manually

# Experiment timeline
SETTLE_TIME = 4.0             # Let first attractor settle
PERTURB_TIME = 30.0           # Apply perturbation
SWITCH_TIME = 70.0            # Switch to different drive pattern
TOTAL_TIME = 120.0            # Total duration

# ==========================
# SHARED STATE
# ==========================
class SharedState:
    """Thread-safe shared state between simulation and audio."""

    def __init__(self):
        self.lock = threading.Lock()
        self.order_params = {'q0': 0.0, 'qpi': 0.0}
        self.running = True
        self.phase = np.zeros(2)  # Carrier phase tracking

        # For WAV generation
        self.audio_buffer = []

    def update_order_params(self, q0: float, qpi: float):
        with self.lock:
            self.order_params = {'q0': q0, 'qpi': qpi}

    def get_order_params(self) -> dict:
        with self.lock:
            return self.order_params.copy()

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
    """
    net = ModalNetwork(params, seed=42)
    t = 0.0
    step_count = 0

    # Event tracking
    perturbed = False
    switched = False

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
        state.update_order_params(q0, qpi)

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
def make_audio_callback(state: SharedState):
    """
    Create audio callback that sonifies order parameters.

    Output routing:
    - Channel 0: q=0 mode amplitude → 220 Hz carrier
    - Channel 1: q=π mode amplitude → 330 Hz carrier
    """

    def audio_callback(outdata, frames, time_info, status):
        if status and status.input_overflow:
            print(f"Audio status: {status}")

        # Time vector for this block
        t = np.arange(frames) / AUDIO_FS

        # Get current order parameters
        params = state.get_order_params()
        q0_amp = np.clip(params['q0'], 0.0, MAX_AMPLITUDE)
        qpi_amp = np.clip(params['qpi'], 0.0, MAX_AMPLITUDE)

        # Generate carriers with current phase continuity
        for i, (fc, amp) in enumerate([(CARRIER_FREQS[0], q0_amp),
                                        (CARRIER_FREQS[1], qpi_amp)]):
            outdata[:, i] = amp * np.sin(2 * np.pi * fc * t + state.phase[i])
            state.phase[i] += 2 * np.pi * fc * frames / AUDIO_FS
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

    # Pre-allocate audio buffer
    total_samples = int(TOTAL_TIME * AUDIO_FS)
    audio_data = np.zeros((total_samples, 2), dtype=np.float32)

    # Simulation to audio sample ratio
    sim_to_audio_ratio = int(AUDIO_FS * params.dt)

    phases = np.zeros(2)
    sim_step = 0
    audio_idx = 0

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

        # Generate audio samples for this simulation step
        for _ in range(sim_to_audio_ratio):
            if audio_idx >= total_samples:
                break

            q0_amp = np.clip(q0, 0.0, MAX_AMPLITUDE)
            qpi_amp = np.clip(qpi, 0.0, MAX_AMPLITUDE)

            # Channel 0: q=0 mode
            audio_data[audio_idx, 0] = q0_amp * np.sin(phases[0])
            phases[0] += 2 * np.pi * CARRIER_FREQS[0] / AUDIO_FS

            # Channel 1: q=π mode
            audio_data[audio_idx, 1] = qpi_amp * np.sin(phases[1])
            phases[1] += 2 * np.pi * CARRIER_FREQS[1] / AUDIO_FS

            audio_idx += 1

        if sim_step % 1000 == 0:
            print(f"[{t:.2f}s] q0={q0:.3f}, qπ={qpi:.3f}")

    # Write WAV file
    audio_data_int = (audio_data * 32767).astype(np.int16)
    wavfile.write(output_path, AUDIO_FS, audio_data_int)

    print(f"\n✓ Generated: {output_path}")
    print(f"  Duration: {TOTAL_TIME}s")
    print(f"  Sample rate: {AUDIO_FS} Hz")
    print(f"  Channels: 2 (q=0 @ {CARRIER_FREQS[0]}Hz, q=π @ {CARRIER_FREQS[1]}Hz)")


# ==========================
# REAL-TIME AUDIO
# ==========================
def run_realtime_audio(params: NetworkParams):
    """Run real-time audio output with live simulation."""

    print("\n=== Real-Time Audio Sonification ===")
    print(f"Sample rate: {AUDIO_FS} Hz")
    print(f"Channels: 2")
    print(f"  Channel 0: q=0 mode @ {CARRIER_FREQS[0]} Hz")
    print(f"  Channel 1: q=π mode @ {CARRIER_FREQS[1]} Hz")
    print(f"\nDuration: {TOTAL_TIME}s")
    print("\nListening guide:")
    print("  - 0-24s: Gentle phase kick inquiries rotating around ring")
    print("  - 24-60s: Mixed triggers - rhythmic plucks, phase nudges, noise")
    print("  - At ~30s: Strong perturbation - hear system wobble and recover")
    print("  - 60-96s: Dense complexity - heterodyne probes, syncopation")
    print("  - At ~70s: Attractor switch - 220 Hz → 330 Hz transition")
    print("  - 96-120s: Resolution - calm impulses, system settles\n")

    # Check available devices
    try:
        devices = sd.query_devices()
        print("Available audio devices:")
        print(devices)
        print()
    except Exception as e:
        print(f"Could not query audio devices: {e}")

    state = SharedState()

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
            channels=2,
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
            state = SharedState()
            generate_wav_simulation(state, params)
    else:
        # No sounddevice, generate WAV
        state = SharedState()
        generate_wav_simulation(state, params)

    print("\n" + "=" * 70)
    print("INTERPRETATION")
    print("=" * 70)
    print("""
What you heard (or will hear in the WAV):

1. INITIAL SETTLING (0-4s):
   - 220 Hz (q=0) grows and stabilizes
   - 330 Hz (q=π) remains quiet
   - This is the ADJACENT attractor (all-in-phase)

2. INTRO TRIGGERS (0-24s):
   - Sparse phase kicks rotate around the ring
   - Gentle coherence inquiries
   - Listen for subtle tremors in the carriers

3. PERTURBATION (~30s):
   - Strong noise perturbation applied
   - Both channels wobble
   - System momentarily loses coherence

4. DEVELOPMENT TRIGGERS (24-60s):
   - Mixed trigger types (impulse + phase + noise)
   - Rhythmic plucks and phase nudges
   - Occasional noise bursts on beat 4
   - System responds with varying recovery times

5. ATTRACTOR SWITCH (70s):
   - Drive pattern changes to OPPOSITE nodes
   - 220 Hz weakens, 330 Hz locks in
   - This is the OPPOSITE attractor (alternating-phase)

6. COMPLEXITY TRIGGERS (60-96s):
   - Dense heterodyne probes create nonlinear interactions
   - Syncopated noise and phase kicks
   - Most chaotic section - listen for mode competition

7. RESOLUTION (96-120s):
   - Simple impulses calm the system
   - Triggers reverse direction around ring
   - System settles into final attractor state

This is AUDIBLE PROOF of:
- Distributed memory (order parameter = persistent tone)
- Multiple stable attractors (different channels)
- Recovery from varied perturbation types
- State switching (mode handover)
- Participatory inquiry (triggers "ask questions", system "responds")

The sound is NOT a visualization metaphor—it is a direct physical
observable (spatial mode amplitude) that encodes the system's state.
The triggers are "songs-as-inquiries" that probe coherence structure.
""")

    print("=" * 70)


if __name__ == "__main__":
    main()
