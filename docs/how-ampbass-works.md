# How AmpBass Works

A guide to what the DSP code *actually does today*, stage by stage, with the
numbers that matter for tuning. For the original design intent and the SVT-3 PRO
circuit background, see [research/AmpBass-signal-chain.md](research/AmpBass-signal-chain.md);
for why things are the way they are, see [decision-log.md](decision-log.md).
Where the code and the research spec disagree, this file describes the code.

---

## 1. The big picture

```
Host audio ─┬─► InputGain ► PreFilters ► TubePreamp ► ToneStack ► GraphicEQ ► MasterVolume ► PowerSection ► Cabinet ─┬─► out
            └──────────────────────────────────────── dry copy ─────────────────────────────────────────────────────┘
                                                                                     (equal-power Dry/Wet)
```

- Each stage is a header-only class in `Source/DSP/` with `prepare()`, `setParams()` and `process()`.
- `PluginProcessor::processBlock` reads all parameters once per host block
  (`updateDSPParameters()`), then runs the chain in sub-blocks no larger than the
  size announced in `prepareToPlay` (some hosts send bigger blocks).
- Parameter changes are never applied instantly: every stage ramps toward the new
  value with a `juce::SmoothedValue` (10–50 ms; Tube Gain 200 ms, see Stage 7) to
  avoid clicks. Filter coefficients are recomputed every 32 samples, not every
  sample (decision D5). The output doesn't depend on the host's block size (bit-exact
  from 32 to 4096 samples).
- Everything runs at the host sample rate. **There is no oversampling**, and none is
  needed: AmpBass aliases no more than the reference (−126 vs −125 dB, plan 6.6).
  Filters run in double precision, because float low-frequency biquads fell into
  limit cycles that looked like aliasing.
- Left and right are processed independently (dual mono), like the reference.

Only one stage is non-linear: **PowerSection**, which holds the power amp and the
Tube Gain stage. That is where the reference has its distortion. Everything before it
is gain or EQ, so it is linear: the preamp, tone stack and EQ only shape the level and
spectrum that reach the power stage.

Most constants are fitted to the reference with `Tests/compare/` (its cabinet bypassed); the
Cabinet stage is fitted to its default Rec Chain. `docs/sound-matching-plan.md` records how each
was found.

---

## 2. Stage by stage

### Stage 1 — InputGain · `Source/DSP/InputGain.h`
Emulates the amp's input jacks.

| Control | Effect |
|---|---|
| Input Pad | −15 dB (×0.178) |
| Input Select LO | −6 dB (×0.501); HI = unity |

Why it matters: everything up to the power stage is linear, so Pad/LO lower the level
the power stage sees. The plugin gets quieter *and* cleaner.

### Stage 2 — PreFilters · `Source/DSP/PreFilters.h`
The three voicing switches, all RBJ biquads, always in the path (0 dB when off):

| Switch | Filter(s) |
|---|---|
| Bright | Peak +5.3 dB @ 3 kHz (Q 0.27) + high shelf +1.2 dB @ 10.9 kHz |
| Ultra Hi | Peak +5.3 dB @ 5.6 kHz (Q 0.37) + high shelf +2.4 dB @ 9 kHz |
| Ultra Lo | **Peak −12.4 dB @ 530 Hz (Q 0.71)** |

All three are fitted to the reference plugin (within 0.3 dB). Its Ultra Lo has no
+2 dB at 40 Hz, although the manual says so. Ultra Lo is the classic "scooped"
SVT voicing. Because these sit *before* the power stage, they also change what
distorts: Bright makes the highs distort earlier; Ultra Lo makes the mids distort later.

### Stage 3 — TubePreamp · `Source/DSP/TubePreamp.h`
**Linear.** Measured on the reference, the preamp is clean at every Gain setting (with
Tube Gain at 10); its distortion comes from the power stage. The class keeps its old
name. Per sample:

1. **25 Hz high-pass** (one-pole): removes subsonics; the low E (41 Hz) loses about 1.3 dB.
2. **Level `19 × Gain`** (smoothed over 10 ms): a linear taper like the reference's,
   +6 dB per doubling, +20 dB from Gain 1 to 10, silent at 0. The default Gain 3.4 is
   ×6.5 (+16.2 dB).

So Gain is a level control. It changes the distortion only by changing how loud the
signal reaches the power stage, and **Gain and Master trade 1:1** (true of the reference
too). The preamp's output peak feeds the PEAK LED: at 1.0 (full scale) Gain is too high
for the input.

### Stage 4 — ToneStack · `Source/DSP/ToneStack.h`
A fixed **voicing** first: −3.76 dB, then peaks at 57.5 Hz (Q 0.44, +8.36 dB) and
5.9 kHz (Q 0.69, +6.08 dB), giving +4.6 dB at 57 Hz, −3.2 dB at 1 kHz and +2.3 dB at
5.9 kHz. It's the reference's response with every tone knob at noon. Then RBJ
biquads in series for the knobs, and one for how Bass and Treble interact:

| Control | Filter | Range |
|---|---|---|
| Bass | Low shelf, shape per knob position, every 0.5 (77–152 Hz, Q 0.42–0.69) | −16.2 … +14.1 dB (fitted to the reference) |
| Midrange | Peak per selector × knob position, every 0.5 (Q 0.11–1.0) | −18 … −20 / +11 … +13 dB (fitted to the reference) |
| Mid Frequency | 249 / 543 / 895 / 1773 / 2884 Hz (measured; manual says 220 / 450 / 800 / 1.6k / 3k) | 5 steps, glides on a log scale |
| Treble | High shelf per knob position, every 0.25 (273–2913 Hz) + peak at 19 kHz, Q 1.2 | −30 … +16.5 dB shelf (fitted to the reference) |
| Bass × Treble | Peak per [bass][treble] knob position, every 1.25 (200–550 Hz) | −3.8 … +1.6 dB |

All knobs centred = the knobs are flat (only the voicing remains). The amp's tone
stack is **passive** (schematic 07S534): Bass and Treble are 1 MΩ pots in one
network, so together they reshape the low mids. Measured on a 5 × 5 × 5 × 5 grid
(Bass × Treble × Mid × Mid Freq), the reference's combined response differs from the
sum of its single knobs only through Bass × Treble, and only around 200–550 Hz:
Bass 10 + Treble 10 scoops 3.8 dB deeper at 464 Hz, Bass 0 + Treble 10 leaves 1.6 dB
more. Mid (an 800 mH inductor resonance) doesn't interact. The interaction filter adds
exactly that, so the stack is independent filters plus one correction.

The reference's knob tapers are curved (Treble changes fastest near 0 and 10), so
every knob is measured in 0.25 steps and tabulated densely; frequencies are
interpolated on a log scale. Any setting matches the reference within about 1.4 dB
(plan step 11).

### Stage 5 — GraphicEQ · `Source/DSP/GraphicEQ.h`
Nine peaking biquads fitted to the reference plugin, centred at 33, 79, 141, 306,
537, 860 Hz, 2.24, 5.05 and 7.69 kHz (the faceplate labels say 33 … 9k), plus a
−10 … +8 dB Level. It behaves like an analog proportional-Q EQ: the slider is
strongly non-linear (+6 is only +1.5 dB, +12 is about +9.8 dB) and a band gets
narrower as it moves away from 0. Switching the EQ on adds a broad +0.46 dB hump
around 500 Hz, as in the reference.
When **EQ is off** the stage crossfades (~64 samples) to the unprocessed signal,
so the sliders and Level have no effect — the UI dims them to show this.

### Stage 6 — MasterVolume · `Source/DSP/MasterVolume.h`
Gain = 0.5 × knob, a linear taper like the reference (+6 dB per doubling, silent at 0).
Default 0.5 → ×0.25 = **−12 dB**; full → ×0.5 = −6 dB.
0 = silence.

### Stage 7 — PowerSection · `Source/DSP/PowerSection.h`
The power amp and the Tube Gain stage, fitted to the reference. Per sample:

1. **Tube Gain level**: +3.1 / +3.1 / +2.9 / +1.5 / 0 dB at Tube Gain 0 / 2.5 / 5 / 7.5 / 10.
   All Tube Gain constants are tables over these five positions, interpolated linearly.
2. **Kink**: a rectified copy of the signal (low-passed at 703 Hz before the rectifier,
   through a low shelf with a 20 Hz pole after it, then DC-blocked at 10 Hz) is added to
   the signal. The two half-waves get slightly different gain, which gives a 2nd
   harmonic that doesn't depend on level and is strongest in the bass: at Tube Gain 0,
   −26 dBc at 41 Hz, −38 at 100 Hz, −48 at 400 Hz. About −50 dB at Tube Gain 5–7.5,
   off at 10.
3. **Emphasis peak** +7.9 dB at 872 Hz (Q 0.33), then the **static curve**
   `u / (1 + |u / 1.6435|^1.778)^(1/1.778)`, then the inverse peak. The curve is a soft
   limiter with a ceiling of 1.64; its 3rd harmonic grows about 1.7 dB per dB. The peak
   makes the low mids distort first. In the hardware (schematic 07S426) this is a tube
   driver (12AX7 + 12AU7, whose plate voltage is Tube Gain) into a MOSFET output stage
   with global negative feedback.
   Below about −30 dBFS at this point it is linear and the two peaks cancel.
4. **Blocking bias**: the curve's operating point moves with the signal level,
   `bias + blocking · max(env − onset, 0)`, where `env` is |u| through a 5 Hz low-pass,
   and the curve's input is scaled by a per-position drive. At Tube Gain 0–2.5 this
   gives the extra compression and the 2nd harmonic that rise at high level (~1.7 dB
   per dB), while the 3rd harmonic stays put. The output is normalised so the
   small-signal level stays at step 1's value. Tube Gain glides over 200 ms, because the
   bias moves the operating point and faster glides thump.
5. **Output low-pass**: fixed 2nd-order low-pass at 14.6 kHz, Q 0.382 (−4 dB at
   10 kHz, −19 dB at 20 kHz at 48 kHz), fitted to the reference plugin with its
   cabinet bypassed, together with the tone-stack voicing. Its roll-off doesn't
   change with level or Master. At 88.2 kHz and up the reference is darker (about −5 dB at
   14–16 kHz), so those rates use their own fit (7.7 kHz / Q 0.59 at 88.2–96 kHz,
   7.1 kHz / Q 0.64 at 176.4–192 kHz).
6. **10 Hz DC blocker**, the only one in the chain (after the non-linearity, where DC
   can appear).

Tube Gain 5 and above adds almost nothing besides level; the character is at 0–2.5.
§3 has measured levels and harmonics.

### Stage 8 — Cabinet · `Source/DSP/Cabinet.h`
A 4x10 speaker cabinet and its horn, fitted to the reference's default Rec Chain
("20 4x10 DYN600+RIBBON") with Horn on. Both are **on by default**, like the reference; sessions saved
before the cabinet existed load with it off (`stateVersion` 3).

| Control | Effect (at 48 kHz) |
|---|---|
| Cabinet | −6 dB at 30 Hz, +7 dB at 120 Hz, a broad dip around 400–800 Hz (−4 dB), +3 … +6 dB at 3–5 kHz, −4 … −6 dB at 8–12 kHz, −26 dB at 16 kHz, with narrow dips from the two-microphone mix (−8 dB at 2.4 kHz) |
| Horn (only with the cabinet) | about −1.5 dB below 1 kHz, +4 … +7 dB at 3–9 kHz, a narrow −14 dB dip at 4 kHz |

The reference's cabinet is a fixed filter: its effect is the same at every level, drive and
setting (within 0.3 dB). So it is a cascade of biquads (24 for the cabinet, 16 for the horn)
fitted to its magnitude response at 1/24-octave resolution. After 1/6-octave averaging it
is within 0.4 dB (cabinet) and 1.1 dB (horn) up to 12 kHz. Like the amp's top end, the
reference's cabinet changes with the sample rate above about 5 kHz (at 96 kHz it cuts much
less: −11 dB at 16 kHz), so 44.1, 48 and 88.2 kHz-and-up each have their own fit. Switching
either control crossfades linearly over 50 ms; off, the stage passes the signal bit for bit.
The fit matches the spectrum, not the phase: the reference's cabinet is a measured response
with a ~1.5 ms microphone delay, so DI nulls with the cabinet on are only about −4 dB.

### Output switches · `Source/DSP/OutputSwitch.h`
Runs last, after Dry/Wet. Both switches fade linearly over 50 ms, like the reference
(10–90 % in 40 ms; the reference takes 43 ms). Fully bypassed, the output is the input
bit for bit; muted, it is digital silence. No latency.

| Control | Effect |
|---|---|
| Bypass (faceplate POWER / toolbar icon; also the host's bypass) | Output = the untouched input |
| Mute | Output = silence (wins over Bypass) |

**PEAK/MUTE LED** (`PluginProcessor::processBlock`, `PluginEditor::timerCallback`):
lit when Mute is on, or for 300 ms after any overload: raw input ≥ 0.98,
preamp output ≥ 1.0 (Gain too high for the input), or signal ≥ 1.0 after the tone
stack and GEQ (too much boost). Same causes the reference manual lists.

### Dry/Wet · `Source/DSP/Primitives/DryWetMixer.h`
Equal-power crossfade (`cos/sin`) between the untouched input and the chain
output. Default 100 % wet.

---

## 3. Level map at default settings

Defaults: Gain 3.4, Tube Gain 5, Master 5, tone knobs centred, GEQ off. Measured with
`AmpBassHostRender` (2026-10-19), 1 kHz sine:

| Point in chain | Quiet note (−30 dBFS in) | Loud note (−6 dBFS in) |
|---|---|---|
| After preamp (×6.5, +16.2 dB) | −13.8 dBFS | +10.2 dBFS (PEAK lights) |
| After tone-stack voicing (−3.2 dB at 1 kHz) | −17.0 dBFS | +7.0 dBFS |
| After Master (×0.25, −12 dB) | −29.0 dBFS | −5.0 dBFS |
| After PowerSection (+2.9 dB at Tube Gain 5) | −26.3 dBFS | −5.5 dBFS: 3.4 dB of compression, H3 −16 dBc |

Values above full scale are fine inside the chain (floating point); only the power
stage limits. Output level and harmonics against input level, 100 Hz sine, Tube Gain
0 / 5 / 10:

| Input | Tube Gain 0 | Tube Gain 5 (default) | Tube Gain 10 |
|---|---|---|---|
| −30 dBFS | −20.5 dBFS · H2 −35, H3 −60 dBc | −20.5 · H2 −77, H3 −64 | −23.4 · H2 −106, H3 −69 |
| −18 dBFS | −11.1 · H2 −21, H3 −47 | −8.8 · H2 −70, H3 −43 | −11.6 · H2 −108, H3 −47 |
| −12 dBFS | −9.3 · H2 −14, H3 −32 | −3.5 · H2 −60, H3 −33 | −6.0 · H2 −86, H3 −37 |
| −6 dBFS | −10.3 · H2 −10, H3 −21 | +0.7 · H2 −52, H3 −25 | −1.2 · H2 −75, H3 −28 |

So at defaults the plugin is **about +9.5 dB for quiet notes and squashes loud ones by
~3 dB**, mostly 3rd harmonic. Tube Gain 0 adds a strong 2nd harmonic and clamps loud
notes hard: a −6 dBFS note comes out quieter than a −12 dBFS one. Tube Gain 10 is
3 dB quieter and the cleanest. The output can pass 0 dBFS; the reference peaks up to
+5.3 dBFS.

---

## 4. Where to tune

The constants below shape the sound and are *not* exposed as parameters.
Changing them doesn't break saved sessions (parameter IDs are untouched). Almost all of
them are fitted to the reference: after changing one, re-run the comparison
(`Tests/compare/`, regression gate in its README) as well as `AmpBassDSPTests`.

| Character | Constant | Where |
|---|---|---|
| Gain law | `kLevelAtFullGain` (19) | `TubePreamp.h` |
| Low-end tightness into the power stage | HPF 25 Hz | `TubePreamp.h` `prepare` |
| Fixed voicing | `kVoicing*` (57.5 Hz +8.36 dB, 5.9 kHz +6.08 dB, −3.76 dB) | `ToneStack.h` |
| Bass / Mid / Treble shapes | `kBassCurve`, `kMidHz` / `kMidGainDb` / `kMidQ`, `kTrebleCurve`, `kTrebleTop*` (per knob position) | `ToneStack.h` |
| Voicing switches | `kBright*`, `kUltraHi*`, `kUltraLo*` | `PreFilters.h` |
| GEQ bands and slider law | `kBandHz`, `kBand12*`, `kLawSlider` / `kBoost*` / `kCut*`, `kOnHump*` | `GraphicEQ.h` |
| Master taper | `kGainAtFull` (0.5 × knob) | `MasterVolume.h` |
| Power-stage softness and ceiling | `kHardness` (1.778), `kCeiling` (1.6435) | `PowerSection.h` |
| Which frequencies distort first | `kEmphasis*` (872 Hz, Q 0.33, +7.9 dB) | `PowerSection.h` |
| Tube Gain | `kLevelDb`, `kKinkDb`, `kDriveDb`, `kBias`, `kBlocking`, `kBlockingOnset` (per position); `kKink*`, `kEnvelopeHz` | `PowerSection.h` |
| Top end | `kOutputLowPass*` (per sample-rate family) | `PowerSection.h` |
| Cabinet and horn | `k441` / `k48` / `k96` (generated by `build-tests/compare/cab/gen_cabinet.py`) | `Cabinet.h` |
| PEAK thresholds | `kPeakInput`, `kPeakDrive`, `kPeakPreMaster` | `PluginProcessor.h` |

Known gaps (accepted at Milestone B, see the plan):
- **Tube Gain 0 "choke"**: after a loud note the reference drops by ~50 dB within
  10–20 ms and recovers in ~60 ms. The 5 Hz envelope can't do that, so DI nulls at
  Tube Gain 0 are −16 … −19 dB (−26 dB at Tube Gain 5 and 10).
- **Very loud input**: 1–3 dB too little compression at the extremes.
- **No Power Soak** (plan 9.2). **One cabinet**: the reference's default Rec Chain 20 (+ Horn);
  its other Rec Chains aren't modelled. Cabinet phase isn't modelled (see Stage 8).
  The amp-only comparisons run with both plugins' cabinets off.
- At 88.2 kHz and up, about 1 dB of top-end difference that depends on the tone settings.

---

## 5. How to experiment

Build the diagnostics once (from the repo root):

```bash
cmake -S . -B build-tests -G Ninja -DAMPBASS_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release \
      -DFETCHCONTENT_SOURCE_DIR_JUCE=$PWD/build/_deps/juce-src
cmake --build build-tests
```

- **`AmpBassDSPTests`** — runs each stage in isolation and prints gain, THD and
  frequency-response checks (PASS/WARN/FAIL). Re-run after any constant change.
- **`AmpBassHostRender`** — loads the *installed* VST3 like a DAW would:

```bash
H=build-tests/Tests/AmpBassHostRender_artefacts/Release/AmpBassHostRender
$H --list                                   # parameter names and defaults
$H --sweep                                  # per-parameter response summary
$H --in di_bass.wav --out wet.wav \
   --set "Gain=0.8" --set "Tube Gain=0.2"   # render a file with settings
```

`--set` takes **normalised** values (0–1). For stepped parameters,
value = step index / (steps − 1); e.g. Mid Frequency 1.6 kHz = 3/4 = 0.75.
`--set-at 2.5 "Bypass=1"` changes a parameter 2.5 s into the render, like automation.
A parameter can also be named by its `--list` index, e.g. `"#0=1"` (the reference has
two parameters called "Bypass").

- **`Tests/compare/`** renders the same signals through AmpBass and the reference and
  measures both; `behaviour.py` there checks sample rates, block sizes, stereo and
  the Bypass/Mute fades. See its [README](../Tests/compare/README.md).

Suggested learning exercises, one change at a time, same DI file:
1. Gain 0.2 → 0.4 with Master 0.8 → 0.4: the same sound, because Gain and Master
   trade 1:1. Then raise both: the power stage squashes and grows 3rd harmonic.
2. Tube Gain 1 → 0 at fixed Gain: listen for the thicker, 2nd-harmonic bass and how
   loud notes clamp.
3. Ultra Lo on/off: the SVT scoop.
4. Gain 1, Master 1: the power stage at its hardest.
