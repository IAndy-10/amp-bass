// AmpBass offline host.
//
// Loads the *built* VST3 (the same binary the DAW loads), drives its parameters
// directly as a host would, and renders audio — no UI involved. Use it to tell
// apart "the DSP doesn't respond" from "the UI never reaches the parameters".
//
//   AmpBassHostRender --list
//   AmpBassHostRender --sweep [--set "Graphic EQ On=1"]
//   AmpBassHostRender --in di.wav --out amp.wav --set "Gain=0.8" --set "Bass=0.7"
//
// Options:
//   --plugin <path>   default ~/Library/Audio/Plug-Ins/VST3/AmpBass.vst3
//   --set Name=v      parameter by display name, v normalised 0..1 (repeatable)
//   --set-text Name=t parameter by display name, value as the plugin displays it,
//                     e.g. "Rec Chain=Cabinet Bypass" (repeatable)
//   --probe Name      print the displayed text at normalised 0, 0.1 … 1
//   --sr <hz>         sample rate when no --in is given (default 48000)
//   --block <n>       block size (default 512)

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>

#include "TestSignals.h"

#include <cstdio>
#include <map>

using namespace TestSignals;

namespace
{
    struct Options
    {
        juce::File plugin = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                                .getChildFile ("Library/Audio/Plug-Ins/VST3/AmpBass.vst3");
        juce::File in, out;
        std::map<juce::String, float> sets;
        std::map<juce::String, juce::String> textSets;
        juce::String probe;
        double sr = 48000.0;
        int block = 512;
        bool list = false, sweep = false;
    };

    Options parse (int argc, char* argv[])
    {
        Options o;
        for (int i = 1; i < argc; ++i)
        {
            const juce::String a (argv[i]);
            auto next = [&] { return i + 1 < argc ? juce::String (argv[++i]) : juce::String(); };

            if      (a == "--plugin") o.plugin = juce::File::getCurrentWorkingDirectory().getChildFile (next());
            else if (a == "--in")     o.in     = juce::File::getCurrentWorkingDirectory().getChildFile (next());
            else if (a == "--out")    o.out    = juce::File::getCurrentWorkingDirectory().getChildFile (next());
            else if (a == "--sr")     o.sr     = next().getDoubleValue();
            else if (a == "--block")  o.block  = next().getIntValue();
            else if (a == "--list")   o.list   = true;
            else if (a == "--sweep")  o.sweep  = true;
            else if (a == "--set")
            {
                const auto kv = next();
                o.sets[kv.upToLastOccurrenceOf ("=", false, false).trim().toLowerCase()]
                    = kv.fromLastOccurrenceOf ("=", false, false).getFloatValue();
            }
            else if (a == "--set-text")
            {
                const auto kv = next();
                o.textSets[kv.upToFirstOccurrenceOf ("=", false, false).trim().toLowerCase()]
                    = kv.fromFirstOccurrenceOf ("=", false, false).trim();
            }
            else if (a == "--probe") o.probe = next().trim().toLowerCase();
            else std::fprintf (stderr, "Ignoring unknown argument: %s\n", argv[i]);
        }
        return o;
    }

    std::unique_ptr<juce::AudioPluginInstance> load (const Options& o, double sr)
    {
        juce::VST3PluginFormat vst3;
        juce::OwnedArray<juce::PluginDescription> descs;
        vst3.findAllTypesForFile (descs, o.plugin.getFullPathName());
        if (descs.isEmpty())
        {
            std::fprintf (stderr, "No VST3 found at %s\n", o.plugin.getFullPathName().toRawUTF8());
            return nullptr;
        }

        juce::String error;
        auto inst = vst3.createInstanceFromDescription (*descs[0], sr, o.block, error);
        if (inst == nullptr)
            std::fprintf (stderr, "Failed to instantiate: %s\n", error.toRawUTF8());
        return inst;
    }

    juce::AudioProcessorParameter* findParam (juce::AudioPluginInstance& p, const juce::String& lowerName)
    {
        for (auto* param : p.getParameters())
            if (param->getName (128).toLowerCase() == lowerName)
                return param;
        return nullptr;
    }

    // Resets every parameter to default, then applies --set overrides.
    bool applyBaseline (juce::AudioPluginInstance& p, const Options& o)
    {
        for (auto* param : p.getParameters())
            param->setValue (param->getDefaultValue());

        for (auto& [name, v] : o.sets)
        {
            auto* param = findParam (p, name);
            if (param == nullptr)
            {
                std::fprintf (stderr, "Unknown parameter '%s' (use --list)\n", name.toRawUTF8());
                return false;
            }
            param->setValue (v);
        }

        for (auto& [name, text] : o.textSets)
        {
            auto* param = findParam (p, name);
            if (param == nullptr)
            {
                std::fprintf (stderr, "Unknown parameter '%s' (use --list)\n", name.toRawUTF8());
                return false;
            }
            const float v = param->getValueForText (text);
            param->setValue (v);
            const auto shown = param->getText (v, 64);
            if (! shown.trim().startsWithIgnoreCase (text) && ! text.startsWithIgnoreCase (shown.trim()))
                std::fprintf (stderr, "Warning: '%s' = '%s' resolved to %.4f, which displays as '%s'\n",
                              name.toRawUTF8(), text.toRawUTF8(), v, shown.toRawUTF8());
        }
        return true;
    }

    int probeParam (juce::AudioPluginInstance& p, const juce::String& name)
    {
        auto* param = findParam (p, name);
        if (param == nullptr) { std::fprintf (stderr, "Unknown parameter '%s'\n", name.toRawUTF8()); return 1; }
        std::printf ("%s (steps: %d)\n", param->getName (128).toRawUTF8(), param->getNumSteps());
        for (int i = 0; i <= 10; ++i)
            std::printf ("  %.1f  %s\n", i / 10.0, param->getText (i / 10.0f, 64).toRawUTF8());
        return 0;
    }

    juce::AudioBuffer<float> renderThrough (juce::AudioPluginInstance& p, const juce::AudioBuffer<float>& input,
                                            double sr, int blockSize)
    {
        // Re-prepare so DSP state from the previous render doesn't leak in.
        p.releaseResources();
        p.prepareToPlay (sr, blockSize);

        juce::AudioBuffer<float> out (input);
        juce::AudioBuffer<float> block (std::max (2, p.getTotalNumOutputChannels()), blockSize);
        juce::MidiBuffer midi;

        for (int pos = 0; pos < input.getNumSamples(); pos += blockSize)
        {
            const int n = std::min (blockSize, input.getNumSamples() - pos);
            block.setSize (block.getNumChannels(), n, false, false, true);
            block.clear();
            for (int ch = 0; ch < 2; ++ch)
                block.copyFrom (ch, 0, input, std::min (ch, input.getNumChannels() - 1), pos, n);

            p.processBlock (block, midi);

            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                out.copyFrom (ch, pos, block, ch, 0, n);
        }
        return out;
    }

    void listParams (juce::AudioPluginInstance& p)
    {
        std::printf ("%-4s %-22s %10s  %s\n", "#", "name", "default", "text");
        int i = 0;
        for (auto* param : p.getParameters())
            std::printf ("%-4d %-22s %10.3f  %s\n", i++, param->getName (128).toRawUTF8(),
                         param->getDefaultValue(), param->getText (param->getDefaultValue(), 32).toRawUTF8());
    }

    void report (const juce::AudioBuffer<float>& in, const juce::AudioBuffer<float>& out)
    {
        const int n = in.getNumSamples();
        const double inRms = rms (in.getReadPointer (0), 0, n), outRms = rms (out.getReadPointer (0), 0, n);

        juce::AudioBuffer<float> diff (1, n);
        diff.copyFrom (0, 0, out, 0, 0, n);
        diff.addFrom (0, 0, in, 0, 0, n, -1.0f);
        const double nullDb = toDb (rms (diff.getReadPointer (0), 0, n) / std::max (inRms, 1e-12));

        std::printf ("in : RMS %7.2f dBFS  peak %7.2f dBFS\n", toDb (inRms), toDb (peak (in.getReadPointer (0), 0, n)));
        std::printf ("out: RMS %7.2f dBFS  peak %7.2f dBFS\n", toDb (outRms), toDb (peak (out.getReadPointer (0), 0, n)));
        std::printf ("null (out - in) rel. input: %.1f dB %s\n", nullDb,
                     nullDb < -60.0 ? "<- output is IDENTICAL to input: plugin is passing audio through" : "");
        std::printf ("finite: %s\n", allFinite (out) ? "yes" : "NO — NaN/Inf in output");
    }

    int sweep (juce::AudioPluginInstance& p, const Options& o)
    {
        const std::vector<double> tones = { 41, 100, 250, 800, 2000, 5000 };
        const auto sig = multitone (o.sr, tones, 0.03f, 1.0);
        juce::AudioBuffer<float> in (2, static_cast<int> (sig.size()));
        for (int ch = 0; ch < 2; ++ch) in.copyFrom (ch, 0, sig.data(), in.getNumSamples());
        const int n = in.getNumSamples();

        auto spectrum = [&]
        {
            const auto out = renderThrough (p, in, o.sr, o.block);
            std::vector<double> s;
            for (auto f : tones) s.push_back (toDb (toneAmplitude (out.getReadPointer (0), n / 2, n, o.sr, f)));
            return s;
        };

        if (! applyBaseline (p, o)) return 1;
        const auto base = spectrum();

        std::printf ("Baseline = defaults%s. Δ dB per tone when each parameter is set to 0 / 1 (normalised).\n",
                     o.sets.empty() ? "" : " + --set overrides");
        std::printf ("%-26s", "");
        for (auto f : tones) std::printf ("%8.0f", f);
        std::printf ("\n");

        int inert = 0;
        for (auto* param : p.getParameters())
        {
            for (float v : { 0.0f, 1.0f })
            {
                if (! applyBaseline (p, o)) return 1;
                param->setValue (v);
                const auto s = spectrum();

                double maxAbs = 0.0;
                const auto label = param->getName (20) + " = " + param->getText (v, 12);
                std::printf ("%-26s", label.toRawUTF8());
                for (size_t i = 0; i < tones.size(); ++i)
                {
                    maxAbs = std::max (maxAbs, std::abs (s[i] - base[i]));
                    std::printf ("%+8.2f", s[i] - base[i]);
                }
                const bool isDefault = std::abs (v - param->getDefaultValue()) < 1e-3f;
                const bool flagged = maxAbs < 0.5 && ! isDefault;
                inert += flagged ? 1 : 0;
                std::printf ("  %s\n", isDefault ? "(= default)" : (flagged ? "<- INERT" : ""));
            }
        }
        std::printf ("\n%d inert settings. GEQ bands/LEVEL are expected to be inert unless run with "
                     "--set \"Graphic EQ On=1\".\n", inert);
        return 0;
    }
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    const auto o = parse (argc, argv);

    // Input: a WAV file, or a 4 s generated test phrase (41 Hz + 100 Hz notes at DI level).
    juce::AudioBuffer<float> input;
    double sr = o.sr;
    if (o.in != juce::File())
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (o.in));
        if (reader == nullptr) { std::fprintf (stderr, "Cannot read %s\n", o.in.getFullPathName().toRawUTF8()); return 1; }
        sr = reader->sampleRate;
        input.setSize (2, static_cast<int> (reader->lengthInSamples));
        reader->read (&input, 0, input.getNumSamples(), 0, true, true);
    }
    else
    {
        const auto a = sine (sr, 41.2, 0.125f, 2.0), b = sine (sr, 98.0, 0.125f, 2.0);
        input.setSize (2, static_cast<int> (a.size() + b.size()));
        for (int ch = 0; ch < 2; ++ch)
        {
            input.copyFrom (ch, 0, a.data(), static_cast<int> (a.size()));
            input.copyFrom (ch, static_cast<int> (a.size()), b.data(), static_cast<int> (b.size()));
        }
    }

    auto plugin = load (o, sr);
    if (plugin == nullptr) return 1;
    plugin->enableAllBuses();
    std::printf ("Loaded %s (%d in / %d out)\n\n", plugin->getName().toRawUTF8(),
                 plugin->getTotalNumInputChannels(), plugin->getTotalNumOutputChannels());

    std::printf ("Latency: %d samples\n", plugin->getLatencySamples());
    if (o.list) { listParams (*plugin); return 0; }
    if (o.probe.isNotEmpty()) return probeParam (*plugin, o.probe);
    if (o.sweep) return sweep (*plugin, Options (o));

    if (! applyBaseline (*plugin, o)) return 1;
    const auto output = renderThrough (*plugin, input, sr, o.block);
    report (input, output);

    if (o.out != juce::File())
    {
        o.out.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer (
            wav.createWriterFor (new juce::FileOutputStream (o.out), sr, 2, 32, {}, 0)); // 32-bit float: no clipping above 0 dBFS
        if (writer == nullptr) { std::fprintf (stderr, "Cannot write %s\n", o.out.getFullPathName().toRawUTF8()); return 1; }
        writer->writeFromAudioSampleBuffer (output, 0, output.getNumSamples());
        std::printf ("wrote %s\n", o.out.getFullPathName().toRawUTF8());
    }
    return 0;
}
