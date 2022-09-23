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

    addParameter(dstBPCutoffFreq = new juce::AudioParameterFloat(
        "DstCutoff", "DistantIntensity", 0.004f, 1000.0f, 10.0f));

    addParameter(dstBPQ = new juce::AudioParameterFloat(
        "DstQ", "DistantQ", 1.0f, 100.0f, 10.0f));

    addParameter(dstAmplitude = new juce::AudioParameterFloat(
        "DstAmp", "DistantAmplitude", 0.0001f, 1.5f, 0.75f));
}

Wind4UnityAudioProcessor::~Wind4UnityAudioProcessor()
{
}

//==============================================================================
const juce::String Wind4UnityAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Wind4UnityAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool Wind4UnityAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool Wind4UnityAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double Wind4UnityAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Wind4UnityAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int Wind4UnityAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Wind4UnityAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String Wind4UnityAudioProcessor::getProgramName(int index)
{
    return {};
}

void Wind4UnityAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void Wind4UnityAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const juce::dsp::ProcessSpec spec{
        sampleRate,
        static_cast<uint32_t>(samplesPerBlock),
        static_cast<uint32_t>(getTotalNumOutputChannels())
    };

    dstPrepare(spec);
}

void Wind4UnityAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Wind4UnityAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void Wind4UnityAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // auto totalNumInputChannels = getTotalNumInputChannels();
    // auto totalNumOutputChannels = getTotalNumOutputChannels();

    buffer.clear();

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
void Wind4UnityAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Wind4UnityAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

void Wind4UnityAudioProcessor::dstPrepare(const juce::dsp::ProcessSpec& spec)
{
    dstNoise1.prepare(spec);

    dstBPF.prepare(spec);
    dstBPF.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    dstBPF.setCutoffFrequency(dstBPCutoffFreq->get());
    dstBPF.setResonance(dstBPQ->get());
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
    dstBPF.setCutoffFrequency(dstBPCutoffFreq->get());
    dstBPF.setResonance(dstBPQ->get());
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Wind4UnityAudioProcessor();
}
