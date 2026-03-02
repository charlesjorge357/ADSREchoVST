#include "BasicDelay.h"

// -----------------------------------------------------------------------------
// Beat multipliers relative to one quarter note.
// delay_ms = (60000.0 / bpm) * multiplier
// Order must match SyncDivision enum declaration.
// -----------------------------------------------------------------------------
static constexpr float kDivisionMultipliers[] =
{
    4.0f,          // Whole
    2.0f,          // Half
    1.0f,          // Quarter        (1 beat)
    0.5f,          // Eighth
    0.25f,         // Sixteenth
    3.0f,          // DottedHalf
    1.5f,          // DottedQuarter
    0.75f,         // DottedEighth
    4.0f / 3.0f,   // TripletHalf
    2.0f / 3.0f,   // TripletQuarter
    1.0f / 3.0f,   // TripletEighth
};

// -----------------------------------------------------------------------------
BasicDelay::BasicDelay()  {}
BasicDelay::~BasicDelay() {}

// -----------------------------------------------------------------------------
float BasicDelay::divisionToMs(float bpm, SyncDivision div) noexcept
{
    return (60000.0f / bpm) * kDivisionMultipliers[static_cast<int>(div)];
}

// -----------------------------------------------------------------------------
void BasicDelay::applyTargetDelayMs(float ms)
{
    delayTimeMs = ms;

    if (sampleRate <= 0.0f)
        return;  // not prepared yet -- prepare() will initialise the smoother

    const float targetSamples = (ms / 1000.0f) * sampleRate;

    if (!juce::approximatelyEqual(targetSamples, smoothedDelaySamples.getTargetValue()))
        smoothedDelaySamples.setTargetValue(targetSamples);
}

// -----------------------------------------------------------------------------
void BasicDelay::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = static_cast<float>(spec.sampleRate);

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    delayLineL.prepare(monoSpec);
    delayLineR.prepare(monoSpec);

    // Determine initial target
    const float initMs      = bpmSyncEnabled ? divisionToMs(currentBpm, syncDivision) : delayTimeMs;
    const float initSamples = (initMs / 1000.0f) * sampleRate;

    // Snap smoother to value immediately -- no audible ramp on first prepare
    smoothedDelaySamples.reset(sampleRate, rampTimeMs / 1000.0);
    smoothedDelaySamples.setCurrentAndTargetValue(initSamples);

    // Filters
    lowpassL.prepare(monoSpec);  lowpassR.prepare(monoSpec);
    highpassL.prepare(monoSpec); highpassR.prepare(monoSpec);

    lowpassL.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
    lowpassR.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
    highpassL.setType(juce::dsp::FirstOrderTPTFilterType::highpass);
    highpassR.setType(juce::dsp::FirstOrderTPTFilterType::highpass);

    lowpassL.setCutoffFrequency(lowpassFreqValue);
    lowpassR.setCutoffFrequency(lowpassFreqValue);
    highpassL.setCutoffFrequency(highpassFreqValue);
    highpassR.setCutoffFrequency(highpassFreqValue);

    reset();
}

// -----------------------------------------------------------------------------
void BasicDelay::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float* leftChannel  = buffer.getWritePointer(0);
    float* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    const float wet      = mixAmount;
    const float dry      = 1.0f - mixAmount;
    const float fb       = feedbackAmount;
    float fbL            = feedbackL;
    float fbR            = feedbackR;

    const float panGainL  = 1.0f - juce::jmax(0.0f, panValue);
    const float panGainR  = 1.0f + juce::jmin(0.0f, panValue);
    const float phaseSign = (delayMode == DelayMode::Inverted) ? -1.0f : 1.0f;
    const bool isPingPong = (delayMode == DelayMode::PingPong) && (rightChannel != nullptr);

    for (int i = 0; i < numSamples; ++i)
    {
        // ---- Advance the smooth read-head position ---------------------------
        // getNextValue() returns the current interpolated delay in samples.
        // We feed this directly into readFractional() which performs linear
        // interpolation between adjacent buffer slots, giving us a continuously
        // moving, artefact-free read head even while BPM is being automated.
        const float delaySamples = smoothedDelaySamples.getNextValue();

        // ---- Write new input into the delay buffer ---------------------------
        const float inputL = leftChannel[i];

        if (rightChannel != nullptr)
        {
            const float inputR = rightChannel[i];

            if (isPingPong)
            {
                // Cross-feed: L reads from R's feedback and vice versa
                delayLineL.pushSample(0, inputL + fbR * fb);
                delayLineR.pushSample(0, inputR + fbL * fb);
            }
            else
            {
                delayLineL.pushSample(0, inputL + fbL * fb);
                delayLineR.pushSample(0, inputR + fbR * fb);
            }

            // ---- Read with fractional interpolation -------------------------
            // readFractional() uses linear interp between floor/ceil buffer
            // positions, so fractional delaySamples produces smooth output
            // with no zipper noise or discontinuities.
            const float delayedL = delayLineL.readFractional(0, delaySamples);
            const float delayedR = delayLineR.readFractional(0, delaySamples);

            // Update feedback state through tone-shaping filters
            fbL = highpassL.processSample(0, lowpassL.processSample(0, delayedL));
            fbR = highpassR.processSample(0, lowpassR.processSample(0, delayedR));

            leftChannel[i]  = inputL * dry + delayedL * phaseSign * wet * panGainL;
            rightChannel[i] = inputR * dry + delayedR * phaseSign * wet * panGainR;
        }
        else
        {
            // Mono path
            delayLineL.pushSample(0, inputL + fbL * fb);

            const float delayedL = delayLineL.readFractional(0, delaySamples);
            fbL = highpassL.processSample(0, lowpassL.processSample(0, delayedL));

            leftChannel[i] = inputL * dry + delayedL * phaseSign * wet;
        }
    }

    feedbackL = fbL;
    feedbackR = fbR;
}

// -----------------------------------------------------------------------------
void BasicDelay::reset()
{
    delayLineL.reset();
    delayLineR.reset();
    feedbackL = 0.0f;
    feedbackR = 0.0f;
    lowpassL.reset();  lowpassR.reset();
    highpassL.reset(); highpassR.reset();

    // Snap the smoother so stale ramps don't bleed into the next session
    smoothedDelaySamples.setCurrentAndTargetValue(
        smoothedDelaySamples.getTargetValue());
}

// -----------------------------------------------------------------------------
void BasicDelay::setDelayTime(float delayMs)
{
    bpmSyncEnabled = false;
    applyTargetDelayMs(delayMs);
}

void BasicDelay::setBpmSync(bool enabled, float bpm, SyncDivision division)
{
    bpmSyncEnabled = enabled;
    currentBpm     = bpm;
    syncDivision   = division;

    if (enabled)
        applyTargetDelayMs(divisionToMs(bpm, division));
}

void BasicDelay::setCurrentBpm(float bpm)
{
    if (!bpmSyncEnabled || juce::approximatelyEqual(bpm, currentBpm))
        return;

    currentBpm = bpm;
    applyTargetDelayMs(divisionToMs(bpm, syncDivision));
}

void BasicDelay::setRampTimeMs(float rampMs)
{
    rampTimeMs = juce::jmax(1.0f, rampMs);
    if (sampleRate > 0.0f)
        smoothedDelaySamples.reset(sampleRate, rampTimeMs / 1000.0);
}

// -----------------------------------------------------------------------------
void BasicDelay::setFeedback(float feedback)
{
    feedbackAmount = juce::jlimit(0.0f, 0.95f, feedback);
}

void BasicDelay::setMix(float mix)
{
    mixAmount = juce::jlimit(0.0f, 1.0f, mix);
}

void BasicDelay::setMode(DelayMode mode)
{
    delayMode = mode;
}

void BasicDelay::setPan(float pan)
{
    panValue = juce::jlimit(-1.0f, 1.0f, pan);
}

void BasicDelay::setLowpassFreq(float freq)
{
    float clamped = juce::jlimit(200.0f, 20000.0f, freq);
    if (!juce::approximatelyEqual(lowpassFreqValue, clamped))
    {
        lowpassFreqValue = clamped;
        lowpassL.setCutoffFrequency(lowpassFreqValue);
        lowpassR.setCutoffFrequency(lowpassFreqValue);
    }
}

void BasicDelay::setHighpassFreq(float freq)
{
    float clamped = juce::jlimit(20.0f, 5000.0f, freq);
    if (!juce::approximatelyEqual(highpassFreqValue, clamped))
    {
        highpassFreqValue = clamped;
        highpassL.setCutoffFrequency(highpassFreqValue);
        highpassR.setCutoffFrequency(highpassFreqValue);
    }
}