#include "PluginProcessor.h"

juce::AudioProcessorValueTreeState::ParameterLayout MiniSynthProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;
    const StringArray waves { "Sine", "Saw", "Square", "Triangle" };

    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "wave1", 1 }, "Osc 1 Wave", waves, 1));
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "wave2", 1 }, "Osc 2 Wave", waves, 1));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "detune", 1 }, "Osc 2 Detune (cents)", NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 7.0f));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "osc2level", 1 }, "Osc 2 Level", NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "cutoff", 1 }, "Filter Cutoff", NormalisableRange<float> (20.0f, 18000.0f, 1.0f, 0.3f), 4000.0f));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "resonance", 1 }, "Filter Resonance", NormalisableRange<float> (0.5f, 8.0f, 0.01f, 0.5f), 0.8f));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "filterenv", 1 }, "Filter Env Amount", NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "attack", 1 }, "Attack", NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.4f), 0.01f));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "decay", 1 }, "Decay", NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.4f), 0.3f));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "sustain", 1 }, "Sustain", NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "release", 1 }, "Release", NormalisableRange<float> (0.001f, 8.0f, 0.001f, 0.4f), 0.4f));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "gain", 1 }, "Master Gain", NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    return { params.begin(), params.end() };
}

MiniSynthProcessor::MiniSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    synth.addSound (new SynthSound());
    for (int i = 0; i < 8; ++i)
        synth.addVoice (new SynthVoice (apvts));
}

void MiniSynthProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            v->prepare (sampleRate, samplesPerBlock);
}

bool MiniSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MiniSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* MiniSynthProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void MiniSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MiniSynthProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MiniSynthProcessor();
}
