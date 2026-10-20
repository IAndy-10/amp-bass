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

- [ ] **5.1 Normalised ↔ value scales** match the reference: knobs linear 0–10, Mid Freq 5 steps, GEQ ±12 linear,
  **GEQ Level −10…0…+8 piecewise** (the reference puts 0 dB at 0.5; AmpBass's linear range puts −1 dB there).
- [ ] **5.2 Display text**: knobs shown as 0.0–10.0 (AmpBass shows 0–1), dB values with one decimal, switches On/Off,
  Mid Freq 1–5. Also make AmpBass's `getValueForText` work, so the harness can use `text` for both plugins.
- [ ] **5.3 Defaults** match a fresh reference: Gain 3.4, Mid Freq 1, others as listed in Context. Changing ranges and
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
gets in step 9.2; its old match was AmpBass being 3.8 dB too loud). **Next: action 6 below.** The rest of this section is
how 6.2 was found, kept for reference.

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
- [ ] **6.3 Frequency dependence**: AmpBass is too dirty at 400 Hz at every level. Look at the filtering before and after the shaper.
- [x] **6.4 Power section = main distortion stage.** (`9b84435`) A linear zone, then a knee at the output level where the reference's
  H3 appears (about −30 dBFS fundamental peak, independent of Master), rising about 1.5–2 dB per dB above it. Check with
  the `clean_master*` cases (0.8) and `probe.py harmonics master=…`, then the ceiling and Master distortion. Target: ramp curve
  ±1 dB, burst levels ±1 dB.
- [ ] **6.5 Dynamics**: attack/release of the `PowerSection` compressor from the burst envelopes (settle times ±20 %).
- [ ] **6.6 Aliasing**: add oversampling around the shapers only if AmpBass aliases more than the reference. If added,
  report the latency (`setLatencySamples`) and re-check the null alignment.
- [ ] **6.7** DI null ≤ −20 dB on the exact-setting drive cases, and `screenshot_preset` ≤ −20 dB.
  **Milestone B: sound matches when driven.** (listen)

## Step 7 — Silence and DC

- [ ] **7.1** *(2026-10-19: the preamp bias is gone since 6.1; all 28 cases with a silence render now read −240 dBFS.
  Left: check there's no step when Tube Gain is automated.)* The "noise" is DC from the preamp bias, not hiss. `TubePreamp` adds `0.1 * (1 - headroom)` before the
  shaper, so at silence the shaper outputs a constant that the 10 Hz DC blocker only removes slowly. That's why every
  Tube Gain 10 case reads −240 dBFS and the others read −104 to −111 (louder at high Master). It also means a thump
  and DC settling when Tube Gain moves. Subtract the shaper's at-rest output (bias through the shaper) so silence in gives
  exactly silence out. Target: ≤ −140 dBFS, and no step when Tube Gain is automated.

## Step 8 — Behaviour

- [ ] **8.1 Sample rate**: `screenshot_preset` and `clean_flat` at 44.1 and 96 kHz, within 0.5 dB of the 48 kHz
  result in both plugins.
- [ ] **8.2 Block size**: render with `--block 32`, `512` and `4096`. AmpBass output must null between them (< −100 dB).
  The 32-sample coefficient updates must not depend on the host block.
- [ ] **8.3 Stereo**: same channel handling as the reference (from 0.8).
- [ ] **8.4 Bypass and Mute**: render each plugin with each switched on. Compare the output (dry, silent, latency-
  compensated?) and whether the switch ramps or hard-cuts.
- [ ] **8.5 PEAK/MUTE LED** *(code check 2026-10-19: the UI already lights it on Mute, `Faceplate.svelte` `$peak || $mute`; only the DAW check is left)* (in a DAW, not measurable offline): per the manual it lights on input overload, high Gain,
  **or too much tone boost**, and **whenever Mute is on**. Check that AmpBass's LED logic (`PluginProcessor.cpp`, PEAK block)
  also lights on Mute.
- [ ] **8.6 Knob moves** (in a DAW): sweep every control during playback in both plugins. No zipper noise or clicks.
  Mid Freq and switches change without clicks.
- **Milestone C: behaviour matches.**

## Step 9 — Reference parameters AmpBass lacks (new IDs only)

One parameter per step: DSP + `ParameterIDs.h`/`ParameterLayout.h` + TS mirror + `plugins.json` map + a case that
uses the per-case reference settings (0.7). No faceplate UI unless the user asks.

- [ ] **9.1** Input gain (±20 dB, linear in dB like the reference).
- [ ] **9.2** Power Soak (−40 … 0 dB). Its model depends on 0.4. Re-check the `master*` cases.
- [ ] **9.3** Tight (Off/Pre/Post + frequency). Probe the reference's frequency range and slope first.
- [ ] **9.4** Smooth (Off/Pre/Post + frequency). Same.
- [ ] **9.5** Noise gate (on/off, Threshold, Range), plus a gate case with a decaying note.
- [ ] **9.6** `screenshot_preset_fxdefault` (Tight Post 50 Hz, Power Soak −4) passes the Milestone B targets.
- Horn and Rec Chain wait for the cabinet phase.

## Step 10 — Presets

- [ ] **10.1** Get exact values for the 4 presets (R&B, Rock, Synth, Upright), including the FX Rack / Rec Chain.
- [ ] **10.2** Add one compare case per preset (cabinet bypassed, with the preset's own FX settings via 0.7).
  Each must pass the Milestone B targets.
- [ ] **10.3** Preset table in C++ applied via the existing `presetSelect` parameter (frozen ID), setting all parameters
  in one go. Note that the reference's A/B/C/D are settings *slots*, not factory presets, so behaviour differs on purpose.
- [ ] **10.4** Prominent preset selector in the UI as its own component (so it moves to the later minimal UI).

## Later — Cabinet

User will compare with the cabinet next. Only the Rec Chains used by the 4 presets (+ Horn) need modelling.
Use per-case reference settings (0.7) instead of the global baseline.

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
