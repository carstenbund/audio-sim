# Modal Attractors AU Plugin Porting Proposal
## ESP32 Modal Resonator System → Mac Audio Unit for Logic X

**Document Version:** 1.0
**Date:** January 7, 2026
**Target Platform:** macOS (Intel & Apple Silicon)
**Target DAW:** Logic Pro X (and compatible AU hosts)
**Primary Language:** Objective-C with C/C++ DSP core

---

## Executive Summary

This proposal outlines the strategy for porting the **Modal Attractors** distributed resonator system from ESP32 embedded hardware to a professional **Audio Unit v2 (AU)** plugin for macOS. The resulting plugin will transform the networked physical computing experiment into a powerful synthesis instrument for Logic Pro X and other AU-compatible hosts.

### Key Value Propositions

1. **Novel Synthesis Engine**: Physics-based modal synthesis with emergent spectral behavior
2. **Proven DSP Core**: Battle-tested real-time audio algorithms from ESP32 firmware
3. **Unique Sound Character**: Evolving, organic timbres from coupled oscillator networks
4. **Professional Integration**: Native Logic X instrument with full MIDI & automation
5. **Performance Ready**: Polyphonic, low-latency, CPU-efficient design

### Core Concept Translation

| ESP32 Distributed System | AU Plugin Equivalent |
|--------------------------|---------------------|
| 16 networked nodes | 16-voice polyphonic synth OR multi-timbral generator |
| ESP-NOW mesh messages | Internal voice coupling + MIDI routing |
| Hub controller + MIDI | Direct MIDI input from Logic |
| I2S audio output | AU render callback (Core Audio) |
| Session configuration | AU presets/programs + Logic automation |
| 4 modes per node | 4-oscillator FM-like synthesis per voice |
| Network pokes | Note triggers + modulation matrix |

---

## 1. Technical Architecture

### 1.1 Audio Unit Structure

```
┌─────────────────────────────────────────────────────┐
│         Modal Attractors AU Plugin                  │
├─────────────────────────────────────────────────────┤
│                                                      │
│  ┌──────────────────────────────────────────┐      │
│  │    Objective-C AU Wrapper Layer          │      │
│  │  • AUInstrumentBase subclass             │      │
│  │  • MIDI event processing                 │      │
│  │  • Parameter management                  │      │
│  │  • Preset serialization                  │      │
│  └──────────────────────────────────────────┘      │
│                      ↕                              │
│  ┌──────────────────────────────────────────┐      │
│  │    C++ DSP Engine Core                   │      │
│  │  • Voice manager (polyphony)             │      │
│  │  • Modal oscillator bank                 │      │
│  │  • Coupling matrix                       │      │
│  │  • Envelope generators                   │      │
│  └──────────────────────────────────────────┘      │
│                      ↕                              │
│  ┌──────────────────────────────────────────┐      │
│  │    Ported ESP32 DSP Core (C)             │      │
│  │  • modal_node.c/.h (exact integration)   │      │
│  │  • audio_synth.c/.h (adapted)            │      │
│  │  • Fast sine approximation               │      │
│  │  • Phase accumulation                    │      │
│  └──────────────────────────────────────────┘      │
│                                                      │
│  ┌──────────────────────────────────────────┐      │
│  │    GUI (Cocoa/AppKit or AUv2 Generic)    │      │
│  │  • Topology visualizer                   │      │
│  │  • Per-voice parameter control           │      │
│  │  • Spectral analyzer                     │      │
│  │  • Preset browser                        │      │
│  └──────────────────────────────────────────┘      │
│                                                      │
└─────────────────────────────────────────────────────┘
```

### 1.2 Core DSP Architecture

#### Modal Voice Engine (per voice)

```c
typedef struct {
    // ESP32 modal_node_t core (direct port)
    mode_state_t modes[4];        // 4 complex modes
    float omega[4];                // Angular frequencies
    float gamma[4];                // Damping coefficients
    float weight[4];               // Audio contribution weights

    // AU-specific additions
    float pitch_bend;              // MIDI pitch bend
    float velocity;                // Note velocity (0-1)
    uint8_t midi_note;             // Triggering note
    bool active;                   // Voice active flag

    // Coupling state
    float coupling_inputs[4];      // From other voices
    float poke_envelope;           // Transient excitation

    // Render state
    uint64_t phase_accum[4];       // Per-mode phase (16.16 fixed-point)
    float smooth_amp[4];           // Smoothed amplitudes
} au_modal_voice_t;
```

#### Voice Manager

```objc
@interface ModalVoiceManager : NSObject {
    au_modal_voice_t voices[MAX_POLYPHONY];  // Default 16 voices
    uint8_t voice_allocation[128];           // MIDI note → voice mapping
    uint32_t voice_age[MAX_POLYPHONY];       // For voice stealing

    // Coupling matrix (sparse, topology-dependent)
    float coupling_matrix[MAX_POLYPHONY][MAX_POLYPHONY];
}

- (void)noteOn:(UInt8)note velocity:(UInt8)vel;
- (void)noteOff:(UInt8)note;
- (void)renderAudio:(float*)outL outR:(float*)outR frames:(UInt32)frames;
- (void)updateCouplingTopology:(TopologyType)type;
@end
```

### 1.3 Audio Rendering Pipeline

```
Logic MIDI Input
     ↓
AU MIDI Event Processing
     ↓
Voice Allocation / Note Triggering
     ↓
Per-Voice Modal Update (500 Hz or per-block)
     ├─ Exponential integration: a_k ← a_k·exp(λΔt) + u_k
     ├─ Coupling input from neighbor voices
     ├─ Poke envelope processing
     └─ Amplitude smoothing
     ↓
Audio Synthesis Loop (per voice, per sample)
     ├─ Mode 0: Carrier oscillator
     ├─ Mode 1: Detuning/beating
     ├─ Mode 2: Timbre modulation (FM-like)
     └─ Mode 3: Structural parameter (LFO-like)
     ↓
Voice Mixing (sum all active voices)
     ↓
Master Output (stereo or multi-channel)
     ↓
Core Audio Buffer (AU Render Callback)
     ↓
Logic Audio Engine
```

---

## 2. Porting Strategy

### 2.1 Direct Code Reuse (High Priority)

The following ESP32 modules can be ported **with minimal modification**:

#### ✅ Core Modal Oscillator (`modal_node.c/.h`)

- **Reuse:** 95%
- **Changes:**
  - Remove FreeRTOS dependencies (mutex → spinlock or atomic)
  - Replace `esp_timer_get_time()` with `mach_absolute_time()`
  - Adapt sample rate from 48kHz to Logic session rate (44.1/48/88.2/96 kHz)
- **Strategy:** Compile as pure C module, wrap in C++ voice class

#### ✅ Audio Synthesis (`audio_synth.c/.h`)

- **Reuse:** 80%
- **Changes:**
  - Replace I2S buffer output → AU render callback
  - Support variable sample rates (not just 48kHz)
  - Add oversampling option for HQ mode
  - Remove DMA/interrupt handling
- **Strategy:** Adapt to pull-based rendering (AU callback model)

#### ✅ Fast Math (`fast_sin`, `cexp` approximations)

- **Reuse:** 100%
- **Changes:** None required
- **Strategy:** Direct compilation, consider vDSP acceleration

#### ✅ Envelope Generators (`excitation_envelope_t`)

- **Reuse:** 90%
- **Changes:** Add ADSR envelope for sustained notes
- **Strategy:** Extend poke envelope with sustain phase

### 2.2 Architectural Translation

#### Network Communication → Voice Coupling

| ESP32 Concept | AU Plugin Equivalent |
|---------------|---------------------|
| ESP-NOW message | Internal function call |
| POKE message | Voice coupling matrix multiplication |
| Network latency (1-5ms) | Zero-latency local coupling |
| Topology configuration | Preset parameter (ring/small-world/cluster) |
| Broadcast message | Apply to all voices |

**Implementation:**

```cpp
class VoiceCouplingEngine {
public:
    void updateCoupling(float dt) {
        for (int i = 0; i < numVoices; i++) {
            if (!voices[i].active) continue;

            // Diffusive coupling (ESP32 equivalent)
            float coupling_input[4] = {0};
            for (int j = 0; j < numVoices; j++) {
                if (topology[i][j] > 0) {
                    float strength = topology[i][j];
                    for (int k = 0; k < 4; k++) {
                        coupling_input[k] += strength *
                            (voices[j].modes[k].a - voices[i].modes[k].a);
                    }
                }
            }

            // Apply to voice (as poke equivalent)
            applyPokeToVoice(&voices[i], coupling_input);
        }
    }
};
```

#### Session Configuration → AU Presets

| ESP32 | AU Plugin |
|-------|-----------|
| Binary config blob | CFPropertyList (Apple plist) |
| CRC32 validation | Built-in plist integrity |
| CFG_CHUNK messages | Direct file I/O |
| session_config_t | AUPreset + custom dictionary |

**Implementation:**

```objc
- (NSDictionary*)presetStateDictionary {
    return @{
        @"topology": @(topologyType),
        @"voiceConfigs": [self encodeVoiceConfigs],
        @"couplingStrength": @(couplingStrength),
        @"masterGain": @(masterGain),
        @"polyphony": @(maxPolyphony)
    };
}

- (void)setPresetState:(NSDictionary*)state {
    topologyType = [state[@"topology"] intValue];
    [self decodeVoiceConfigs:state[@"voiceConfigs"]];
    couplingStrength = [state[@"couplingStrength"] floatValue];
    // ... etc
}
```

### 2.3 New Components (AU-Specific)

#### MIDI Processing

```objc
- (OSStatus)handleMIDIEvent:(UInt8)status
                      data1:(UInt8)data1
                      data2:(UInt8)data2
                     offset:(UInt32)offsetFrames {
    switch (status & 0xF0) {
        case 0x90:  // Note On
            [voiceManager noteOn:data1 velocity:data2];
            break;
        case 0x80:  // Note Off
            [voiceManager noteOff:data1];
            break;
        case 0xE0:  // Pitch Bend
            [self setPitchBend:((data2 << 7) | data1)];
            break;
        case 0xB0:  // CC
            [self handleCC:data1 value:data2];
            break;
    }
    return noErr;
}
```

#### Parameter Automation

```objc
// AU parameters (Logic-automatable)
enum {
    kParam_CouplingStrength = 0,
    kParam_DampingGlobal,
    kParam_DetuneAmount,
    kParam_Mode0Weight,
    kParam_Mode1Weight,
    kParam_Mode2Weight,
    kParam_Mode3Weight,
    kParam_PokeStrength,
    kParam_PokeDuration,
    kParam_Topology,
    kParam_MasterGain,
    kNumParams
};
```

---

## 3. Implementation Phases

### Phase 1: Core DSP Port (2-3 weeks)

**Goal:** Single-voice modal oscillator working in standalone test app

**Tasks:**
1. ✅ Port `modal_node.c/.h` to macOS (remove FreeRTOS)
2. ✅ Port `audio_synth.c/.h` synthesis routines
3. ✅ Create C++ wrapper class `ModalVoice`
4. ✅ Build standalone test app (no AU yet)
5. ✅ Validate audio output matches ESP32 behavior
6. ✅ Profile CPU usage per voice

**Deliverables:**
- `modal_voice.cpp/.h` (C++ wrapper)
- `modal_dsp_port.c/.h` (ported ESP32 code)
- `test_modal_voice` (macOS command-line test)
- Performance report (voices per core)

**Success Criteria:**
- [ ] Single voice renders at 44.1/48/96 kHz
- [ ] Output spectrum matches Python simulation
- [ ] CPU usage <1% per voice @ 48kHz (Intel i7)
- [ ] No audible clicks or artifacts

---

### Phase 2: Audio Unit Shell (2 weeks)

**Goal:** Basic AU plugin loads in Logic, responds to MIDI

**Tasks:**
1. ✅ Create Xcode AU project (v2 instrument)
2. ✅ Implement `AUInstrumentBase` subclass
3. ✅ MIDI event handling (note on/off, velocity)
4. ✅ Voice allocation system (16-voice polyphony)
5. ✅ AU render callback integration
6. ✅ Basic parameter definitions (10-15 params)
7. ✅ Test in Logic Pro X & GarageBand

**Deliverables:**
- `ModalAttractors.component` (AU bundle)
- Installer script (.pkg)
- Logic project with test MIDI

**Success Criteria:**
- [ ] Plugin loads in Logic without crashes
- [ ] MIDI notes trigger voices correctly
- [ ] Polyphony works (up to 16 voices)
- [ ] Parameters respond to automation
- [ ] No audio glitches under normal load

---

### Phase 3: Voice Coupling & Topologies (2-3 weeks)

**Goal:** Multi-voice coupling system with topology presets

**Tasks:**
1. ✅ Implement coupling matrix system
2. ✅ Port topology generators (ring, small-world, cluster, hub)
3. ✅ Optimize coupling computation (sparse matrix)
4. ✅ Add topology selection parameter
5. ✅ Create 10-15 preset topologies
6. ✅ Implement per-voice routing controls
7. ✅ Add coupling visualization (optional GUI)

**Deliverables:**
- `topology_engine.cpp/.h`
- Preset library (JSON or plist)
- Documentation: "Topology Guide"

**Success Criteria:**
- [ ] Topology presets load correctly
- [ ] Coupling creates audible spectral evolution
- [ ] CPU overhead <10% for full 16-voice coupling
- [ ] Emergent behaviors match Python simulation

---

### Phase 4: Preset System & GUI (3 weeks)

**Goal:** Professional preset management and native macOS interface

**Tasks:**
1. ✅ AU preset serialization (plist format)
2. ✅ Factory preset library (20+ presets)
3. ✅ Preset browser UI (Cocoa)
4. ✅ Real-time topology visualizer (OpenGL or Metal)
5. ✅ Per-mode parameter editors
6. ✅ Spectral analyzer display
7. ✅ User preset save/load
8. ✅ Integration with Logic preset system

**Deliverables:**
- Native Cocoa GUI (or generic AU view)
- Factory preset bank (20+ sounds)
- User manual with preset guide

**Success Criteria:**
- [ ] GUI renders at 60fps without audio glitches
- [ ] Presets recall perfectly (no state drift)
- [ ] Topology visualizer shows real-time coupling
- [ ] Compatible with Logic preset menu

---

### Phase 5: Optimization & Polish (2 weeks)

**Goal:** Production-ready performance and stability

**Tasks:**
1. ✅ SIMD optimization (vDSP, Accelerate framework)
2. ✅ Apple Silicon native build (ARM64)
3. ✅ Oversampling option for HQ mode
4. ✅ Sample-accurate MIDI timing
5. ✅ Thread safety audit (AU callback constraints)
6. ✅ Memory leak testing (Instruments)
7. ✅ Logic certification testing
8. ✅ Beta testing with musicians

**Deliverables:**
- Universal binary (Intel + Apple Silicon)
- Performance optimization report
- Beta tester feedback summary

**Success Criteria:**
- [ ] 32+ voices @ 96kHz on M1 Mac
- [ ] Zero crashes in 24-hour stress test
- [ ] Passes Logic Pro X validation
- [ ] Positive beta tester feedback

---

### Phase 6: Release & Distribution (1 week)

**Goal:** Public release with documentation

**Tasks:**
1. ✅ Code signing & notarization (Apple)
2. ✅ Installer package (.pkg)
3. ✅ User manual (PDF)
4. ✅ Demo project for Logic
5. ✅ Video tutorial (YouTube)
6. ✅ Website/landing page
7. ✅ Submit to plugin marketplaces (optional)

**Deliverables:**
- Signed installer package
- User manual (30+ pages)
- Demo video (10-15 min)
- Logic demo project

---

## 4. Technical Challenges & Solutions

### Challenge 1: Real-Time Performance at High Polyphony

**Problem:** 16 voices × 4 modes × complex math = potential CPU bottleneck

**Solutions:**
1. **SIMD Optimization**
   - Use Accelerate framework for vector operations
   - Process 4 modes in parallel (SSE/NEON)
   - Batch voice updates (cache-friendly)

2. **Lookup Tables**
   - Pre-compute `exp(λΔt)` for common parameter ranges
   - Fast sine/cosine via 4096-point LUT + interpolation

3. **Dynamic Voice Management**
   - Voice stealing (oldest/quietest first)
   - Cull silent voices (amplitude threshold)
   - Adaptive polyphony based on CPU load

**Expected Performance:**
- Intel i7 (2020): 32 voices @ 48kHz, 16 voices @ 96kHz
- Apple M1: 64+ voices @ 48kHz, 32 voices @ 96kHz

---

### Challenge 2: Sample Rate Flexibility

**Problem:** ESP32 hardcoded to 48kHz, AU must support 44.1/48/88.2/96 kHz

**Solutions:**
1. **Parameterized Update Rate**
   ```c
   float control_dt = 1.0f / control_rate_hz;  // e.g., 500 Hz
   int samples_per_update = (int)(sample_rate / control_rate_hz);
   ```

2. **Frequency Scaling**
   ```c
   float omega_scaled = omega_base * (sample_rate / 48000.0f);
   ```

3. **Oversampling (Optional)**
   - Internal 2x/4x oversampling for HQ mode
   - Polyphase decimation filter

---

### Challenge 3: Zero-Latency AU Requirements

**Problem:** AU render callback cannot block or allocate memory

**Solutions:**
1. **Lock-Free MIDI Queue**
   ```cpp
   boost::lockfree::spsc_queue<MIDIEvent> midiQueue(256);
   ```

2. **Pre-Allocated Voice Pool**
   - No dynamic allocation in audio thread
   - Fixed-size circular buffers

3. **Deferred Parameter Updates**
   - Atomic parameter swaps
   - Interpolation for smooth changes

---

### Challenge 4: Topology Configuration Complexity

**Problem:** 16×16 coupling matrix = 256 parameters (too many for UI)

**Solutions:**
1. **Preset-Based Topologies**
   - 10-15 pre-designed topologies
   - Single parameter: topology selector

2. **Macro Parameters**
   - "Coupling Density" (0-1)
   - "Rewiring Probability" (small-world)
   - "Cluster Size" (clustered topology)

3. **Visualization**
   - Real-time graph view (nodes + edges)
   - Node highlighting on activity

---

### Challenge 5: Preset Compatibility with ESP32

**Problem:** Users may want to port ESP32 configurations to AU

**Solutions:**
1. **Import/Export Bridge**
   - Parser for ESP32 binary config blobs
   - Converter to AU plist format

2. **Topology Naming Convention**
   - Shared preset names between platforms
   - Version metadata for compatibility

---

## 5. Features & Capabilities

### Core Features (MVP)

- [x] **Polyphonic Synthesis:** 16-voice polyphony (expandable)
- [x] **Modal Oscillators:** 4 complex modes per voice
- [x] **Exact Integration:** Numerically stable exponential method
- [x] **Voice Coupling:** Diffusive coupling with topology control
- [x] **Topology Presets:** Ring, small-world, cluster, hub-spoke, custom
- [x] **MIDI Input:** Full MIDI 1.0 support (notes, CC, pitch bend)
- [x] **Parameter Automation:** 20+ automatable parameters
- [x] **AU Preset System:** Save/load/recall presets
- [x] **Sample Rate Support:** 44.1/48/88.2/96 kHz
- [x] **Universal Binary:** Intel + Apple Silicon

### Advanced Features (Post-MVP)

- [ ] **MPE Support:** Per-note expression (slide, pressure)
- [ ] **Modulation Matrix:** Flexible routing (LFOs → parameters)
- [ ] **Spectral Analyzer:** Real-time FFT display
- [ ] **Topology Editor:** Custom graph builder
- [ ] **Attractor Classifier:** Automatic pattern labeling (like Python version)
- [ ] **Recording/Playback:** Record/recall modal state evolution
- [ ] **Multi-Channel Output:** Spatial audio (5.1/7.1/Atmos)
- [ ] **AUv3 Version:** iOS/iPadOS compatibility

### Unique Selling Points

1. **Physics-Based Synthesis:** Not a traditional synth architecture
2. **Emergent Behavior:** Unpredictable, organic timbres
3. **Proven Algorithm:** Based on published research implementation
4. **Low CPU Overhead:** Efficient C core (not bloated C++)
5. **Network-Inspired:** Translates distributed system to polyphony
6. **Open Architecture:** Extensible for research/experimentation

---

## 6. Development Resources & Dependencies

### 6.1 Development Environment

**Required:**
- macOS 12.0+ (Monterey or later)
- Xcode 14.0+ (for AU SDK)
- Logic Pro X 10.7+ (testing)
- CMake 3.20+ (build system)

**Recommended:**
- Apple Silicon Mac (M1/M2/M3) for native testing
- Intel Mac for cross-platform validation
- Audio interface (low-latency testing)
- MIDI controller (Arturia KeyLab, Akai MPK, etc.)

### 6.2 Software Dependencies

**System Frameworks:**
- **AudioUnit.framework** (AU SDK)
- **CoreAudio.framework** (audio I/O)
- **CoreMIDI.framework** (MIDI)
- **Accelerate.framework** (vDSP, BLAS)
- **Cocoa.framework** (GUI)
- **OpenGL.framework** or **Metal.framework** (visualization)

**Third-Party Libraries:**
- **Boost.Lockfree** (lock-free queues) - optional
- **JUCE** (alternative AU wrapper) - optional, not recommended
- **Eigen** (linear algebra) - optional for topology math

### 6.3 Build System

**Recommended Structure:**
```
ModalAttractorsAU/
├── CMakeLists.txt              # Root build config
├── src/
│   ├── au_wrapper/             # Objective-C AU code
│   │   ├── ModalAttractorsAU.mm
│   │   ├── ModalVoiceManager.mm
│   │   └── ModalParameters.h
│   ├── dsp_core/               # C++ DSP engine
│   │   ├── ModalVoice.cpp
│   │   ├── TopologyEngine.cpp
│   │   └── VoiceAllocator.cpp
│   ├── esp32_port/             # Ported ESP32 C code
│   │   ├── modal_node.c
│   │   ├── audio_synth.c
│   │   └── fast_math.c
│   └── gui/                    # Cocoa interface
│       ├── ModalAttractorsView.mm
│       └── TopologyVisualizer.mm
├── Resources/
│   ├── Presets/
│   └── Info.plist
└── Tests/
    ├── test_modal_voice.cpp
    └── test_topology.cpp
```

### 6.4 Team & Expertise

**Recommended Team:**
1. **Audio DSP Engineer** (Senior)
   - C/C++ expertise
   - Real-time audio experience
   - Apple Accelerate framework knowledge

2. **Objective-C Developer** (Mid-Senior)
   - AU SDK experience (critical)
   - Cocoa/AppKit GUI development
   - macOS audio plugin lifecycle

3. **QA/Tester** (Junior-Mid)
   - Logic Pro X power user
   - MIDI controller experience
   - Bug reporting & regression testing

**Estimated Effort:**
- Senior DSP Engineer: 8-10 weeks full-time
- Objective-C Developer: 6-8 weeks full-time
- QA Tester: 2-3 weeks (part-time)

**Total:** ~4-5 person-months

---

## 7. Testing Strategy

### 7.1 Unit Testing

**DSP Core Tests:**
```cpp
TEST(ModalVoice, ExponentialIntegration) {
    // Verify stability at low damping
    // Compare to analytical solution
}

TEST(ModalVoice, FrequencyAccuracy) {
    // FFT analysis of output
    // Verify fundamental matches omega[0]
}

TEST(TopologyEngine, RingTopology) {
    // Verify neighbor connectivity
    // Check matrix sparsity
}
```

**Tools:**
- Google Test (C++ unit tests)
- XCTest (Objective-C tests)
- Instruments (profiling)

### 7.2 Integration Testing

**AU Validation:**
- **auval** (Apple's AU validation tool)
  ```bash
  auval -v aumu Mdal Anth  # Manufacturer code "Anth"
  ```
- Logic Pro X preset recall test
- Automation recording/playback
- Sample rate switching (44.1 ↔ 96 kHz)

**Stress Testing:**
- 24-hour continuous render (detect leaks)
- Rapid preset switching (state corruption)
- Extreme polyphony (64+ voices)
- Parameter flood (automation spam)

### 7.3 Comparison Testing

**Reference Validation:**
1. **ESP32 Parity Test**
   - Same configuration, same MIDI input
   - Compare spectrograms (Python vs AU output)
   - Validate coupling behavior

2. **Python Simulation Match**
   - Export modal state history from plugin
   - Compare to `network.py` simulation
   - Verify attractor convergence

**Metrics:**
- Spectral centroid deviation <1%
- Phase coherence delta <0.05
- Amplitude envelope correlation >0.99

### 7.4 Beta Testing

**Target Users:**
- 5-10 electronic musicians
- 2-3 sound designers (film/game)
- 1-2 academic researchers (for scientific validation)

**Feedback Focus:**
- Sound quality & character
- CPU performance
- UI usability
- Preset usefulness
- Crash/bug reports

---

## 8. Deliverables

### 8.1 Software

| Deliverable | Format | Description |
|------------|--------|-------------|
| **ModalAttractors.component** | AU Bundle | Signed, notarized AU v2 plugin |
| **Installer** | .pkg | macOS installer with uninstaller |
| **Factory Presets** | AU Presets | 20+ presets in standard AU format |
| **Demo Project** | Logic Pro X | Showcase project with MIDI examples |

### 8.2 Documentation

| Deliverable | Pages | Description |
|------------|-------|-------------|
| **User Manual** | 30-40 | Installation, workflow, parameter reference |
| **Topology Guide** | 10-15 | Explanation of topologies & coupling |
| **Preset Guide** | 15-20 | Factory preset descriptions & tips |
| **Developer Notes** | 20-30 | Architecture, porting notes, API reference |

### 8.3 Media

| Deliverable | Length | Description |
|------------|--------|-------------|
| **Installation Tutorial** | 5-7 min | Video: install & first sound |
| **Sound Design Tutorial** | 15-20 min | Video: creating custom patches |
| **Topology Exploration** | 10-15 min | Video: network behaviors |
| **Demo Tracks** | 3-5 tracks | Audio examples (SoundCloud/Bandcamp) |

---

## 9. Development Timeline

### Gantt Chart Overview

```
Week 1-3:   [████████████] Phase 1: Core DSP Port
Week 4-5:   [████████] Phase 2: AU Shell
Week 6-8:   [████████████] Phase 3: Voice Coupling
Week 9-11:  [████████████] Phase 4: Preset System & GUI
Week 12-13: [████████] Phase 5: Optimization & Polish
Week 14:    [████] Phase 6: Release & Distribution
            ↑
         TODAY
```

### Milestones

| Milestone | Week | Deliverable |
|-----------|------|-------------|
| **M1: DSP Core Complete** | 3 | Standalone test app renders audio |
| **M2: AU Loads in Logic** | 5 | Basic MIDI playback works |
| **M3: Coupling Functional** | 8 | Topology presets create emergent behavior |
| **M4: GUI Complete** | 11 | Professional interface ready |
| **M5: Beta Release** | 13 | Beta version to testers |
| **M6: Public Release** | 14 | Signed installer & documentation |

### Critical Path

1. **Weeks 1-3**: DSP port (blocking for all else)
2. **Weeks 4-5**: AU integration (blocking for GUI)
3. **Weeks 6-8**: Coupling system (key feature)
4. **Weeks 9-11**: GUI (parallel with optimization)
5. **Weeks 12-13**: Polish (parallel with beta testing)
6. **Week 14**: Launch (packaging only)

**Total Duration:** 14 weeks (~3.5 months)

---

## 10. Risk Assessment

### High-Risk Items

1. **AU SDK Learning Curve** (Mitigation: hire AU expert)
2. **CPU Performance at High Polyphony** (Mitigation: early profiling)
3. **Logic Certification Failure** (Mitigation: early auval testing)

### Medium-Risk Items

4. **GUI Complexity** (Mitigation: use generic AU view initially)
5. **Preset Compatibility** (Mitigation: versioning scheme)
6. **Apple Notarization Issues** (Mitigation: test early)

### Low-Risk Items

7. **DSP Port** (Well-understood C code)
8. **MIDI Processing** (Standard API)
9. **Documentation** (Can be completed in parallel)

---

## 11. Budget Estimate

### Development Costs (Contracted)

| Role | Rate | Duration | Subtotal |
|------|------|----------|----------|
| Senior DSP Engineer | $150/hr | 400 hrs | $60,000 |
| Objective-C Developer | $120/hr | 300 hrs | $36,000 |
| QA Tester | $60/hr | 100 hrs | $6,000 |
| **Total Labor** | | | **$102,000** |

### Software/Hardware

| Item | Cost |
|------|------|
| Apple Developer Account | $99/year |
| Logic Pro X (testing) | $200 |
| MIDI Controller | $300 |
| Audio Interface | $400 |
| macOS Hardware (if needed) | $2,000 |
| **Total** | **$2,999** |

### Contingency (20%)

| Category | Amount |
|----------|--------|
| Labor overrun | $20,400 |
| Unexpected tools | $1,000 |
| **Total Contingency** | **$21,400** |

### **Grand Total:** $126,399

---

## 12. Business Considerations

### 12.1 Pricing Strategy

**Recommended Price:** $79-$129 USD

**Rationale:**
- Professional synthesis plugin market range
- Unique algorithm justifies premium pricing
- Comparable: Arturia Pigments ($199), Native Instruments Reaktor ($199)
- Lower than above due to niche market

### 12.2 Distribution Options

1. **Direct Sales** (Gumroad, Paddle)
   - 95% revenue share
   - Full customer control
   - Requires own marketing

2. **Plugin Boutique / Splice**
   - 70-80% revenue share
   - Built-in audience
   - Credibility boost

3. **Open Source + Donations**
   - Community-driven development
   - Academic credibility
   - Patreon/GitHub Sponsors

### 12.3 Licensing

**Recommended:** Dual licensing
- **Non-Commercial:** GNU GPL v3 (open source)
- **Commercial:** Proprietary license for closed-source products

**Benefits:**
- Academic/research community can use freely
- Commercial developers must purchase license
- Source code available for transparency

---

## 13. Success Criteria

### Technical Success

- [ ] Passes Logic Pro X validation (auval)
- [ ] Matches ESP32 audio output (spectral correlation >0.95)
- [ ] CPU usage <50% @ 16 voices, 48kHz (M1 Mac)
- [ ] Zero crashes in 24-hour stress test
- [ ] Sample-accurate MIDI timing (<5ms latency)

### User Success

- [ ] 50+ beta testers provide feedback
- [ ] Average rating >4.0/5.0
- [ ] 20+ user-generated presets (community)
- [ ] Featured in 1+ music production blog/magazine
- [ ] 100+ sales in first month (if commercial)

### Scientific Success

- [ ] Matches Python simulation behavior (observables within 5%)
- [ ] Academic paper published (if applicable)
- [ ] Adopted by 3+ research groups
- [ ] Cited in modal synthesis literature

---

## 14. Future Roadmap (Post-V1)

### V1.1: Polish & Expansion
- MPE support (MIDI Polyphonic Expression)
- AUv3 version (iOS/iPadOS)
- Additional topology presets (20 → 50+)
- Spectral analyzer overlay

### V2.0: Advanced Features
- **Custom Topology Editor**: Visual graph builder
- **Attractor Classifier**: Real-time pattern recognition
- **Modulation Matrix**: LFOs, envelopes → parameters
- **Multi-Channel Output**: Spatial audio (Dolby Atmos)

### V3.0: Ecosystem
- **Standalone App**: Host-independent version
- **VST3 Port**: Cross-platform (Windows, Linux)
- **Cloud Presets**: Community sharing platform
- **Max/MSP Integration**: External object

---

## 15. Conclusion

The Modal Attractors AU plugin represents a unique opportunity to bring cutting-edge synthesis research to professional music production. By leveraging the proven ESP32 implementation, we can deliver a high-quality, performant plugin in a reasonable timeframe (14 weeks) with manageable risk.

The core DSP code is mature and well-tested, and the AU wrapper is a well-understood engineering task. The resulting instrument will offer sounds unavailable in conventional synthesizers, appealing to sound designers, electronic musicians, and researchers alike.

### Key Advantages of This Approach

1. **Proven Algorithm**: ESP32 code has been validated in real-world use
2. **Minimal Risk**: Direct code porting reduces unknowns
3. **Unique Sound**: No competing products with this architecture
4. **Research Credibility**: Based on published computational model
5. **Scalable Design**: Easy to extend with new features post-launch

### Recommended Next Steps

1. **Approve proposal** and secure budget
2. **Hire AU developer** (critical path dependency)
3. **Begin Phase 1** (DSP port) immediately
4. **Establish beta tester list** (recruit early)
5. **Create project repository** (GitHub private or public)

---

## Appendix A: Technical References

### ESP32 Codebase
- `/home/user/audio-sim/esp32/` - Main firmware
- `/home/user/audio-sim/esp32/docs/DESIGN_SPEC.md` - Architecture
- `/home/user/audio-sim/esp32/docs/API.md` - Protocol reference

### Python Reference Implementation
- `/home/user/audio-sim/src/network.py` - Core simulation
- `/home/user/audio-sim/experiments/audio_sonification.py` - Audio rendering

### Apple Documentation
- [Audio Unit Programming Guide](https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioUnitProgrammingGuide/)
- [Core Audio Overview](https://developer.apple.com/documentation/coreaudio)
- [Accelerate Framework](https://developer.apple.com/documentation/accelerate)

### Related Academic Work
- Modal synthesis theory: Fletcher & Rossing, "The Physics of Musical Instruments"
- Coupled oscillators: Strogatz, "Sync: The Emerging Science of Spontaneous Order"
- Network dynamics: Barabási, "Network Science"

---

## Appendix B: Code Examples

### Example 1: Modal Voice Render Loop (C++)

```cpp
void ModalVoice::renderBlock(float* outL, float* outR, int numFrames, float sampleRate) {
    const float omega_scale = 2.0f * M_PI / sampleRate;

    for (int i = 0; i < numFrames; i++) {
        // Update modal state (if control rate boundary)
        if (samplesSinceUpdate >= samplesPerUpdate) {
            updateModalState(controlDt);
            samplesSinceUpdate = 0;
        }

        // Synthesize audio from modal state
        float sample = 0.0f;
        for (int k = 0; k < 4; k++) {
            // Phase accumulation (16.16 fixed-point)
            phaseAccum[k] += (uint32_t)(modes[k].omega * omega_scale * 65536.0f);

            // Fast sine approximation
            float phase = (phaseAccum[k] >> 16) / 65536.0f * 2.0f * M_PI;
            float sine = fast_sin(phase);

            // Amplitude from complex mode (with smoothing)
            float amp = cabsf(modes[k].a);
            smoothAmp[k] += 0.12f * (amp - smoothAmp[k]);  // LP filter

            // Weighted contribution
            sample += smoothAmp[k] * modes[k].weight * sine;
        }

        // Apply velocity and master gain
        sample *= velocity * masterGain;

        // Output (mono for now, duplicate to stereo)
        outL[i] = sample;
        outR[i] = sample;

        samplesSinceUpdate++;
    }
}
```

### Example 2: AU MIDI Handler (Objective-C)

```objc
- (OSStatus)handleMIDIEvent:(UInt8)status
                      data1:(UInt8)data1
                      data2:(UInt8)data2
                     offset:(UInt32)offsetFrames {

    UInt8 channel = status & 0x0F;
    UInt8 command = status & 0xF0;

    switch (command) {
        case 0x90:  // Note On
            if (data2 > 0) {  // Velocity > 0
                ModalVoice* voice = [voiceManager allocateVoiceForNote:data1];
                if (voice) {
                    [voice noteOn:data1 velocity:data2/127.0f];
                    [self applyPokeToVoice:voice strength:data2/127.0f];
                }
            } else {  // Velocity 0 = Note Off
                [voiceManager releaseVoiceForNote:data1];
            }
            break;

        case 0x80:  // Note Off
            [voiceManager releaseVoiceForNote:data1];
            break;

        case 0xE0:  // Pitch Bend
            {
                int bend = (data2 << 7) | data1;
                float normalized = (bend - 8192) / 8192.0f;  // -1 to +1
                [self setPitchBendForAllVoices:normalized];
            }
            break;

        case 0xB0:  // Control Change
            [self handleCC:data1 value:data2];
            break;
    }

    return noErr;
}
```

### Example 3: Topology Generator (C++)

```cpp
void TopologyEngine::generateRingTopology(int numVoices, float couplingStrength) {
    // Clear existing matrix
    memset(couplingMatrix, 0, sizeof(couplingMatrix));

    // Connect each voice to neighbors in ring
    for (int i = 0; i < numVoices; i++) {
        int left = (i - 1 + numVoices) % numVoices;
        int right = (i + 1) % numVoices;

        couplingMatrix[i][left] = couplingStrength;
        couplingMatrix[i][right] = couplingStrength;
    }

    // Normalize (diffusive coupling: out = in - self)
    for (int i = 0; i < numVoices; i++) {
        float sum = 0.0f;
        for (int j = 0; j < numVoices; j++) {
            sum += couplingMatrix[i][j];
        }
        if (sum > 0.0f) {
            for (int j = 0; j < numVoices; j++) {
                couplingMatrix[i][j] /= sum;
            }
        }
    }
}
```

---

**End of Proposal**

---

**Contact Information:**

For technical questions about this proposal, please contact:
- Email: [Your Email]
- GitHub: https://github.com/carstenbund/audio-sim

**Document History:**
- V1.0 (2026-01-07): Initial proposal
