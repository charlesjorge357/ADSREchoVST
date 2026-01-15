// ================================================================
// MLReverbPresets.h - Generated ML-optimized reverb presets
// These presets were selected from 100 variations using:
//   1. Latin Hypercube Sampling for parameter generation
//   2. Quality filtering based on RT60, clarity, spectral balance
//   3. K-means clustering to ensure diversity
// ================================================================
#pragma once

#include "MLReverbParameters.h"  // Your existing MLDatorroParameters struct
#include <array>
#include <string>

struct ReverbPreset
{
    std::string name;
    float qualityScore;
    MLDatorroParameters mlParams;
};

// 10 diverse, high-quality presets generated via ML pipeline
inline std::array<ReverbPreset, 10> getMLReverbPresets()
{
    std::array<ReverbPreset, 10> presets;
    
    // Cluster 0 - Tight Room (quality score: 4.0)
    presets[0].name = "Tight Room";
    presets[0].qualityScore = 4.0f;
    presets[0].mlParams.earlyAllpassScales = { 1.6206f, 0.6384f };
    presets[0].mlParams.loopDelayScalesL = { 2.0140f, 1.0591f, 1.1425f, 0.4017f };
    presets[0].mlParams.loopDelayScalesR = { 2.2690f, 1.4217f, 0.8902f, 0.5891f };
    presets[0].mlParams.diffusionCoeff1 = 0.5521f;
    presets[0].mlParams.diffusionCoeff2 = 0.6801f;
    presets[0].mlParams.decayScale = 0.5979f;
    presets[0].mlParams.dampingScale = 1.5477f;
    
    // Cluster 1 - Warm Chamber (quality score: 2.0)
    presets[1].name = "Warm Chamber";
    presets[1].qualityScore = 2.0f;
    presets[1].mlParams.earlyAllpassScales = { 0.5773f, 1.7440f };
    presets[1].mlParams.loopDelayScalesL = { 1.9923f, 0.7845f, 0.9068f, 0.9945f };
    presets[1].mlParams.loopDelayScalesR = { 0.4442f, 0.3970f, 0.8942f, 1.1851f };
    presets[1].mlParams.diffusionCoeff1 = 0.7137f;
    presets[1].mlParams.diffusionCoeff2 = 0.5494f;
    presets[1].mlParams.decayScale = 0.8276f;
    presets[1].mlParams.dampingScale = 1.3146f;
    
    // Cluster 2 - Medium Hall (quality score: 3.5)
    presets[2].name = "Medium Hall";
    presets[2].qualityScore = 3.5f;
    presets[2].mlParams.earlyAllpassScales = { 0.9591f, 1.6590f };
    presets[2].mlParams.loopDelayScalesL = { 1.0124f, 0.5272f, 2.7462f, 2.2413f };
    presets[2].mlParams.loopDelayScalesR = { 2.4102f, 0.8357f, 1.0084f, 1.0185f };
    presets[2].mlParams.diffusionCoeff1 = 0.4458f;
    presets[2].mlParams.diffusionCoeff2 = 0.7702f;
    presets[2].mlParams.decayScale = 1.2673f;
    presets[2].mlParams.dampingScale = 0.9075f;
    
    // Cluster 3 - Bright Plate (quality score: 4.0)
    presets[3].name = "Bright Plate";
    presets[3].qualityScore = 4.0f;
    presets[3].mlParams.earlyAllpassScales = { 1.6754f, 1.6429f };
    presets[3].mlParams.loopDelayScalesL = { 0.4246f, 1.1240f, 2.0467f, 0.5234f };
    presets[3].mlParams.loopDelayScalesR = { 2.0610f, 2.4628f, 1.8169f, 2.5770f };
    presets[3].mlParams.diffusionCoeff1 = 0.3135f;
    presets[3].mlParams.diffusionCoeff2 = 0.5556f;
    presets[3].mlParams.decayScale = 0.6722f;
    presets[3].mlParams.dampingScale = 2.3118f;
    
    // Cluster 4 - Wide Space (quality score: 3.5)
    presets[4].name = "Wide Space";
    presets[4].qualityScore = 3.5f;
    presets[4].mlParams.earlyAllpassScales = { 0.6925f, 0.9791f };
    presets[4].mlParams.loopDelayScalesL = { 0.6275f, 2.3918f, 1.2931f, 0.8762f };
    presets[4].mlParams.loopDelayScalesR = { 1.3610f, 2.3507f, 0.6266f, 0.4192f };
    presets[4].mlParams.diffusionCoeff1 = 0.6353f;
    presets[4].mlParams.diffusionCoeff2 = 0.7131f;
    presets[4].mlParams.decayScale = 1.1545f;
    presets[4].mlParams.dampingScale = 1.6220f;
    
    // Cluster 5 - Dense Diffusion (quality score: 2.0)
    presets[5].name = "Dense Diffusion";
    presets[5].qualityScore = 2.0f;
    presets[5].mlParams.earlyAllpassScales = { 1.1639f, 1.8088f };
    presets[5].mlParams.loopDelayScalesL = { 0.3686f, 0.4846f, 1.3746f, 2.4734f };
    presets[5].mlParams.loopDelayScalesR = { 2.5285f, 0.4842f, 0.8036f, 0.8832f };
    presets[5].mlParams.diffusionCoeff1 = 0.9117f;
    presets[5].mlParams.diffusionCoeff2 = 0.3315f;
    presets[5].mlParams.decayScale = 0.7046f;
    presets[5].mlParams.dampingScale = 2.0026f;
    
    // Cluster 6 - Smooth Decay (quality score: 3.5)
    presets[6].name = "Smooth Decay";
    presets[6].qualityScore = 3.5f;
    presets[6].mlParams.earlyAllpassScales = { 1.1586f, 1.2797f };
    presets[6].mlParams.loopDelayScalesL = { 1.1734f, 2.0275f, 0.8401f, 2.3740f };
    presets[6].mlParams.loopDelayScalesR = { 1.8723f, 0.8732f, 1.7324f, 1.3016f };
    presets[6].mlParams.diffusionCoeff1 = 0.8196f;
    presets[6].mlParams.diffusionCoeff2 = 0.4547f;
    presets[6].mlParams.decayScale = 0.8375f;
    presets[6].mlParams.dampingScale = 0.6820f;
    
    // Cluster 7 - Long Tail (quality score: 2.5)
    presets[7].name = "Long Tail";
    presets[7].qualityScore = 2.5f;
    presets[7].mlParams.earlyAllpassScales = { 1.9704f, 1.4749f };
    presets[7].mlParams.loopDelayScalesL = { 1.3374f, 2.4240f, 1.9587f, 1.3563f };
    presets[7].mlParams.loopDelayScalesR = { 1.2750f, 1.9261f, 2.0195f, 2.9852f };
    presets[7].mlParams.diffusionCoeff1 = 0.9308f;
    presets[7].mlParams.diffusionCoeff2 = 0.8228f;
    presets[7].mlParams.decayScale = 1.4266f;
    presets[7].mlParams.dampingScale = 2.9836f;
    
    // Cluster 8 - Dark Hall (quality score: 3.5)
    presets[8].name = "Dark Hall";
    presets[8].qualityScore = 3.5f;
    presets[8].mlParams.earlyAllpassScales = { 1.3707f, 1.7851f };
    presets[8].mlParams.loopDelayScalesL = { 2.2381f, 0.9931f, 0.5297f, 1.1678f };
    presets[8].mlParams.loopDelayScalesR = { 1.8658f, 1.6445f, 1.1189f, 2.7426f };
    presets[8].mlParams.diffusionCoeff1 = 0.3451f;
    presets[8].mlParams.diffusionCoeff2 = 0.6159f;
    presets[8].mlParams.decayScale = 1.3432f;
    presets[8].mlParams.dampingScale = 2.8128f;
    
    // Cluster 9 - Crystal Clear (quality score: 4.0)
    presets[9].name = "Crystal Clear";
    presets[9].qualityScore = 4.0f;
    presets[9].mlParams.earlyAllpassScales = { 1.4642f, 0.6797f };
    presets[9].mlParams.loopDelayScalesL = { 2.1687f, 0.4577f, 1.0250f, 1.1360f };
    presets[9].mlParams.loopDelayScalesR = { 0.3332f, 1.7850f, 0.4513f, 1.4939f };
    presets[9].mlParams.diffusionCoeff1 = 0.9239f;
    presets[9].mlParams.diffusionCoeff2 = 0.3837f;
    presets[9].mlParams.decayScale = 1.3599f;
    presets[9].mlParams.dampingScale = 1.7359f;
    
    return presets;
}

// Helper to get a preset by index
inline MLDatorroParameters getMLPreset(int index)
{
    auto presets = getMLReverbPresets();
    if (index >= 0 && index < 10)
        return presets[index].mlParams;
    return MLDatorroParameters(); // Default
}

// Helper to get preset name
inline std::string getMLPresetName(int index)
{
    auto presets = getMLReverbPresets();
    if (index >= 0 && index < 10)
        return presets[index].name;
    return "Default";
}
