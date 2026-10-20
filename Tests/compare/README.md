# AmpBass vs reference comparison

Renders the same test signals through AmpBass and the reference plugin with
`AmpBassHostRender`, then measures both. Outputs go to `build-tests/compare/` (not in git).

## One-time setup
1. Build the tools: `cmake --build build-tests` (see `docs/how-ampbass-works.md` §5).
2. Find the reference's parameters:
   ```bash
   H=build-tests/Tests/AmpBassHostRender_artefacts/Release/AmpBassHostRender
   $H --plugin ~/Library/Audio/Plug-Ins/VST3/<reference>.vst3 --list
   $H --plugin ... --probe "<param name>"     # how 0-1 maps to the displayed value
   ```
3. Fill in the `reference` section of `plugins.json` (path, parameter names, `as`)
   until no `TODO` is left. The baseline must select **Cabinet Bypass**, noise and
   gate off, Tight/Smooth off, Input Gain 0 dB.

## Each run
```bash
python3 Tests/compare/signals.py --di my_di_48k.wav   # once (DI optional, 48 kHz)
python3 Tests/compare/render.py                       # skips files that exist; --force to redo
python3 Tests/compare/analyze.py                      # -> build-tests/compare/report/report.md
```
After changing AmpBass, rebuild the plugin and re-render it only:
`python3 Tests/compare/render.py --plugins ampbass --force`.

Self-test (AmpBass on both sides, every difference must be 0):
`render.py --self-test --out build-tests/compare/selftest` then
`analyze.py --self-test --renders build-tests/compare/selftest`.

## Files
| File | Purpose |
|---|---|
| `signals.py` | sweeps, stepped tones, level ramp, bursts, silence, DI |
| `cases.py` | settings to compare, in faceplate units |
| `plugins.json` | how each plugin's parameters map to those units |
| `render.py` | batch rendering |
| `analyze.py` | frequency response, THD/harmonics, compression, dynamics, noise, DI null, aliasing, crosstalk; `--compare-to` regression gate |
| `probe.py` | measuring and fitting tools: FR rows, knob curves, shelf/peak fits, harmonics vs level, topology, reference nulls (see its docstring) |
| `behaviour.py` | step-8 checks with their own renders: sample rates (44.1 / 96 kHz vs 48), host block sizes (must null), stereo, Bypass / Mute fades (uses the host's `--set-at` automation) |
