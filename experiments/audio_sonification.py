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
from src.score import ScorePlayer

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
MAX_AMPLITUDE = 0.7           # Headroom for safety
SMOOTH = 0.12                 # Amplitude smoothing per callback block
DEVICE_INDEX = None           # Auto-detect, or set manually

# Experiment timeline
TOTAL_TIME = 120.0            # Total duration

# ==========================
# SHARED STATE
# ==========================
class SharedState:
    """Thread-safe shared state between simulation and audio."""

    def __init__(self, N: int):
        self.lock = threading.Lock()
        self.running = True

        # For sonification of q0/qpi (keep if you want)
        self.order_params = {'q0': 0.0, 'qpi': 0.0}

        # Per-node audio (8 channels)
        self.a0 = np.zeros(N, dtype=np.complex64)      # snapshot of net.a[:,0]
        self.freq = np.zeros(N, dtype=np.float32)      # per-node pitch
        self.vel = np.zeros(N, dtype=np.float32)       # per-node velocity gate

        # Per-node oscillator phase continuity
        self.phase = np.zeros(N, dtype=np.float64)

        # For WAV generation
        self.audio_buffer = []

    def update_order_params(self, q0: float, qpi: float):
        with self.lock:
            self.order_params = {'q0': q0, 'qpi': qpi}

    def get_order_params(self) -> dict:
        with self.lock:
            return self.order_params.copy()

    def update_node_audio(self, a0: np.ndarray, freq: np.ndarray, vel: np.ndarray):
        with self.lock:
            self.a0[:] = a0
            self.freq[:] = freq
            self.vel[:] = vel

    def get_node_audio(self):
        with self.lock:
            return self.a0.copy(), self.freq.copy(), self.vel.copy()

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
    Real-time simulation loop with score-driven modal synth.

    Timeline:
    - Constant drive on sustain_nodes (baseline energizer)
    - CSV score controls per-node pitch and velocity
    - Network amplitude (a[:,0]) shapes the sound
    """
    net = ModalNetwork(params, seed=42)
    t = 0.0
    step_count = 0

    # Load score and set sustain nodes
    score = ScorePlayer.from_csv("score.csv", num_nodes=params.N)
    sustain_nodes = [0, 1]  # Keep fixed baseline energizer (adjust if needed)

    if verbose:
        print("\n=== Modal Synth Player ===")
        print(f"Score: score.csv")
        print(f"Sustain nodes: {sustain_nodes}")
        print(f"Duration: {TOTAL_TIME}s")
        print(f"Channels: {params.N}")
        print()

    while state.is_running() and t < TOTAL_TIME:
        # Constant drive (no switching)
        drive = make_drive(t, sustain_nodes, params.N)
        net.step(drive)

        # Update score (freq/vel) and publish audio snapshot
        score.update(t)
        state.update_node_audio(net.a[:, 0], score.freq, score.vel)

        # Keep q0/qpi sonification metrics if you still want them
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
def make_audio_callback_nodes(state: SharedState, N: int):
    """
    8-channel audio callback with velocity gating.

    Each node:
    - Pitch: score.freq[j] (from CSV)
    - Velocity gate: score.vel[j] (0 = silent)
    - Amplitude: network state |a[j,0]| (shaped by network dynamics)
    - Output: node j → channel j
    """
    amp_smooth = np.zeros(N, dtype=np.float32)

    def audio_callback(outdata, frames, time_info, status):
        nonlocal amp_smooth
        outdata[:] = 0.0
        tvec = np.arange(frames, dtype=np.float32) / AUDIO_FS

        a0, freq, vel = state.get_node_audio()

        # Network-derived amplitude per node
        amp_raw = np.abs(a0).astype(np.float32)

        # Normalize gently to avoid one node dominating forever
        amp_raw = amp_raw / (np.max(amp_raw) + 1e-6)
        amp_raw = np.clip(amp_raw, 0.0, 1.0)

        # Smooth to avoid clicks
        amp_smooth = amp_smooth + SMOOTH * (amp_raw - amp_smooth)

        for j in range(N):
            # Velocity gates sound: if vel=0 => silent, regardless of network amplitude
            if vel[j] <= 1e-4 or freq[j] <= 1.0:
                continue

            a = np.clip(vel[j] * amp_smooth[j], 0.0, MAX_AMPLITUDE)
            if a <= 1e-6:
                continue

            ph0 = state.phase[j]
            outdata[:, j] = a * np.sin(2*np.pi*freq[j]*tvec + ph0)

            # advance phase accumulator for continuity
            state.phase[j] = (ph0 + 2*np.pi*freq[j]*frames / AUDIO_FS) % (2*np.pi)

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

    # Load score
    score = ScorePlayer.from_csv("score.csv", num_nodes=params.N)
    sustain_nodes = [0, 1]

    # Pre-allocate audio buffer for N nodes
    total_samples = int(TOTAL_TIME * AUDIO_FS)
    audio_data = np.zeros((total_samples, params.N), dtype=np.float32)

    # Simulation to audio sample ratio
    sim_to_audio_ratio = int(AUDIO_FS * params.dt)

    phases = np.zeros(params.N)
    amp_smoothed = np.zeros(params.N, dtype=np.float32)
    sim_step = 0
    audio_idx = 0

    print("Simulating...")

    for sim_step in range(int(TOTAL_TIME / params.dt)):
        t = sim_step * params.dt

        # Constant drive (no switching)
        drive = make_drive(t, sustain_nodes, params.N)
        net.step(drive)

        # Update score
        score.update(t)

        # Get order parameters
        q0 = net.order_parameter_q0(mode=0)
        qpi = net.order_parameter_qpi(mode=0)

        # Get network state and score state
        a0 = net.a[:, 0]
        freq = score.freq
        vel = score.vel

        # Network-derived amplitude
        amp_raw = np.abs(a0).astype(np.float32)
        amp_raw = amp_raw / (np.max(amp_raw) + 1e-6)
        amp_raw = np.clip(amp_raw, 0.0, 1.0)

        # Smooth amplitudes
        amp_smoothed = amp_smoothed + SMOOTH * (amp_raw - amp_smoothed)

        # Generate audio samples for this simulation step
        for _ in range(sim_to_audio_ratio):
            if audio_idx >= total_samples:
                break

            # Generate each node
            for j in range(params.N):
                # Velocity gate
                if vel[j] <= 1e-4 or freq[j] <= 1.0:
                    audio_data[audio_idx, j] = 0.0
                    continue

                a = np.clip(vel[j] * amp_smoothed[j], 0.0, MAX_AMPLITUDE)
                if a <= 1e-6:
                    audio_data[audio_idx, j] = 0.0
                    continue

                # Generate carrier
                audio_data[audio_idx, j] = a * np.sin(phases[j])

                # Update phase
                phases[j] += 2 * np.pi * freq[j] / AUDIO_FS

            audio_idx += 1

        if sim_step % 1000 == 0:
            print(f"[{t:.2f}s] q0={q0:.3f}, qπ={qpi:.3f}")

    # Write WAV file
    audio_data_int = (audio_data * 32767).astype(np.int16)
    wavfile.write(output_path, AUDIO_FS, audio_data_int)

    print(f"\n✓ Generated: {output_path}")
    print(f"  Duration: {TOTAL_TIME}s")
    print(f"  Sample rate: {AUDIO_FS} Hz")
    print(f"  Channels: {params.N}")


# ==========================
# REAL-TIME AUDIO
# ==========================
def run_realtime_audio(params: NetworkParams):
    """Run real-time audio output with live simulation."""

    print("\n=== Real-Time Modal Synth Player ===")
    print(f"Sample rate: {AUDIO_FS} Hz")
    print(f"Channels: {params.N}")
    print(f"Duration: {TOTAL_TIME}s")
    print("\nListening guide:")
    print("  - Each node = one audio output channel")
    print("  - Pitch comes from score.csv (MIDI notes)")
    print("  - Velocity gates sound on/off")
    print("  - Network amplitude shapes the feel")
    print("\nYou'll hear:")
    print("  - Notes played by the score")
    print("  - Amplitude modulation from network dynamics")
    print("  - Clean velocity-gated envelopes\n")

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
        callback = make_audio_callback_nodes(state, params.N)

        with sd.OutputStream(
            samplerate=AUDIO_FS,
            channels=params.N,
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
    print("MODAL SYNTH PLAYER")
    print("=" * 70)
    print("\nThis experiment uses the modal network as an 8-channel synthesizer.")
    print("Each node is an audio output channel with score-driven pitch and velocity.")

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

=== THE MODAL SYNTH SYSTEM ===

You just experienced an 8-channel modal synthesizer where:
  - Each node = one audio output channel
  - Pitch comes from CSV score (MIDI note numbers)
  - Velocity gates sound on/off (0 = silent)
  - Network state |a[:,0]| shapes amplitude

=== HOW IT WORKS ===

1. SCORE-DRIVEN PITCH:
   - score.csv defines note events (t_on, dur, node, note, velocity)
   - MIDI note numbers converted to Hz (A4=440Hz)
   - Monophonic per node (later notes override)

2. VELOCITY GATING:
   - Velocity (0..1) controls whether sound plays
   - velocity=0 → silent, regardless of network amplitude
   - velocity=1 → full amplitude from network
   - This gives clean note on/off envelopes

3. NETWORK AMPLITUDE SHAPING:
   - Network state |a[j,0]| provides per-node amplitude
   - Normalized and smoothed to avoid clicks
   - Multiplied by velocity gate
   - Result: network dynamics shape the "feel" of each note

4. CONSTANT DRIVE:
   - Sustain nodes {[0, 1]} provide baseline energy
   - No switching, no perturbation needed
   - Just clean score playback with network-shaped dynamics

=== WHAT THIS DEMONSTRATES ===

This is a CLEAN ARCHITECTURE for:

1. MUSICAL CONTROL:
   - Pitch and rhythm from human-readable CSV
   - Network provides organic amplitude variation
   - Separation of concerns: score vs. dynamics

2. VELOCITY AS GATE:
   - Not just amplitude scaling
   - True on/off control independent of network state
   - Network can't force sound when velocity=0

3. 8-CHANNEL ROUTING:
   - Each node gets its own output
   - Spatial distribution of notes
   - Suitable for multi-speaker or DAW routing

4. SIMPLICITY:
   - No complex routing schedules
   - No switching logic
   - Score + network + velocity = sound

This is the CLEANEST setup for making your network an instrument.
""")

    print("=" * 70)


if __name__ == "__main__":
    main()
