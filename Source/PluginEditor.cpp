/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void ColemanJP05CompressorAudioProcessorEditor::createKnob(juce::Slider& slider, float x, float y, std::string suffix,
                                                       float interval, float skew, parameterMap paramNum) {
    auto& params = processor.getParameters();
    
    juce::AudioParameterFloat* audioParam = (juce::AudioParameterFloat*) params.getUnchecked(paramNum);
    slider.setBounds(x*UNIT_LENGTH_X, y*UNIT_LENGTH_Y, 4*UNIT_LENGTH_X, 4*UNIT_LENGTH_Y);
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, UNIT_LENGTH_X*3, UNIT_LENGTH_Y*.6);
    slider.setTextValueSuffix(suffix);
    slider.setRange(audioParam->range.start, audioParam->range.end, interval);
    slider.setSkewFactor(skew);
    slider.addListener(this);
    addAndMakeVisible(slider);
}

//==============================================================================
ColemanJP05CompressorAudioProcessorEditor::ColemanJP05CompressorAudioProcessorEditor (ColemanJP05CompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (CONTAINER_WIDTH, CONTAINER_HEIGHT);
    
    // create knobs
    createKnob(thresholdSlider, 1, 1, " dB", THRESH_INTERVAL, THRESH_SKEW, threshold);
    createKnob(ratioSlider, 1, 6, ":1", RATIO_INTERVAL, RATIO_SKEW, ratio);
    createKnob(preGainSlider, 1, 11, " dB", GAIN_INTERVAL, GAIN_SKEW, preGain);
    createKnob(attackSlider, 6, 11, " ms", ATTACK_INTERVAL, ATTACK_SKEW, attack);
    createKnob(releaseSlider, 11, 11, " ms", RELEASE_INTERVAL, RELEASE_SKEW, release);
    createKnob(postGainSlider, 16, 11, " dB", GAIN_INTERVAL, GAIN_SKEW, postGain);
    
    no_fill.setOpacity(0);
 
    // create outline for input/output graph
    graphOutline.setRectangle(graphOutlineCoords);
    graphOutline.setFill(no_fill);
    graphOutline.setStrokeFill(white);
    graphOutline.setStrokeThickness(2);
    addAndMakeVisible(graphOutline);
    
    // add response lines to view and set color
    belowThresholdLine.setFill(white);
    addAndMakeVisible(belowThresholdLine);
    aboveThresholdLine.setFill(white);
    addAndMakeVisible(aboveThresholdLine);
    
    // create dynamic compression meter
    meterValue.setFill(red);
    addAndMakeVisible(meterValue);

    // create compression beter box outline
    // outline is created after meter so meter
    // is displayed under the outline
    meterOutline.setRectangle(meterOutlineCoords);
    meterOutline.setFill(no_fill);
    meterOutline.setStrokeFill(white);
    meterOutline.setStrokeThickness(2);
    addAndMakeVisible(meterOutline);
    
    // create audio sample circle on
    // input/output graph
    audioSampleCircle.setFill(no_fill);
    audioSampleCircle.setStrokeFill(white);
    audioSampleCircle.setStrokeThickness(1);
    addAndMakeVisible(audioSampleCircle);
    
    // start GUI refreshing
    startTimerHz(timerFreq);
    
    // determine coefficient for audio sample circle smoother
    graphSampleB0 = 1 - exp(-1/(graphSampleTau*timerFreq));
}

ColemanJP05CompressorAudioProcessorEditor::~ColemanJP05CompressorAudioProcessorEditor()
{
}

void ColemanJP05CompressorAudioProcessorEditor::sliderValueChanged(juce::Slider *slider) {
    auto& params = processor.getParameters();
    
    for (auto& slider_param : sliderParamMap) {
        if (slider == slider_param.slider) {
            // set parameter from slider value
            juce::AudioParameterFloat* audioParam = (juce::AudioParameterFloat*)params.getUnchecked(slider_param.param);
            *audioParam = slider_param.slider->getValue();
            break;
        }
    }
}

void ColemanJP05CompressorAudioProcessorEditor::timerCallback() {
    auto& params = processor.getParameters();

    for (auto& slider_param : sliderParamMap) {
        // set slider value from parameter
        juce::AudioParameterFloat* param = (juce::AudioParameterFloat*)params.getUnchecked(slider_param.param);
        slider_param.slider->setValue(param->get(), juce::dontSendNotification);
    }
    
    updateGUI();
}

struct point {
    float x;
    float y;
};

// takes coordinates for the in/out gain display
// and converts them to GUI coordinates
point getGUICoords(float x, float y) {
    if (x < -40) x = -40;
    if (x > 0) x = 0;
    if (y < -40) y = -40;
    if (y > 0) y = 0;
    if (std::isnan(x)) x = -40;
    if (std::isnan(y)) y = -40;
    
    point p;
    p.x = (8*x/40.0 + 15)*UNIT_LENGTH_X;
    p.y = (-8*y/40.0 + 1)*UNIT_LENGTH_Y;
    return p;
}

void ColemanJP05CompressorAudioProcessorEditor::updateGraph(float postGainValue,
                                                            float thresholdValue,
                                                            float ratioValue) {
    // intersection point between the two lines
    point center;
    // start point of the line below the threshold
    point underThreshStart;
    // end point of line above the threshold
    point overThreshEnd;
    
    bool overLineDisplay = true;
    bool underLineDisplay = true;
    
    /*
     Equations:
        line below the threshold -> y = x + postGain
        line above the threshold -> y = (x - threshold)/ratio + threshold + postGain
     Bounds:
        -40 <= x <= 0
        -40 <= y <= 0
     */
    
    center.x = thresholdValue;
    center.y = postGainValue + thresholdValue;

    // determine the start point of line below threshold
    underThreshStart.y = postGainValue - 40;
    underThreshStart.x = -40;
    if (underThreshStart.y < -40) {
        underThreshStart.x = -40 - postGainValue;
        underThreshStart.y = -40;
    }
    
    // determine the end point of the line above the threshold
    overThreshEnd.y = thresholdValue*(1-1/ratioValue) + postGainValue;
    overThreshEnd.x = 0;
    if (overThreshEnd.y > 0) {
        overThreshEnd.y = 0;
        overThreshEnd.x = thresholdValue - ratioValue*(postGainValue + thresholdValue);
    }
    
    // determine which lines should be displayed
    if (center.y > 0) {
        overLineDisplay = false;
        center.x = -postGainValue;
        center.y = 0;
    } else if (center.y < -40) {
        underLineDisplay = false;
        center.x = thresholdValue - 40*ratioValue - postGainValue*ratioValue - ratioValue*thresholdValue;
        center.y = -40;
    }
    if (center.x > 0) {
        overLineDisplay = false;
    }
       
    // convert cartesian coordinates to GUI coordinates
    underThreshStart = getGUICoords(underThreshStart.x, underThreshStart.y);
    center = getGUICoords(center.x, center.y);
    overThreshEnd = getGUICoords(overThreshEnd.x, overThreshEnd.y);
    
    // update GUI lines
    if (underLineDisplay) {
        juce::Path below;
        below.addLineSegment(juce::Line<float>(underThreshStart.x, underThreshStart.y, center.x, center.y), 2);
        belowThresholdLine.setPath(below);
        belowThresholdLine.setVisible(true);
    } else {
        belowThresholdLine.setVisible(false);
    }
    
    if (overLineDisplay) {
        juce::Path above;
        above.addLineSegment(juce::Line<float>(center.x, center.y, overThreshEnd.x, overThreshEnd.y), 2);
        aboveThresholdLine.setPath(above);
        aboveThresholdLine.setVisible(true);
    } else {
        aboveThresholdLine.setVisible(false);
    }
}

void ColemanJP05CompressorAudioProcessorEditor::updateGUI() {
    // get parameter values
    auto& params = processor.getParameters();
    float postGainValue = ((juce::AudioParameterFloat*)params.getUnchecked(postGain))->get();
    float thresholdValue = ((juce::AudioParameterFloat*)params.getUnchecked(threshold))->get();
    float ratioValue = ((juce::AudioParameterFloat*)params.getUnchecked(ratio))->get();
    
    // update the input/output graph
    updateGraph(postGainValue, thresholdValue, ratioValue);
    
    // plot audio sample on input/output graph
    sample_x += graphSampleB0*(audioProcessor.rmsEnvelopeDb - sample_x);
    float sample_y;
    if (sample_x < thresholdValue)
        sample_y = sample_x + postGainValue;
    else
        sample_y = thresholdValue + (sample_x - thresholdValue)/ratioValue + postGainValue;
    
    if (sample_x > 0 || sample_y > 0 || sample_x < - 40 || sample_x < - 40 - postGainValue) {
        audioSampleCircle.setVisible(false);
    } else {
        point sample_center = getGUICoords(sample_x, sample_y);
        juce::Path samplePath;
        float radius = 0.15*UNIT_LENGTH_X;
        samplePath.addEllipse(sample_center.x - radius, sample_center.y - radius, radius*2, radius*2);
        audioSampleCircle.setPath(samplePath);
        audioSampleCircle.setVisible(true);
    }
        
    // compression meter
    float compLevelDB = 20*log10(audioProcessor.gainOutLinear);
    if (compLevelDB < -40) compLevelDB = -40;
    if (compLevelDB > 0) compLevelDB = 0;
    float rectHeight = -compLevelDB/40*8;
    meterValue.setRectangle(juce::Rectangle<float>(17*UNIT_LENGTH_X, 1*UNIT_LENGTH_Y, 1*UNIT_LENGTH_X, rectHeight*UNIT_LENGTH_Y));
}

//==============================================================================
void ColemanJP05CompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // background color
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // graph backgrounds
    g.setColour(graphsBackgroundColor);
    g.fillRect(graphOutlineCoords);
    g.fillRect(meterOutlineCoords);
    
    // lines and ticks color
    g.setColour(graphLineColor);
    
    // graph tick marks on graphs
    float rightSpace = 0.2;
    float upperSpace = rightSpace;
    float leftSpace = rightSpace;
    int dBMarks[] = {-40, -30, -20, -10, 0};
    
    for (int i = 0; i < 5; i++) {
        std::string text = std::to_string(dBMarks[i]);
        if (dBMarks[i] == 0) text = "dB";
        // horizontal graph
        g.drawText(text, 5*UNIT_LENGTH_X, (0.5 - dBMarks[i]/5)*UNIT_LENGTH_Y,
                   (2-rightSpace)*UNIT_LENGTH_X, 1*UNIT_LENGTH_Y, juce::Justification::centredRight);
        g.drawLine(7*UNIT_LENGTH_X, (1 - dBMarks[i]/5.0)*UNIT_LENGTH_Y, 15*UNIT_LENGTH_X,
                   (1 - dBMarks[i]/5.0)*UNIT_LENGTH_Y);
        
        // vertical graph
        g.drawText(text, (15 - 1 + dBMarks[i]/5)*UNIT_LENGTH_X, (9+upperSpace)*UNIT_LENGTH_Y,
                   2*UNIT_LENGTH_X, 1*UNIT_LENGTH_Y, juce::Justification::centredTop);
        g.drawLine((15 + dBMarks[i]/5)*UNIT_LENGTH_X, 1*UNIT_LENGTH_Y,
                   (15 + dBMarks[i]/5)*UNIT_LENGTH_X, 9*UNIT_LENGTH_Y);
        
        // horizontal meter
        g.drawText(text, (18 + leftSpace)*UNIT_LENGTH_X, (0.5 - dBMarks[i]/5)*UNIT_LENGTH_Y,
                   2*UNIT_LENGTH_X, 1*UNIT_LENGTH_Y, juce::Justification::centredLeft);
        g.drawLine(17*UNIT_LENGTH_X, (1 - dBMarks[i]/5)*UNIT_LENGTH_Y, 18*UNIT_LENGTH_X, (1 - dBMarks[i]/5)*UNIT_LENGTH_Y);
    }
    
    
    // Label Knobs
    struct namesToPlaces {
        std::string name;
        int x;
        int y;
    };
    
    std::vector<namesToPlaces> mapping {
        {"Threshold", 1, 1},
        {"Ratio", 1, 6},
        {"Pre Gain", 1, 11},
        {"Attack", 6, 11},
        {"Release", 11, 11},
        {"Post Gain", 16, 11}
    };
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(15.0f, juce::Font::bold));
    for (auto& knob : mapping) {
        g.drawText(knob.name, knob.x*UNIT_LENGTH_X, (knob.y - 0.8)*UNIT_LENGTH_Y,
                   4*UNIT_LENGTH_X, 1*UNIT_LENGTH_Y, juce::Justification::centred);
    }
}

void ColemanJP05CompressorAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
