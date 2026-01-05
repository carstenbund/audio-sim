from dataclasses import dataclass
from typing import List, Optional
import numpy as np

@dataclass
class TriggerEvent:
    t_on: float
    duration: float
    strength: float
    target_nodes: List[int]
    mode: Optional[int] = None       # None => all modes / network-defined
    kind: str = "impulse"            # "impulse" | "noise" | "phase_kick"

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
