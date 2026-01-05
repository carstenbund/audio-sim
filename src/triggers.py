from dataclasses import dataclass
from typing import List, Optional, Sequence, Literal
import numpy as np

@dataclass
class TriggerEvent:
    t_on: float
    target_nodes: Sequence[int]
    kind: Literal["noise", "impulse", "phase_kick", "heterodyne"] = "impulse"
    strength: float = 0.3
    phase: float = 0.0
    mode: Optional[int] = 0
    delta_phi: float = 0.0           # For phase_kick
    mode_a: int = 0                  # For heterodyne
    mode_b: int = 1                  # For heterodyne
    out_mode: int = 0                # For heterodyne

class TriggerSchedule:
    def __init__(self, events: List[TriggerEvent]):
        self.events = events
        self._fired = set()

    def active_events(self, t: float) -> List[TriggerEvent]:
        active = []
        for i, ev in enumerate(self.events):
            if ev.t_on <= t < ev.t_on + ev.duration:
                active.append(ev)
        return active

    def fire_once_events(self, t: float) -> List[TriggerEvent]:
        """Events that should execute once at t_on (impulse-like)."""
        fired = []
        for i, ev in enumerate(self.events):
            if abs(t - ev.t_on) < 1e-12 and i not in self._fired:
                fired.append(ev)
                self._fired.add(i)
        return fired


class TriggerSong:
    """
    A rich musical trigger generator with multiple movements.

    Creates a 120-second composition with varied trigger types,
    target nodes, and rhythmic patterns to explore system response.
    """
    def __init__(self, bpm: float = 90.0, duration: float = 120.0):
        self.beat_s = 60.0 / bpm
        self.bar_s = 4 * self.beat_s
        self.duration = duration
        self.num_bars = int(duration / self.bar_s)

    def events(self) -> List[TriggerEvent]:
        """
        Generate a varied trigger sequence across multiple movements.

        Movement structure:
        - Intro (0-24s): Sparse phase kicks, establishing baseline
        - Development (24-60s): Mixed triggers exploring all types
        - Complexity (60-96s): Dense heterodyne probes and noise
        - Resolution (96-120s): Return to simple impulses
        """
        evs: List[TriggerEvent] = []

        for bar in range(self.num_bars):
            t_bar = bar * self.bar_s

            # Determine movement section
            if t_bar < 24.0:
                # INTRO: Sparse phase kicks rotating around the ring
                node = bar % 8
                evs.append(TriggerEvent(
                    t_on=t_bar,
                    target_nodes=[node],
                    kind="phase_kick",
                    delta_phi=np.pi / (8 + bar % 4),  # Varying phase
                    mode=0
                ))

            elif t_bar < 60.0:
                # DEVELOPMENT: Mixed trigger types
                for beat in range(4):
                    t = t_bar + beat * self.beat_s

                    if beat == 0:
                        # Downbeat: impulse pluck
                        evs.append(TriggerEvent(
                            t_on=t,
                            target_nodes=[bar % 8],
                            kind="impulse",
                            strength=0.1 + 0.05 * (bar % 3),
                            phase=beat * np.pi / 4,
                            mode=0
                        ))
                    elif beat == 2:
                        # Mid-bar: phase kick on opposite node
                        evs.append(TriggerEvent(
                            t_on=t,
                            target_nodes=[(bar + 4) % 8],
                            kind="phase_kick",
                            delta_phi=np.pi / 4,
                            mode=0
                        ))

                    # Add occasional noise bursts
                    if bar % 3 == 0 and beat == 3:
                        evs.append(TriggerEvent(
                            t_on=t,
                            target_nodes=[bar % 8, (bar + 1) % 8],
                            kind="noise",
                            strength=0.08,
                            mode=0
                        ))

            elif t_bar < 96.0:
                # COMPLEXITY: Heterodyne probes and dense patterns
                for beat in range(4):
                    t = t_bar + beat * self.beat_s

                    # Heterodyne probes on beats 0 and 2
                    if beat in [0, 2]:
                        evs.append(TriggerEvent(
                            t_on=t,
                            target_nodes=[(bar + beat) % 8],
                            kind="heterodyne",
                            mode_a=0,
                            mode_b=1,
                            out_mode=0,
                            strength=0.15 + 0.05 * (bar % 2)
                        ))

                    # Phase kicks on offbeats
                    if beat in [1, 3]:
                        evs.append(TriggerEvent(
                            t_on=t,
                            target_nodes=[(bar * 2 + beat) % 8],
                            kind="phase_kick",
                            delta_phi=np.pi / (3 + beat),
                            mode=0
                        ))

                    # Syncopated noise
                    if bar % 2 == 0 and beat == 1:
                        evs.append(TriggerEvent(
                            t_on=t + self.beat_s * 0.5,  # Off-beat
                            target_nodes=[(bar + 3) % 8, (bar + 5) % 8],
                            kind="noise",
                            strength=0.06,
                            mode=0
                        ))

            else:
                # RESOLUTION: Simple impulses, calming down
                for beat in [0, 2]:
                    t = t_bar + beat * self.beat_s
                    evs.append(TriggerEvent(
                        t_on=t,
                        target_nodes=[(8 - bar) % 8],  # Reverse direction
                        kind="impulse",
                        strength=0.12,
                        phase=0.0,
                        mode=0
                    ))

        # Sort by time
        evs.sort(key=lambda e: e.t_on)
        return evs
