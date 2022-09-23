/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

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
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    // Wind Methods
    void dstPrepare(const juce::dsp::ProcessSpec& spec);
    void dstProcess(juce::AudioBuffer<float>& buffer);
    void dstUpdateSettings();

    // Global Params
    juce::AudioParameterFloat* gain;

    // Distant Wind params
    juce::AudioParameterFloat* dstBPCutoffFreq;
    juce::AudioParameterFloat* dstBPQ;
    juce::AudioParameterFloat* dstAmplitude;

    juce::dsp::Oscillator<float> dstNoise1
    {
        [](float x)
        {
            juce::Random r;
            return r.nextFloat() * 2.0f - 1.0f;
        }
    };

    juce::dsp::StateVariableTPTFilter<float> dstBPF;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Wind4UnityAudioProcessor)
};
