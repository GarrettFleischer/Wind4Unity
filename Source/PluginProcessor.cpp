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

    addParameter(gustActive = new juce::AudioParameterBool(
        "Gust Active", "Gust Active", false));

    addParameter(gustDepth = new juce::AudioParameterFloat(
        "Gust Depth", "Gust Depth", 0.0f, 1.0f, 0.5f));

    addParameter(gustInterval = new juce::AudioParameterFloat(
        "Gust Interval", "Gust Interval", 0.0f, 1.0f, 0.5f));

    addParameter(squallActive = new juce::AudioParameterBool(
        "Squall Active", "Squall Active", false));

    addParameter(squallDepth = new juce::AudioParameterFloat(
        "Squall Depth", "Squall Depth", 0.0f, 1.0f, 0.5f));

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

    if (++currentWSComponentCounter > targetWSComponentCount)
        windSpeedSet();
    else
        currentWindSpeed += deltaWindSpeed;

    if (gustActive->get() || gustStatus == Closing)
        computeGust();
    else if (gustStatus == Active)
        gustClose();


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

    dstBPF.setCutoffFrequency(juce::jlimit (0.004f, 1500.0f, currentWindSpeed + currentGust) * 30.0f);
    dstBPF.setResonance(1.0f + log(juce::jmax(1.0f, (currentWindSpeed + currentGust) * 0.1f)));
}

float Wind4UnityAudioProcessor::randomNormal()
{
    return distribution(generator);
}

void Wind4UnityAudioProcessor::windSpeedSet()
{
    int force = windForce->get();
    if (force == 0)
    {
        // update every second
        targetWindSpeed = 0.0f;
        targetWSComponentCount = static_cast<int>(currentSpec.sampleRate / currentSpec.maximumBlockSize);
    }
    else
    {
        // the stronger the force, the more erratic the length of time is
        targetWindSpeed = meanWS[force] + sdWS[force] * randomNormal();
        targetWSComponentCount = static_cast<int>(10.0f + 2.0f * randomNormal() / (force / 2.0f) * currentSpec.
            sampleRate / currentSpec.maximumBlockSize);
    }

    currentWSComponentCounter = 0;
    deltaWindSpeed = (targetWindSpeed - currentWindSpeed) / static_cast<float>(targetWSComponentCount);
}

void Wind4UnityAudioProcessor::computeGust()
{
    if (!gustWasActive)
    {
        gustIntervalSet();
        return;
    }

    if (gustStatus == Waiting)
    {
        if (++currentGustIntervalCounter > targetGustIntervalCount)
        {
            if (squallActive->get())
            {
                squallSet();
                squallLengthSet();
            }
            else
            {
                gustSet();
                gustLengthSet();
            }
        }

        return;
    }

    if (gustStatus == Active)
    {
        if (++currentGustLengthCounter > targetGustLengthCount)
        {
            gustClose();
            return;
        }

        if (++currentGustComponentCounter > targetGustComponentCount)
        {
            if (squallActive->get())
            {
                squallSet();
            }
            else
            {
                gustSet();
            }
        }
        else
        {
            currentGust += deltaGust;
        }
        return;
    }

    if (gustStatus == Closing)
    {
        if (++currentGustComponentCounter > targetGustComponentCount)
        {
            gustWasActive = false;
            gustStatus = Off;
            currentGust = 0.0f;
            return;
        }
        currentGust += deltaGust;
    }
}

void Wind4UnityAudioProcessor::gustSet()
{
    int force = windForce->get();
    if (force < 3)
    {
        gustClose();
        return;
    }
    targetGust = 15.0f * gustDepth->get() + 2.0f * sdWS[force] * randomNormal();
    targetGustComponentCount = static_cast<int>((1.0f + 0.25f * randomNormal()) / (force / 2.0f) * currentSpec.
        sampleRate / currentSpec.
        maximumBlockSize);
    targetGustComponentCount = juce::jmax(targetGustComponentCount, 1);

    currentGustComponentCounter = 0;
    deltaGust = (targetGust - currentGust) / static_cast<float>(targetGustComponentCount);
    gustStatus = Active;
}

void Wind4UnityAudioProcessor::squallSet()
{
    int force = windForce->get();
    if (force < 3)
    {
        gustClose();
        return;
    }
    
    targetGust = 20.0f * squallDepth->get() + 2.0f * sdWS[force] * randomNormal();
    targetGustComponentCount = static_cast<int>((0.75f + 0.25f * randomNormal()) / (force / 2.0f) * currentSpec.
        sampleRate / currentSpec.
        maximumBlockSize);
    targetGustComponentCount = juce::jmax(targetGustComponentCount, 1);

    currentGustComponentCounter = 0;
    deltaGust = (targetGust - currentGust) / static_cast<float>(targetGustComponentCount);
    gustStatus = Active;
}

void Wind4UnityAudioProcessor::gustClose()
{
    targetGust = 0.0f;
    targetGustComponentCount = static_cast<int>(2 * currentSpec.sampleRate / currentSpec.maximumBlockSize);
    currentGustComponentCounter = 0;
    deltaGust = (targetGust - currentGust) / static_cast<float>(targetGustComponentCount);
    gustStatus = Closing;
}

void Wind4UnityAudioProcessor::gustIntervalSet()
{
    currentGustIntervalCounter = 0;
    targetGustIntervalCount = (int)((5.0f + gustInterval->get() * 100.0f + 10.0f * randomNormal()) * currentSpec.sampleRate / currentSpec.maximumBlockSize);
    targetGustIntervalCount = juce::jmax(targetGustIntervalCount, 1);
    gustStatus = Waiting;
    gustWasActive = true;
}

void Wind4UnityAudioProcessor::gustLengthSet()
{
    currentGustLengthCounter = 0;
    targetGustLengthCount = (int)((5.0f + 1.5f * randomNormal()) * currentSpec.sampleRate / currentSpec.maximumBlockSize);
    targetGustLengthCount = juce::jmax(targetGustLengthCount, 1);
    gustStatus = Active;
}

void Wind4UnityAudioProcessor::squallLengthSet()
{
    currentGustLengthCounter = 0;
    targetGustLengthCount = (int)((30.0f + 5.0f * randomNormal()) * currentSpec.sampleRate / currentSpec.maximumBlockSize);
    targetGustLengthCount = juce::jmax(targetGustLengthCount, 1);
    gustStatus = Active;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Wind4UnityAudioProcessor();
}
