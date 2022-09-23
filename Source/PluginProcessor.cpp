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
    // Global params
    addParameter(gain = new juce::AudioParameterFloat(
        "gain", "Master Gain", 0.0f, 1.0f, 0.5f));

    // Wind Params
    addParameter(dstAmplitude = new juce::AudioParameterFloat(
    "dstAmplitude", "Distant Gain", 0.0001f, 1.5f, 0.75f));
    addParameter(windSpeed = new juce::AudioParameterFloat(
    "windSpeed", "Wind Speed", 0.01f, 40.0f, 1.0f));
    addParameter(dstIntensity = new juce::AudioParameterFloat(
    "dstIntensity", "Intensity", 1.0f, 50.0f, 30.0f));
    addParameter(dstResonance = new juce::AudioParameterFloat(
    "dstResonance", "Resonance", 0.1f, 50.0f, 1.0f));
    addParameter(dstPan = new juce::AudioParameterFloat(
        "dstPan", "Distant Pan", 0.0f, 1.0f, 0.5f));
    addParameter(whsAmplitude = new juce::AudioParameterFloat(
        "WhsAmp", "Whistle Gain", 0.0001f, 1.5f, 0.75f));
    addParameter(whsPan1 = new juce::AudioParameterFloat(
        "WhsPan1", "Whistle Pan1", 0.0f, 1.0f, 0.5f));
    addParameter(whsPan2 = new juce::AudioParameterFloat(
        "WhsPan2", "Whistle Pan2", 0.0f, 1.0f, 0.5f));
    addParameter(howlAmplitude = new juce::AudioParameterFloat(
        "HowlAmp", "Howl Gain", 0.0001f, 1.5f, 0.75f));
    addParameter(howlPan1 = new juce::AudioParameterFloat(
        "HowlPan1", "Howl Pan1", 0.0f, 1.0f, 0.5f));
    addParameter(howlPan2 = new juce::AudioParameterFloat(
        "HowlPan2", "Howl Pan2", 0.0f, 1.0f, 0.5f));
}

Wind4UnityAudioProcessor::~Wind4UnityAudioProcessor()
{
}


void Wind4UnityAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSpec = juce::dsp::ProcessSpec {
        sampleRate,
        static_cast<uint32_t>(samplesPerBlock),
        1
    };

    prepare(currentSpec);
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

    updateSettings();
    dstProcess(buffer);
    whsProcess(buffer);
    howlProcess(buffer);

    buffer.applyGain(gain->get());
}

//==============================================================================
bool Wind4UnityAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* Wind4UnityAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================

void Wind4UnityAudioProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    //    Prepare DST
    dstBPF.prepare(spec);
    dstBPF.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    dstBPF.setCutoffFrequency(10.0f);
    dstBPF.setResonance(1.0f);
    dstBPF.reset();

    //    Prepare Whistle
    whsBPF1.prepare(spec);
    whsBPF1.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    whsBPF1.setCutoffFrequency(1000.0f);
    whsBPF1.setResonance(60.0f);
    whsBPF1.reset();

    whsBPF2.prepare(spec);
    whsBPF2.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    whsBPF2.setCutoffFrequency(1000.0f);
    whsBPF2.setResonance(60.0f);
    whsBPF2.reset();

    //    Prepare Howl
    howlBPF1.prepare(spec);
    howlBPF1.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    howlBPF1.setCutoffFrequency(400.0f);
    howlBPF1.setResonance(40.0f);
    howlBPF1.reset();

    howlBPF2.prepare(spec);
    howlBPF2.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    howlBPF2.setCutoffFrequency(200.0f);
    howlBPF2.setResonance(40.0f);
    howlBPF2.reset();

    howlOsc1.initialise([](float x) { return std::sin(x); }, 128);
    howlOsc2.initialise([](float x) { return std::sin(x); }, 128);

    howlBlockLPF1.prepare(0.5f, spec.maximumBlockSize, spec.sampleRate);
    howlBlockLPF2.prepare(0.4f, spec.maximumBlockSize, spec.sampleRate);
}

void Wind4UnityAudioProcessor::dstProcess(juce::AudioBuffer<float>& buffer)
{
    //    Get Buffer info
    const int numSamples = buffer.getNumSamples();
    const float FrameAmp = dstAmplitude->get();

    //    Distant Wind DSP Loop
    float pan[2];
    cosPan(pan, dstPan->get());

    for (int s = 0; s < numSamples; ++s)
    {
        const float output = dstBPF.processSample(0, r.nextFloat() * 2.0f - 1.0f) * FrameAmp;
        buffer.addSample(0, s, output * pan[0]);
        buffer.addSample(1, s, output * pan[1]);
    }
  
    dstBPF.snapToZero();
}

void Wind4UnityAudioProcessor::whsProcess(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float FrameAmp = whsAmplitude->get();

    //    Whistle DSP Loop
    float pan1[2];
    cosPan(pan1, pd.whistlePan1);
    float pan2[2];
    cosPan(pan2, pd.whistlePan2);
    const float ampMod1 = juce::square(juce::jmax(0.0f, wd.whsWindSpeed1 * 0.02f - 0.1f));
    const float ampMod2 = juce::square(juce::jmax(0.0f, wd.whsWindSpeed2 * 0.02f - 0.1f));

    for (int s = 0; s < numSamples; ++s)
    {
        const float noiseOutput = r.nextFloat() * 2.0f - 1.0f;
        const float output1 = whsBPF1.processSample(0, noiseOutput) * FrameAmp * ampMod1;
        const float output2 = whsBPF2.processSample(0, noiseOutput) * FrameAmp * ampMod2;
        buffer.addSample(0, s, output1 * pan1[0] + output2 * pan2[0]);
        buffer.addSample(1, s, output1 * pan1[1] + output2 * pan2[1]);
    }

    whsBPF1.snapToZero();
    whsBPF2.snapToZero();
}

void Wind4UnityAudioProcessor::howlProcess(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float FrameAmp = howlAmplitude->get();

    //    Howl DSP Loop
    float pan1[2];
    cosPan(pan1, pd.howlPan1);
    float pan2[2];
    cosPan(pan2, pd.howlPan2);
    const float ampMod1 = howlBlockLPF1.processSample(juce::dsp::FastMathApproximations::cos(
        ((juce::jlimit(0.35f, 0.6f, hd.howlWindSpeed1 * 0.02f)
         - 0.35f) * 2.0f - 0.25f) * juce::MathConstants<float>::twoPi));
    const float ampMod2 = howlBlockLPF2.processSample( juce::dsp::FastMathApproximations::cos(
        ((juce::jlimit(0.25f, 0.5f, hd.howlWindSpeed2 * 0.02f) 
         - 0.25f) * 2.0f - 0.25f) * juce::MathConstants<float>::twoPi));
    howlOsc1.setFrequency(ampMod1 * 200.0f + 30.0f);
    howlOsc2.setFrequency(ampMod2 * 100.0f + 20.0f);
    for (int s = 0; s < numSamples; ++s)
    {
        const float noiseOutput = r.nextFloat() * 2.0f - 1.0f;
        const float output1 = howlBPF1.processSample(0, noiseOutput) * FrameAmp * ampMod1 * howlOsc1.processSample(0.0f);
        const float output2 = howlBPF2.processSample(0, noiseOutput) * FrameAmp * ampMod2 * howlOsc2.processSample(0.0f);
        buffer.addSample(0, s, output1 * pan1[0] + output2 * pan2[0]);
        buffer.addSample(1, s, output1 * pan1[1] + output2 * pan2[1]);
    }

    howlBPF1.snapToZero();
    howlBPF2.snapToZero();
}

void Wind4UnityAudioProcessor::updateSettings()
{
    //  UpdateWSCircularBuffer;
    const float currentWindSpeed = windSpeed->get();
    gd.windSpeedCircularBuffer[gd.wSCBWriteIndex] = currentWindSpeed;
    ++gd.wSCBWriteIndex;
    gd.wSCBWriteIndex = (gd.wSCBWriteIndex < wSCBSize) ? gd.wSCBWriteIndex : 0;

    // Get Pan Data
    pd.whistlePan1 = whsPan1->get();
    pd.whistlePan2 = whsPan2->get();
    pd.howlPan1 = howlPan1->get();
    pd.howlPan2 = howlPan2->get();

    // Update DST
    const float currentDstIntensity = dstIntensity->get();
    const float currentDstResonance = dstResonance->get();
    // Update DST Filter Settings
    dstBPF.setCutoffFrequency(currentWindSpeed * currentDstIntensity);
    dstBPF.setResonance(currentDstResonance);

    // Update Whistle
    wd.whsWSCBReadIndex1 = gd.wSCBWriteIndex - (int)(pd.whistlePan1 * maxPanFrames);
    wd.whsWSCBReadIndex1 = (wd.whsWSCBReadIndex1 < 0) ? wd.whsWSCBReadIndex1 + wSCBSize : wd.whsWSCBReadIndex1;
    wd.whsWSCBReadIndex2 = gd.wSCBWriteIndex - (int)(pd.whistlePan2 * maxPanFrames);
    wd.whsWSCBReadIndex2 = (wd.whsWSCBReadIndex2 < 0) ? wd.whsWSCBReadIndex2 + wSCBSize : wd.whsWSCBReadIndex2;
    wd.whsWindSpeed1 = gd.windSpeedCircularBuffer[wd.whsWSCBReadIndex1];
    wd.whsWindSpeed2 = gd.windSpeedCircularBuffer[wd.whsWSCBReadIndex2];
    whsBPF1.setCutoffFrequency(wd.whsWindSpeed1 * 8.0f + 600.0f);
    whsBPF2.setCutoffFrequency(wd.whsWindSpeed2 * 20.0f + 1000.0f);

    //    Update Howl
    hd.howlWSCBReadIndex1 = gd.wSCBWriteIndex - (int)(pd.howlPan1 * maxPanFrames);
    hd.howlWSCBReadIndex1 = (hd.howlWSCBReadIndex1 < 0) ? hd.howlWSCBReadIndex1 + wSCBSize : hd.howlWSCBReadIndex1;
    hd.howlWSCBReadIndex2 = gd.wSCBWriteIndex - (int)(pd.howlPan2 * maxPanFrames);
    hd.howlWSCBReadIndex2 = (hd.howlWSCBReadIndex2 < 0) ? hd.howlWSCBReadIndex2 + wSCBSize : hd.howlWSCBReadIndex2;
    hd.howlWindSpeed1 = gd.windSpeedCircularBuffer[hd.howlWSCBReadIndex1];
    hd.howlWindSpeed2 = gd.windSpeedCircularBuffer[hd.howlWSCBReadIndex2];
}

void Wind4UnityAudioProcessor::cosPan(float* output, float pan)
{
    output[0] = juce::dsp::FastMathApproximations::cos((pan * 0.25f - 0.5f) * juce::MathConstants<float>::twoPi);
    output[1] = juce::dsp::FastMathApproximations::cos((pan * 0.25f - 0.25f) * juce::MathConstants<float>::twoPi);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Wind4UnityAudioProcessor();
}
