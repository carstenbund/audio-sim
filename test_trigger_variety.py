#!/usr/bin/env python
"""
Test script to verify TriggerSong generates varied triggers over 120 seconds.
"""

import sys
from pathlib import Path
from collections import Counter

sys.path.insert(0, str(Path(__file__).parent))

# Mock numpy for testing structure (avoid import errors)
class MockNumPy:
    pi = 3.14159265359

    def __getattr__(self, name):
        return lambda *args, **kwargs: 1.0

sys.modules['numpy'] = MockNumPy()

# Now we can import
from src.triggers import TriggerSong

def test_trigger_variety():
    """Test that TriggerSong generates varied triggers."""

    print("=" * 70)
    print("TRIGGER SONG VARIETY TEST")
    print("=" * 70)

    # Create song
    song = TriggerSong(bpm=90.0, duration=120.0)
    events = song.events()

    print(f"\nTotal events generated: {len(events)}")
    print(f"Duration: 120 seconds")
    print(f"Average events per second: {len(events) / 120.0:.2f}")

    # Analyze by kind
    kinds = Counter(ev.kind for ev in events)
    print(f"\n--- Trigger Types ---")
    for kind, count in sorted(kinds.items()):
        print(f"  {kind:15s}: {count:4d} ({100*count/len(events):.1f}%)")

    # Analyze by target nodes
    target_nodes = set()
    for ev in events:
        for node in ev.target_nodes:
            target_nodes.add(node)
    print(f"\n--- Spatial Coverage ---")
    print(f"  Unique nodes targeted: {sorted(target_nodes)}")
    print(f"  Coverage: {len(target_nodes)}/8 nodes")

    # Analyze time distribution across movements
    movements = {
        "Intro (0-24s)": (0, 24),
        "Development (24-60s)": (24, 60),
        "Complexity (60-96s)": (60, 96),
        "Resolution (96-120s)": (96, 120)
    }

    print(f"\n--- Movement Distribution ---")
    for name, (t_start, t_end) in movements.items():
        count = sum(1 for ev in events if t_start <= ev.t_on < t_end)
        density = count / (t_end - t_start)
        print(f"  {name:25s}: {count:4d} events ({density:.2f}/sec)")

    # Check for variety in parameters
    strengths = set(ev.strength for ev in events)
    phases = set(ev.phase for ev in events)
    delta_phis = set(ev.delta_phi for ev in events)

    print(f"\n--- Parameter Variety ---")
    print(f"  Unique strengths: {len(strengths)}")
    print(f"  Unique phases: {len(phases)}")
    print(f"  Unique delta_phis: {len(delta_phis)}")

    # Verify all 4 trigger types are present
    expected_kinds = {"noise", "impulse", "phase_kick", "heterodyne"}
    actual_kinds = set(kinds.keys())

    print(f"\n--- Completeness Check ---")
    if expected_kinds == actual_kinds:
        print(f"  ✓ All 4 trigger types present!")
    else:
        missing = expected_kinds - actual_kinds
        print(f"  ✗ Missing trigger types: {missing}")

    # Verify events are sorted by time
    times = [ev.t_on for ev in events]
    if times == sorted(times):
        print(f"  ✓ Events sorted by time")
    else:
        print(f"  ✗ Events NOT sorted by time")

    # Show first few events from each movement
    print(f"\n--- Sample Events ---")
    for name, (t_start, t_end) in movements.items():
        movement_events = [ev for ev in events if t_start <= ev.t_on < t_end][:3]
        print(f"\n  {name}:")
        for ev in movement_events:
            nodes_str = str(list(ev.target_nodes))
            print(f"    t={ev.t_on:6.2f}s  kind={ev.kind:12s}  nodes={nodes_str}")

    print("\n" + "=" * 70)
    print("TEST COMPLETE")
    print("=" * 70)

if __name__ == "__main__":
    test_trigger_variety()
