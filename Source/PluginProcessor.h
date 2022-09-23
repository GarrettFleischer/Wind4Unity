/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <random>

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

    enum GustStatus
    {
        Off,
        Waiting,
        Active,
        Closing
    };

private:
    // Wind Methods
    void dstPrepare(const juce::dsp::ProcessSpec& spec);
    void dstProcess(juce::AudioBuffer<float>& buffer);
    void dstUpdateSettings();

    float randomNormal();
    void windSpeedSet();

    // gusts and squalls
    void computeGust();
    void gustSet();
    void squallSet();
    void gustClose();
    void gustIntervalSet();
    void gustLengthSet();
    void squallLengthSet();

    // Global Params
    juce::AudioParameterFloat* gain;

    // Distant Wind params
    // juce::AudioParameterFloat* dstBPCutoffFreq;
    // juce::AudioParameterFloat* dstBPQ;
    juce::AudioParameterFloat* dstAmplitude;

    // Wind Speed params
    juce::AudioParameterInt* windForce;
    juce::AudioParameterBool* gustActive;
    juce::AudioParameterFloat* gustDepth;
    juce::AudioParameterFloat* gustInterval;
    juce::AudioParameterBool* squallActive;
    juce::AudioParameterFloat* squallDepth;

    juce::dsp::Oscillator<float> dstNoise1
    {
        [](float x)
        {
            juce::Random r;
            return r.nextFloat() * 2.0f - 1.0f;
        }
    };

    juce::dsp::StateVariableTPTFilter<float> dstBPF;

    // RNG
    std::normal_distribution<double> distribution{};
    std::default_random_engine generator;

    // wind speed arrays
    float meanWS[13]{0.0f, 1.0f, 2.0f, 4.0f, 6.0f, 9.0f, 12.0f, 15.0f, 18.0f, 22.0f, 26.0f, 30.0f, 34.0f};
    float sdWS[13]{0.0f, 0.125f, 0.25f, 0.5f, 0.75f, 1.125f, 1.5f, 1.875f, 2.25f, 2.75f, 3.25f, 3.75f, 4.25f};

    // Internal vars
    juce::dsp::ProcessSpec currentSpec;
    float currentWindSpeed{0.0f};
    float deltaWindSpeed{0.0f};
    float targetWindSpeed{0.0f};
    int currentWSComponentCounter{0};
    int targetWSComponentCount{0};

    float currentGust{0.0f};
    float deltaGust{0.0f};
    float targetGust{0.0f};
    int currentGustComponentCounter{0};
    int targetGustComponentCount{0};
    int currentGustLengthCounter{0};
    int targetGustLengthCount{0};
    int currentGustIntervalCounter{0};
    int targetGustIntervalCount{0};
    GustStatus gustStatus;
    bool gustWasActive{false};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Wind4UnityAudioProcessor)
};
