/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "defines.h"

//==============================================================================
/**
*/
class ColemanJP05CompressorAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ColemanJP05CompressorAudioProcessor();
    ~ColemanJP05CompressorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    float rmsEnvelopeDb;
    float gainOutLinear = 1;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColemanJP05CompressorAudioProcessor)
    
    juce::AudioParameterFloat* thresholdParam; // dB
    juce::AudioParameterFloat* ratioParam; // ratio
    juce::AudioParameterFloat* attackParam; // ms
    juce::AudioParameterFloat* releaseParam; // ms
    juce::AudioParameterFloat* preGainParam; // dB
    juce::AudioParameterFloat* postGainParam; // dB
    
    float fs;               // sampling rate
    float envOut = 0.0;    // output & delay memory for RMS env follower
    float envB0;           // filter coeff for RMS env follower
    float envTau = 0.01;   // integration time in sec for RMS env follower: TO CHANGE?
    
    float attackCoeff;
    float releaseCoeff;
    float thresholdDb;
    float ratio;
    float preGainLinear;
    float postGainLinear;
    
    void calcAlgorithmParams();
};
