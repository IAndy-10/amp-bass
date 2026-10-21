# Sound-matching plan: AmpBass → Ampeg SVT-3 PRO (cabinet bypassed)

Guide for future sessions. Work **one step at a time**: change one thing, measure, and keep it only if the
targets improve and nothing else gets worse. Tick the step and fill in the log at the bottom.

Goal for this scenario: with the reference's cabinet bypassed, AmpBass matches it in **parameters** (same
controls, ranges, scales, defaults), **sound** (frequency response, level, distortion, dynamics, noise) and
**behaviour** (bypass, mute, PEAK, sample rate, block size, stereo, latency).

## Context (as of 2026-10-19)

- Reference: Plugin Alliance VST3 1.6.0, `/Library/Audio/Plug-Ins/VST3/Ampeg SVT3Pro.vst3` (2 in / 2 out,
  3 samples latency). The user's screenshots are of the UAD build, which is assumed to be the same engine.
- The reference's text→value parsing is broken (`getValueForText` returns the typed number unchanged), so
  `Tests/compare/plugins.json` sets it with normalised values (`knob10`, `norm`, `points`). Its fixed settings are
  Rec Chain = last entry (`27 EMPTY PLAY REAL CAB` = Cabinet Bypass), Horn off, gate bypassed, Tight/Smooth off,
  Input 0 dB, **Power Soak 0 dB**.
- Reference defaults (fresh instance): Gain 3.4, Bass/Mid/Treble/Master/Tube Gain 5, Mid Freq 1, EQ off,
  Input 0 dB, Power Soak −4 dB, Rec Chain `20 4x10 DYN600+RIBBON`, Horn on, gate bypass On / threshold −88 dB /
  range 50, Tight Post 50 Hz, Smooth Post 21 (kHz).
- `screenshot_preset` case in `Tests/compare/cases.py`: settings read off the 4 PNGs in `building-process/references/`,
  which all show the same settings (±0.5 knob, ±1.5 dB slider): Gain 3.4, Bass 7, Mid 6.75, Freq 3, Treble 7,
  Master 6, Tube Gain 3.5, BRT on, EQ on with 80 +5, 150 +4, 2k +1.5, 5k +3.5 dB. The FX Rack is not visible.
  If it was left at its defaults, the user heard Tight Post 50 Hz and Power Soak −4 dB too.
- The 4 real presets (R&B, Rock, Synth, Upright) are still unknown. Get their exact values (ideally as
  `.vstpreset` files from the installed reference) before step 9.

### Baseline: `screenshot_preset` (2026-10-19). **Partly invalid, see step 0.1**

| Metric | Value | Target |
|---|---:|---:|
| Level (ref − AmpBass) | +1.3 dB | ±0.5 dB |
| FR shape worst, 40 Hz–8 kHz | 4.9 dB | ≤ 1 dB (20 Hz–20 kHz after step 1) |
| THD 100 Hz @ −16 dBFS (AmpBass / ref) | 8.2 / 6.5 % | see THD rule below |
| DI null | −15.3 dB | ≤ −20 dB (stretch −25) |
| Noise floor (AmpBass / ref) | −104 / −240 dBFS | ≤ −140 dBFS |

Observed differences, before the step-0.1 fix:
1. Above about 7 kHz the reference rolls off (−25 dB at 20 kHz). AmpBass is flat there.
2. The reference has a broad bump of about +18 dB at 50–60 Hz. AmpBass peaks narrower, at 80 Hz, and is about 7 dB short at 50 Hz.
3. From 100 Hz to 3 kHz AmpBass is 2–5 dB low, with dips at about 300 Hz and 1.2 kHz that the reference doesn't have.
4. At low and mid input levels the preamp distorts too much (12 % vs 5 % THD at −20 dBFS, 100 Hz). At 400 Hz it is too dirty at every level.
5. The loud ceiling is about 4.5 dB too low (ramp tops out at −6.5 vs −2 dBFS), so AmpBass over-compresses. *(Probably the Master bug.)*
6. AmpBass has a "noise floor" on silent input. The reference is digitally silent. *(It's DC, not noise, see step 7.)*

Earlier generic cases: with all knobs at 5 and the preamp clean, AmpBass is flat where the reference scoops about 8 dB at 1 kHz, and AmpBass is about 12 dB louder.
Master 2.5: AmpBass 39 % THD vs reference 0.5 %. Gain 0: the reference outputs silence. *(Level and Master numbers are invalid, see step 0.1.)*

### Findings from step 0 (2026-10-19)

- **0.1 fixed:** Master/Dry-Wet ranges (`2eb287f`). Clean cases now match in level (−0.4 dB); drive cases are 8–23 dB
  quieter in AmpBass because the reference gets louder with Gain (step 4.2).
- **0.3:** Horn has no effect with the cabinet bypassed (exact null). `NgByPass` On = gate **active**; the baseline is now Off.
- **0.4:** Power Soak is a pure output gain in this build (level-matched null −149 dB). The 0 dB baseline is fine and step 9.2 is an output trim.
- **0.5:** `CLEAN` is clean on the reference (H2/H3 ≤ −55 dBc up to −12 dBFS; FR shape stable to 0.04 dB).
- **Harmonics (clean setting, 100 Hz):**
  - **Reference:** no distortion until a knee, then mostly H3. H2 is a constant −102 dBc, inaudible and not worth modelling.
  - **AmpBass:** H2 and H3 rise from the lowest level (−59 / −68 dBc at −40 dBFS). The cause is the `x·|x|` term in the
    asymmetric negative half of the `TubePreamp` shaper, not the bias (which is 0 at Tube Gain 10).
- **Master test:** the reference's H3 knee sits at a fixed **output** level of about −30 dBFS (fundamental peak) for
  Master 2.5–10, so it's in the **power stage** and the preamp is clean at these Gain settings. Above the knee H3 rises
  about 1.5–2 dB per dB. AmpBass's H3 doesn't change with Master at all (only its preamp distorts).
- **Topology (0.8):** in the reference, Bass, Treble and the GEQ all sit **before** the main distortion. A boost shrinks
  at high input (bass 41 Hz: +25 dB at −40 dBFS → +1.7 dB at 0 dBFS; GEQ 80 Hz: +14.5 → +1.3; treble 400 Hz: +4.8 → +0.2)
  and raises H3. In AmpBass the change is constant (+20.4 / +13.1 / +0.9 dB), because its distortion is in the preamp,
  before the tone stack. AmpBass's order (preamp → tone → GEQ → Master → power) already puts tone and GEQ before the
  power stage, so **no reorder is needed** once the main distortion moves to `PowerSection` (6.4). Re-run `topo_*` then.
  Bass swing at 41 Hz: reference 25 dB, AmpBass 20.4 dB.
- **Aliasing (0.8):** AmpBass −92 dB vs reference −125 dB (non-harmonic energy of a 3 kHz −6 dBFS tone). AmpBass aliases
  more, but it's probably inaudible. Re-check after 6.4.
- **Channels (0.8):** both plugins keep L/R independent. AmpBass's −96 dB on a silent right channel is the DC residual (step 7).
- **Master taper:** the reference rises about 12 dB from 2.5 to 10 (+6, +3.5, +3.5 per step). AmpBass rises 24 dB (knob²).

## Decisions (user, 2026-10-19). These bind every session

1. **Distortion lives where the reference has it:** the main distortion is in the power stage, after Master; the preamp
   is clean at low and moderate Gain. The preamp's own break-up at high Gain (reference 35–44 % THD at Gain 7.5–10) is
   measured and placed separately (step 6).
2. **Gain 0 = silence**, as in the reference (step 4.2).
3. **Parameters behave like a professional plugin** (step 5): keep every ID and the parameter order; match the
   reference's scales (same 0–1 value = same setting), display text (0.0–10.0, On/Off, typed values work) and defaults
   (Gain 3.4, Mid Freq 1). Add a state-version attribute to the saved state so later range changes can be migrated on
   load. No migration code now (unreleased). The user re-saves existing sessions after step 5.
4. **`inputHiLo` and `dryWet` stay** in the plugin and UI, neutral by default (HI = unity, 100 % wet). They are pinned
   to neutral in `plugins.json`'s AmpBass baseline, so they never affect the comparison.
5. **Presets are deferred** to a session the user runs (step 10).
6. **The user does the listening checks** at milestones A, B and C.
7. **Regression rule exception** (see Rules): a regression in a case owned by a later step is allowed if the log names it.

## Tools

- `Tests/compare/probe.py`: the measuring and fitting methods used in this plan (`fr --minus clean_flat` for a knob's
  own curve, `fit-shelf`, `fit-peaks --refit-lp`, `harmonics`, `topo`, `refnull`). See its docstring.
- The reference renders headless in `AmpBassHostRender` without the editor's license prompt. A 90 s render had no
  dropouts or drift. The license itself is the user's to keep valid.

## The loop for every step

```bash
# 1. build + install the plugin (CLAUDE.md), then:
cp build-tests/compare/report/summary.json build-tests/compare/summary-prev.json   # keep the last kept step
python3 Tests/compare/render.py --plugins ampbass --force          # all cases, AmpBass only
python3 Tests/compare/analyze.py --compare-to build-tests/compare/summary-prev.json  # report + regression gate
build-tests/Tests/AmpBassDSPTests_artefacts/Release/AmpBassDSPTests # must stay 0 FAIL
```
Faster while iterating: `render.py --plugins ampbass --force --cases <the step's cases>`, but run **all** cases
before deciding to keep a step.

Rules:
- **One change per step**, one commit per kept step (`fix:`/`perf:`/`test:` …, on `feat/sound-matching`).
- **Tune against the exact-setting cases** (one control at a time, values known exactly). `screenshot_preset` is a
  **validation** case only: its settings are read off screenshots (±0.5 knob), so don't tune to close its last 1–2 dB.
- Keep the step only if its own metric improves **and** the regression gate (step 0.6) shows no case worse by more
  than 0.5 dB level/FR or the THD tolerance. Otherwise revert and write down why in the log.
  Exception: a regression in a case that depends on a control a **later** step will fix (for example a knob case that
  gets worse once a missing fixed curve stops hiding the knob's own error) is acceptable if the log names it and
  that step re-checks it.
- **THD rule:** compare harmonics in dB relative to the fundamental (H2 and H3 separately), within ±3 dB wherever the
  harmonic is above −60 dBc. Below that, both only need to be below −60 dBc. A relative-% rule is meaningless at 0.3 %.
- **Stop rule:** if a step misses its target after 3 attempts, log the best attempt and the reason and move on.
  Come back after later steps, because they can interact.
- Don't render the reference again unless the baseline or the cases change. Its renders are the fixed target.
- Each stage stays real-time safe (CLAUDE.md). Parameter IDs are frozen. New parameters get new IDs.
- If `AmpBassDSPTests` checks encode the *old* behaviour, update the expected values in the same commit
  and say so in the message.
- **Milestones** (A, B, C below): the user listens to level-matched DI renders of both plugins (A/B) before moving on.
  Numbers can pass while it still sounds different.

---

## Step 0 — Fix the blocker, then make the test trustworthy

- [x] **0.1 BLOCKER: Master and Dry/Wet parameter ranges.** Commit `83544da` ("match graphic EQ ranges") also
  changed `master` and `dryWet` in `Source/Parameters/ParameterLayout.h` from `0…1` to `-10…8` (the GEQ Level range).
  The processor passes the raw value to the DSP, and `MasterVolume` squares it. So the Master knob is **not
  monotonic**: knob 0 → ×100 (+40 dB), knob 5 → ×1, knob 10 → ×64 (+36 dB). The designed default is ×0.25.
  In the comparisons, Master 5 ran +12 dB hotter into `PowerSection` than designed, Master 2.5 +29.6 dB, and the
  screenshot preset (6) +5 dB. Dry/Wet is outside `0…1` for any value except the default, so its cos/sin mix breaks.
  Fix: restore both to `NormalisableRange (0, 1)`, add a DSP test that checks Master is monotonic, re-render AmpBass and
  **record a new baseline row**. Re-check findings 1–6 above before trusting any of them.
- [x] **0.2 Commit the test harness changes**: `plugins.json` reference map, `render.py` (`points` map,
  numeric baseline), `screenshot_preset` case. Self-test must still null:
  `render.py --self-test --out build-tests/compare/selftest` then `analyze.py --self-test --renders build-tests/compare/selftest`.
- [x] **0.3 Check the cabinet-bypass baseline does nothing.** Render the reference with Horn on vs off and gate
  bypass On vs Off. The output must null (< −90 dB). If the gate turns out *active* with `NgByPass` On, flip it.
- [x] **0.4 Characterise Power Soak.** Render the reference with Power Soak 0 / −4 / −10 dB on a drive case.
  If −10 nulls against 0 after a −10 dB level match, it's a pure post-gain and the 0 dB baseline is fine. If not,
  it changes power-amp drive and must be modelled with Master in step 6. **Decide the baseline with the user.**
- [x] **0.5 Check the clean cases are clean on the reference.** The sweep at −40 and −30 dBFS with `CLEAN` must give the
  same FR shape (±0.3 dB) and H2/H3 below −50 dBc. If not, lower `CLEAN` gain for both plugins.
- [x] **0.6 Analyzer upgrades** (test-only commit):
  - Write `summary.json` with numeric metrics per case: level, FR max error over 20 Hz–20 kHz and 40 Hz–8 kHz,
    H2/H3 error per tone level at 41/100/400 Hz, max ramp-curve error, burst levels and settle times, DI null,
    noise, and L−R difference.
  - Add `analyze.py --compare-to <previous summary.json>` to print which cases got better or worse. That's the regression gate.
  - Read **both channels** (it only reads channel 0 today) and report L/R mismatch.
- [x] **0.7 Per-case reference settings.** Allow a case to override the reference baseline, for example
  `screenshot_preset_fxdefault` with Tight Post 50 Hz and Power Soak −4. Today the baseline is global. Steps 8 and 9 and the cabinet need this.
- [x] **0.8 Add cases** (render both plugins once):
  - Knob scales: Bass/Treble/Mid/Gain/Tube Gain/Master at 0, 2.5, 5, 7.5, 10 (linear cases for tone knobs, drive cases for gain knobs).
  - Tone-stack interaction: Bass 10 + Treble 0, Bass 0 + Treble 10, Mid 0 + Bass 10 + Treble 10.
  - **Topology probes** (drive): Gain 7.5 with Bass 0 vs 10, with Treble 0 vs 10, and with GEQ 80 Hz ±12.
    If the reference's harmonics change with a control, that control sits **before** the distortion.
    AmpBass's order is Pre-filters → Preamp → Tone stack → GEQ → Master → Power. If the reference differs,
    reorder before tuning, because no amount of tuning fixes a wrong order.
  - Power-stage knee: `clean_master2.5/5/7.5/10`, the `CLEAN` setting with the drive signals.
  - Aliasing: a 3 kHz tone at −6 dBFS with Gain 10. Measure the energy that isn't at a harmonic.
  - Stereo: a left-only input. Does each plugin treat the channels independently, or sum to mono?
- [x] **0.9 Re-baseline** after 0.1–0.8 and fill in the log. From here on, every step compares against this row.

## Step 1 — Output stage: high-frequency roll-off

- [x] **1.1** First check that the roll-off is the same in clean, drive, `treble_*` and `master*` cases (ref minus
  AmpBass, 5–20 kHz). If it moves with level or Master, it belongs in the power stage, not a fixed filter.
- [x] **1.2** Fit corner and slope from `clean_flat` (2nd-order Butterworth first, then 2× 2nd-order if needed).
- [x] **1.3** Add the low-pass at the end of `PowerSection` (existing `Biquad`), with coefficients computed from the
  sample rate. Target: FR error above 5 kHz ≤ 1 dB in `clean_flat`.
- [x] **1.4** Low end below 40 Hz: **moved to 2.2.** The reference's bass bump is much larger and broader (Bass 0→10 swing
  at 50 Hz: 23.6 dB vs 18.6 dB), and below 30 Hz it falls more steeply (about −10 dB at 20 Hz below its plateau vs AmpBass −4.4,
  close to a 2nd-order high-pass near 30 Hz). Fit both together in 2.2.

Step 1 result (2026-10-19): the reference's roll-off is fixed (`clean_flat` = `clean_master2.5` = `clean_master10` within 0.2 dB).
Fitted a 2nd-order low-pass at 12.06 kHz, Q 0.5, at the end of `PowerSection`. `clean_flat` above 10 kHz is now within
−0.5 to +1.3 dB, wide-band FR 11.4 → 5.6 dB. What's left at 5–8 kHz (+3 dB) is the reference's treble bump (step 2.1).
`treble_10` above 10 kHz is −3.2 dB (AmpBass's treble boost reaches too high, step 2.3). Open: at heavy drive with Tube Gain 0
the reference has more HF (harmonics) than AmpBass, so its low-pass may sit *before* the power-stage clipping. Decide in 6.4.

## Step 2 — Tone stack shape (clean, linear cases)

Do these in order. Each one uses a single knob's cases.

- [x] **2.1 Noon curve.** With all tone knobs at 5 the reference scoops about 8 dB at 1 kHz, with bumps near 60 Hz
  and 6 kHz. AmpBass is flat. Target: `clean_flat` ≤ 1 dB.
  **Done (2026-10-19):** the manual says Bass, Mid and Treble are active and **flat at centre**, so the noon curve is a
  fixed voicing, not knob interaction. Added to `ToneStack` (before the knobs): peak 57.5 Hz Q 0.44 +8.36 dB, peak 5906 Hz
  Q 0.69 +6.08 dB, gain −3.76 dB. Fitted jointly with the output low-pass, which moved to **14.6 kHz Q 0.382**. The fit is
  within 0.5 dB from 30 Hz to 20 kHz (`clean_flat` shape +0.2 dB). `defaults` DI null −8.7 → −23.5 dB. A 3-peak fit was
  overfitted (±20 dB peaks) and rejected. Regressions exposed for later steps: mid/treble knob cases (2.2–2.4),
  `screenshot_preset` null −15.4 → −11.1 (knob curves), `clean_master7.5/10` ramp (Master taper, 4.4), `topo_*` (step 6).
- [x] **2.2 Bass**: depth, corner and knob scale vs `bass_0`, `bass_10` and the knob-scale cases (manual: ±12 dB at 50 Hz).
  **Done:** the reference's shelf changes frequency and Q with the knob (77–152 Hz, Q 0.42–0.69, −16.3 … +14.1 dB), and one
  shared shape misses by 1.7 dB. Added `ToneStack::KnobCurve` (gain/Hz/Q at 0/2.5/5/7.5/10, interpolated). All `bass_*` cases
  are within ±0.6 dB. The low end below 30 Hz (from 1.4) is within 1 dB after the voicing, so no high-pass change.
  Regressions for later: `topo_bass*` (step 6), screenshot ramp/bursts +1 dB louder (steps 4/6).
- [x] **2.3 Treble**: `treble_0`, `treble_10`, knob scale (manual: +14 / −19 dB at 5 kHz).
  **Done:** the reference's treble isn't a plain shelf. At 2.5/7.5 it's a broad band that drifts back toward 0 dB at 20 kHz
  (a shelf alone misses by 1.9 dB; a peak alone misses by 2.6 dB at Treble 10). Model: high shelf per knob position
  (`kTrebleCurve`, 282–2929 Hz, −30 … +16.5 dB) plus a peak at 19 kHz, Q 1.2 with per-position gain (`kTrebleTopDb`), fitted
  together (≤ 1.1 dB, 200 Hz–20 kHz). Treble cases ±0.9 dB; bass/treble interaction ±1.4 dB. Regressions for later:
  `topo_treble*` (step 6), screenshot level +0.6 dB (step 4), `mid0_bass10_treble10` (Mid, 2.4).
- [x] **2.4 Midrange**: `mid_0_f3`, `mid_10_f1…f5`. Centre frequencies already match (220/450/800/1.6k/3k).
  Fit the Q and depth (±15 dB).
  **Done:** the reference's mid is wide (Q 0.15–0.9) and asymmetric (−18 to −20 dB cut, +11 to +13 dB boost), with a knob
  that barely acts near the centre (−3.6 / +1.6 dB at 2.5 / 7.5). Measured centres: 249 / 543 / 895 / 1773 / 2884 Hz. Added
  `mid_0_f1/f2/f4/f5` cases. Tables `kMidGainDb` / `kMidQ` [selector][knob]; knob 2.5/7.5 scaled from position 3. The
  selector is smoothed as a position with log-Hz interpolation (also fixes the BACKLOG item about linear Hz smoothing).
  All 13 mid cases are within ±0.3 dB.
- [x] **2.5** The interaction cases from 0.8 must be ≤ 1.5 dB. If not, step 2.1 chose the wrong model.
  **Result:** `bass10_treble0` / `bass0_treble10` ±1.4 dB. The reference's controls interact a little (AmpBass's are exactly
  additive): `mid0_bass10_treble10` is −3.6 dB deeper at 400 Hz than the sum of its knobs, `bass0_treble10` +1.5 dB. At
  realistic settings (`tone_shot` = Bass 7, Mid 6.75, Treble 7) AmpBass is within ±0.7 dB, so the interaction is **not
  modelled**. Known limitation: up to 3.6 dB at extreme combinations. Revisit if a preset uses them.
- [x] **2.6 Bright** (+6 dB at 5 kHz per the manual), **Ultra Hi**, **Ultra Lo**, **−15 dB pad**: one case each.
  **Done:** the pad already matched (−15.0 dB). Bright and Ultra Hi are broad peaks that ease off at the top, not shelves:
  Bright = peak 2991 Hz Q 0.269 +5.31 dB + high shelf 10.9 kHz +1.17 dB; Ultra Hi = peak 5602 Hz Q 0.369 +5.32 dB + high shelf
  9.0 kHz +2.36 dB. Ultra Lo = one peak 530 Hz Q 0.713 −12.36 dB; the manual's +2 dB at 40 Hz isn't in the reference. All within
  ±0.3 dB, 0 regressions. On the real amp Bright usually depends on Gain (bright cap), but **not in the reference**: its curve is identical within 0.03 dB from Gain 1 to 10 (`step6-fits/bright_gain.py`, 2026-10-19).
  Re-check with drive cases after step 6.

## Step 3 — Graphic EQ

- [x] **3.1** Band gain scale and Q per band from `geq*_+12` (reference ±12 dB). One commit for Q, one for scale if both change.
- [x] **3.2** `geq_on_flat`: the EQ switched on but flat must match (any fixed colouring or level offset).
- [x] **3.3** EQ Level: `geqLevel_+8`, plus a −10 dB case.
- [x] **3.4** `screenshot_preset` FR ≤ 1.5 dB (validation). **Milestone A: tone matches when clean.** (listen)
  **Done (one commit, the parts depend on each other):** the reference's EQ behaves like an analog proportional-Q design.
  Boost and cut are symmetric; the slider is strongly non-linear (±3 → ±0.6 dB, ±6 → ±1.5, ±9 → ±3.1, ±10.5 → ±5.0,
  ±11.25 → ±6.5, ±12 → ±9.6 dB) and Q rises with it (0.34 → 1.33). Measured centres 33 / 79 / 141 / 306 / 537 / 860 Hz /
  2.24 / 5.05 / 7.69 kHz; ±12 gain is ±9.4–9.8 dB except 33 Hz (±14.6) and 9k (±11.6). Switching the EQ on adds a +0.46 dB
  hump at 496 Hz (Q 0.1). EQ Level is already exact in dB (only its 0–1 scale differs, step 5). Added `geq*_-12`,
  `geq300_±3…±11.25` and `geqLevel_-10/+4` cases. Assumption: every band follows the 300 Hz law scaled to its ±12 values
  (checked only at ±12). All 33 EQ cases are within ±0.5 dB. `screenshot_preset`: FR shape −0.7 dB, DI null −21.5 dB.

## Step 4 — Level and gain staging

- [x] **4.1** Overall output level: `clean_flat` level error to ±0.5 dB (one output trim).
  **Done without a change:** `clean_flat` is −0.1 dB after steps 1–3 and 4.4.
- [x] **4.2** Gain knob: output level and harmonics vs Gain at fixed Tube Gain. The reference is silent at Gain 0 and
  its loudness rises with Gain (AmpBass compensates level, the reference seems not to). Gain may be a pot in front of
  the preamp rather than a drive multiplier. Copy the reference's behaviour, including silence at 0.
  **Done:** the reference's Gain is a **linear-taper level** (+6.0 dB per doubling, +20 dB from 1 to 10, silent at 0).
  `TubePreamp` keeps its drive curve, so the harmonics in dBc don't change, but its output is normalised to a small-signal gain
  of `19 × knob` (unchanged at Gain 1). `law_gain*` within 0.1 dB at every position. Drive-case level gaps shrank from
  +12…+24 dB to +1…+8 dB; the rest is AmpBass's preamp compressing early (step 6). The 18 regressions are the "noise" metric:
  the bias DC residual, now amplified with Gain (step 7). Step 6 must keep this small-signal law (DSP test).
- [x] **4.3** Tube Gain: in the reference it changes compression and character and interacts with power-amp drive
  (manual §5). Check level and harmonics at Gain 2.5/7.5 × Tube 0/5/10.
  **Measured, moved to step 6:** small-signal level is −1.0 / −1.0 / −1.2 / −2.6 / −4.1 dB at Tube Gain 0 / 2.5 / 5 / 7.5 / 10
  (lower Tube Gain is about 3 dB *louder*; AmpBass does the opposite, −5.5 dB at 0). Tube Gain also sets the even harmonics
  (H2 −37 dBc at −32 dBFS with Tube 0, −80 with Tube 5, −102 with Tube 10). Level and harmonics come from the same
  stage, so both are fitted together when the Tube Gain stage is rebuilt in 6.2.
- [x] **4.4** Master scale vs `master*` and the knob-scale cases, now that 0.1 is fixed. Target the reference's taper (about 12 dB from 2.5 to 10), not knob².
  **Done (before 4.2, because Master is a pure level after the preamp):** `law_*` cases measure small-signal level per
  knob from the −40 dBFS step. The reference's Master is a **linear taper** (+6.0 dB per doubling, silent at 0). AmpBass now
  uses `0.5 × knob` (unchanged at 5): `law_master*` within 0.1 dB at every position. Regressions for 4.2: the Gain 5 drive
  cases (`master7.5/10`, screenshot levels), because AmpBass is still ~12 dB too quiet at Gain 5 and the old steep Master
  partly hid that.

## Step 5 — Parameter contract (ranges, scales, defaults, display)

The goal is that the same normalised value (automation, presets, `.vstpreset`) means the same setting in both plugins.

- [x] **5.1 Normalised ↔ value scales** match the reference: knobs linear 0–10, Mid Freq 5 steps, GEQ ±12 linear,
  **GEQ Level −10…0…+8 piecewise** (the reference puts 0 dB at 0.5; AmpBass's linear range puts −1 dB there).
- [x] **5.2 Display text**: knobs shown as 0.0–10.0 (AmpBass shows 0–1), dB values with one decimal, switches On/Off,
  Mid Freq 1–5. Also make AmpBass's `getValueForText` work, so the harness can use `text` for both plugins.
- [x] **5.3 Defaults** match a fresh reference: Gain 3.4, Mid Freq 1, others as listed in Context. Changing ranges and
  defaults keeps the IDs but changes what old sessions load. That's acceptable before release, and should be noted in CHANGELOG.
- [x] **5.4** AmpBass-only parameters (`inputHiLo`, `dryWet`) stay neutral by default (Hi, 100 % wet) and stay off the
  faceplate. Confirm with the user whether to keep them at all.
- [x] **5.5** Update `WebUI/src/types/parameters.ts`, the `amp.ts` display view and the parameter count in `CLAUDE.md`.

Step 5 result (2026-10-19): no sound change (gate 0 / 0). IDs and order unchanged. Knobs keep their raw 0–1 range (the DSP
reads it) but show and accept 0.0–10.0; GEQ bands and Level show one decimal; GEQ Level uses the reference's piecewise
scale (−10 / 0 / +8 dB at 0 / 0.5 / 1, probe identical to the reference); defaults Gain 3.4 and Mid Freq 1. `amp.ts`
mirrors the Level mapping and the new reset values (`npm run build` and `svelte-check` clean). Saved state carries
`stateVersion` 2 with a place for future conversions. `inputHiLo` / `dryWet` unchanged (neutral defaults, pinned in the
comparison). `parameters.ts` and the parameter count needed no change. Session save/load is on the BACKLOG
manual-verification list (not covered by an automated test).

## Step 6 — Distortion and dynamics (preamp, then power amp)

### Handoff: where step 6 stands (2026-10-19)

**6.1, 6.2 and 6.4 are committed** (`9b84435` power stage, then the linear preamp + Tube Gain stage). DSP tests 0 FAIL.
Gate vs step 5: 188 better, 3 worse, all `screenshot_preset_fxdefault` (the reference's Power Soak −4 dB, which AmpBass
gets in step 9.2; its old match was AmpBass being 3.8 dB too loud). **Steps 6 and 7 are done (Milestone B passed); 8.1–8.4 are
done; next are 8.5–8.6 (in a DAW, with the user).** The rest of this section is how 6.2 was found, kept for reference.

| File | State |
|---|---|
| `Source/DSP/PowerSection.h` | **6.4 done.** Old compressor + soft clip removed. Per channel: emphasis peak 872.07 Hz Q 0.3254 +7.9087 dB → static curve `u/(1+\|u/1.6435\|^1.7781)^(1/1.7781)` → inverse peak → output LP 14602 Hz Q 0.382 |
| `Source/DSP/TubePreamp.h` | **6.1 done.** Linear: 25 Hz HPF → level `19 × gainKnob` (smoothed). Its 10 Hz DC blocker moved to the end of `PowerSection`, after the nonlinearity (two in series cost 0.5 dB of FR at 20–40 Hz in `bass_10` / `geq33_-12` / `clean_flat`). `setParams (gain)`. The bias and `x·\|x\|` shaper are gone (so step 7's DC source is gone too; re-check 7 after 6.2) |
| `Source/Plugin/PluginProcessor.h` | `kPeakDrive = 1.0f` (PEAK = preamp output at full scale) |
| `Tests/DSPTests.cpp` | Preamp clean at Gain 10 + level law (+20 dB 1→10, silent at 0); power-stage H3 (−48 dBc @100 Hz −10 dBFS, −39 @400 Hz −11.5 dBFS, clean at −40); output bound ≤ 2.0 (the reference itself peaks +5.3 dBFS); output LP (−4 dB @10 k, −19.2 @20 k) |
| `Tests/compare/{signals,cases,analyze}.py` | `tones_1600` added (signal, DRIVE_SIGNALS, harm_err). **Rendered for both plugins already** |

**Result of 6.4 + 6.1 together** (`analyze.py --compare-to summary-prev.json`, where `summary-prev.json` = step 5 state,
current one saved as `build-tests/compare/step6-fits/summary-6.4-6.1-wip.json`): **173 metrics better, 14 worse**. All 14 worse are
Tube Gain 0 / 2.5 / `screenshot_preset*` cases, i.e. the missing Tube Gain stage (6.2). Tube Gain 10 cases now match:
THD at Gain 2.5 / 7.5 / 10 = 0.4 / 2.3 / 3.4 % vs reference 0.3 / 1.9 / 3.1 % (Gain 10 was 15.3 %). DI null −26 dB.
How 6.4 was fitted: the power-stage knee depends only on the would-be-linear level after Master; fitted on clean-preamp
(Tube Gain 10) stepped tones at 41 / 100 / 400 / 1600 Hz up to +16 dBFS. A shelf emphasis over-compressed the treble;
the wide peak fits all four frequencies (H3 ≈ ±1 dB, compression ±0.4 dB to +2 dBFS, 1–3 dB at Gain 10).

**What 6.2 has to model (measured on the reference, Tube Gain stage):**
- Sits after the tone stack / GEQ (it reacts to them), before or around Master (to confirm, see next actions).
- Small-signal **level** rises as Tube Gain goes down, flat across frequency: +3.1 / +3.1 / +2.9 / +1.5 / 0 dB at
  Tube Gain 0 / 2.5 / 5 / 7.5 / 10 (relative to 10). This is part of the stage (4.3 moved here).
- **H2 at −40 dBFS, Tube Gain 0** (Gain 1, Master 5): −26.5 / −37.5 / −47.9 / −57.3 dBc at 41 / 100 / 400 / 1600 Hz
  (falls with frequency, ~8 dB/oct 41→100, ~5 dB/oct above). Tube Gain 2.5 ≈ 10 dB less H2; ≥ 5 ≈ none.
- **At high level, Tube Gain 0 is a strong asymmetric clipper**, not a gentle bias shift: at input 0 dBFS (Gain 1,
  Master 5) the reference compresses −7 to −8 dB (power stage alone: −2 to −4) and H2 reaches −7 to −11 dBc;
  at Gain 2.5 / Master 10 compression reaches −29 dB with H2 −1 to −6 dBc, H3 −2 to −15 dBc.

**Attempt 1 (failed, stop rule count 1/3):** envelope bias `y = x + k·(HP1_fh(LP1_fc(|x|)) + β|x|)` → fitted power stage
→ 10 Hz DC blocker, fitted on the real stepped tones (`build-tests/compare/step6-fits/fittube.py`, log `fittube.log`).
Best k +3.58 dB, fc 8.0 Hz, fh ≈ 0, β 0.0037, error 308. Low-level H2 is 2–7 dB too high with too shallow a frequency
slope, and at high level it compresses only about half as much (−3.7 vs −8.2 dB) with H2 10–20 dB too low. **An
envelope/bias term alone can't produce the high-level behaviour.**

**Location (done, `step6-fits/locate.py`, `locate.log`):** Gain 2.5 / Master 10 and Gain 5 / Master 5 give *identical*
compression, H2 and H3 at every Tube Gain, so Tube Gain acts **in the power stage** (after Master), not before it.
Targets for every position: `step6-fits/tube_target2.json` (`gather.py`: Tube Gain 0 / 2.5 / 5 / 7.5 / 10 × Gain/Master
(1,5) (2.5,5) (2.5,10) (7.5,5) × 41/100/400/1600 Hz). Read off them:
- Tube Gain 5 / 7.5 / 10 share the power stage's knee exactly; 5 and 7.5 only add high-level H2 (−45…−50 vs −65…−70 dBc).
- The −1 dB knee moves down 4.2 dB at Tube Gain 2.5 and 9.8 dB at 0, **the same at all four frequencies** (it follows
  the emphasis, so it's inside the emphasis sandwich).
- Low-level H2 is **constant with level** (a kink, degree-1: the half-waves get different gain), same frequency shape
  at 0 and 2.5, 2.5 is ≈10 dB below 0.
- At Tube Gain 0 the extra compression comes with H2 but **H3 stays at Tube Gain 10's value** for the same level, and at
  extreme drive the fundamental *falls* as input rises: a level-driven bias shift (blocking), not extra drive.

**Attempt 2 (works, `step6-fits/fittube2.py`):** inside the power stage, per Tube Gain position:
`v = w + k·(LP1_p(|LP1_q(w)|) + β|w|)` (kink) → `u = emph(v)` → `e = LP1_5Hz(|u|)`, `s = b + κ·max(e − e0, 0)` →
`c = curve(D·u + s) − curve(s)` → de-emph → LP → DC 10 Hz, divided by `D·curve'(b)` (unity small-signal gain), and the
level table in front. Tube Gain 0 fit (`fit_t0.json`, score 69.8 vs attempt 1's 308): q 14.9 Hz, p 88.3 Hz, β −34.5 dB,
k −9.6 dB, D +3.9 dB, b 0.086, κ 2.72. Compression within ~1 dB at high drive, H2/H3 within ~3–5 dB; known gap: the
compression onset is ~1–1.5 dB early at moderate levels (an envelope threshold `e0` only gained 69.8 → 68.9, so dropped).
Refit with low-level H2 weighted ×4 (THD rule) → `fit_t0w.json`, then the other positions with the shape fixed
(`fittube_per.py` → `fit_per.json`). The C++ is written (`PowerSection.h`: `setParams (tubeGainKnob)`, tables per knob
position; `TubePreamp::setParams (gain)`; processor and test `Chain` rewired; DSP test for Tube Gain 0 H2 and level)
with provisional constants until `fit_per.json` exists.

**Attempt 3 (kept, `step6-fits/fittube3.py`, all positions in `fit3_all.json`):** the attempt-2 fit missed the Tube Gain 0
low-level H2 by 2.5–4.5 dB at 41/100 Hz (C++ test −28.6 / −41.5 vs −26.5 / −37.5 dBc). Two causes, fixed separately:
- *Kink shape.* Its flat `β|w|` path taps a different point than the filtered path, so they interfere; the best this
  shape can reach is ~5 dB off. New kink: `r = |LP1_q(w)|`, `v = w + k·HP1_10Hz(LP1_p(r) + β·r)` (a low shelf after the
  rectifier, DC-blocked so it can't act as a bias). At low level its H2 is `k·4/(3π)·|LP_q(f)|·|shelf(2f)|`, fitted
  in closed form: q 703.4 Hz, p 20 Hz, β 0.02011 (zero at 995 Hz), k −8.71 dB at Tube Gain 0 and −18.60 at 2.5
  (2.5 is the same shape 9.9 dB lower). Simulated −28.4 / −35.9 / −47.2 / −58.0 vs −26.5 / −37.5 / −47.9 / −57.3 dBc.
- *Fixed bias.* The reference's H2 is flat at low level, then rises ~1.6–1.8 dB per dB. On this curve a fixed bias gives
  ~0.8 dB/dB and an envelope-driven bias ~1.8, so the reference's bias is all envelope: `b = 0` at Tube Gain 0 and 2.5.
Then only D, κ, e0 refitted per position (b fixed 0; 5 and 7.5 only k, now −49.8 dB; 10 kept). Scores vs attempt 2:
Tube Gain 0 66.4 vs 73.4, 2.5 16.5 vs 23.7, 5 9.96 vs 11.47, 7.5 9.75 vs 11.30, 10 5.57 = same. Rows off the THD rule at
Tube Gain 0: H2 22 → 19, H3 35 → 34, compression > 1 dB 11 → 8 (max 8.0 → 5.2 dB). Trade-off at 2.5: H3 rows 6 → 0, but
H2 8 → 11 and compression > 1 dB 8 → 14 (max 3.1 dB, under-compressing at extreme drive).
Known gaps left: H2 rises 3–5 dB early at 1600 Hz around −24 dBFS; 41 Hz compression 2–5 dB off at extreme drive.

**Next actions for 6.2 (in order):**
1. *(done: in the power stage, see Location above)* **Locate it vs Master**: render the reference at Tube Gain 0, Gain 2.5, Master 2.5 / 5 / 10 on `tones_100` /
   `tones_400` (`probe.py harmonics`). If H2 and its onset (input level) don't move with Master → the stage is before
   Master; if they follow the output level → after.
2. *(done differently: attempt 2 above)* **Model as an asymmetric saturating curve** driven by Tube Gain: drive and asymmetry rise as Tube Gain goes down
   (Tube Gain 10 ≈ bypass), placed where step 1 says (a new header-only stage, e.g. `Source/DSP/TubeStage.h`, between
   GraphicEQ and MasterVolume if before Master), with the level table above, a DC blocker after it, and a frequency
   tilt before it if the H2-vs-frequency slope needs it. Possibly keep a small envelope term for the low-level H2 slope.
3. *(done: attempt 3 above)* Fit on `step6-fits/tube_target.json` (cached reference steps for Tube Gain 0 at three Gain/Master settings ×
   41/100/400/1600 Hz; reuse `fittube.py`'s harness, `err()` takes ~0.4 s) **plus** Tube Gain 2.5 and 5 renders.
4. *(done)* `TubePreamp::setParams (gain)` only; update the processor, the DSPTests `Chain` mirror and the tests (Tube Gain H2
   targets above at −40 dBFS); expect 0 FAIL.
5. *(done 2026-10-19, see the log)* Full loop: render AmpBass all cases, gate vs `summary-prev.json` (still the step-5 state, **don't overwrite it before
   the 6.x commits**), commit (two commits), log rows for 6.4 and 6.1+6.2, then copy `summary.json` → `summary-prev.json`.
6. Then: 6.3 (frequency dependence, likely mostly solved), re-run `topo_*` and `alias_*` (0.8 notes), 6.5 dynamics
   (burst settle times. Checked 2026-10-19 on existing renders: at Tube Gain 10 the reference settles at once, so it has
   no slow compressor and removing the old one was right; at Tube Gain 0 it settles in 60–120 ms (quiet 0.06–0.09 s,
   loud 0.09–0.12 s at Gain 2.5 / 7.5) = the blocking bias moving. So 6.5 = fit the envelope's attack/release
   (`kEnvelopeHz`, 5 Hz placeholder) on the Tube Gain 0 bursts, after the static fit),
   step 7 re-check, 6.7 DI nulls → **Milestone B** (user listens, `probe.py listen`).
   Note for 6.5 / 6.7: with a second 10 Hz HP in the chain the `screenshot_preset` DI null was −25.5 dB instead of
   −22.9, but clean FR got 0.5 dB worse at 20–40 Hz, so it was removed. Look at the low-frequency dynamics (the
   envelope, 6.5) before adding any filtering. Driven DI nulls now: −26 dB at Tube Gain 5/10, −15.6 to −19 at 0.

Fitting scratch scripts from this session (`fitpower*.py`, `fittube.py`, targets and logs) are kept in
`build-tests/compare/step6-fits/` (gitignored, local only). They import `Tests/compare/probe.py` / `analyze.py`.

- [x] **6.1 Preamp onset.** Done differently: the preamp is now fully linear (the
  reference's preamp is clean even at Gain 10, Tube Gain 10). Original target: H2/H3 vs level at 41/100/400 Hz within
  the THD rule.
- [x] **6.2 Harmonic balance / Tube Gain stage**: even/odd per level. Done in the power stage (attempt 3, see handoff):
  DC-blocked kink for the low-level H2, envelope-driven bias for the high-level H2 and compression.
- [x] **6.3 Frequency dependence**: AmpBass is too dirty at 400 Hz at every level. Look at the filtering before and after the shaper.
  *(2026-10-19: solved by 6.4's emphasis. `defaults` / `gain10_tube10` at 400 and 1600 Hz: H3 within 1–3 dB of the
  reference at every level, output within 0.3 dB up to −8 dBFS. Left: 1–1.5 dB too little compression at 0 dBFS
  input, the 6.4 ceiling gap.)* Topology re-run (0.8): boosts now shrink with level like the reference's (bass 41 Hz
  +24.9 → +4.4 vs +25.1 → +1.7 dB, treble 400 Hz +5.4 → +0.6 vs +4.8 → +0.2, GEQ 80 Hz +14.5 → +1.9 vs +14.5 → +1.3)
  and H3 rises with them. No reorder needed. AmpBass keeps 1.5–3 dB more boost above −16 dBFS (the same ceiling gap).
- [x] **6.4 Power section = main distortion stage.** (`9b84435`) A linear zone, then a knee at the output level where the reference's
  H3 appears (about −30 dBFS fundamental peak, independent of Master), rising about 1.5–2 dB per dB above it. Check with
  the `clean_master*` cases (0.8) and `probe.py harmonics master=…`, then the ceiling and Master distortion. Target: ramp curve
  ±1 dB, burst levels ±1 dB.
- [x] **6.5 Dynamics** *(accepted at Milestone B with the choke gap below)*: attack/release of the `PowerSection` compressor from the burst envelopes (settle times ±20 %).
  *(2026-10-19, attempt 1/3.)* Settle times with the 5 Hz envelope: reference 60/90, 90/120, 110/130 ms (quiet/loud,
  `gain2.5/7.5/10_tube0`) vs AmpBass 50/80, 110/120, 120/120 (one +22 %, 10 ms resolution). Attack (quiet → loud) matches
  within ±0.6 dB. **Release doesn't:** after loud → quiet the reference chokes: at Gain 7.5 it falls to −50.5 dB within
  10–20 ms (33 dB under its quiet level) and recovers in ~60 ms; AmpBass only dips to −34 dB and recovers more slowly.
  Fitting separate attack/release (`step6-fits/fittube4.py`, tones + burst envelopes, C envelope in `envasym.c`) returns
  to ~5 Hz both ways from every start (score 75.22 vs 75.25): this model can't make the choke at any timing without
  breaking the static tone fit. Next attempt would need a cutoff-type element (bias driving the stage past cutoff).
- [x] **6.6 Aliasing**: add oversampling around the shapers only if AmpBass aliases more than the reference. If added,
  report the latency (`setLatencySamples`) and re-check the null alignment.
  *(2026-10-19: no oversampling needed.)* The −93.8 dB was not aliasing: 3 kHz divides 48 kHz, so a static curve's aliases
  land on harmonics and this metric can't see them. It was a steady ~−91 dBFS pattern near 14 Hz (period ~3340 samples,
  plus ±28.8 Hz sidebands on the harmonics), present in every case incl. clean ones: float32 limit cycles in the
  low-frequency biquads. `Biquad` (and `PowerSection`'s one-poles) now run in double: −125.9 / −126.0 dB vs the
  reference's −125.0. The metric still can't detect real aliasing; a non-divisor tone (e.g. 2.9 kHz) would.
- [x] **6.7** DI null ≤ −20 dB on the exact-setting drive cases, and `screenshot_preset` ≤ −20 dB.
  **Milestone B: sound matches when driven.** (listen)
  *(2026-10-19: met except Tube Gain 0, accepted at Milestone B.)* `screenshot_preset` −22.9 dB, every Tube Gain 5 / 10
  drive case −26 dB, `topo_*` −19.8 to −26.8. Tube Gain 0: `gain2.5_tube0` −19.0, `gain7.5_tube0` −17.3,
  `gain10_tube0` −15.6 (the 6.5 choke). The user listened to level-matched DI pairs (`build-tests/compare/listen/step6/`:
  `screenshot_preset`, `gain7.5_tube10`, `gain7.5_tube5`, `gain2.5_tube0`, `gain7.5_tube0`): "they seem similar".
  Known gaps kept: the loud → quiet choke at Tube Gain 0 (6.5), and 1–3 dB too little compression at extreme input
  (ramp up to 2.7 dB, loud bursts −1 to −2 dB, `topo_*` boosts 1.5–3 dB too big above −16 dBFS).

## Step 7 — Silence and DC

- [x] **7.1** *(2026-10-19: the preamp bias is gone since 6.1; all 28 cases with a silence render now read −240 dBFS.
  Tube Gain automation, DSP test: jumping 10 ↔ 0 every 85 ms keeps silence exactly silent. With a −20 dBFS 100 Hz tone
  the bias shift made a subsonic bump of −31 dB (one-cycle mean rel. the tone) with the 50 ms knob glide; now 200 ms,
  −48 dB (held at 0: −84, at 10: −127). `prepareToPlay` sets the parameters before preparing, so Tube Gain doesn't
  glide from its default when playback starts.)* The "noise" is DC from the preamp bias, not hiss. `TubePreamp` adds `0.1 * (1 - headroom)` before the
  shaper, so at silence the shaper outputs a constant that the 10 Hz DC blocker only removes slowly. That's why every
  Tube Gain 10 case reads −240 dBFS and the others read −104 to −111 (louder at high Master). It also means a thump
  and DC settling when Tube Gain moves. Subtract the shaper's at-rest output (bias through the shaper) so silence in gives
  exactly silence out. Target: ≤ −140 dBFS, and no step when Tube Gain is automated.

## Step 8 — Behaviour

- [x] **8.1 Sample rate**: `screenshot_preset` and `clean_flat` at 44.1 and 96 kHz, within 0.5 dB of the 48 kHz
  result in both plugins.
  *(2026-10-19, `behaviour.py rates`; target changed to "follow the reference's change", since the reference
  misses its own 0.5 dB.)* Drive doesn't depend on the rate in either plugin (stepped
  100 Hz tones, H3 and DI level within 0.01 dB). Only the top octave moves, and **the reference isn't rate-invariant
  either** (FR shape vs 48 kHz, 0 dB at 1 kHz): 44.1 kHz +0.8 dB at 15.9 kHz, +2.7 at 20 kHz; 96 kHz a broad dip
  from 4 kHz, −5 dB at 14–16 kHz, −1 at 20 kHz. AmpBass vs reference: 48 kHz within 0.3 dB; 44.1 kHz within 0.5 dB to
  14 kHz, −1.6 at 15.9 kHz, −6.8 at 20 kHz; 96 kHz +1 dB at 8 kHz rising to +7 at 20 kHz. AmpBass's own change is the
  14.6 kHz output low-pass (bilinear, fitted at 48 kHz: predicted −0.4 / −6.0 dB at 15.9 / 20 kHz for 44.1 kHz,
  measured −0.5 / −4.1). The reference behaves the same at 88.2 / 96 / 176.4 / 192 kHz (two families within 0.2 dB).
  **Fix:** the output low-pass has one fit per rate family (< 46k, 48k, 88.2/96, 176.4/192 kHz), fitted to the
  reference's change from 48 kHz on `clean_flat` + `screenshot_preset` together (`step8/fitlp2.py`). Worst AmpBass −
  reference change, 1–16 kHz: 44.1 kHz 1.3 → 0.6 dB, 88.2/96 kHz 6.5 → 1.1, 176.4/192 kHz 7.0 → 1.3. The ~1 dB
  left is case-dependent (the reference's tone stack also moves at high rates), not followed. 48 kHz unchanged.
- [x] **8.2 Block size**: render with `--block 32`, `512` and `4096`. AmpBass output must null between them (< −100 dB).
  The 32-sample coefficient updates must not depend on the host block.
  *(2026-10-19, `behaviour.py blocks`: bit-exact, −240 dB at 32 / 100 / 4096 vs 512 on `screenshot_preset`,
  `gain7.5_tube0`, `defaults` × DI / bursts / sweep_12.)*
- [x] **8.3 Stereo**: same channel handling as the reference (from 0.8).
  *(2026-10-19, `behaviour.py stereo`: both dual mono. Left-only input → right −240 dB in both; identical L/R → L − R
  −240 dB in both. The 0.8 note's −96 dB was the old DC residual.)*
- [x] **8.4 Bypass and Mute**: render each plugin with each switched on. Compare the output (dry, silent, latency-
  compensated?) and whether the switch ramps or hard-cuts.
  *(2026-10-19, `behaviour.py switches`, host `--set-at` automation.)* Reference: 0 latency, bypass = input bit for bit,
  mute = digital silence, both fade linearly (10–90 % in 43 ms; it reacts ~12 ms after the change, likely its control
  rate, not copied). AmpBass had 10 ms fades and a −138 dB bypass residue; now 50 ms linear (40 ms 10–90 %) and
  bit-exact bypass. (Dry/Wet keeps its equal-power crossfade; this is the bypass switch.)
- [x] **8.5 PEAK/MUTE LED** *(user, Ableton, 2026-10-21: OK)* *(code check 2026-10-19: the UI already lights it on Mute, `Faceplate.svelte` `$peak || $mute`; only the DAW check is left)* (in a DAW, not measurable offline): per the manual it lights on input overload, high Gain,
  **or too much tone boost**, and **whenever Mute is on**. Check that AmpBass's LED logic (`PluginProcessor.cpp`, PEAK block)
  also lights on Mute.
- [x] **8.6 Knob moves** (in a DAW): sweep every control during playback in both plugins. No zipper noise or clicks. *(user, Ableton, 2026-10-21: OK. Separate open issue: a steady Treble hiss, BACKLOG P1.)*
  Mid Freq and switches change without clicks.
- **Milestone C: behaviour matches.**

## Step 9 — Reference parameters AmpBass lacks (new IDs only)

One parameter per step: DSP + `ParameterIDs.h`/`ParameterLayout.h` + TS mirror + `plugins.json` map + a case that
uses the per-case reference settings (0.7). No faceplate UI unless the user asks.

- [x] **9.1** Input gain (±20 dB, linear in dB like the reference). *(2026-10-20: −20 + 40 × normalised dB. In the
  reference a pure gain before everything: Input +6 / −10 dB nulls to −112 / −109 dB against the input scaled by the
  same amount (gate off). `inputGain` parameter (appended; default 0 dB, so old sessions are unchanged) applied in
  `InputGain.h`. `input_+10_gain5`: level −0.9, FR +2.5 dB, THD 9.0 vs 11.1 %, null −25.9 dB: the known 1–3 dB of
  too little compression at the hardest drive, not the control.)*
- [x] **9.2** Power Soak (−40 … 0 dB). *(2026-10-20: linear in dB, −40 + 40 × normalised, default 0.9 = −4 dB; a pure
  output gain (0.4), so `PowerSoak.h` is a 50 ms-smoothed gain after the cabinet. `powersoak_-20_gain7.5` level −0.05 dB.
  `master*` cases unchanged: they run with Power Soak 0 dB in both plugins.)*
- [x] **9.3** Tight (Off/Pre/Post + frequency). *(2026-10-20: frequency 30 × 100^normalised Hz (default 0.111 = 50 Hz);
  Off / Pre / Post at 0 / 0.5 / 1. Measured at 30 Hz–3 kHz (`build-tests/compare/fx/tight.py`): a first-order high-pass
  at the set frequency (within 2 %) plus makeup gain 2.468 × log10(f / 30) dB (within 0.02 dB); whole curve within
  0.06–0.65 dB. Pre and Post identical when clean. `Tight.h`, one instance before the amp and one after the power stage.
  `tight_post_50/300/1500` FR 0.2 dB; `tight_pre_300_gain7.5` / `tight_post_300_gain7.5` FR 1.3 / 1.2 dB, THD 0.97 vs
  0.72 % / 8.3 vs 7.4 %, nulls −25.6 dB, like the plain drive cases.)*
- [x] **9.4** Smooth (Off/Pre/Post + frequency). Same. *(2026-10-20: frequency 3 × (35/3)^normalised kHz (default
  0.792 = 21 kHz), Off / Pre / Post at 0 / 0.5 / 1. Measured at 3–35 kHz and 44.1 / 48 / 96 kHz
  (`build-tests/compare/fx/smooth.py`): an analog first-order low-pass at the displayed frequency, the same curve at
  every rate (so not bilinear, which would fall to nothing at Nyquist). `Smooth.h`: a one-pole matched to the analog
  magnitude at DC, Nyquist and min(fc, fs / 4), cutoff 1.02 × the displayed value: within 0.17 dB everywhere. Pre and
  Post identical when clean. `smooth_post_3k` / `_21k` FR −0.2 / +0.3 dB; `smooth_pre_6k_gain7.5` FR +0.8, THD 3.7 vs
  3.3 %, null −26.1 dB.)*
- [x] **9.5** Noise gate (on/off, Threshold, Range), plus a gate case with a decaying note. *(2026-10-21, scripts in
  `build-tests/compare/gate/`. Reference: Threshold −120 + 120 × normalised dB, Range 0–100 dB (100 = muted), default
  on / −88 / 50 (`NgByPass` On = active). It listens to the input after Input, before the pad (Input ±10 dB moves the
  switch level ±10 dB; pad, Gain, Treble, Master, Power Soak don't) and turns the amp's output down by Range dB (a driven
  render gated by 15 dB = the ungated one −14.8 dB, residual −46 dB). Dual mono. Detector: band-limited input, switch
  level − threshold +18.6 / +11.3 / +2.5 / +0.8 / +4.2 / +9.1 dB at 41 / 100 / 400 / 1k / 5k / 10k Hz, the same at every
  threshold and at 44.1 / 48 / 96 kHz → first-order high-pass 348 Hz + low-pass 3.9 kHz (`sidechain.py`, ≤ 0.33 dB);
  a one-pole on |x|, 32 ms up and down (12 level steps, rms error 0.4 ms, `fitdet.py`; mean-|x| not RMS: a two-tone
  moves the switch 2.18 dB, predicted 2.18 vs RMS 3.06, `twotone.py`); no hysteresis; gain one-pole 1.6 ms. Below about
  −120 dBFS output the reference fades to exact zero with or without the gate (its silence floor, step 7), not
  modelled. `NoiseGate.h` before the cabinet, detector on the dry input with the threshold minus Input dB; parameters
  `gate` / `gateThreshold` / `gateRange` appended, `stateVersion` 6 loads older sessions with the gate off. Switch levels
  within 0.2 dB of the reference to 5 kHz (≤ 0.85 dB to 10 kHz, 3 dB at 20 kHz: the simple one-pole low-pass), switch
  times within 1.8 ms (DSP tests). Signal `decay` (four notes E1–G2 fading 15 dB/s over −80 dBFS noise), metric
  `decay_env_db`, cases `gate_-60_r20`, `gate_-50_mute`, `gate_-70_r50_gain7.5`, `gate_defaults`: 0.25 / 0.70 / 0.40 /
  0.18 dB (gate off in both: 0.19). Known gap: on these harmonic-rich notes AmpBass closes 5–20 ms early, up to 75–90 ms
  on the 41 Hz note at Range 50–100, where both gates flutter for ~150 ms; pure tones flutter identically (`flutter.py`).
  On the user's bass file (first 60 s) the time closed matches within 0.1 %.)*
- [x] **9.6** `screenshot_preset_fxdefault` (Tight Post 50 Hz, Power Soak −4) passes the Milestone B targets.
  *(2026-10-20: level −4.13 → +0.04 dB, FR −4.05 → −0.66 dB, THD 7.00 vs 7.07 %, null −23.7 dB, like `screenshot_preset`.
  New `factory_defaults` / `factory_screenshot_preset`: both plugins as fresh instances (cabinet, horn, FX Rack, the
  reference's gate): level −0.01 / +0.29 dB, FR −1.4 / +1.4 dB; about −2 dB at 16 kHz is the reference's Smooth, 9.4.)*
- Horn and Rec Chain wait for the cabinet phase.

## Step 10 — Presets

**Deferred (user, 2026-10-21):** the toolbar's current R&B values are good enough for now; skip this step until the user
brings it back. Note for then: the reference's factory presets are installed as `.vstpreset` files in
`~/Library/Audio/Presets/Plugin Alliance/Ampeg SVT3Pro/` (31 + Zero; four-letter tags such as `gain`, `bass`, `pmid`,
`mifq`, `trbl`, `tbgn`, `mstr`, `eqb1`–`eqb9`, `cabt`, `ngth`, then four big-endian floats, one per A–D slot), and none
of them is called R&B, Rock, Synth or Upright.

- [ ] **10.1** Get exact values for the 4 presets (R&B, Rock, Synth, Upright), including the FX Rack / Rec Chain.
- [ ] **10.2** Add one compare case per preset (cabinet bypassed, with the preset's own FX settings via 0.7).
  Each must pass the Milestone B targets.
- [ ] **10.3** Preset table in C++ applied via the existing `presetSelect` parameter (frozen ID), setting all parameters
  in one go. Note that the reference's A/B/C/D are settings *slots*, not factory presets, so behaviour differs on purpose.
- [ ] **10.4** Prominent preset selector in the UI as its own component (so it moves to the later minimal UI).
  *(2026-10-20: `PresetBar.svelte` in the toolbar (in place of the A–D stubs): R&B / Rock / Synth / Upright, data in
  `WebUI/src/lib/presets.ts`. Only R&B is set, as a tuning aid: the settings that reproduce the user's Ampeg test file
  (`Bass for testing amp - Ampeg version.wav`). A preset is one undo step and stays lit while every value equals it.
  Replace its values with the reference's R&B once 10.1 has them.)*

## Step 11 — Tone stack, from the schematics (2026-10-20)

The amp schematics (`building-process/references/ampeg-svt-3-pro*-schematics.pdf`, local) show a **passive** Bass /
Treble network and an LC (800 mH) mid, not the active, independent controls assumed in step 2. They also confirm Tube Gain
= the power-amp driver tubes' plate voltage (step 6). Grid: Bass × Treble × Mid at 0 / 2.5 / 5 / 7.5 / 10 × Mid Freq 1–5,
625 clean sweeps per plugin (`build-tests/compare/tonegrid/`, `render_grid.py`).

- [x] **11.1 Bass × Treble interaction.** In the reference, combined minus the sum of single knobs depends only on Bass ×
  Treble (within 0.3 dB over Mid / Mid Freq), at 200–550 Hz: −3.8 dB at Bass 10 + Treble 10, +1.6 at Bass 0 + Treble
  10. Added one peak per [bass][treble] position (`fitpeak.py`, residual ≤ 0.35 dB, 0.72 at Bass 10 + Treble 10).
- [x] **11.2 Mid at 2.5 / 7.5, every position.** They were scaled from position 3: up to 3.1 dB off (2.5 at position 5).
  Refit gain + Q from the grid (`fitmid.py`, ≤ 0.5 dB).
- [x] **11.3 Knob tapers between grid positions.** Off-grid single knobs were off by up to 5 dB: Treble 1.25 −5.0 dB
  and 8.75–9 +3.6–3.9 dB at 8 kHz, Bass 8.75–9 +2.4 dB at 70 Hz, Mid 1.25 (position 1) −3.4 dB, Mid 8.75 −1.4 dB
  (`offgrid.py`). Each knob measured every 0.25 (`render_taper.py`), its filter fitted at every step (`fittaper.py`,
  warm-started from the neighbour so the parameters move smoothly) and tabulated every 0.5 (bass, mid) or 0.25 (treble,
  whose shelf moves fastest near 0 and 10). Interaction table refined to 1.25 steps (9 × 9, `fitpeak9.py`): it rises
  steeply above 7.5. Checked at points between the table entries (`render_mid8.py`, `checktable.py`): single knobs
  ≤ 1.33 dB (Mid ≤ 0.5), off-grid combinations ≤ 1.37 dB (were 2.2–6.0). New cases `treble_1.25`, `treble_8.75`,
  `bass_8.75`, `mid_1.25_f1`, `bass8.75_treble8.75`: 0.2–1.2 dB.

## Step 12 — Cabinet (2026-10-20)

The colleague's listening test ("medioso", missing the Ampeg treble) was mostly the reference's default cabinet: AmpBass
had none (D9). The user chose the cabinet next. Only the reference's default Rec Chain is modelled for now; the presets'
chains wait for 10.1. Amp-only cases keep both cabinets off (`plugins.json` sets AmpBass `Cabinet` 0); the `cab_*` cases
turn them on through per-case baselines (0.7).

- [x] **12.1 Is it a fixed filter?** Cabinet effect (Rec Chain 20 minus `27 EMPTY PLAY REAL CAB`) under clean / driven /
  Tube Gain 0 / Master 10 / two sweep levels: same within 0.3 dB (`build-tests/compare/cab/linearity.py`). Horn the same.
  So a linear EQ is enough. Its IR peak sits 73 samples (1.5 ms) later than the bypass (85 with the horn).
- [x] **12.2 Sample rate.** Above ~5 kHz the reference's cabinet depends on the rate, like its amp top end (8.1): at
  16 kHz −25 dB at 44.1 / 48 kHz but −11 dB at 96 kHz (`rates.py`). One fit per rate family, as in 8.1.
- [x] **12.3 Fit.** Biquad cascade per rate (high-pass + low-pass + greedy peaks, joint refit, `fitcab.py`). The first
  fit, on the 1/6-octave FR, looked good point by point (≤ 1.2 dB) but scored 2.3–3.4 dB in the harness: its narrow peaks
  are not what 1/6-octave averaging sees. The 1/24-octave data (`rates_fine.py`) shows why: narrow dips from the
  two-microphone mix (−8 dB at 2.4 kHz; −14 dB at 4 kHz with the horn). Refit at 1/24 octave with 24 (cabinet) + 16
  (horn) filters, Q ≤ 12: after 1/6-octave averaging ≤ 0.37 dB (cabinet) and ≤ 1.12 dB (horn) to 12 kHz at all three
  rates (`smooth6.py`). Tables generated into `Source/DSP/Cabinet.h` (`gen_cabinet.py`).
- [x] **12.4 Plugin.** Stage 8 after the power stage; parameters `cabinet` and `horn` (new IDs, appended, both on by
  default like the reference; `stateVersion` 3 loads older sessions with the cabinet off); 50 ms linear crossfades;
  off is bit-exact. No faceplate control yet (BACKLOG).
- [ ] **12.5 Phase (open).** DI nulls with the cabinet on are −4 dB: the fit is minimum phase, the reference's
  cabinet is a measured response with a microphone delay. Spectrum matches; only worth doing if listening says so.
- [ ] **12.6 Listening (user).** Level-matched A/B of `cab_horn_screenshot_preset` / `cab_horn_defaults` DI renders,
  and the colleague's check in a DAW with both plugins at their defaults.

## Step 13 — Project clean-up (added 2026-10-21)

Goal: when this step is done, the repository holds only what a JUCE C++ plugin project normally has. The matching
material (this plan, the comparison harness, the backlog, the AI notes) is kept in a git tag and in local folders, not
in the tree. Target tree:

```
amp-bass/
├── .gitignore
├── CMakeLists.txt
├── CHANGELOG.md          # user-facing, one line per release
├── LICENSE
├── README.md             # what it is, build, signal chain, parameters, tests
├── Source/
│   ├── DSP/              # stages + Primitives/
│   ├── Parameters/       # ParameterIDs.h (frozen), ParameterLayout.h
│   └── Plugin/           # PluginProcessor, PluginEditor, NativeBridge
├── Tests/                # C++ unit tests only (DSPTests.cpp, TestSignals.h, CMakeLists.txt), run with ctest
└── WebUI/                # the editor's UI source (Svelte), built into BinaryData
```

Gone from the tree: `docs/`, `BACKLOG.md`, `CLAUDE.md`, `WebUI/CLAUDE.md`, `Tests/compare/`, `Tests/HostRender.cpp`.

**Start only after** the matching work is finished or explicitly dropped (9.5, 10.1–10.4, 12.5, 12.6, 8.5, 8.6,
Milestone C), because those steps need the comparison harness, the reference renders and this plan.

**Decisions to make first (user).** The recommended option is listed first.
- **D13.1 Development notes** (`CLAUDE.md`, `WebUI/CLAUDE.md`, this plan, `BACKLOG.md`): (a) take them out of git
  but keep them on disk and add them to `.gitignore`, as `building-process/` already is, so Claude Code still reads
  them locally; or (b) delete them (the archive tag still holds them).
- **D13.2 Tests**: (a) keep `AmpBassDSPTests` as a ctest unit-test target and remove `HostRender` + `Tests/compare/`;
  or (b) remove `Tests/` entirely.
- **D13.3 `docs/how-ampbass-works.md`**: (a) move a short version (signal chain, level map, where to tune) into the
  README and delete `docs/`; or (b) keep `docs/` with that one file.
- **D13.4 Licence**: the README says "All rights reserved". JUCE 8 is AGPLv3 or commercial, so a closed-source
  release needs a commercial JUCE licence (a free tier with a revenue cap exists). Choose the plugin's licence and
  write `LICENSE` to match.
- **D13.5 Fitting scripts** (`build-tests/compare/**/*.py`, `step6-fits/envasym.c`, about 60 files; they generated the
  tables in `Cabinet.h`, `ToneStack.h`, `PowerSection.h` and the output low-pass): (a) commit them once, with
  repo-relative paths, so the archive tag holds them; or (b) keep them only in the local backup (13.1).

**User's answers (2026-10-21):** D13.1 (a) untrack, keep on disk. D13.2 (b) remove `Tests/` entirely (no unit tests in
the repo; the archive tag and the local backup hold them). D13.4 decide later: 13.8's licence part stays open. D13.3 and
D13.5 not asked: (a) README and (b) local backup only (the user declined committing the fit scripts earlier the same day).
`inputHiLo`: remove the parameter (13.2). The user wants to focus on this step now; the open matching items (Treble hiss,
12.5 cabinet phase, 12.6 listening, which the user links to the Treble hiss) continue from the archive tag if needed.

Steps (one commit each, `chore:` / `docs:` / `test:` / `refactor:`):

- [ ] **13.1 Archive.** Copy everything local-only that is worth keeping to a backup outside the repo: `build-tests/compare/`
  without renders (fit scripts, fit `.json`, `summary-*.json`, `report/`), `build-tests/*.wav` (the user's test files),
  `docs/research/`, `docs/ui/`, `docs/decision-log.md`, `docs/findings.md`. Then make an annotated tag
  `sound-matching-archive` on the last commit that still has the harness and this plan, and push it. Check
  `git show sound-matching-archive:docs/sound-matching-plan.md` works.
- [ ] **13.2 Backlog.** Close or move every open `BACKLOG.md` item to a GitHub issue (P1 DAW checks first: they are
  release blockers, not notes). Items that change code are done as normal fixes before this step ends: `dryWet`
  smoothing or removal, hidden `inputHiLo` / `dryWet` / `presetSelect` (hide, wire up or document), the DSP test that
  runs at Gain 0, the `Chain` mirror in `DSPTests.cpp`, `npm install` on every build, idle coefficient updates.
- [ ] **13.3 Source comments.** Rewrite comments that point to removed material so each one states the fact itself:
  "sound-matching plan N" / "step N" / "plan N" (`Cabinet.h`, `InputGain.h`, `PowerSection.h`, `PowerSoak.h`, `Smooth.h`,
  `ToneStack.h`, `Primitives/Biquad.h`, and the rest that `git grep -nE "[Ss]tep [0-9]|plan [0-9]"` finds), build-tests
  script paths (`Cabinet.h:109`), "UI spec §" (`PluginEditor.cpp`, most of `WebUI/src/lib/`), and `presets.ts`'s
  test-file path. Example: `(plan 7.1)` → nothing, the comment already says why. Done when the grep over `Source/`,
  `WebUI/src/`, `Tests/` returns nothing.
- [ ] **13.4 Trademarks.** No reference brand in shipped code or UI: the placeholder "Ampeg" wordmark in
  `Chassis.svelte` (replace with the product's own wordmark; user sign-off), `AMPEG_FILE_MATCH` and its note in
  `presets.ts`, "Ampeg SVT-3 PRO" in `ParameterLayout.h`'s comment ("the reference plugin" or plain units). Check
  with `git grep -niE "ampeg|svt|brainworx|plugin alliance" -- Source WebUI/src CMakeLists.txt`. The README may name
  the inspiration only if the user wants that.
- [ ] **13.5 Tests** (D13.2). Delete `Tests/compare/` and `Tests/HostRender.cpp` and their target in
  `Tests/CMakeLists.txt`. Register `AmpBassDSPTests` with `enable_testing()` / `add_test()` so `ctest --test-dir
  build-tests` runs it; rename the option text to "Build unit tests". Remove checks that only make sense against the
  reference renders. Still 0 FAIL.
- [ ] **13.6 Docs** (D13.3). README sections: what it is, build, development (`npm run dev`, `svelte-check`), tests
  (ctest), signal chain, parameters, project structure (the target tree above), licence. Fold in what is kept from
  `how-ampbass-works.md`, drop `docs/`, drop the README's `Tests/compare` and plan bullets.
- [ ] **13.7 Development notes** (D13.1). `git rm --cached` (option a) or `git rm` (option b) for `CLAUDE.md`,
  `WebUI/CLAUDE.md`, `BACKLOG.md`, `docs/sound-matching-plan.md`. Before that, write the last log row here and update
  `CLAUDE.md`'s "Project files" so it no longer sends sessions to removed files. Do this last of the content steps:
  it removes this plan.
- [ ] **13.8 `.gitignore` and licence.** `.gitignore` covers only real outputs: build folders, `node_modules/`,
  `WebUI/dist/`, plugin binaries, OS / IDE files (`.DS_Store`, `.idea/`, `.vscode/`, `.cache/`,
  `compile_commands.json`, `*.tsbuildinfo`), audio files (`*.wav`, `*.aif*`, `*.flac`), logs, `.env`; plus the local
  folders from D13.1. Drop the `docs/*` rules and `__pycache__/` once Python is gone. Add `LICENSE` (D13.4) and
  make the README's licence section match. `package-release.sh` stays local or becomes a committed, generic release
  script (no signing identities in it).
- [ ] **13.9 Fresh-clone check.** `git clone` into a temporary folder, then:
  `cmake -S . -B build -DAMPBASS_COPY_PLUGIN=OFF && cmake --build build` (VST3, AU, Standalone build);
  tests configure + `ctest` 0 FAIL; `cd WebUI && npm ci && npm run build && npx svelte-check` 0 errors;
  `git ls-files` matches the target tree. Optional: pluginval on the VST3 and AU. Then the plugin in a DAW: loads,
  UI shows, a saved session recalls.
- [ ] **13.10 Release notes.** Rewrite `CHANGELOG.md`'s Unreleased list in user terms (no step numbers, no
  "matches reference"), one line per merge to `dev` from here on. Merge to `dev` when 13.9 passes.
- **Milestone D: clean JUCE project.**

---

## Log

| Step | Date | Change | screenshot_preset (level / FR / THD / null) | Other cases | Kept? |
|---|---|---|---|---|---|
| baseline | 2026-10-19 | — (Master range bug present) | +1.3 / 4.9 / 8.2 vs 6.5 % / −15.3 | see report | — |
| 0.1 | 2026-10-19 | Master/Dry-Wet range 0–1 (`2eb287f`) | +6.3 / 4.9 / 8.2 vs 6.5 % / −15.3 | clean levels −0.4 dB; drive 8–23 dB quiet | yes |
| 0.3 | 2026-10-19 | reference gate off in baseline (`be4c3c6`), reference re-rendered | unchanged | — | yes |
| 0.9 | 2026-10-19 | step-0 baseline (`f792fc0`); `build-tests/compare/summary-baseline.json` | +6.3 / 4.9 / 8.2 vs 6.5 % / −15.3 | clean_flat level −0.4, FR full 11.3 | — |
| 1 | 2026-10-19 | output low-pass 12.06 kHz Q 0.5 | +7.8 / 4.9 / 8.2 vs 6.5 % / −15.3 | FR full better in 69 metrics; levels +0.7 dB (HF energy removed, step 4.1) | yes |
| 2.1 | 2026-10-19 | fixed noon voicing + LP refit | +6.9 / −5.2 / 5.2 vs 6.5 % / −11.1 | 197 better, 36 worse (explained in 2.1) | yes |
| 2.2 | 2026-10-19 | bass shelf per knob position | +7.1 / −5.5 / 5.4 vs 6.5 % / −10.2 | bass cases ±0.6 dB; 19 better, 8 worse (drive levels) | yes |
| 2.3 | 2026-10-19 | treble shelf + 19 kHz peak per knob position | +7.7 / −3.6 / 5.9 vs 6.5 % / −12.3 | treble ±0.9 dB; 24 better, 7 worse (drive/level/mid) | yes |
| 2.4 | 2026-10-19 | mid peak per selector × knob position | +7.5 / −4.0 / 6.0 vs 6.5 % / −12.3 | mid cases ±0.3 dB; 27 better, 1 worse (fxdefault FR, Tight missing) | yes |
| 2.6 | 2026-10-19 | bright / ultra hi / ultra lo refit | +7.5 / −3.6 / 6.1 vs 6.5 % / −14.2 | switch cases ±0.3 dB; 12 better, 0 worse | yes |
| 3 | 2026-10-19 | graphic EQ: proportional-Q law, measured centres, on-hump | +8.0 / −0.7 / 6.2 vs 6.5 % / −21.5 | EQ cases ±0.5 dB; 35 better, 4 worse (screenshot ramp/bursts, step 4/6) | yes |
| A | 2026-10-19 | **Milestone A passed**: user listened to `clean_flat`, `tone_shot`, `screenshot_preset gain=1 tubeGain=10` pairs (`probe.py listen`): "very close" | nulls −27.9 / −24.0 / −24.4 dB | — | — |
| 4.4 | 2026-10-19 | Master linear taper (0.5 × knob) | +9.5 / −0.7 / 6.2 vs 6.5 % / −21.5 | `law_master*` ±0.1 dB; 12 better, 13 worse (Gain 5 levels, 4.2) | yes |
| 4.2 | 2026-10-19 | Gain linear level law (19 × knob small-signal) | +3.8 / −0.7 / 7.6 vs 6.5 % / −21.5 | `law_gain*` ±0.1 dB; 77 better, 18 worse (noise = bias DC, step 7) | yes |
| 5 | 2026-10-19 | parameter contract: display 0–10, GEQ Level piecewise, defaults, stateVersion | unchanged | 0 / 0 | yes |
| 6.4+6.1 | 2026-10-19 | power stage (peak emphasis + static curve) + linear preamp, **uncommitted WIP** | Tube Gain 3.5 missing (6.2) | 173 better, 14 worse (all Tube Gain < 10, 6.2); Tube 10 THD 0.4/2.3/3.4 vs 0.3/1.9/3.1 %; DI null −26 dB | pending 6.2 |
| 6.2 try 1 | 2026-10-19 | envelope bias + power stage fit (`step6-fits/fittube.py`) | — | low-level H2 +2…7 dB, high-level compression half the reference's | no (model can't clip asymmetrically) |
| 6.2 try 2 | 2026-10-19 | kink + fixed and level-driven bias in the power stage (`fittube2.py`, `fit_per.json`) | — | Tube Gain 0 low-level H2 2.5–4.5 dB low at 41/100 Hz (DSP test FAIL) | no (superseded by try 3) |
| 6.4 | 2026-10-19 | power stage committed on its own (`9b84435`) | — (old preamp still in) | DSP tests 0 FAIL, 1 WARN (old preamp clipping at default Gain) | yes |
| 6.1+6.2 | 2026-10-19 | linear preamp + Tube Gain in the power stage: DC-blocked shelf kink, envelope-only bias (`fittube3.py`); one 10 Hz DC blocker at the end | +0.05 / 0.67 / 6.5 vs 6.5 % / −22.9 | 188 better, 3 worse (`screenshot_preset_fxdefault` = Power Soak, 9.2); Tube 10 THD 0.4/2.3/3.5 vs 0.3/1.9/3.1 %; noise −240 dBFS | yes |
| 6.6 | 2026-10-19 | filters in double precision (float32 limit cycles in the biquads, not aliasing) | unchanged | alias −93.8 → −125.9 dB (ref −125.0); gate 0 / 0 | yes |
| 6.5 try 1 | 2026-10-19 | separate attack/release for the blocking envelope (`fittube4.py`) | — | fit returns to 5 Hz / 5 Hz; loud → quiet choke still 16 dB too shallow | no (needs a cutoff-type element) |
| B | 2026-10-19 | **Milestone B passed**: user listened to `screenshot_preset`, `gain7.5_tube10`, `gain7.5_tube5`, `gain2.5_tube0`, `gain7.5_tube0` pairs: "they seem similar" | −22.9 dB null | Tube Gain 0 nulls −15.6…−19 accepted (choke, 6.5); ceiling 1–3 dB | — |
| 7.1 | 2026-10-19 | Tube Gain glide 50 → 200 ms; parameters set before prepare | unchanged | jump bump −31 → −48 dB rel. tone; silence exact; gate 0 / 0 | yes |
| 8.1 | 2026-10-19 | output low-pass per rate family | unchanged at 48 kHz | 44.1 / 96 / 192 kHz top end within 0.6 / 1.1 / 1.3 dB of the reference's change (was 1.3 / 6.5 / 7.0); gate 0 / 0 | yes |
| 11.1–11.2 | 2026-10-20 | Bass × Treble interaction peak (5 × 5 table); mid 2.5 / 7.5 refit at every position | unchanged | 625-point grid: worst 4.74 → 1.29 dB, > 2 dB 75 → 0; `mid0_bass10_treble10` 3.98 → 1.29; gate 0 worse / 2 better | yes |
| 9.2–9.3 | 2026-10-20 | Tight (first-order high-pass + makeup, Pre/Post) and Power Soak (output gain); defaults Post 50 Hz / −4 dB, `stateVersion` 4 | unchanged (FX off in amp cases) | gate 0 worse / 6 better; `screenshot_preset_fxdefault` level −4.13 → +0.04, FR −4.05 → −0.66; user's bass file at the same settings: level 4.2 → 0.35 dB, sub-bass 4.5 → 1.1 dB | yes |
| 12 | 2026-10-20 | cabinet stage: Rec Chain 20 + Horn, per-rate biquad fits at 1/24 octave; `cabinet` / `horn` parameters | unchanged (cabinet off in amp cases) | gate 0 / 0; `cab_clean_flat` FR 0.40 dB, `cab_horn_*` 1.2–1.6 dB; DI null with cabinet −4 dB (phase, 12.5) | yes |
| 11.3 | 2026-10-20 | knob tables every 0.5 / 0.25 (log Hz), interaction 9 × 9 | unchanged | single knobs anywhere ≤ 1.33 dB (were up to 5.0); off-grid combos ≤ 1.37 (were up to 6.0); gate 0 / 0; 5 new cases | yes |
| 8.2–8.4 | 2026-10-19 | `behaviour.py` + host `--set-at`; Bypass/Mute fades 10 → 50 ms, bit-exact bypass | unchanged | block sizes bit-exact; stereo dual mono like the reference; fades 40 vs 43 ms; gate 0 / 0 | yes |
| 8.5a | 2026-10-20 | user: "treble seems to add noise" → zipper noise while a filter knob moves (coefficients jumped every 32 samples): `Biquad::glideTo`, used by ToneStack / GraphicEQ / PreFilters; Tight glides too (and no longer skips its last update) | bit-identical when still | moving a knob, energy above 2 kHz vs held (98 Hz tone, 44.1 kHz, full chain): Treble +46 → +4 dB, Mid +34 → −1, Mid Freq +21 → −2, GEQ 150 Hz–9k +6…+28 → ≤ 0, Tight Freq +15 → 0 (reference ±5); still settings: AmpBass adds no more distortion or noise than the reference on the user's file at any treble; new DSP test; blocks bit-exact | — |
| 9.4 | 2026-10-20 | Smooth (matched analog one-pole, Pre/Post), default Post 21 kHz, `stateVersion` 5 | unchanged (FX off in amp cases) | gate 0 worse / 3 better: `factory_defaults` full-band FR 2.98 → 1.93 dB, `factory_screenshot_preset` 3.61 → 1.94; user's bass file at the R&B settings: spectrum rms 1.60 → 1.43 dB, worst 5.73 → 4.99 (10 kHz, where the reference's own offline render differs from the file by the same 5.6 dB) | — |
| 9.1 | 2026-10-20 | Input Gain (−20 … +20 dB pure pre-gain, like the reference's Input) | unchanged (0 dB) | gate 0 / 0; `input_+10_gain5` level −0.9, FR +2.5 dB (heavy-drive gap) | — |
| 9.5 | 2026-10-21 | Noise Gate (input-keyed, band-limited 32 ms detector, Range dB on the amp output), default on −88 dB / 50 dB, `stateVersion` 6 | unchanged (gate off in amp cases) | gate 0 worse / 0 better; all 430 non-factory AmpBass renders bit-identical; `gate_*` decay envelope 0.18–0.70 dB (gate off 0.19); factory cases with both gates on within tolerance | — |
