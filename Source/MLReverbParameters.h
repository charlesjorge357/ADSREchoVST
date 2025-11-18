// ================================================================
// MLReverbParameters.h - Shared ML parameter structures
// ================================================================
#pragma once
#include <array>
#include <vector>
#include <algorithm>

// ML Parameters for HybridPlate
struct MLReverbParameters
{
    // Diffuser scaling factors (one per diffuser)
    std::array<float, 4> diffuserScales { 1.0f, 1.0f, 1.0f, 1.0f };
    
    // FDN delay line scaling factors (one per FDN line)
    std::array<float, 4> fdnDelayScales { 1.0f, 1.0f, 1.0f, 1.0f };
    
    // Feedback matrix scaling (overall gain)
    float feedbackMatrixScale = 1.0f;
    
    // Per-line damping frequency multipliers
    std::array<float, 4> dampingScales { 1.0f, 1.0f, 1.0f, 1.0f };
    
    // Allpass feedback coefficients (can be learned)
    float diffuserFeedback = 0.7f;
    
    // Decay multiplier
    float decayScale = 1.0f;
    
    // Stereo width multiplier
    float stereoWidthScale = 1.0f;
    
    void clipToSafeRanges()
    {
        for (auto& scale : diffuserScales)
            scale = std::clamp(scale, 0.1f, 10.0f);
        
        for (auto& scale : fdnDelayScales)
            scale = std::clamp(scale, 0.1f, 10.0f);
        
        feedbackMatrixScale = std::clamp(feedbackMatrixScale, 0.1f, 2.0f);
        
        for (auto& scale : dampingScales)
            scale = std::clamp(scale, 0.1f, 10.0f);
        
        diffuserFeedback = std::clamp(diffuserFeedback, 0.0f, 0.99f);
        decayScale = std::clamp(decayScale, 0.1f, 2.0f);
        stereoWidthScale = std::clamp(stereoWidthScale, 0.0f, 5.0f);
    }
    
    std::vector<float> toVector() const
    {
        std::vector<float> params;
        params.insert(params.end(), diffuserScales.begin(), diffuserScales.end());
        params.insert(params.end(), fdnDelayScales.begin(), fdnDelayScales.end());
        params.push_back(feedbackMatrixScale);
        params.insert(params.end(), dampingScales.begin(), dampingScales.end());
        params.push_back(diffuserFeedback);
        params.push_back(decayScale);
        params.push_back(stereoWidthScale);
        return params;
    }
    
    void fromVector(const std::vector<float>& params)
    {
        if (params.size() < 15) return;
        
        size_t idx = 0;
        for (size_t i = 0; i < 4; ++i)
            diffuserScales[i] = params[idx++];
        
        for (size_t i = 0; i < 4; ++i)
            fdnDelayScales[i] = params[idx++];
        
        feedbackMatrixScale = params[idx++];
        
        for (size_t i = 0; i < 4; ++i)
            dampingScales[i] = params[idx++];
        
        diffuserFeedback = params[idx++];
        decayScale = params[idx++];
        stereoWidthScale = params[idx++];
        
        clipToSafeRanges();
    }
    
    static constexpr size_t getParameterCount() { return 15; }
};

// ML Parameters for DatorroHall
struct MLDatorroParameters
{
    // Early reflection allpass scaling
    std::array<float, 2> earlyAllpassScales { 1.0f, 1.0f };
    
    // Main delay line scaling (4 per channel, L and R)
    std::array<float, 4> loopDelayScalesL { 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 4> loopDelayScalesR { 1.0f, 1.0f, 1.0f, 1.0f };
    
    // Diffusion coefficients (learnable)
    float diffusionCoeff1 = 0.75f;
    float diffusionCoeff2 = 0.625f;
    
    // Decay scaling
    float decayScale = 1.0f;
    
    // Damping multiplier
    float dampingScale = 1.0f;
    
    void clipToSafeRanges()
    {
        for (auto& scale : earlyAllpassScales)
            scale = std::clamp(scale, 0.1f, 10.0f);
        
        for (auto& scale : loopDelayScalesL)
            scale = std::clamp(scale, 0.1f, 10.0f);
        
        for (auto& scale : loopDelayScalesR)
            scale = std::clamp(scale, 0.1f, 10.0f);
        
        diffusionCoeff1 = std::clamp(diffusionCoeff1, 0.0f, 0.99f);
        diffusionCoeff2 = std::clamp(diffusionCoeff2, 0.0f, 0.99f);
        decayScale = std::clamp(decayScale, 0.1f, 2.0f);
        dampingScale = std::clamp(dampingScale, 0.1f, 10.0f);
    }
    
    std::vector<float> toVector() const
    {
        std::vector<float> params;
        params.insert(params.end(), earlyAllpassScales.begin(), earlyAllpassScales.end());
        params.insert(params.end(), loopDelayScalesL.begin(), loopDelayScalesL.end());
        params.insert(params.end(), loopDelayScalesR.begin(), loopDelayScalesR.end());
        params.push_back(diffusionCoeff1);
        params.push_back(diffusionCoeff2);
        params.push_back(decayScale);
        params.push_back(dampingScale);
        return params;
    }
    
    void fromVector(const std::vector<float>& params)
    {
        if (params.size() < 13) return;
        
        size_t idx = 0;
        for (size_t i = 0; i < 2; ++i)
            earlyAllpassScales[i] = params[idx++];
        
        for (size_t i = 0; i < 4; ++i)
            loopDelayScalesL[i] = params[idx++];
        
        for (size_t i = 0; i < 4; ++i)
            loopDelayScalesR[i] = params[idx++];
        
        diffusionCoeff1 = params[idx++];
        diffusionCoeff2 = params[idx++];
        decayScale = params[idx++];
        dampingScale = params[idx++];
        
        clipToSafeRanges();
    }
    
    static constexpr size_t getParameterCount() { return 13; }
};