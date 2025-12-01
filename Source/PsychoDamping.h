#pragma once
#include <cmath>
#include <JuceHeader.h>

namespace PsychoDamping
{
    float mapPsychoDamping(float userDamping,
                           float minHz = 400.0f,
                           float maxHz = 16000.0f);

    void getDampingStages(float userDamping,
                          float& preHz, float& midHz, float& lateHz);

    float mapTilt(float tilt);

    // ======= NEW STATEFUL FILTER CLASS =======
    class OnePole
    {
    public:
        void reset() { z = 0.0f; }
        
        void prepare(float sampleRate, float userDamping)
        {
            cutoffHz = mapPsychoDamping(userDamping);
            const float pi = 3.14159265358979323846f;
            g = std::exp(-2.0f * pi * cutoffHz / sampleRate);
        }

        inline float process(float x)
        {
            z = g * z + (1.0f - g) * x;
            return z;
        }

    private:
        float z = 0.0f;
        float g = 0.0f;
        float cutoffHz = 8000.0f;
    };
}

class PsychoOnePole
{
public:
    PsychoOnePole() = default;

    void prepare(float sampleRate, float userDamping)
    {
        sr = sampleRate;
        setDamping(userDamping);
        z = 0.0f;
    }

    void setDamping(float userDamping)
    {
        // Convert user "damping" into psychoacoustic lowpass frequency
        float cutoffHz = PsychoDamping::mapPsychoDamping(userDamping);

        const float pi = 3.14159265359f;
        g = std::exp(-2.0f * pi * cutoffHz / sr);
    }

    float process(float x)
    {
        // Stable one-pole smoothing
        z = g * z + (1.0f - g) * x;
        return z;
    }

    void reset()
    {
        z = 0.0f;
    }

private:
    float sr = 44100.0f;
    float g  = 0.99f;
    float z  = 0.0f;
};

//==============================================================================
// Simple custom one-pole high-pass filter (no JUCE dependencies)
//==============================================================================

class PsychoHighPass
{
public:
    PsychoHighPass() = default;

    void prepare(float sampleRate, float cutoffHz)
    {
        sr = sampleRate;

        // One-pole HPF coefficient
        const float pi = 3.14159265359f;
        a = std::exp(-2.0f * pi * cutoffHz / sr);

        x1 = 0.0f;
        y1 = 0.0f;
    }

    inline float process(float x)
    {
        // Standard one-pole HPF:
        // y[n] = x[n] - x[n-1] + a * y[n-1]
        float y = (x - x1) + a * y1;
        x1 = x;
        y1 = y;
        return y;
    }

    void reset()
    {
        x1 = 0.0f;
        y1 = 0.0f;
    }

private:
    float sr = 44100.0f;
    float a  = 0.99f;   // pole coefficient
    float x1 = 0.0f;    // previous input
    float y1 = 0.0f;    // previous output
};

