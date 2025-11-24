#include "PsychoDamping.h"
#include <cmath>

namespace PsychoDamping
{
    float mapPsychoDamping(float userDamping,
                           float minHz,
                           float maxHz)
    {
        userDamping = juce::jlimit(0.0f, 1.0f, userDamping);
        float perceptual = std::pow(userDamping, 0.35f);
        return minHz * std::pow(maxHz / minHz, 1.0f - perceptual);
    }

    void getDampingStages(float userDamping,
                          float& preHz, float& midHz, float& lateHz)
    {
        preHz  = mapPsychoDamping(userDamping * 0.40f);
        midHz  = mapPsychoDamping(userDamping * 0.70f);
        lateHz = mapPsychoDamping(userDamping * 1.00f);
    }

    float mapTilt(float tilt)
    {
        tilt = juce::jlimit(0.0f, 1.0f, tilt);
        return 1200.0f + 8000.0f * (1.0f - tilt);
    }
}