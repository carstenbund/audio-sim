"""
Rhythmic Computation: Bar-Based Trigger Scheduler with Feature Logging

This script implements a **clocked computation substrate** using the modal network:

1. Fixed sustainer drive (no node switching)
2. Bar-based rhythmic triggers (4 distinct symbols)
3. Fixed readout window per bar
4. Logs full feature vectors (q-spectrum + mode ratio) to CSV

This is the bridge from "nice dynamics" → "operational, labeled dataset".

Usage:
    python rhythmic_computation.py

Outputs:
    - bar_log.csv: Feature vectors per bar (bar_idx, t_read, symbol, q0, qpi, mode_ratio, S_q0..S_q{N-1})
"""

import numpy as np
import threading
import time
import sys
import csv
from pathlib import Path
from dataclasses import dataclass
from typing import List

sys.path.insert(0, str(Path(__file__).parent.parent))

from src.network import ModalNetwork, NetworkParams
from src.drive import make_drive


# ==========================
# GLOBAL PARAMETERS
# ==========================
SIMULATION_DT = 1e-3          # 1 kHz simulation rate
TOTAL_TIME = 120.0            # Total duration (seconds)


# ==========================
# RHYTHM CONFIGURATION
# ==========================
@dataclass
class RhythmConfig:
    """Configuration for bar-based rhythmic computation."""
    bar_s: float = 1.0                    # Bar duration in seconds
    trigger_offset_s: float = 0.05        # When to apply trigger in the bar
    readout_offset_s: float = 0.85        # When to read state in the bar
    delta_phi: float = np.pi / 6          # Phase kick magnitude
    impulse_strength: float = 0.12        # Impulse strength


class RhythmProgram:
    """
    Produces one symbol per bar from a repeating sequence.

    Symbol set = {0, 1, 2, 3}:
    - 0: phase_kick +Δφ on node 0
    - 1: phase_kick −Δφ on node 0
    - 2: impulse on node 0
    - 3: impulse on node 4
    """

    def __init__(self, sequence: List[int]):
        """
        Initialize rhythm program.

        Args:
            sequence: List of symbol indices (0-3) to repeat
        """
        self.sequence = sequence

    def symbol_for_bar(self, bar_idx: int) -> int:
        """Get the symbol for a given bar index."""
        return self.sequence[bar_idx % len(self.sequence)]


class BarLogger:
    """Logger for bar-by-bar feature vectors."""

    def __init__(self, path: str, N: int):
        """
        Initialize logger.

        Args:
            path: Output CSV file path
            N: Number of nodes (determines q-spectrum size)
        """
        self.path = path
        self.N = N
        self.rows: List[List[float]] = []
        self.header = (
            ["bar_idx", "t_read", "symbol", "q0", "qpi", "mode_ratio"]
            + [f"S_q{m}" for m in range(N)]
        )

    def add(self, bar_idx: int, t_read: float, symbol: int,
            q0: float, qpi: float, mode_ratio: float, S_all: np.ndarray):
        """Add a bar's feature vector to the log."""
        row = [bar_idx, t_read, symbol, q0, qpi, mode_ratio] + list(map(float, S_all))
        self.rows.append(row)

    def write_csv(self):
        """Write accumulated data to CSV file."""
        with open(self.path, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(self.header)
            w.writerows(self.rows)


# ==========================
# SHARED STATE
# ==========================
class SharedState:
    """Thread-safe shared state."""

    def __init__(self):
        self.lock = threading.Lock()
        self.running = True

    def stop(self):
        with self.lock:
            self.running = False

    def is_running(self) -> bool:
        with self.lock:
            return self.running


# ==========================
# SIMULATION LOOP
# ==========================
def simulation_loop(state: SharedState, params: NetworkParams,
                   rhythm_cfg: RhythmConfig, program: RhythmProgram,
                   logger: BarLogger, verbose: bool = True):
    """
    Bar-based rhythmic computation loop.

    Timeline:
    - Each bar: trigger @ offset_s → relaxation → readout @ offset_s
    - One symbol per bar (from program sequence)
    - Fixed sustainer drive (no switching)
    - Logs full feature vector per bar
    """
    net = ModalNetwork(params, seed=42)
    t = 0.0
    step_count = 0

    # Fixed sustainer drive (no switching)
    sustain_nodes = [0, 1]   # Keeps baseline energy; choose for stable baseline

    # Precompute bar timings in steps
    bar_steps = int(round(rhythm_cfg.bar_s / params.dt))
    trig_step = int(round(rhythm_cfg.trigger_offset_s / params.dt))
    read_step = int(round(rhythm_cfg.readout_offset_s / params.dt))

    total_bars = int(TOTAL_TIME / rhythm_cfg.bar_s)

    if verbose:
        print("\n=== Rhythmical Calculation Mode ===")
        print(f"Bar duration: {rhythm_cfg.bar_s}s")
        print(f"Trigger offset: {rhythm_cfg.trigger_offset_s}s")
        print(f"Readout offset: {rhythm_cfg.readout_offset_s}s")
        print(f"Symbol sequence: {program.sequence}")
        print(f"Total bars: {total_bars}")
        print(f"Sustainer nodes: {sustain_nodes}")
        print(f"Output: {logger.path}\n")

    bar_idx = 0
    step_in_bar = 0
    symbol = program.symbol_for_bar(bar_idx)

    while state.is_running() and t < TOTAL_TIME:
        # Drive stays the same (baseline energizer)
        drive = make_drive(t, sustain_nodes, params.N)
        net.step(drive)

        # Trigger event at fixed time in the bar
        if step_in_bar == trig_step:
            symbol = program.symbol_for_bar(bar_idx)

            if symbol == 0:
                # Symbol 0: phase kick +Δφ on node 0
                net.phase_kick(+rhythm_cfg.delta_phi, target_nodes=[0], mode=0)
            elif symbol == 1:
                # Symbol 1: phase kick −Δφ on node 0
                net.phase_kick(-rhythm_cfg.delta_phi, target_nodes=[0], mode=0)
            elif symbol == 2:
                # Symbol 2: impulse on node 0
                net.perturb_nodes(
                    rhythm_cfg.impulse_strength,
                    target_nodes=[0],
                    mode=0,
                    kind="impulse",
                    phase=0.0
                )
            elif symbol == 3:
                # Symbol 3: impulse on node 4
                net.perturb_nodes(
                    rhythm_cfg.impulse_strength,
                    target_nodes=[4],
                    mode=0,
                    kind="impulse",
                    phase=0.0
                )

        # Readout event at fixed time in the bar
        if step_in_bar == read_step:
            q0 = net.order_parameter_q0(mode=0)
            qpi = net.order_parameter_qpi(mode=0)
            S_all = net.order_parameters_all(mode=0)
            mr = net.mode_ratio(0, 1) if params.K > 1 else 0.0

            logger.add(bar_idx, t, symbol, q0, qpi, mr, S_all)

            if verbose and (bar_idx % 1 == 0):
                # Quick human-readable scalar
                y = qpi - q0
                print(f"[bar {bar_idx:02d} @ {t:5.2f}s] sym={symbol} q0={q0:.3f} qπ={qpi:.3f} y={y:+.3f} mode_ratio={mr:.3f}")

        # Advance time/bar counters
        t += params.dt
        step_count += 1
        step_in_bar += 1

        if step_in_bar >= bar_steps:
            step_in_bar = 0
            bar_idx += 1
            if bar_idx >= total_bars:
                break

        time.sleep(params.dt)

    logger.write_csv()
    if verbose:
        print(f"\n✓ Wrote {logger.path}")
        print(f"  Total bars logged: {len(logger.rows)}")
    state.stop()


# ==========================
# MAIN
# ==========================
def main():
    """Main entry point."""

    print("=" * 70)
    print("RHYTHMIC COMPUTATION: Bar-Based Trigger + Readout")
    print("=" * 70)
    print("\nThis script implements a clocked computation substrate:")
    print("  - Fixed sustainer drive (no switching)")
    print("  - 4 distinct input symbols per bar")
    print("  - Fixed readout window")
    print("  - Full q-spectrum + mode ratio logging")
    print("\nOutput: bar_log.csv")

    # Setup parameters with light pinning and stable dynamics
    params = NetworkParams(
        N=8,
        K=2,
        dt=SIMULATION_DT,
        coupling=1.2,
        omega=np.array([20.0, 31.4]),
        gamma=np.array([0.55, 0.55]),
        drive_gain=np.array([1.0, 0.4]),
        pin_node=0,
        pin_strength=0.02,   # Small symmetry break
    )

    # Rhythm configuration
    rhythm_cfg = RhythmConfig(bar_s=1.0)

    # Program: repeating symbol sequence
    # Start with all 4 symbols to verify separation
    program = RhythmProgram(sequence=[0, 1, 2, 3, 0, 2, 1, 3])

    # Logger
    logger = BarLogger(path="bar_log.csv", N=params.N)

    # Shared state
    state = SharedState()

    # Run simulation
    print("\nStarting rhythmic computation...\n")
    simulation_loop(state, params, rhythm_cfg, program, logger, verbose=True)

    print("\n" + "=" * 70)
    print("NEXT STEPS")
    print("=" * 70)
    print("""
Now you have a labeled dataset in bar_log.csv. To prove separability:

1. Load the CSV:
   ```python
   import pandas as pd
   df = pd.read_csv('bar_log.csv')
   ```

2. Visualize feature clusters:
   ```python
   import matplotlib.pyplot as plt

   # Plot q0 vs qπ colored by symbol
   for sym in range(4):
       data = df[df['symbol'] == sym]
       plt.scatter(data['q0'], data['qpi'], label=f'Symbol {sym}')
   plt.xlabel('q0')
   plt.ylabel('qπ')
   plt.legend()
   plt.show()
   ```

3. Check separation quality:
   - Are the 4 symbol clusters distinct?
   - Which features separate them best (q0, qπ, mode_ratio, or specific S_qm)?
   - How stable is readout over time?

4. If needed, tune:
   - pin_strength (0.01-0.05)
   - gamma (0.45-0.65)
   - trigger_offset_s and readout_offset_s
   - delta_phi and impulse_strength

This is your operational computation substrate.
""")

    print("=" * 70)


if __name__ == "__main__":
    main()
