// AmpBass DSP diagnostics.
//
// Exercises every DSP stage in isolation and the full chain, prints measured
// responses, and flags behaviour that would make the plugin sound "unmodified"
// or wrong in a DAW. Exit code = number of FAIL results.
//
// The Chain struct below mirrors AmpBassAudioProcessor::processBlock. Keep it in
// sync; the HostRender tool tests the real compiled plugin end-to-end.

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "InputGain.h"
#include "PreFilters.h"
#include "TubePreamp.h"
#include "ToneStack.h"
#include "GraphicEQ.h"
#include "MasterVolume.h"
#include "PowerSection.h"
#include "OutputSwitch.h"
#include "Primitives/DryWetMixer.h"

#include "TestSignals.h"
#include "../Parameters/ParameterLayout.h"

#include <cstdio>
#include <string>

using namespace TestSignals;

namespace
{
    constexpr double kSr = 48000.0;
    constexpr int kBlock = 512;

    int failures = 0;
    int warnings = 0;

    void section (const char* title) { std::printf ("\n=== %s ===\n", title); }

    void check (bool ok, const std::string& name, const std::string& detail, bool warnOnly = false)
    {
        const char* tag = ok ? "PASS" : (warnOnly ? "WARN" : "FAIL");
        if (! ok) (warnOnly ? warnings : failures)++;
        std::printf ("[%s] %s — %s\n", tag, name.c_str(), detail.c_str());
    }

    std::string fmt (const char* f, double a, double b = 0.0, double c = 0.0)
    {
        char buf[256];
        std::snprintf (buf, sizeof (buf), f, a, b, c);
        return buf;
    }

    // Steady-state gain (dB) of a stage for a sine at `freq`. Measures the
    // second half of a 1 s render so parameter smoothing has settled.
    template <typename Stage, typename Setup>
    double stageGainDb (double freq, Setup&& setup, double sr = kSr, float amp = 0.05f)
    {
        Stage stage;
        stage.prepare (sr, kBlock);
        setup (stage);

        const auto in = sine (sr, freq, amp, 1.0);
        const auto out = render (in, kBlock, [&] (auto& b) { stage.process (b); });
        const int n = static_cast<int> (in.size());
        return toDb (rms (out.getReadPointer (0), n / 2, n) / rms (in.data(), n / 2, n));
    }

    // ── Mirror of the processor's signal chain ──
    struct Params
    {
        bool  pad = false, hiInput = true;
        bool  bright = false, ultraHi = false, ultraLo = false;
        float gain = 0.5f, tubeGain = 0.5f;
        float bass = 0.5f, mid = 0.5f, treble = 0.5f;
        int   midFreq = 3;
        bool  geqOn = false;
        float geq[GraphicEQ::kNumBands] = {};
        float geqLevel = 0.0f;
        float master = 0.5f;
        float dryWet = 1.0f;
    };

    struct Chain
    {
        InputGain inputGain; PreFilters preFilters; TubePreamp tubePreamp; ToneStack toneStack;
        GraphicEQ graphicEQ; MasterVolume masterVolume; PowerSection powerSection; DryWetMixer dryWet;

        int preparedBlock = kBlock;

        void prepare (double sr, int block)
        {
            preparedBlock = block;
            inputGain.prepare (sr, block);  preFilters.prepare (sr, block); tubePreamp.prepare (sr, block);
            toneStack.prepare (sr, block);  graphicEQ.prepare (sr, block);  masterVolume.prepare (sr, block);
            powerSection.prepare (sr, block); dryWet.prepare (2, block);
        }

        void process (juce::AudioBuffer<float>& b, const Params& p)
        {
            inputGain.setParams (p.pad, p.hiInput);
            preFilters.setParams (p.bright, p.ultraHi, p.ultraLo);
            tubePreamp.setParams (p.gain, p.tubeGain);
            toneStack.setParams (p.bass, p.mid, p.midFreq, p.treble);
            graphicEQ.setParams (p.geqOn, p.geq, p.geqLevel);
            masterVolume.setParams (p.master);
            dryWet.setMix (p.dryWet);

            // Sub-block chunking, as in the processor
            for (int start = 0; start < b.getNumSamples(); start += preparedBlock)
            {
                const int n = std::min (preparedBlock, b.getNumSamples() - start);
                juce::AudioBuffer<float> sub (b.getArrayOfWritePointers(), b.getNumChannels(), start, n);
                dryWet.saveDry (sub);
                inputGain.process (sub); preFilters.process (sub); tubePreamp.process (sub); toneStack.process (sub);
                graphicEQ.process (sub); masterVolume.process (sub); powerSection.process (sub);
                dryWet.mixToOutput (sub);
            }
        }
    };

    juce::AudioBuffer<float> renderChain (const std::vector<float>& in, const Params& p,
                                          double sr = kSr, int block = kBlock, int hostBlock = 0)
    {
        Chain chain;
        chain.prepare (sr, block);
        return render (in, hostBlock > 0 ? hostBlock : block, [&] (auto& b) { chain.process (b, p); });
    }

    // ─────────────────────────────────────────────────────────────

    void testInputGain()
    {
        section ("Stage 1 — InputGain");
        const double def = stageGainDb<InputGain> (200.0, [] (auto& s) { s.setParams (false, true); });
        const double pad = stageGainDb<InputGain> (200.0, [] (auto& s) { s.setParams (true, true); });
        const double lo  = stageGainDb<InputGain> (200.0, [] (auto& s) { s.setParams (false, false); });
        check (std::abs (def) < 0.1,          "default is unity", fmt ("%.2f dB", def));
        check (std::abs (pad + 15.0) < 0.2,   "-15 dB pad",       fmt ("%.2f dB", pad));
        check (std::abs (lo + 6.0) < 0.2,     "LO input -6 dB",   fmt ("%.2f dB", lo));
    }

    void testPreFilters()
    {
        section ("Stage 2 — PreFilters");
        const double freqs[] = { 40, 100, 500, 2000, 5000, 10000 };
        std::printf ("%-10s", "Hz");
        for (auto f : freqs) std::printf ("%8.0f", f);
        std::printf ("\n");

        auto row = [&] (const char* label, bool br, bool uh, bool ul)
        {
            std::printf ("%-10s", label);
            std::vector<double> g;
            for (auto f : freqs)
            {
                g.push_back (stageGainDb<PreFilters> (f, [=] (auto& s) { s.setParams (br, uh, ul); }));
                std::printf ("%8.2f", g.back());
            }
            std::printf ("\n");
            return g;
        };

        const auto off = row ("all off", false, false, false);
        const auto br  = row ("bright",  true,  false, false);
        const auto uh  = row ("ultraHi", false, true,  false);
        const auto ul  = row ("ultraLo", false, false, true);

        double maxDev = 0.0;
        for (auto g : off) maxDev = std::max (maxDev, std::abs (g));
        check (maxDev < 0.1,              "all off is flat",            fmt ("max dev %.2f dB", maxDev));
        // Values measured on the reference plugin (switch on minus off).
        check (std::abs (br[3] - 5.1) < 0.5 && std::abs (br[5] - 3.3) < 0.5, "bright matches the reference (+5.1 dB 2 kHz, +3.3 dB 10 kHz)",
               fmt ("%.2f / %.2f dB", br[3], br[5]));
        check (std::abs (uh[4] - 6.0) < 0.5 && std::abs (uh[5] - 5.3) < 0.5, "ultraHi matches the reference (+6.0 dB 5 kHz, +5.3 dB 10 kHz)",
               fmt ("%.2f / %.2f dB", uh[4], uh[5]));
        check (std::abs (ul[2] + 12.2) < 0.5, "ultraLo -12 dB at 500 Hz",  fmt ("%.2f dB", ul[2]));
        check (std::abs (ul[0]) < 0.5,        "ultraLo leaves 40 Hz flat (no +2 dB in the reference)", fmt ("%.2f dB", ul[0]));
        check (std::abs (ul[4] + 0.4) < 0.5,  "ultraLo leaves 5 kHz flat", fmt ("%.2f dB", ul[4]));
    }

    void testToneStack()
    {
        section ("Stage 4 — ToneStack");
        auto g = [] (double f, float b, float m, int mf, float t)
        {
            return stageGainDb<ToneStack> (f, [=] (auto& s) { s.setParams (b, m, mf, t); });
        };

        const double flat41 = g (41, .5f, .5f, 3, .5f), flat800 = g (800, .5f, .5f, 3, .5f), flat8k = g (8000, .5f, .5f, 3, .5f);
        // All centred = the fixed voicing fitted to the reference (bass bump, mid scoop, presence).
        check (std::abs (flat41 - 3.8) < 0.3 && std::abs (flat800 + 3.3) < 0.3 && std::abs (flat8k - 1.1) < 0.3,
               "all centred gives the reference voicing (+3.8 / -3.3 / +1.1 dB)",
               fmt ("41Hz %.2f, 800Hz %.2f, 8k %.2f dB", flat41, flat800, flat8k));

        const double bassSwing41  = g (41, 1, .5f, 3, .5f)  - g (41, 0, .5f, 3, .5f);
        const double bassSwing100 = g (100, 1, .5f, 3, .5f) - g (100, 0, .5f, 3, .5f);
        const double bassSwing200 = g (200, 1, .5f, 3, .5f) - g (200, 0, .5f, 3, .5f);
        check (bassSwing41 > 15.0, "bass knob swing @41 Hz", fmt ("%.2f dB", bassSwing41));
        check (bassSwing100 > 10.0, "bass knob swing @100 Hz (where a bass DI has most energy)",
               fmt ("%.2f dB  (@200 Hz: %.2f dB)", bassSwing100, bassSwing200));

        // Bass extremes vs noon, as measured on the reference: -11.9 / +13.6 dB at 40 Hz, -6.4 / +10.2 dB at 100 Hz.
        const double noon40 = g (40, .5f, .5f, 3, .5f), noon100 = g (100, .5f, .5f, 3, .5f);
        const double cut40 = g (40, 0, .5f, 3, .5f) - noon40, boost40 = g (40, 1, .5f, 3, .5f) - noon40;
        const double cut100 = g (100, 0, .5f, 3, .5f) - noon100, boost100 = g (100, 1, .5f, 3, .5f) - noon100;
        check (std::abs (cut40 + 11.9) < 0.7 && std::abs (boost40 - 13.6) < 0.7
                   && std::abs (cut100 + 6.4) < 0.7 && std::abs (boost100 - 10.2) < 0.7,
               "bass 0 / 10 match the reference",
               fmt ("40 Hz %.1f / %+.1f, 100 Hz %.1f", cut40, boost40, cut100) + fmt (" / %+.1f dB", boost100));

        // Treble extremes vs noon, as measured on the reference: -11.9 / +12.7 dB at 2 kHz, -23.6 / +16.1 dB at 8 kHz.
        {
            const double n2k = g (2000, .5f, .5f, 3, .5f), n8k = g (8000, .5f, .5f, 3, .5f);
            const double c2 = g (2000, .5f, .5f, 3, 0) - n2k, b2 = g (2000, .5f, .5f, 3, 1) - n2k;
            const double c8 = g (8000, .5f, .5f, 3, 0) - n8k, b8 = g (8000, .5f, .5f, 3, 1) - n8k;
            check (std::abs (c2 + 11.9) < 1.0 && std::abs (b2 - 12.7) < 1.0 && std::abs (c8 + 23.6) < 1.0 && std::abs (b8 - 16.1) < 1.0,
                   "treble 0 / 10 match the reference",
                   fmt ("2 kHz %.1f / %+.1f, 8 kHz %.1f", c2, b2, c8) + fmt (" / %+.1f dB", b8));
        }
        const double trebSwing8k = g (8000, .5f, .5f, 3, 1) - g (8000, .5f, .5f, 3, 0);
        const double trebSwing2k = g (2000, .5f, .5f, 3, 1) - g (2000, .5f, .5f, 3, 0);
        check (trebSwing8k > 20.0, "treble knob swing @8 kHz", fmt ("%.2f dB", trebSwing8k));
        check (trebSwing2k > 6.0, "treble knob swing @2 kHz (bass DI has little energy above 5k)",
               fmt ("%.2f dB", trebSwing2k));

        // Mid at 0 and 10 vs noon at each position's centre, as measured on the reference.
        const double midHz[]    = { 249, 543, 895, 1773, 2884 };
        const double midCut[]   = { -18.4, -18.3, -19.9, -19.8, -19.0 };
        const double midBoost[] = { 13.2, 12.6, 12.0, 10.8, 11.3 };
        for (int pos = 1; pos <= 5; ++pos)
        {
            const double f = midHz[pos - 1], noon = g (f, .5f, .5f, pos, .5f);
            const double cut = g (f, .5f, 0, pos, .5f) - noon, boost = g (f, .5f, 1, pos, .5f) - noon;
            check (std::abs (cut - midCut[pos - 1]) < 0.7 && std::abs (boost - midBoost[pos - 1]) < 0.7,
                   "mid 0 / 10 match the reference at position " + std::to_string (pos),
                   fmt ("%.0f Hz → %.1f / %+.1f dB", f, cut, boost));
        }
    }

    void testGraphicEQ()
    {
        section ("Stage 5 — GraphicEQ");
        // Measured on the reference: band centre and peak gain with the slider at +12 (EQ-on hump removed).
        const double centres[GraphicEQ::kNumBands] = { 33.1, 78.6, 141.4, 305.9, 537.4, 859.7, 2240, 5046, 7690 };
        const float  maxDb[GraphicEQ::kNumBands]   = { 14.9f, 9.8f, 9.8f, 9.8f, 9.8f, 9.8f, 9.8f, 9.7f, 11.9f };
        float noBands[GraphicEQ::kNumBands] = {};
        const auto hump = [&] (double f) { return stageGainDb<GraphicEQ> (f, [&] (auto& s) { s.setParams (true, noBands, 0.0f); }); };

        float allUp[GraphicEQ::kNumBands];
        for (int b = 0; b < GraphicEQ::kNumBands; ++b) allUp[b] = maxDb[b];

        const double offGain = stageGainDb<GraphicEQ> (600, [&] (auto& s) { s.setParams (false, allUp, 10.0f); });
        check (std::abs (offGain) < 0.1, "EQ OFF ignores band sliders and LEVEL (UI dims them)",
               fmt ("%.2f dB with every band maxed", offGain));

        for (int b = 0; b < GraphicEQ::kNumBands; ++b)
        {
            float bands[GraphicEQ::kNumBands] = {};
            bands[b] = 12.0f;
            const double measured = stageGainDb<GraphicEQ> (centres[b], [&] (auto& s) { s.setParams (true, bands, 0.0f); })
                                  - hump (centres[b]);
            check (std::abs (measured - maxDb[b]) < 0.5, fmt ("band %.0f Hz at max matches the reference", centres[b]),
                   fmt ("expected %.1f, got %.2f dB", maxDb[b], measured));
        }

        const double on = hump (500);
        check (std::abs (on - 0.46) < 0.1, "EQ on, all flat: +0.46 dB hump at 500 Hz like the reference", fmt ("%.2f dB", on));
        const double lvl = stageGainDb<GraphicEQ> (500, [&] (auto& s) { s.setParams (true, noBands, 8.0f); }) - on;
        check (std::abs (lvl - 8.0) < 0.2, "LEVEL +8 dB (EQ on)", fmt ("%.2f dB", lvl));

        // Slider law (300 Hz band): +6 on the slider is only +1.5 dB, like the reference
        float half[GraphicEQ::kNumBands] = {};
        half[3] = 6.0f;
        const double halfDb = stageGainDb<GraphicEQ> (305.9, [&] (auto& s) { s.setParams (true, half, 0.0f); }) - hump (305.9);
        check (std::abs (halfDb - 1.5) < 0.3, "slider +6 gives +1.5 dB (reference law)", fmt ("%.2f dB", halfDb));
    }

    void testMasterVolume()
    {
        section ("Stage 6 — MasterVolume");
        const double half = stageGainDb<MasterVolume> (200, [] (auto& s) { s.setParams (0.5f); });
        const double full = stageGainDb<MasterVolume> (200, [] (auto& s) { s.setParams (1.0f); });
        check (std::abs (half + 12.04) < 0.2, "knob 0.5 → -12 dB (quadratic taper)", fmt ("%.2f dB", half));
        check (std::abs (full) < 0.1,         "knob 1.0 → unity",                    fmt ("%.2f dB", full));
    }

    void testTubePreamp()
    {
        section ("Stage 3 — TubePreamp (nonlinear)");
        const float diAmp = 0.125f; // ≈ -18 dBFS peak, a typical bass DI
        const auto in = sine (kSr, 100.0, diAmp, 1.0);
        const int n = static_cast<int> (in.size());
        const double inRms = rms (in.data(), n / 2, n);

        std::printf ("100 Hz sine @ -18 dBFS.  rows = GAIN, cols = TUBE GAIN.  cell = out level dB / THD %%\n");
        std::printf ("%-6s %18s %18s %18s\n", "", "tube 0.0", "tube 0.5", "tube 1.0");

        double thdAtGain[5] = {}, lvlAtGain[5] = {};
        const float gains[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
        for (int gi = 0; gi < 5; ++gi)
        {
            std::printf ("%-6.2f", gains[gi]);
            for (float tube : { 0.0f, 0.5f, 1.0f })
            {
                TubePreamp tp;
                tp.prepare (kSr, kBlock);
                tp.setParams (gains[gi], tube);
                const auto out = render (in, kBlock, [&] (auto& b) { tp.process (b); });
                const double lvl = toDb (rms (out.getReadPointer (0), n / 2, n) / inRms);
                const double d = 100.0 * thd (out.getReadPointer (0), n / 2, n, kSr, 100.0);
                std::printf ("   %+7.2f / %5.1f%%", lvl, d);
                if (tube == 0.5f) { thdAtGain[gi] = d; lvlAtGain[gi] = lvl; }
            }
            std::printf ("\n");
        }

        check (thdAtGain[4] > thdAtGain[0] + 1.0, "GAIN increases distortion",
               fmt ("THD %.1f%% → %.1f%%", thdAtGain[0], thdAtGain[4]));
        check (thdAtGain[2] < 10.0, "default GAIN (0.5) is not already heavily clipped",
               fmt ("THD %.1f%% at defaults with a -18 dBFS DI", thdAtGain[2]), true);
        check (lvlAtGain[4] - lvlAtGain[0] < 12.0, "level jump across GAIN range",
               fmt ("%.1f dB", lvlAtGain[4] - lvlAtGain[0]));

        // Small-signal low-frequency response (80 Hz pre-emphasis HPF).
        auto lf = [] (double f)
        {
            return stageGainDb<TubePreamp> (f, [] (auto& s) { s.setParams (0.0f, 1.0f); }, kSr, 0.01f);
        };
        const double ref = lf (400), e1 = lf (41.2), a1 = lf (55);
        check (e1 - ref > -3.0, "low E (41 Hz) survives preamp",
               fmt ("41 Hz %.2f dB, 55 Hz %.2f dB rel. 400 Hz", e1 - ref, a1 - ref));
    }

    void testPowerSection()
    {
        section ("Stage 7 — PowerSection transfer curve");
        std::printf ("%10s %12s %12s\n", "in peak", "out peak", "out RMS dB");
        double prevRms = -1e9;
        bool monotonic = true;
        std::string where;

        for (float amp : { 0.2f, 0.4f, 0.6f, 0.8f, 0.9f, 1.0f, 1.2f, 1.6f, 2.0f, 3.0f })
        {
            PowerSection ps;
            ps.prepare (kSr, kBlock);
            const auto in = sine (kSr, 100.0, amp, 1.0);
            const auto out = render (in, kBlock, [&] (auto& b) { ps.process (b); });
            const int n = static_cast<int> (in.size());
            const double r = toDb (rms (out.getReadPointer (0), n / 2, n));
            std::printf ("%10.2f %12.3f %12.2f\n", amp, peak (out.getReadPointer (0), n / 2, n), r);
            if (r < prevRms - 0.05 && monotonic) { monotonic = false; where = fmt ("output drops at input %.2f", amp); }
            prevRms = r;
        }
        check (monotonic, "louder input never gives quieter output",
               monotonic ? "ok" : where + " — compressor gain is T/sqrt(env/T)/env, i.e. slope -1, not 2:1");

        // Output low-pass (fitted to the reference): flat in the bass, rolls off above ~8 kHz.
        const auto lp = [] (double f) { return stageGainDb<PowerSection> (f, [] (auto&) {}); };
        const double at1k = lp (1000), at10k = lp (10000), at20k = lp (20000);
        check (std::abs (at1k) < 0.2, "output low-pass flat at 1 kHz", fmt ("%.2f dB", at1k));
        check (std::abs (at10k + 4.0) < 0.5, "output low-pass -4 dB at 10 kHz", fmt ("%.2f dB", at10k));
        check (std::abs (at20k + 19.2) < 1.5, "output low-pass -19 dB at 20 kHz", fmt ("%.2f dB", at20k));
    }

    void testOutputSwitch()
    {
        section ("Output — bypass / mute");

        // "processed" signal = input inverted, so bypass is easy to tell apart
        auto run = [] (bool bypass, bool mute, bool switchMidway, double& maxStep)
        {
            OutputSwitch sw;
            sw.prepare (kSr, kBlock);
            const auto in = sine (kSr, 100.0, 0.5f, 0.5);
            const int total = static_cast<int> (in.size());
            juce::AudioBuffer<float> out (1, total);
            maxStep = 0.0;
            float prev = 0.0f;

            for (int start = 0; start < total; start += kBlock)
            {
                const int n = std::min (kBlock, total - start);
                const bool on = ! switchMidway || start >= total / 2;
                sw.setParams (bypass && on, mute && on);

                juce::AudioBuffer<float> dry (1, n), buf (1, n);
                for (int i = 0; i < n; ++i)
                {
                    dry.setSample (0, i, in[(size_t) (start + i)]);
                    buf.setSample (0, i, -in[(size_t) (start + i)]);
                }
                sw.process (buf, dry);
                for (int i = 0; i < n; ++i)
                {
                    const float v = buf.getSample (0, i);
                    out.setSample (0, start + i, v);
                    maxStep = std::max (maxStep, (double) std::abs (v - prev));
                    prev = v;
                }
            }
            return out;
        };

        double step = 0.0;
        const auto in = sine (kSr, 100.0, 0.5f, 0.5);
        const int n = static_cast<int> (in.size());

        const auto active = run (false, false, false, step);
        double err = 0.0;
        for (int i = 0; i < n; ++i) err = std::max (err, (double) std::abs (active.getSample (0, i) + in[(size_t) i]));
        check (err < 1e-6, "active passes the processed signal", fmt ("max err %.2g", err));

        const auto byp = run (true, false, false, step);
        err = 0.0;
        for (int i = n / 2; i < n; ++i) err = std::max (err, (double) std::abs (byp.getSample (0, i) - in[(size_t) i]));
        check (err < 1e-6, "bypass outputs the dry input", fmt ("max err %.2g", err));

        const auto muted = run (false, true, false, step);
        const double mutedPeak = peak (muted.getReadPointer (0), n / 2, n);
        check (mutedPeak < 1e-6, "mute is silent", fmt ("peak %.2g", mutedPeak));

        const auto bypMuted = run (true, true, false, step);
        const double bmPeak = peak (bypMuted.getReadPointer (0), n / 2, n);
        check (bmPeak < 1e-6, "mute wins over bypass", fmt ("peak %.2g", bmPeak));

        // A 100 Hz sine at 0.5 changes by at most ~0.0033 per sample at 48 kHz;
        // a hard switch from -x to +x would jump up to 1.0.
        run (true, false, true, step);
        check (step < 0.01, "bypass switch has no click", fmt ("max step %.4f", step));
        run (false, true, true, step);
        check (step < 0.01, "mute switch has no click", fmt ("max step %.4f", step));
    }

    void testChainSensitivity()
    {
        section ("Full chain — does every control change the sound?");
        const std::vector<double> tones = { 41, 100, 250, 800, 2000, 5000 };
        const auto in = multitone (kSr, tones, 0.03f, 1.0);
        const int n = static_cast<int> (in.size());

        auto spectrum = [&] (const Params& p)
        {
            const auto out = renderChain (in, p);
            std::vector<double> s;
            for (auto f : tones) s.push_back (toDb (toneAmplitude (out.getReadPointer (0), n / 2, n, kSr, f)));
            return s;
        };

        struct Case { const char* label; Params a, b; bool expectInert; };
        std::vector<Case> cases;
        using Mutate = std::function<void (Params&)>;
        auto add = [&] (const char* label, Mutate mutateB, bool expectInert = false,
                        Mutate mutateA = [] (Params&) {})
        {
            Params a, b;
            mutateA (a); mutateA (b); mutateB (b);
            cases.push_back ({ label, a, b, expectInert });
        };

        add ("pad on",            [] (Params& p) { p.pad = true; });
        add ("input LO",          [] (Params& p) { p.hiInput = false; });
        add ("bright",            [] (Params& p) { p.bright = true; });
        add ("ultraHi",           [] (Params& p) { p.ultraHi = true; });
        add ("ultraLo",           [] (Params& p) { p.ultraLo = true; });
        add ("gain 0",            [] (Params& p) { p.gain = 0; });
        add ("gain 1",            [] (Params& p) { p.gain = 1; });
        add ("tubeGain 0",        [] (Params& p) { p.tubeGain = 0; });
        add ("tubeGain 1",        [] (Params& p) { p.tubeGain = 1; });
        add ("bass 0",            [] (Params& p) { p.bass = 0; });
        add ("bass 1",            [] (Params& p) { p.bass = 1; });
        add ("mid 0",             [] (Params& p) { p.mid = 0; });
        add ("mid 1",             [] (Params& p) { p.mid = 1; });
        add ("midFreq 3→1 (mid 1)", [] (Params& p) { p.midFreq = 1; }, false, [] (Params& p) { p.mid = 1; });
        add ("treble 0",          [] (Params& p) { p.treble = 0; });
        add ("treble 1",          [] (Params& p) { p.treble = 1; });
        add ("geq600 +8, EQ off", [] (Params& p) { p.geq[4] = 8; }, true);
        add ("geqLevel +8, EQ off", [] (Params& p) { p.geqLevel = 8; }, true);
        add ("geq600 +8, EQ on",  [] (Params& p) { p.geq[4] = 8; }, false, [] (Params& p) { p.geqOn = true; });
        add ("master 1",          [] (Params& p) { p.master = 1; });
        add ("dryWet 0 (dry)",    [] (Params& p) { p.dryWet = 0; });

        std::printf ("Δ dB per tone vs. baseline (multitone, 6 × -30 dBFS)\n%-24s", "");
        for (auto f : tones) std::printf ("%8.0f", f);
        std::printf ("\n");

        for (auto& c : cases)
        {
            const auto sa = spectrum (c.a), sb = spectrum (c.b);
            double maxAbs = 0.0;
            std::printf ("%-24s", c.label);
            for (size_t i = 0; i < tones.size(); ++i)
            {
                const double d = sb[i] - sa[i];
                maxAbs = std::max (maxAbs, std::abs (d));
                std::printf ("%+8.2f", d);
            }
            const bool inert = maxAbs < 0.5;
            std::printf ("  %s\n", inert ? "<- INERT" : "");
            if (inert != c.expectInert)
                check (false, std::string ("control '") + c.label + "'",
                       inert ? "changes nothing" : "unexpectedly changes the sound");
        }
    }

    void testDefaultsAndSafety()
    {
        section ("Defaults, stability, block size, sample rate");

        // Defaults with a DI-level tone
        {
            const auto in = sine (kSr, 100.0, 0.125f, 1.0);
            const int n = static_cast<int> (in.size());
            const auto out = renderChain (in, Params {});
            const double lvl = toDb (rms (out.getReadPointer (0), n / 2, n) / rms (in.data(), n / 2, n));
            const double d = 100.0 * thd (out.getReadPointer (0), n / 2, n, kSr, 100.0);
            std::printf ("defaults, 100 Hz @ -18 dBFS: level %+.2f dB, THD %.1f%%\n", lvl, d);
        }

        // Extreme settings + full-scale noise must stay finite and bounded
        {
            Params p;
            p.gain = p.tubeGain = p.bass = p.mid = p.treble = p.master = 1.0f;
            p.bright = p.ultraHi = p.ultraLo = p.geqOn = true;
            for (auto& g : p.geq) g = 8.0f;
            p.geqLevel = 8.0f;
            juce::Random rng (1234);
            std::vector<float> noise (static_cast<size_t> (kSr * 2.0));
            for (auto& s : noise) s = rng.nextFloat() * 2.0f - 1.0f;
            const auto out = renderChain (noise, p);
            const double pk = peak (out.getReadPointer (0), 0, out.getNumSamples());
            check (allFinite (out), "no NaN/Inf at extreme settings", "full-scale noise, everything maxed");
            check (pk <= 1.05, "output bounded by power section", fmt ("peak %.3f", pk));
        }

        // Silence after signal decays to (near) zero
        {
            auto sig = sine (kSr, 100.0, 0.25f, 1.0);
            sig.resize (static_cast<size_t> (kSr * 3.0), 0.0f);
            const auto out = renderChain (sig, Params {});
            const int n = out.getNumSamples();
            const double tail = peak (out.getReadPointer (0), n - static_cast<int> (kSr * 0.5), n);
            check (tail < 1e-4, "output decays to silence", fmt ("tail peak %.2e", tail));
        }

        // Host block larger than prepare()'s maxBlockSize (some hosts do this)
        {
            float bands[GraphicEQ::kNumBands] = {};
            bands[4] = 8.0f;
            const auto in = sine (kSr, 600.0, 0.05f, 1.0);
            auto run = [&] (int preparedBlock)
            {
                GraphicEQ eq;
                eq.prepare (kSr, preparedBlock);
                eq.setParams (true, bands, 0.0f);
                return render (in, 1024, [&] (auto& b) { eq.process (b); });
            };
            const auto ref = run (1024), small = run (256);
            double maxDiff = 0.0;
            for (int i = 0; i < ref.getNumSamples(); ++i)
                maxDiff = std::max (maxDiff, static_cast<double> (std::abs (ref.getSample (0, i) - small.getSample (0, i))));
            check (maxDiff < 1e-4, "GraphicEQ handles blocks > maxBlockSize",
                   fmt ("max diff %.2e", maxDiff));
        }

        // Full chain: host blocks larger than prepared must match a correctly sized run
        {
            Params p;
            p.geqOn = true;
            p.geq[4] = 8.0f;
            p.gain = 0.8f;
            const auto in = multitone (kSr, { 41, 100, 600, 2000 }, 0.05f, 1.0);
            const auto ref = renderChain (in, p, kSr, 256, 256);
            const auto big = renderChain (in, p, kSr, 256, 1024);
            double maxDiff = 0.0;
            for (int i = 0; i < ref.getNumSamples(); ++i)
                maxDiff = std::max (maxDiff, static_cast<double> (std::abs (ref.getSample (0, i) - big.getSample (0, i))));
            check (maxDiff < 1e-4, "chain handles host blocks > maxBlockSize", fmt ("max diff %.2e", maxDiff));
        }

        // Sample-rate independence
        {
            auto bassAt = [] (double sr)
            {
                return stageGainDb<ToneStack> (41, [] (auto& s) { s.setParams (1, .5f, 3, .5f); }, sr);
            };
            const double a = bassAt (44100), b = bassAt (48000), c = bassAt (96000);
            check (std::abs (a - b) < 0.5 && std::abs (c - b) < 0.5, "bass response same at 44.1/48/96 kHz",
                   fmt ("%.2f / %.2f / %.2f dB", a, b, c));
        }
    }
}

namespace
{
    // Minimal processor so the real parameter layout can be instantiated.
    struct LayoutProbe : juce::AudioProcessor
    {
        juce::AudioProcessorValueTreeState apvts { *this, nullptr, "PARAMS", createParameterLayout() };

        const juce::String getName() const override { return "LayoutProbe"; }
        void prepareToPlay (double, int) override {}
        void releaseResources() override {}
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        double getTailLengthSeconds() const override { return 0.0; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        bool hasEditor() const override { return false; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}
        void getStateInformation (juce::MemoryBlock&) override {}
        void setStateInformation (const void*, int) override {}
    };

    void testParameterLayout()
    {
        section ("Parameter layout");
        juce::ScopedJuceInitialiser_GUI juce;
        LayoutProbe probe;

        // Knob-style parameters are read raw by the DSP as 0-1.
        for (const char* id : { ParameterIDs::gain, ParameterIDs::tubeGain, ParameterIDs::bass,
                                ParameterIDs::midrange, ParameterIDs::treble, ParameterIDs::master,
                                ParameterIDs::dryWet })
        {
            const auto range = probe.apvts.getParameterRange (id);
            check (range.start == 0.0f && range.end == 1.0f, std::string (id) + " range is 0-1",
                   fmt ("%.1f to %.1f", range.start, range.end));
        }

        // Master gain (knob^2) must rise monotonically across the knob's travel.
        auto* master = probe.apvts.getParameter (ParameterIDs::master);
        float prev = -1.0f;
        bool monotonic = true;
        for (int i = 0; i <= 20; ++i)
        {
            const float raw = master->convertFrom0to1 (i / 20.0f);
            const float g = raw * raw;
            monotonic = monotonic && g >= prev;
            prev = g;
        }
        check (monotonic, "master gain rises with the knob", fmt ("gain at full travel %.2f", prev));
    }
}

int main()
{
    testParameterLayout();
    testInputGain();
    testPreFilters();
    testTubePreamp();
    testToneStack();
    testGraphicEQ();
    testMasterVolume();
    testPowerSection();
    testOutputSwitch();
    testChainSensitivity();
    testDefaultsAndSafety();

    std::printf ("\n%d FAIL, %d WARN\n", failures, warnings);
    return failures;
}
