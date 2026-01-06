"""
Real-time MIDI Input for Modal Network

Listens to MIDI controller input and routes notes to network nodes.
Different MIDI channels control different behaviors:
- Channel 1: Trigger notes (perturbations at note frequency)
- Channel 2: Drive notes (sustained energy at note frequency)
"""

import threading
import time
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass

try:
    import mido
    HAS_MIDI = True
except ImportError:
    HAS_MIDI = False


def midi_to_hz(note: int) -> float:
    """Convert MIDI note number to frequency in Hz."""
    return 440.0 * (2.0 ** ((note - 69.0) / 12.0))


@dataclass
class MidiNote:
    """Active MIDI note."""
    note: int          # MIDI note number
    velocity: float    # Velocity (0..1)
    freq_hz: float     # Frequency in Hz
    channel: int       # MIDI channel (1-16)
    node: int          # Target network node (0-7)


class MidiListener:
    """
    Thread-safe MIDI input listener.

    Routes MIDI channels to different behaviors:
    - Channel 1: Triggers (short perturbations)
    - Channel 2: Drive notes (sustained)
    """

    def __init__(self, num_nodes: int = 8, port_name: Optional[str] = None):
        """
        Initialize MIDI listener.

        Args:
            num_nodes: Number of network nodes (for note mapping)
            port_name: MIDI port name (None = auto-select first available)
        """
        if not HAS_MIDI:
            raise ImportError("mido not available. Install with: pip install mido python-rtmidi")

        self.num_nodes = num_nodes
        self.lock = threading.Lock()
        self.running = False

        # Active notes per channel {channel: {note: MidiNote}}
        self.active_notes: Dict[int, Dict[int, MidiNote]] = {}

        # Recent trigger events (channel 1) - cleared after read
        self.trigger_queue: List[MidiNote] = []

        # MIDI port
        self.port_name = port_name
        self.port = None
        self.thread = None

    @staticmethod
    def list_ports() -> List[str]:
        """List available MIDI input ports."""
        if not HAS_MIDI:
            return []
        return mido.get_input_names()

    def _note_to_node(self, note: int) -> int:
        """Map MIDI note to network node (0-7)."""
        # Simple modulo mapping - could be customized
        return note % self.num_nodes

    def _process_message(self, msg):
        """Process incoming MIDI message."""
        if msg.type == 'note_on' and msg.velocity > 0:
            # Note on
            note = msg.note
            velocity = msg.velocity / 127.0
            channel = msg.channel + 1  # MIDI channels are 0-indexed, we use 1-indexed
            node = self._note_to_node(note)

            midi_note = MidiNote(
                note=note,
                velocity=velocity,
                freq_hz=midi_to_hz(note),
                channel=channel,
                node=node,
            )

            with self.lock:
                # Add to active notes
                if channel not in self.active_notes:
                    self.active_notes[channel] = {}
                self.active_notes[channel][note] = midi_note

                # If channel 1, add to trigger queue
                if channel == 1:
                    self.trigger_queue.append(midi_note)

        elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
            # Note off
            note = msg.note
            channel = msg.channel + 1

            with self.lock:
                if channel in self.active_notes and note in self.active_notes[channel]:
                    del self.active_notes[channel][note]
                    if not self.active_notes[channel]:
                        del self.active_notes[channel]

    def _listen_loop(self):
        """Background thread that listens to MIDI input."""
        try:
            while self.running:
                for msg in self.port.iter_pending():
                    self._process_message(msg)
                time.sleep(0.001)  # 1ms polling
        except Exception as e:
            print(f"MIDI listener error: {e}")

    def start(self):
        """Start listening to MIDI input."""
        if self.running:
            return

        # Open MIDI port
        ports = self.list_ports()
        if not ports:
            raise RuntimeError("No MIDI input ports available")

        port_name = self.port_name or ports[0]
        print(f"Opening MIDI port: {port_name}")

        self.port = mido.open_input(port_name)
        self.running = True

        # Start listener thread
        self.thread = threading.Thread(target=self._listen_loop, daemon=True)
        self.thread.start()

        print(f"✓ MIDI listener started on {port_name}")

    def stop(self):
        """Stop listening to MIDI input."""
        self.running = False
        if self.thread:
            self.thread.join(timeout=1.0)
        if self.port:
            self.port.close()
        print("MIDI listener stopped")

    def get_active_notes(self, channel: int) -> List[MidiNote]:
        """Get currently active notes on a channel."""
        with self.lock:
            if channel in self.active_notes:
                return list(self.active_notes[channel].values())
            return []

    def get_drive_notes(self) -> List[MidiNote]:
        """Get active drive notes (channel 2)."""
        return self.get_active_notes(channel=2)

    def pop_triggers(self) -> List[MidiNote]:
        """Get and clear recent trigger events (channel 1)."""
        with self.lock:
            triggers = self.trigger_queue.copy()
            self.trigger_queue.clear()
            return triggers

    def get_all_active_notes(self) -> Dict[int, List[MidiNote]]:
        """Get all active notes grouped by channel."""
        with self.lock:
            return {
                ch: list(notes.values())
                for ch, notes in self.active_notes.items()
            }


def demo_midi_listener():
    """Demo: Print MIDI input in real-time."""
    print("\n=== MIDI Input Demo ===")
    print("Available MIDI ports:")
    ports = MidiListener.list_ports()
    for i, port in enumerate(ports):
        print(f"  {i}: {port}")

    if not ports:
        print("No MIDI ports available!")
        return

    print("\nListening for MIDI input...")
    print("  Channel 1: Trigger notes")
    print("  Channel 2: Drive notes")
    print("Press Ctrl+C to stop\n")

    listener = MidiListener(num_nodes=8)
    listener.start()

    try:
        while True:
            # Check for triggers (channel 1)
            triggers = listener.pop_triggers()
            for t in triggers:
                print(f"[TRIGGER] Ch{t.channel} Note {t.note} ({t.freq_hz:.1f}Hz) vel={t.velocity:.2f} → node {t.node}")

            # Check active drive notes (channel 2)
            drive_notes = listener.get_drive_notes()
            if drive_notes:
                notes_str = ", ".join(f"{n.note}({n.freq_hz:.1f}Hz)" for n in drive_notes)
                print(f"[DRIVE] Active: {notes_str}")

            time.sleep(0.1)

    except KeyboardInterrupt:
        print("\nStopping...")

    finally:
        listener.stop()


if __name__ == "__main__":
    if HAS_MIDI:
        demo_midi_listener()
    else:
        print("mido not available. Install with: pip install mido python-rtmidi")
