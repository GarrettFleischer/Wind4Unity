#include "BlockLPF.h"
#include <complex>
#include "juce_core/maths/juce_MathsFunctions.h"

Wind4Unity::BlockLPF::BlockLPF() = default;

Wind4Unity::BlockLPF::~BlockLPF() = default;

void Wind4Unity::BlockLPF::prepare(float cutoff, int samplesPerBlock, double sampleRate)
{
    const float c0 = std::tan(juce::MathConstants<double>::pi * cutoff / (sampleRate / samplesPerBlock));
    coeff = c0 / (1 + c0);
}

float Wind4Unity::BlockLPF::processSample(float input)
{
    lastOutput = (1.0f - coeff) * lastOutput + coeff * input;
    lastOutput = lastOutput < 0.00001f ? 0.0f : lastOutput;
    return lastOutput;
}
