#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

struct SynthSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    explicit SynthVoice (juce::AudioProcessorValueTreeState& s) : apvts (s) {}

    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<SynthSound*> (s) != nullptr; }

    void prepare (double sampleRate, int blockSize)
    {
        sr = sampleRate;
        adsr.setSampleRate (sr);
        juce::dsp::ProcessSpec spec { sr, (juce::uint32) blockSize, 1 };
        filter.prepare (spec);
        filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        filter.reset();
    }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
    {
        freq  = (float) juce::MidiMessage::getMidiNoteInHertz (midiNote);
        level = velocity;
        phase1 = phase2 = 0.0f;
        filter.reset();
        readParams();
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff) adsr.noteOff();
        else { adsr.reset(); clearCurrentNote(); }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int start, int num) override
    {
        if (! isVoiceActive()) return;
        readParams();

        const float dt1 = freq / (float) sr;
        const float dt2 = freq * std::pow (2.0f, detuneCents / 1200.0f) / (float) sr;

        for (int i = 0; i < num; ++i)
        {
            float s = osc (wave1, phase1, dt1) + osc (wave2, phase2, dt2) * level2;
            s *= 0.5f;

            const float env = adsr.getNextSample();
            const float fc = juce::jlimit (20.0f, 18000.0f, cutoff * std::pow (2.0f, envAmt * 5.0f * env));
            filter.setCutoffFrequency (fc);
            s = filter.processSample (0, s);
            s *= env * level * gain;

            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                out.addSample (ch, start + i, s);

            phase1 += dt1; if (phase1 >= 1.0f) phase1 -= 1.0f;
            phase2 += dt2; if (phase2 >= 1.0f) phase2 -= 1.0f;

            if (! adsr.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

private:
    static float polyBlep (float t, float dt)
    {
        if (t < dt)         { t /= dt;               return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt)  { t = (t - 1.0f) / dt;   return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    static float osc (int type, float ph, float dt)
    {
        switch (type)
        {
            case 0:  return std::sin (juce::MathConstants<float>::twoPi * ph);
            case 1:  return 2.0f * ph - 1.0f - polyBlep (ph, dt);
            case 2:
            {
                float v = ph < 0.5f ? 1.0f : -1.0f;
                v += polyBlep (ph, dt);
                v -= polyBlep (std::fmod (ph + 0.5f, 1.0f), dt);
                return v;
            }
            default: return 4.0f * std::abs (ph - 0.5f) - 1.0f;
        }
    }

    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    void readParams()
    {
        wave1       = (int) p ("wave1");
        wave2       = (int) p ("wave2");
        detuneCents = p ("detune");
        level2      = p ("osc2level");
        cutoff      = p ("cutoff");
        filter.setResonance (p ("resonance"));
        envAmt      = p ("filterenv");
        gain        = p ("gain");
        adsr.setParameters ({ p ("attack"), p ("decay"), p ("sustain"), p ("release") });
    }

    juce::AudioProcessorValueTreeState& apvts;
    juce::ADSR adsr;
    juce::dsp::StateVariableTPTFilter<float> filter;

    double sr = 44100.0;
    float freq = 440.0f, level = 0.0f, phase1 = 0.0f, phase2 = 0.0f;
    int wave1 = 1, wave2 = 1;
    float detuneCents = 7.0f, level2 = 0.7f, cutoff = 4000.0f, envAmt = 0.3f, gain = 0.5f;
};

class MiniSynthProcessor : public juce::AudioProcessor
{
public:
    MiniSynthProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MiniSynth"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::Synthesiser synth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniSynthProcessor)
};
