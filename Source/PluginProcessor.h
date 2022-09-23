/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "BlockLPF.h"

//==============================================================================
/**
*/
class Wind4UnityAudioProcessor : public juce::AudioProcessor
#if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
#endif
{
public:
    //==============================================================================
    Wind4UnityAudioProcessor();
    ~Wind4UnityAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
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

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override
    {
        return JucePlugin_Name;
    }

    bool acceptsMidi() const override
    {
#if JucePlugin_WantsMidiInput
        return true;
#else
        return false;
#endif
    }

    bool producesMidi() const override
    {
#if JucePlugin_ProducesMidiOutput
        return true;
#else
        return false;
#endif
    }

    bool isMidiEffect() const override
    {
#if JucePlugin_IsMidiEffect
    return true;
#else
        return false;
#endif
    }

    double getTailLengthSeconds() const override
    {
        return 0.0;
    }

    //==============================================================================
    int getNumPrograms() override
    {
        return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
    }

    int getCurrentProgram() override
    {
        return 0;
    }

    void setCurrentProgram(int index) override
    {
    }

    const juce::String getProgramName(int index) override
    {
        return {};
    }

    void changeProgramName(int index, const juce::String& newName) override
    {
    }

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override
    {
        // You should use this method to store your parameters in the memory block.
        // You could do that either as raw data, or use the XML or ValueTree classes
        // as intermediaries to make it easy to save and load complex data.
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        // You should use this method to restore your parameters from this memory block,
        // whose contents will have been created by the getStateInformation() call.
    }

    // enum GustStatus
    // {
    //     Off,
    //     Waiting,
    //     Active,
    //     Closing
    // };

    static const int wSCBSize = 500;
    static const int numOutputChannels = 2;
    static const int maxPanFrames = 20;

    struct GlobalData
    {
        float windSpeedCircularBuffer[wSCBSize];
        int wSCBWriteIndex{0};
    };

    struct WhistleData
    {
        int whsWSCBReadIndex1 = wSCBSize - 6;
        int whsWSCBReadIndex2 = wSCBSize - 16;
        float whsWindSpeed1;
        float whsWindSpeed2;
    };

    struct HowlData
    {
        int howlWSCBReadIndex1 = wSCBSize - 6;
        int howlWSCBReadIndex2 = wSCBSize - 51;
        float howlWindSpeed1;
        float howlWindSpeed2;
    };
    
    struct PanData
    {
        float whistlePan1;
        float whistlePan2;
        float howlPan1;
        float howlPan2;
    };
    
private:
    // Wind Methods
    void prepare(const juce::dsp::ProcessSpec& spec);
    void dstProcess(juce::AudioBuffer<float>& buffer);
    void whsProcess(juce::AudioBuffer<float>& buffer);
    void howlProcess(juce::AudioBuffer<float>& buffer);
    void updateSettings();
    void cosPan(float* output, float pan);

    juce::AudioParameterFloat* gain;
    juce::AudioParameterFloat* windSpeed;

    juce::AudioParameterFloat* dstAmplitude;
    juce::AudioParameterFloat* dstIntensity;
    juce::AudioParameterFloat* dstResonance;
    juce::AudioParameterFloat* dstPan;

    juce::AudioParameterFloat* whsPan1;
    juce::AudioParameterFloat* whsPan2;
    juce::AudioParameterFloat* whsAmplitude;
    WhistleData wd;
    
    juce::AudioParameterFloat* howlPan1;
    juce::AudioParameterFloat* howlPan2;
    juce::AudioParameterFloat* howlAmplitude;
    HowlData hd;

    juce::Random r;
    juce::dsp::StateVariableTPTFilter<float> dstBPF;
    
    juce::dsp::StateVariableTPTFilter<float> whsBPF1;
    juce::dsp::StateVariableTPTFilter<float> whsBPF2;

    juce::dsp::StateVariableTPTFilter<float> howlBPF1;
    juce::dsp::StateVariableTPTFilter<float> howlBPF2;
    juce::dsp::Oscillator<float> howlOsc1;
    juce::dsp::Oscillator<float> howlOsc2;
    Wind4Unity::BlockLPF howlBlockLPF1;
    Wind4Unity::BlockLPF howlBlockLPF2;

    juce::dsp::ProcessSpec currentSpec;
    GlobalData gd;
    PanData pd;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Wind4UnityAudioProcessor)
};
