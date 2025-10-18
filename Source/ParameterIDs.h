/*
  ==============================================================================

    ParameterIDs.h
    Central definition of all plugin parameter IDs and ranges

  ==============================================================================
*/

#pragma once

namespace ParameterIDs
{
    // DELAY PARAMETERS
    inline constexpr auto delayTimeLeft { "delayTimeLeft" };
    inline constexpr auto delayTimeRight { "delayTimeRight" };
    inline constexpr auto delayFeedback { "delayFeedback" };
    inline constexpr auto delayMix { "delayMix" };
    inline constexpr auto delaySyncEnabled { "delaySyncEnabled" };
    inline constexpr auto delaySyncDivisionLeft { "delaySyncDivisionLeft" };
    inline constexpr auto delaySyncDivisionRight { "delaySyncDivisionRight" };
    inline constexpr auto delayStereoMode { "delayStereoMode" };  // Stereo, PingPong, Haas
    inline constexpr auto delayHaasTime { "delayHaasTime" };
    inline constexpr auto delayLowCut { "delayLowCut" };
    inline constexpr auto delayHighCut { "delayHighCut" };
    inline constexpr auto delayDistortion { "delayDistortion" };
    inline constexpr auto delayDistortionMix { "delayDistortionMix" };

    // REVERB PARAMETERS (Algorithmic: Hall and Plate)
    inline constexpr auto reverbType { "reverbType" };  // Hall or Plate
    inline constexpr auto reverbSize { "reverbSize" };
    inline constexpr auto reverbDiffusion { "reverbDiffusion" };
    inline constexpr auto reverbDecay { "reverbDecay" };
    inline constexpr auto reverbDecayTime { "reverbDecayTime" };
    inline constexpr auto reverbDamping { "reverbDamping" };
    inline constexpr auto reverbWidth { "reverbWidth" };
    inline constexpr auto reverbModulation { "reverbModulation" };
    inline constexpr auto reverbMix { "reverbMix" };
    inline constexpr auto reverbPreDelay { "reverbPreDelay" };

    // CONVOLUTION REVERB PARAMETERS
    inline constexpr auto convolutionEnabled { "convolutionEnabled" };
    inline constexpr auto convolutionIRIndex { "convolutionIRIndex" };
    inline constexpr auto convolutionMix { "convolutionMix" };
    inline constexpr auto convolutionPreDelay { "convolutionPreDelay" };
    inline constexpr auto convolutionStretch { "convolutionStretch" };

    // COMPRESSOR PARAMETERS
    inline constexpr auto compressorEnabled { "compressorEnabled" };
    inline constexpr auto compressorThreshold { "compressorThreshold" };
    inline constexpr auto compressorRatio { "compressorRatio" };
    inline constexpr auto compressorAttack { "compressorAttack" };
    inline constexpr auto compressorRelease { "compressorRelease" };
    inline constexpr auto compressorKnee { "compressorKnee" };
    inline constexpr auto compressorMakeup { "compressorMakeup" };

    // EQUALIZER PARAMETERS (3-Band)
    inline constexpr auto eqEnabled { "eqEnabled" };
    inline constexpr auto eqLowFreq { "eqLowFreq" };
    inline constexpr auto eqLowGain { "eqLowGain" };
    inline constexpr auto eqMidFreq { "eqMidFreq" };
    inline constexpr auto eqMidGain { "eqMidGain" };
    inline constexpr auto eqMidQ { "eqMidQ" };
    inline constexpr auto eqHighFreq { "eqHighFreq" };
    inline constexpr auto eqHighGain { "eqHighGain" };

    // GLOBAL PARAMETERS
    inline constexpr auto masterBypass { "masterBypass" };
    inline constexpr auto outputGain { "outputGain" };
    inline constexpr auto dryWetMix { "dryWetMix" };

    // ROUTING/TOPOLOGY PARAMETERS
    inline constexpr auto routingTopology { "routingTopology" };  // Classic, ReverbBlend, etc.
    inline constexpr auto reverbBlendRatio { "reverbBlendRatio" };  // Algorithmic vs Convolution blend
    inline constexpr auto effectOrderCustom { "effectOrderCustom" };  // Custom routing serialization
}
