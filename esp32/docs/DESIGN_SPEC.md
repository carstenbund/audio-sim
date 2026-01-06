# Distributed Modal Resonator Network

**Audio-First, Asynchronous Collective Wave Instrument**

**Version:** 2.0
**Date:** 2026-01-06
**Status:** Implementation Ready

---

## 1. Design Goals

### Primary Goals
- **Audio continuity is sacred**: each node must generate stable, uninterrupted audio independently.
- **No hard real-time network dependency**: the system must not rely on 1 kHz inter-node synchronization.
- **Collective resonance, not global sync**: the network should exhibit coherent, emergent behavior without shared clocks.
- **Configurability before performance**: topology and behavior are fixed per session.
- **Scalable to 16 nodes** using ESP-NOW.

### Non-Goals
- Sample-accurate phase alignment across nodes
- All-to-all continuous state synchronization
- Quantum claims beyond classical analogies

---

## 2. Core Concept

Each node is a **self-directed modal resonator**:
- It runs continuously and autonomously.
- Network input does not sustain the sound — it **excites, nudges, or perturbs** it.
- Delay, jitter, and packet loss are treated as part of the physical medium, not errors.

The system behaves like an **excitable distributed medium**, not a distributed real-time simulation.

---

## 3. Node Model

### 3.1 Modal State

Each node maintains up to **K ≤ 4 complex modes**:

```
a_k(t) ∈ ℂ, k = 0..K-1
```

Each mode encodes:
- **amplitude** → energy
- **phase** → internal temporal structure

### 3.2 Dynamics (local, continuous)

Each mode evolves locally:

```
ȧ_k = (-γ_k + iω_k)a_k + u_k(t)
```

Where:
- `γ_k > 0` ensures stability
- `ω_k` defines modal character
- `u_k(t)` is external excitation, never mandatory

Discrete-time implementation uses **exact exponential integration** for numerical stability.

### 3.3 Node Personality

Two supported personalities:

#### 1. Resonator
- Decays to silence without stimulation
- Ideal for percussive / wave-medium behavior

#### 2. Self-Oscillator
- Gentle limit-cycle (negative damping at low energy + saturation)
- Produces continuous sound without input
- Pokes reshape behavior rather than start/stop sound

---

## 4. Audio Architecture (Audio-First)

### 4.1 Separation of Rates
- **Audio rate (48 kHz)**: oscillator, envelopes, synthesis
- **Control rate (200–1000 Hz)**: modal integration, excitation shaping
- **Network rate (event-based)**: stimulation only

**Network messages never run audio logic directly.**

### 4.2 K-Mode Audio Mapping

Recommended mapping:
- **Mode 0**: audible carrier
- **Mode 1**: detune / beating
- **Mode 2**: timbre / brightness modulation
- **Mode 3**: slow structural / behavioral state

Only Mode 0 must be audible; others may be latent.

### 4.3 Sync-Dependent Audio Tricks (Local Only)

Because sync is local, nodes may safely implement:
- hard sync
- phase reset
- FM/PM
- sample-accurate envelopes
- stereo phase tricks

**No global sync is required for these.**

---

## 5. Network Semantics

### 5.1 No Continuous State Coupling

Nodes do **not** exchange full modal states at high rate.

Instead, the network carries **events, not states**.

### 5.2 Poke / Event Coupling

A **poke** represents energy injection, not force balancing.

Conceptual effect:

```
a_k ← a_k + α · w_k · e^(iθ)
```

Where:
- `α`: strength
- `w_k`: mode weighting
- `θ`: phase hint (optional or random)

Excitation is always applied via a **short envelope (1–20 ms)**.

### 5.3 Delay as Physics
- Network delay becomes **propagation time**.
- Jitter becomes **textural variation**.
- Packet loss becomes **stochastic excitation thinning**.

The system remains stable because:
- nodes are autonomous
- excitation is bounded and saturated

---

## 6. Topology and Collective Resonance

### 6.1 Fixed Topology per Session

Topology is selected **before** a session and **locked** during performance.

**Reasons:**
- stable attractors
- learnable behavior
- predictable physics

### 6.2 Supported Topologies (16 Nodes)

Recommended presets:
- **Ring** (degree 2)
- **Ring + sparse long links** (small-world)
- **Two clusters with weak bridge**
- **Hub-and-spokes** (conductor model)

All topologies respect ESP-NOW peer limits.

### 6.3 What "Collective Wave Function" Means Here

**Not:**
- instantaneous shared complex state

**But:**
- persistent network-wide modes
- correlated excitation patterns
- metastable energy distributions
- order emerging from local rules

This aligns with:
- excitable media
- Kuramoto-style coherence
- reservoir computing

---

## 7. Configuration and Session Model

### 7.1 Message-Only Control Plane

**No web server required.**

ESP-NOW is reused for:
- discovery
- registration
- configuration
- session start

### 7.2 Node API (Conceptual)

Minimal message types:
- `HELLO` / `OFFER` / `JOIN`
- `CFG_BEGIN` / `CFG_CHUNK` / `CFG_END`
- `CFG_ACK` / `CFG_NACK`
- `START` / `STOP`
- `POKE` (runtime)

### 7.3 Configuration Blob

A single config string (JSON or compact binary) defines:
- topology
- node parameters
- coupling semantics
- stimulation rules
- run limits

Nodes extract their own section by `node_id`.

### 7.4 Session Lock

On `START`:
- configuration is frozen
- nodes enter run mode
- only poke/events are processed

---

## 8. Scalability and Robustness

### Target Scale
- Up to **16 nodes**

### Why This Works
- No continuous sync traffic
- Small packets
- Event-driven network load
- Autonomous audio generation

### Failure Behavior
- Node drop-out → local silence or decay
- Rejoin → fresh excitation, not desync
- Network congestion → fewer pokes, not audio failure

---

## 9. Musical and Computational Interpretation

### As Instrument
- A modular, spatial, excitable resonant body
- Topology = instrument geometry
- Pokes = gestures

### As Computation
- Dynamics act as a reservoir
- Attractors encode memory
- Classification is optional, observational

### As Analog to Quantum Concepts
- Superposition → multi-mode excitation
- Coherence → phase correlation
- Decoherence → damping + noise
- Measurement → energy observation

**No entanglement, no collapse, no speedup claims.**

---

## 10. Summary

This design replaces the fragile idea of hard synchronized distributed simulation with a robust, audio-first excitable medium:
- Local nodes are strong and expressive
- The network stimulates, not dictates
- Collective behavior emerges from topology + excitation
- Delay is musical, not fatal
- Sync is local, where it belongs

The result is a **playable, scalable, and conceptually coherent instrument** that survives real wireless conditions without compromise.

---

## 11. Implementation Notes

### Differences from Original ESP32 Proposal

This design deviates from the original proposal in key ways:

1. **No 1 kHz Sync Requirement**: Nodes operate asynchronously
2. **4 Modes per Node** (not 2): Richer timbre and behavior
3. **Event-Based Coupling**: "Pokes" replace continuous state exchange
4. **Audio-First**: Audio generation decoupled from network
5. **Message-Only Config**: No web server required

### Next Steps

1. Implement single-node firmware with 4-mode resonator
2. Design poke message protocol
3. Build two-node coupling test
4. Scale to 16-node network
5. Create configuration presets

---

**End of Design Specification**

For hardware details, see: `ESP32_HARDWARE_PROPOSAL.md`
