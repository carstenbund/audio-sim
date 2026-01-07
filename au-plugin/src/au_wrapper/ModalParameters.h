/**
 * @file ModalParameters.h
 * @brief AU parameter definitions for Modal Attractors plugin
 *
 * Defines all automatable parameters for the Audio Unit.
 */

#ifndef MODAL_PARAMETERS_H
#define MODAL_PARAMETERS_H

// ============================================================================
// Parameter IDs
// ============================================================================

enum {
    // Global parameters
    kParam_MasterGain = 0,
    kParam_CouplingStrength,
    kParam_Topology,

    // Per-mode parameters (Mode 0)
    kParam_Mode0_Frequency,
    kParam_Mode0_Damping,
    kParam_Mode0_Weight,

    // Per-mode parameters (Mode 1)
    kParam_Mode1_Frequency,
    kParam_Mode1_Damping,
    kParam_Mode1_Weight,

    // Per-mode parameters (Mode 2)
    kParam_Mode2_Frequency,
    kParam_Mode2_Damping,
    kParam_Mode2_Weight,

    // Per-mode parameters (Mode 3)
    kParam_Mode3_Frequency,
    kParam_Mode3_Damping,
    kParam_Mode3_Weight,

    // Excitation parameters
    kParam_PokeStrength,
    kParam_PokeDuration,

    // Voice parameters
    kParam_Polyphony,
    kParam_Personality,

    kNumParams
};

// ============================================================================
// Parameter Ranges
// ============================================================================

// Master gain: 0.0 to 1.0 (linear)
#define kMasterGain_Min 0.0f
#define kMasterGain_Max 1.0f
#define kMasterGain_Default 0.7f

// Coupling strength: 0.0 to 1.0
#define kCouplingStrength_Min 0.0f
#define kCouplingStrength_Max 1.0f
#define kCouplingStrength_Default 0.3f

// Topology: 0-6 (discrete)
#define kTopology_Min 0
#define kTopology_Max 6
#define kTopology_Default 0  // Ring

// Mode frequency multipliers: 0.5 to 8.0
#define kModeFreq_Min 0.5f
#define kModeFreq_Max 8.0f

// Mode damping: 0.1 to 5.0
#define kModeDamping_Min 0.1f
#define kModeDamping_Max 5.0f

// Mode weight: 0.0 to 1.0
#define kModeWeight_Min 0.0f
#define kModeWeight_Max 1.0f

// Poke strength: 0.0 to 1.0
#define kPokeStrength_Min 0.0f
#define kPokeStrength_Max 1.0f
#define kPokeStrength_Default 0.5f

// Poke duration: 1.0 to 50.0 ms
#define kPokeDuration_Min 1.0f
#define kPokeDuration_Max 50.0f
#define kPokeDuration_Default 10.0f

// Polyphony: 1 to 32 voices
#define kPolyphony_Min 1
#define kPolyphony_Max 32
#define kPolyphony_Default 16

// Personality: 0 = Resonator, 1 = Self-oscillator
#define kPersonality_Min 0
#define kPersonality_Max 1
#define kPersonality_Default 0

// ============================================================================
// Parameter Names
// ============================================================================

#define kParamName_MasterGain "Master Gain"
#define kParamName_CouplingStrength "Coupling Strength"
#define kParamName_Topology "Topology"
#define kParamName_PokeStrength "Poke Strength"
#define kParamName_PokeDuration "Poke Duration"
#define kParamName_Polyphony "Polyphony"
#define kParamName_Personality "Personality"

#endif // MODAL_PARAMETERS_H
