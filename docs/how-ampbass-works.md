# How AmpBass Works

A guide to what the DSP code *actually does today*, stage by stage, with the
numbers that matter for tuning. For the original design intent and the SVT-3 PRO
circuit background, see [research/AmpBass-signal-chain.md](research/AmpBass-signal-chain.md);
for why things are the way they are, see [decision-log.md](decision-log.md).
Where the code and the research spec disagree, this file describes the code.

---

## 1. The big picture

```
Host audio ─┬─► InputGain ► PreFilters ► TubePreamp ► ToneStack ► GraphicEQ ► MasterVolume ► PowerSection ─┬─► out
            └──────────────────────────────── dry copy ─────────────────────────────────────────────────┘
                                                                                     (equal-power Dry/Wet)
```

- Each stage is a header-only class in `Source/DSP/` with `prepare()`, `setParams()` and `process()`.
- `PluginProcessor::processBlock` reads all parameters once per host block
  (`updateDSPParameters()`), then runs the chain in sub-blocks no larger than the
  size announced in `prepareToPlay` (some hosts send bigger blocks).
- Parameter changes are never applied instantly: every stage ramps toward the new
  value with a `juce::SmoothedValue` (10–50 ms) to avoid clicks. Filter
  coefficients are recomputed every 32 samples, not every sample (decision D5).
- Everything runs at the host sample rate. **There is no oversampling**, which
  matters for the distortion stages (see §5).

Only two stages are non-linear: **TubePreamp** (where the character comes from)
and **PowerSection** (a safety clipper + gentle compressor). Everything else is
gain or EQ, so it's linear and order-independent within its own group.

---

## 2. Stage by stage

### Stage 1 — InputGain · `Source/DSP/InputGain.h`
Emulates the amp's input jacks.

| Control | Effect |
|---|---|
| Input Pad | −15 dB (×0.178) |
| Input Select LO | −6 dB (×0.501); HI = unity |

Why it matters: it sets **how hard the preamp is hit**. Pad/LO don't make the
plugin quieter so much as *cleaner*, because less level reaches the waveshaper.

### Stage 2 — PreFilters · `Source/DSP/PreFilters.h`
The three voicing switches, all RBJ biquads, always in the path (0 dB when off):

| Switch | Filter(s) |
|---|---|
| Bright | Peak +5.3 dB @ 3 kHz (Q 0.27) + high shelf +1.2 dB @ 10.9 kHz |
| Ultra Hi | Peak +5.3 dB @ 5.6 kHz (Q 0.37) + high shelf +2.4 dB @ 9 kHz |
| Ultra Lo | **Peak −12.4 dB @ 530 Hz (Q 0.71)** |

All three are fitted to the reference plugin (within 0.3 dB). Its Ultra Lo has no
+2 dB at 40 Hz, although the manual says so. Ultra Lo is the classic "scooped"
SVT voicing. Because these sit *before* the preamp, they also change what
distorts: Bright makes the highs clip earlier; Ultra Lo makes the mids clip later.

### Stage 3 — TubePreamp · `Source/DSP/TubePreamp.h`
The heart of the sound. Per sample:

1. **25 Hz high-pass** (one-pole) — removes subsonics so they don't eat headroom.
2. **Bias**: `x + 0.1·(1 − headroom)` — shifts the signal off-centre so the
   shaper clips the two halves differently → **even harmonics** (tube warmth).
3. **Drive**: `x · drive / headroom`
4. **Asymmetric soft clip**: `tanh` (Padé approximation); the negative half is
   squashed by up to 25 % more than the positive half.
5. **Output level**: `× level · headroom / drive`, with `level = 19 × Gain`, so the
   small-signal gain is `19 × Gain` (linear taper like the reference: +6 dB per
   doubling, +20 dB from Gain 1 to 10, silent at 0).
6. **10 Hz DC blocker** — removes the offset the bias introduced.

Parameter mapping:

| Knob (0–1) | Internal value |
|---|---|
| Gain | drive = 1 → 10 |
| Tube Gain | headroom = 0.3 → 1.0 (lower = "starved plate", decision D2) |

**What that means in practice** (worked out from the formulas above):

- **Small-signal gain is `19 × Gain`**: Gain is a volume control for quiet notes
  (matched to the reference within 0.1 dB), while the drive sets how early and
  how hard the shaper distorts.
- **Clipping starts** when the input reaches about `headroom / drive`.
  At defaults (Gain 0.5, Tube Gain 0.5) that's ≈ 0.12, i.e. **≈ −18.5 dBFS**.
  A typical bass DI peaks around −12…−6 dBFS, so **the default setting is already
  driving the shaper** (this is the ~10 % THD the test suite reports).
- **Output ceiling** = `level · headroom / drive` = `19·Gain·headroom / (1 + 9·Gain)`,
  which rises with Gain toward ≈ 2.1 × headroom.
- **Tube Gain down** = less headroom *and* more bias → earlier clipping, more
  even harmonics, lower ceiling.

### Stage 4 — ToneStack · `Source/DSP/ToneStack.h`
A fixed **voicing** first: −3.76 dB, then peaks at 57.5 Hz (Q 0.44, +8.36 dB) and
5.9 kHz (Q 0.69, +6.08 dB), giving +4.6 dB at 57 Hz, −3.2 dB at 1 kHz and +2.3 dB at
5.9 kHz. It's the reference's response with every tone knob at noon. Then three
RBJ biquads in series for the knobs:

| Control | Filter | Range |
|---|---|---|
| Bass | Low shelf, shape per knob position (77–152 Hz, Q 0.42–0.69) | −16.3 … +14.1 dB (fitted to the reference) |
| Midrange | Peak per selector × knob position (Q 0.07–0.9) | −18 … −20 / +11 … +13 dB (fitted to the reference) |
| Mid Frequency | 249 / 543 / 895 / 1773 / 2884 Hz (measured; manual says 220 / 450 / 800 / 1.6k / 3k) | 5 steps, glides on a log scale |
| Treble | High shelf per knob position (282–2929 Hz) + peak at 19 kHz, Q 1.2 | −30 … +16.5 dB shelf (fitted to the reference) |

All knobs centred = the knobs are flat (only the voicing remains). The SVT-3 PRO
manual describes its Bass, Mid and Treble as active controls that are flat at
centre, so independent filters are the right model.

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
1. **Symmetric soft clip** above 0.8 (linear below; tanh-shaped between 0.8 and 1.0).
2. **2:1 compressor** with an envelope follower (1 ms attack, 50 ms release),
   threshold 0.8.
3. **Output low-pass**: fixed 2nd-order low-pass at 14.6 kHz, Q 0.382 (−4 dB at
   10 kHz, −19 dB at 20 kHz at 48 kHz), fitted to the reference plugin with its
   cabinet bypassed, together with the tone-stack voicing. Its roll-off doesn't
   change with level or Master.

Combined with the numbers above: the preamp ceiling (≈ 0.39 at defaults) times
the Master gain (0.25) is ≈ 0.1, far below 0.8. **At default settings the power
section does nothing.** It only engages with low Gain + high Tube Gain (ceiling
near 1.0) and Master near the top. If the reference plugin has audible
"power amp" squash, this stage is under-driven by comparison.

### Output switches · `Source/DSP/OutputSwitch.h`
Runs last, after Dry/Wet. Both switches fade over 10 ms (no clicks).

| Control | Effect |
|---|---|
| Bypass (faceplate POWER / toolbar icon; also the host's bypass) | Output = the untouched input |
| Mute | Output = silence (wins over Bypass) |

**PEAK/MUTE LED** (`PluginProcessor::processBlock`, `PluginEditor::timerCallback`):
lit when Mute is on, or for 300 ms after any overload — raw input ≥ 0.98,
preamp drive ≥ 4 (hard saturation), or signal ≥ 1.0 after the tone stack and GEQ
(too much boost). Same causes the reference manual lists.

### Dry/Wet · `Source/DSP/Primitives/DryWetMixer.h`
Equal-power crossfade (`cos/sin`) between the untouched input and the chain
output. Default 100 % wet.

---

## 3. Level map at default settings

> These numbers predate the sound-matching work (docs/sound-matching-plan.md) and are refreshed after its
> step 6 (distortion), which changes the preamp and power section.

| Point in chain | Quiet note (−30 dBFS in) | Loud note (−6 dBFS in) |
|---|---|---|
| After preamp | ≈ −19.5 dBFS (+10.5 dB, clean) | ≈ −8 dBFS (clipped at ceiling) |
| After Master (×0.25) | ≈ −31.5 dBFS | ≈ −20 dBFS |
| After PowerSection | unchanged | unchanged |

So at defaults the plugin is roughly **unity for quiet playing and turns loud
playing down by ~14 dB** — strong compression from the preamp. Approximate:
EQ is flat and a real signal is not a sine, but the shape holds.

---

## 4. Where to tune

The constants below shape the sound and are *not* exposed as parameters.
Changing them doesn't break saved sessions (parameter IDs are untouched).

| Character | Constant | Where |
|---|---|---|
| How fast Gain ramps up distortion | `1 + gainKnob·9` (drive range) | `TubePreamp.h` `setParams` |
| Starved-plate range | `0.3 + tubeGainKnob·0.7` | `TubePreamp.h` `setParams` |
| Even-harmonic amount | bias `0.1` | `TubePreamp.h` process, step 2 |
| Asymmetry of clipping | `0.25` in the negative branch | `TubePreamp.h` process, step 4 |
| Loudness vs Gain | `0.3` in `normGain` | `TubePreamp.h` process, step 5 |
| Low-end tightness into the preamp | HPF 25 Hz | `TubePreamp.h` `prepare` |
| Tone-stack corners / widths | 100 Hz, 2.5 kHz, Q 0.5 / mid Q 1.5 | `ToneStack.h` `prepare` |
| Voicing switches | shelf freqs/gains, Ultra Lo +2 dB @ 40 / −10 dB @ 500 | `PreFilters.h` |
| GEQ band width | Q 2.0 | `GraphicEQ.h` `prepare` |
| Master taper | 0.5 × knob (linear, measured on the reference) | `MasterVolume.h` |
| Power amp onset / squash | threshold 0.8, 2:1, 1 ms / 50 ms | `PowerSection.h` |

Known gaps worth considering while tuning:
- **No oversampling.** At drive ×15 the shaper creates harmonics above Nyquist
  that fold back as inharmonic aliasing, most audible on high notes with high Gain.
  2×–4× oversampling around TubePreamp (`juce::dsp::Oversampling`) is the
  standard fix.
- **No cabinet** (decision D9). A DI-style output will sound much brighter and
  fizzier than an amp + cab; check what the reference plugin does.
- **Independent tone controls** (see Stage 4).
- **Static shaper**: no sag, no frequency-dependent clipping (real tubes clip
  bass and treble differently because of coupling caps).
- Mid Frequency is smoothed linearly in Hz; CLAUDE.md asks for logarithmic.

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

Suggested learning exercises, one change at a time, same DI file:
1. Gain 0 → 0.25 → 0.5 → 1 with Master compensated: hear the switch from
   "volume" to "distortion" at ~0.26.
2. Tube Gain 1 → 0 at fixed Gain: listen for the thicker, more even-harmonic tone.
3. Ultra Lo on/off: the SVT scoop.
4. Gain 0, Tube Gain 1, Master 1: the only way to hear the PowerSection.
