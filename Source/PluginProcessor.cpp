/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

//==============================================================================
Wind4UnityAudioProcessor::Wind4UnityAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    )
{
    addParameter(gain = new juce::AudioParameterFloat(
        "Master Gain", "Master Gain", 0.0f, 1.0f, 0.5f));

    // addParameter(dstBPCutoffFreq = new juce::AudioParameterFloat(
    //     "DstCutoff", "DistantIntensity", 0.004f, 1000.0f, 10.0f));
    //
    // addParameter(dstBPQ = new juce::AudioParameterFloat(
    //     "DstQ", "DistantQ", 1.0f, 100.0f, 10.0f));

    addParameter(dstAmplitude = new juce::AudioParameterFloat(
        "DstAmp", "DistantAmplitude", 0.0001f, 1.5f, 0.75f));

    addParameter(windForce = new juce::AudioParameterInt(
        "Wind Force", "Wind Force", 0, 12, 3));

    int seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count());
    generator.seed(seed);

    windSpeedSet();
}

Wind4UnityAudioProcessor::~Wind4UnityAudioProcessor()
{
}


void Wind4UnityAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const juce::dsp::ProcessSpec spec{
        sampleRate,
        static_cast<uint32_t>(samplesPerBlock),
        static_cast<uint32_t>(getTotalNumOutputChannels())
    };

    currentSpec = spec;

    dstPrepare(spec);
}

void Wind4UnityAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void Wind4UnityAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // auto totalNumInputChannels = getTotalNumInputChannels();
    // auto totalNumOutputChannels = getTotalNumOutputChannels();

    buffer.clear();

    if(++currentWSComponentCounter > targetWSComponentCount)
        windSpeedSet();
    else
        currentWindSpeed += deltaWindSpeed;

    dstUpdateSettings();
    dstProcess(buffer);

    buffer.applyGain(gain->get());
}

//==============================================================================
bool Wind4UnityAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Wind4UnityAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================

void Wind4UnityAudioProcessor::dstPrepare(const juce::dsp::ProcessSpec& spec)
{
    dstNoise1.prepare(spec);

    dstBPF.prepare(spec);
    dstBPF.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    // dstBPF.setCutoffFrequency(dstBPCutoffFreq->get());
    // dstBPF.setResonance(dstBPQ->get());
    dstBPF.setCutoffFrequency(10.0f);
    dstBPF.setResonance(1.0f);
    dstBPF.reset();
}

void Wind4UnityAudioProcessor::dstProcess(juce::AudioBuffer<float>& buffer)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    float dstFrameAmp = dstAmplitude->get();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            float output = dstBPF.processSample(ch, dstNoise1.processSample(0.0f));
            buffer.addSample(ch, s, output * dstFrameAmp);
        }
    }
}

void Wind4UnityAudioProcessor::dstUpdateSettings()
{
    // dstBPF.setCutoffFrequency(dstBPCutoffFreq->get());
    // dstBPF.setResonance(dstBPQ->get());

    if (currentWindSpeed < 0)
        currentWindSpeed = 0;

    dstBPF.setCutoffFrequency(currentWindSpeed * 30.0f + 0.004f);
    dstBPF.setResonance(currentWindSpeed * 0.2f + 0.01f);
}

float Wind4UnityAudioProcessor::randomNormal()
{
    return distribution(generator);
}

void Wind4UnityAudioProcessor::windSpeedSet()
{
    int force = windForce->get();
    if(force == 0)
    {
        // update every second
        targetWindSpeed = 0.0f;
        targetWSComponentCount = (int)(currentSpec.sampleRate / currentSpec.maximumBlockSize);
    }
    else
    {
        // the stronger the force, the more erratic the length of time is
        targetWindSpeed = meanWS[force] + sdWS[force] * randomNormal();
        targetWSComponentCount = (int)(10.0f + 2.0f * randomNormal() / (force / 2.0f) * currentSpec.sampleRate / currentSpec.maximumBlockSize);
    }

    currentWSComponentCounter = 0;
    deltaWindSpeed = (targetWindSpeed - currentWindSpeed) / (float)targetWSComponentCount;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Wind4UnityAudioProcessor();
}
