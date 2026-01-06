"""
Score Player for Modal Synth

Velocity-gated, monophonic-per-node score playback system for the modal network.
Each node can play one note at a time, with pitch from MIDI and amplitude gating by velocity.
"""

import csv
import numpy as np
from dataclasses import dataclass
from typing import List


def midi_to_hz(note: float) -> float:
    """
    Convert MIDI note number to frequency in Hz.

    Args:
        note: MIDI note number (A4 = 69)

    Returns:
        Frequency in Hz (A4 = 440 Hz)
    """
    return 440.0 * (2.0 ** ((note - 69.0) / 12.0))


@dataclass
class NoteEvent:
    """Single note event in the score."""
    t_on: float       # Note start time (seconds)
    t_off: float      # Note end time (seconds)
    node: int         # Which node/channel plays this note
    freq_hz: float    # Frequency in Hz
    velocity: float   # Velocity (0..1) for amplitude gating


class ScorePlayer:
    """
    Velocity-gated, monophonic-per-node score playback.
    If multiple notes overlap on the same node, later note overrides.
    """

    def __init__(self, events: List[NoteEvent], num_nodes: int):
        """
        Initialize score player.

        Args:
            events: List of note events
            num_nodes: Number of nodes in the network
        """
        self.events = sorted(events, key=lambda e: e.t_on)
        self.num_nodes = num_nodes

        self.freq = np.zeros(num_nodes, dtype=np.float32)
        self.vel = np.zeros(num_nodes, dtype=np.float32)
        self._active_off = np.full(num_nodes, -1.0, dtype=np.float64)
        self._idx = 0

    @staticmethod
    def from_csv(path: str, num_nodes: int) -> "ScorePlayer":
        """
        Load score from CSV file.

        CSV format:
            t_on,dur,node,note,velocity
            0.00,0.50,0,57,0.9
            0.00,0.50,1,64,0.8
            ...

        Args:
            path: Path to CSV file
            num_nodes: Number of nodes in the network

        Returns:
            ScorePlayer instance
        """
        evs: List[NoteEvent] = []
        with open(path, "r", newline="") as f:
            r = csv.DictReader(f)
            for row in r:
                t_on = float(row["t_on"])
                dur = float(row["dur"])
                node = int(row["node"])
                note = float(row["note"])
                vel = float(row.get("velocity", 1.0))
                if 0 <= node < num_nodes and dur > 0:
                    evs.append(NoteEvent(
                        t_on=t_on,
                        t_off=t_on + dur,
                        node=node,
                        freq_hz=midi_to_hz(note),
                        velocity=float(np.clip(vel, 0.0, 1.0)),
                    ))
        return ScorePlayer(evs, num_nodes=num_nodes)

    def update(self, t: float):
        """
        Update score state at given time.

        Turns off expired notes and starts new ones.

        Args:
            t: Current time in seconds
        """
        # Turn off expired notes
        off = (self._active_off >= 0.0) & (t >= self._active_off)
        if np.any(off):
            self.vel[off] = 0.0
            self._active_off[off] = -1.0

        # Start notes whose t_on <= t
        while self._idx < len(self.events) and self.events[self._idx].t_on <= t:
            ev = self.events[self._idx]
            self.freq[ev.node] = ev.freq_hz
            self.vel[ev.node] = ev.velocity
            self._active_off[ev.node] = ev.t_off
            self._idx += 1
