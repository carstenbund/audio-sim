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
    A simple musical trigger generator:
    - pulses every beat
    - alternates motifs A/B per bar
    """
    def __init__(self, bpm: float = 90.0, bars: int = 8):
        self.beat_s = 60.0 / bpm
        self.bar_s = 4 * self.beat_s
        self.bars = bars

    def events(self) -> List[TriggerEvent]:
        evs: List[TriggerEvent] = []
        t = 0.0
        for bar in range(self.bars):
            motif = "A" if (bar % 2 == 0) else "B"
            # 4 beats per bar
            for b in range(4):
                t_on = t + b * self.beat_s

                if motif == "A":
                    # Motif A: phase kick on node 0, mode 0
                    evs.append(TriggerEvent(
                        t_on=t_on,
                        target_nodes=[0],
                        kind="phase_kick",
                        mode=0,
                        delta_phi=np.pi/6  # gentle phase nudge
                    ))
                else:
                    # Motif B: deterministic impulse "pluck" on node 4
                    evs.append(TriggerEvent(
                        t_on=t_on,
                        target_nodes=[4],
                        kind="impulse",
                        strength=0.15,
                        phase=0.0,
                        mode=0
                    ))
            t += self.bar_s
        return evs
