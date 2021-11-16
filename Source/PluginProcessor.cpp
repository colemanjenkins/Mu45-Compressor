/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ColemanJP05CompressorAudioProcessor::ColemanJP05CompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    addParameter(thresholdParam = new juce::AudioParameterFloat("threshold",
                                                                "Threshold (dB)",
                                                                THRESH_MIN,
                                                                THRESH_MAX,
                                                                THRESH_DEFAULT));
    addParameter(ratioParam = new juce::AudioParameterFloat("ratio",
                                                            "Ratio",
                                                            RATIO_MIN,
                                                            RATIO_MAX,
                                                            RATIO_DEFAULT));
    addParameter(attackParam = new juce::AudioParameterFloat("attack",
                                                             "Attack (ms)",
                                                             ATTACK_MIN,
                                                             ATTACK_MAX,
                                                             ATTACK_DEFAULT));
    addParameter(releaseParam = new juce::AudioParameterFloat("release",
                                                              "Release (ms)",
                                                              RELEASE_MIN,
                                                              RELEASE_MAX,
                                                              RELEASE_DEFAULT));
    
    addParameter(preGainParam = new juce::AudioParameterFloat("preGain",
                                                                "Pre Gain (dB)",
                                                              GAIN_MIN,
                                                              GAIN_MAX,
                                                              GAIN_DEFAULT));
    addParameter(postGainParam = new juce::AudioParameterFloat("postGain",
                                                                "Post Gain (dB)",
                                                               GAIN_MIN,
                                                               GAIN_MAX,
                                                               GAIN_DEFAULT));

}

ColemanJP05CompressorAudioProcessor::~ColemanJP05CompressorAudioProcessor()
{
}

//==============================================================================
const juce::String ColemanJP05CompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ColemanJP05CompressorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ColemanJP05CompressorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ColemanJP05CompressorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ColemanJP05CompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ColemanJP05CompressorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ColemanJP05CompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ColemanJP05CompressorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ColemanJP05CompressorAudioProcessor::getProgramName (int index)
{
    return {};
}

void ColemanJP05CompressorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ColemanJP05CompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    fs = sampleRate;
    
    envB0 = 1 - exp(-1/(envTau*fs));
}

void ColemanJP05CompressorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ColemanJP05CompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (
        //layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
     layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
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

void ColemanJP05CompressorAudioProcessor::calcAlgorithmParams() {
    
    envB0 = 1 - exp(-1/(envTau*fs));
    
    attackCoeff = 1.0 - exp(-1.0/(attackParam->get()*fs/1000.0));
    releaseCoeff = 1.0 - exp(-1.0/(releaseParam->get()*fs/1000.0));
    
    thresholdDb = thresholdParam->get();
    ratio = ratioParam->get();
    
    preGainLinear = pow(10,preGainParam->get()/20.0);
    postGainLinear = pow(10,postGainParam->get()/20.0);
}

void ColemanJP05CompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    calcAlgorithmParams();

    auto* channelDataLeft = buffer.getWritePointer(0);
    auto* channelDataRight = buffer.getWritePointer(1);
    
    float leftSquared, rightSquared, rmsEnvelopeLin,
        preDynamicsGainDb, preDynamicsGainLinear, gainCoeff,
        leftIn, rightIn, finalGainLinear;
        
    for (int samp = 0; samp < buffer.getNumSamples(); samp++) {
        // pre gain
        leftIn = channelDataLeft[samp]*preGainLinear;
        rightIn = channelDataRight[samp]*preGainLinear;
        
        // RMS level detector
        leftSquared = leftIn*leftIn;
        rightSquared = rightIn*rightIn;
        rmsEnvelopeLin = 0.5*(leftSquared + rightSquared);
        envOut += envB0*(rmsEnvelopeLin - envOut); // leaky integrator
        rmsEnvelopeLin = sqrt(envOut);
        rmsEnvelopeDb = 20*log10(rmsEnvelopeLin);
        
        // gain calculation
        if (rmsEnvelopeDb <= thresholdDb) {
            preDynamicsGainDb = 0;
        } else {
            preDynamicsGainDb = (1.0/ratio - 1)*(rmsEnvelopeDb - thresholdDb);
        }
        preDynamicsGainLinear = pow(10,preDynamicsGainDb/20.0);
        
        // gain dynamics
        if (preDynamicsGainLinear < gainOutLinear) {
            gainCoeff = attackCoeff;
        } else {
            gainCoeff = releaseCoeff;
        }
        gainOutLinear += gainCoeff*(preDynamicsGainLinear - gainOutLinear); // leaky integrator
        
        // post gain
        finalGainLinear = gainOutLinear*postGainLinear;
        
        // apply gain to outputs
        channelDataLeft[samp] = finalGainLinear*leftIn;
        channelDataRight[samp] = finalGainLinear*rightIn;
    }
}

//==============================================================================
bool ColemanJP05CompressorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ColemanJP05CompressorAudioProcessor::createEditor()
{
    return new ColemanJP05CompressorAudioProcessorEditor (*this);
}

//==============================================================================
void ColemanJP05CompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml ("Parameters");
    for (int i = 0; i < getParameters().size(); ++i)
    {
        juce::AudioParameterFloat* param = (juce::AudioParameterFloat*)getParameters().getUnchecked(i);
        juce::XmlElement* paramElement = new juce::XmlElement ("parameter" + juce::String(std::to_string(i)));
        paramElement->setAttribute ("value", param->get());
        xml.addChildElement (paramElement);
    }
    copyXmlToBinary (xml, destData);
}

void ColemanJP05CompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState->hasTagName ("Parameters")) // read Parameters tag
    {
        juce::AudioParameterFloat* param;
        for (auto* element : xmlState->getChildIterator()) // loop through the saved parameter values and update them
        {
            int paramNum = std::stoi(element->getTagName().substring(9).toStdString()); // chops off beginnging "parameter"
            param = (juce::AudioParameterFloat*) getParameters().getUnchecked(paramNum);
            *param = element->getDoubleAttribute("value"); // set parameter value
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ColemanJP05CompressorAudioProcessor();
}
