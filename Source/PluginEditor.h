/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class ColemanJP05CompressorAudioProcessorEditor  : public juce::AudioProcessorEditor,
public juce::Slider::Listener, public juce::Timer
{
public:
    ColemanJP05CompressorAudioProcessorEditor (ColemanJP05CompressorAudioProcessor&);
    ~ColemanJP05CompressorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void sliderValueChanged(juce::Slider* slider) override;
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ColemanJP05CompressorAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColemanJP05CompressorAudioProcessorEditor)
    
    // controls
    juce::Slider thresholdSlider;
    juce::Slider ratioSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    
    juce::Slider preGainSlider;
    juce::Slider postGainSlider;

    // mappings
    enum parameterMap {
        threshold,
        ratio,
        attack,
        release,
        preGain,
        postGain
    };
    struct SliderToParam {
        juce::Slider* slider;
        parameterMap param;
    };
    std::vector<SliderToParam> sliderParamMap {
        {&thresholdSlider, threshold},
        {&ratioSlider, ratio},
        {&attackSlider, attack},
        {&releaseSlider, release},
        {&preGainSlider, preGain},
        {&postGainSlider, postGain}
    };
    
    // GUI Response elements
    juce::DrawableRectangle meterOutline;
    juce::DrawableRectangle graphOutline;
    juce::DrawableRectangle meterValue;
    juce::DrawablePath belowThresholdLine;
    juce::DrawablePath aboveThresholdLine;
    juce::DrawablePath audioSampleCircle;
    
    // Colors
    juce::FillType white = juce::FillType(juce::Colours::white);
    juce::FillType no_fill; // opacity set in constructor
    juce::FillType red = juce::FillType(juce::Colour(190,42,42));
    juce::Colour   graphsBackgroundColor = juce::Colour(65, 65, 75);
    juce::Colour   graphLineColor = juce::Colour(100, 100, 120);
    
    juce::Rectangle<float> graphOutlineCoords =
        juce::Rectangle<float>(7*UNIT_LENGTH_X, 1*UNIT_LENGTH_Y,
                               8*UNIT_LENGTH_X, 8*UNIT_LENGTH_Y);
    juce::Rectangle<float> meterOutlineCoords =
        juce::Rectangle<float>(17*UNIT_LENGTH_X, 1*UNIT_LENGTH_Y,
                               1*UNIT_LENGTH_X, 8*UNIT_LENGTH_Y);
    
    void updateGUI();
    void updateGraph(float,float,float);
    void createKnob(juce::Slider& slider, float x, float y, std::string suffix,
                    float interval, float skew, parameterMap paramNum);
    
    // Leaky integrator for compressor audio circle
    float graphSampleB0;
    float graphSampleTau = 35.0/1000.0; // sec
    float timerFreq = 120; // Hz
    float sample_x = -40; // dB
};
