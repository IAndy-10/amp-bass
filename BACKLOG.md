# Backlog

Open work that isn't scheduled yet. One line per item; move it to an issue or a
branch when someone starts on it, and delete it here when it's done.
Priority: **P1** = affects sound or correctness, **P2** = visible to users, **P3** = polish/cleanup.

## Needs manual verification (UI + wiring, after 2026-10-17 merge)
- [ ] **P1** New faceplate fills the plugin window; resizing keeps the 2562:558 aspect.
- [ ] **P1** Knobs, GEQ sliders, toggles and FREQUENCY change the sound; DAW automation and session recall move them.
- [ ] **P1** Undo: one wheel step / click is undoable; one whole drag is one undo step.
- [ ] **P1** POWER rocker and toolbar icon switch together and pass the dry signal; MUTE silences.
- [ ] **P2** PEAK LED lights on hard playing with high GAIN/BASS, holds ~300 ms, stays off while bypassed.
- [ ] **P2** Layout within 4 ref px of spec §2.4 at 1280, 1600 and 1800 px wide (spec §13).

## UI
- [ ] **P2** Context menu can spill past the right/bottom edge of the window — clamp it inside.
- [ ] **P2** Toolbar stubs (Presets A–D, Copy, Paste, Reset, FX Rack, About) show hover styling although disabled.
- [ ] **P2** Wheel notches and arrow-key presses are separate undo steps — merge steps that happen close together.
- [ ] **P3** Caption sizes: knobs 24 / toggles 20 ref px instead of the spec's 28 (avoids MIDRANGE/FREQUENCY overlap) — needs sign-off.
- [ ] **P3** Chassis colour (neutral charcoal tokens) — needs sign-off against the reference screenshot (spec §12 #2).
- [ ] **P3** Wordmark is placeholder text — replace with a cleared SVG; trademark question open (spec §12 #14).
- [ ] **P3** In `npm run dev` every control starts at 0 (no backend) — seed dev-mode stores with plugin defaults.

## DSP
- [ ] **P2** Start-up thump: preamp bias offset settles through the DC blocker in the first ~0.2 s (≈ −31 dBFS). Start the DC blocker in steady state.
- [ ] **P2** Mid Frequency is smoothed linearly in Hz (CLAUDE.md asks for logarithmic).
- [ ] **P3** No oversampling around the preamp — aliasing at high GAIN.

## Sanity check (2026-10-17)

### Bugs
- [ ] **P2** Keyboard control probably never reaches the UI inside a DAW: `CMakeLists.txt` sets `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`, so arrow keys, Home/End and Cmd/Ctrl+Z (`Plugin.svelte`) may go to the host instead. Verify in Ableton; set it TRUE if they don't work.
- [ ] **P2** `dryWet` is applied without smoothing (`DryWetMixer::setMix`), so automating it clicks. It is hidden in the UI but still automatable from the DAW. Smooth it (CLAUDE.md: every float param uses SmoothedValue) or remove it from the host.
- [ ] **P3** PEAK check short-circuits: when the input check fires, `tubePreamp.takePeakDrive()` isn't called, so the stale drive peak carries into the next sub-block (`PluginProcessor.cpp`, overload expression). Call `takePeakDrive()` first, unconditionally.
- [ ] **P3** Hidden parameters still exposed to the host: `inputHiLo` (LO = −6 dB, no faceplate control), `dryWet`, and `presetSelect` (does nothing). A recalled session with LO on is invisible in the UI. Decide: hide from host, wire up, or document.
- [ ] **P3** Clipboard (context menu Copy/Paste) may be blocked in the plugin WebView; it falls back to an in-plugin clipboard silently. Verify in the plugin.

### Legacy / dead code
- [ ] **P3** Old UI, unused since the new faceplate: `WebUI/src/components/` (NeuButton, NeuKnob, NeuKnobDiscrete, NeuNumber, NeuPanel, NeuSelector, NeuSlider, NeuXYPad, Tooltip, header) and `WebUI/src/stores/tooltip.ts`. Holds all 31 svelte-check errors.
- [ ] **P3** Unused DSP primitives: `Source/DSP/Primitives/DelayLine.h`, `LFO.h`, `OnePoleFilter.h`, `SmoothedParam.h` (only listed in `CMakeLists.txt`; stages use `juce::SmoothedValue` and inline one-poles).
- [ ] **P3** Unused npm packages: `three`, `@types/three`, `vite-plugin-glsl` (still loaded in `vite.config.ts`), `tailwindcss`, `postcss`, `autoprefixer`.
- [ ] **P3** `copy_resources_to_target` in `CMakeLists.txt` copies `WebUI/dist` next to the bundles, but the UI is embedded via BinaryData; for the `AmpBass` shared-code target it writes into `build/AmpBass_artefacts/../Resources`. Remove.
- [ ] **P3** Worktree `../amp-bass-ui` and branches `feat/ui-layout`, `feat/bridge-and-dsp-fixes` are fully merged into `dev`; remove when no longer needed.

### Build / platform
- [ ] **P3** `BuildSvelteUI` runs `npm install` on every build (slow, needs network). Use `npm ci` only when `package-lock.json` changes, or run it at configure time.
- [ ] **P3** Editor asks for the WebView2 backend, but `NEEDS_WEBVIEW2` isn't set in CMake: a Windows build would fall back or fail. Mac-only today.
- [ ] **P3** `COMPANY_NAME "YourCompany"` placeholder in `CMakeLists.txt` (shows in hosts and bundle IDs). Set before any release; changing it later changes the AU/VST3 identity.

### Performance (CPU only, not audible)
- [ ] **P3** ToneStack, PreFilters and GraphicEQ recompute every biquad's coefficients every 32 samples even when no parameter is moving (9 GEQ bands × 2 ch + 5 + 3 filters). Skip when the smoothed values aren't changing. `ToneStack` also recomputes the mid filter twice per update (`setGainDb` + `setFrequency`).
- [ ] **P3** GraphicEQ runs all 9 bands while EQ is off.

### Tests
- [ ] **P3** The `Chain` mirror in `Tests/DSPTests.cpp` doesn't include `OutputSwitch` or the peak logic, so it can drift from `processBlock`. Consider testing the processor itself instead of a mirror.
- [ ] **P3** The comparison self-test (`Tests/compare`, AmpBass vs AmpBass must null) isn't automated.

### Docs out of date
- [ ] **P3** `README.md`: says 24 parameters (now 28), GEQ to 8 kHz, "neumorphic components", old project tree (no `lib/`, `Tests/`, `OutputSwitch`), HI/LO input.
- [ ] **P3** `CLAUDE.md`: says "No test suite exists yet", 24 parameters, Three.js shader layer, Neumorphic library, SmoothedParam usage; omits Bypass/Mute, Tests, compare pipeline, BACKLOG.
- [ ] **P3** `WebUI/CLAUDE.md` is the brief for the finished parallel UI session (spec gaps table, worktree build steps). Trim to current UI conventions.

## Later (decided, not started)
- [ ] Presets A–D with Copy/Paste/Reset, persisted in plugin state.
- [ ] Power amp model + Power Soak (Master drives a power stage).
- [ ] Cabinet: built-in cab EQ and/or user-loaded IR, with bypass.
- [ ] Missing reference controls: Input Gain (dB), Tight/Smooth filters (pre/post), Noise Gate.
