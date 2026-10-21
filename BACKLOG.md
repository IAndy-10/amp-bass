# Backlog

Open work that isn't scheduled yet. One line per item; move it to an issue or a
branch when someone starts on it, and delete it here when it's done.
Priority: **P1** = affects sound or correctness, **P2** = visible to users, **P3** = polish/cleanup.

## Needs manual verification (UI + wiring, after 2026-10-17 merge)
- [ ] **P1** DAW automation and session recall move the knobs, sliders, toggles and FREQUENCY (that they change the sound: checked 2026-10-21).
- [ ] **P2** Layout within 4 ref px of spec §2.4 at 1280, 1600 and 1800 px wide (spec §13).

## UI
- [ ] **P2** Context menu can spill past the right/bottom edge of the window — clamp it inside.
- [ ] **P2** Toolbar stubs (Copy, Paste, Reset, About) show hover styling although disabled.
- [ ] **P2** Wheel notches and arrow-key presses are separate undo steps — merge steps that happen close together.
- [ ] **P3** Caption sizes: knobs 24 / toggles 20 ref px instead of the spec's 28 (avoids MIDRANGE/FREQUENCY overlap) — needs sign-off.
- [ ] **P3** Chassis colour (neutral charcoal tokens) — needs sign-off against the reference screenshot (spec §12 #2).
- [ ] **P3** Wordmark is placeholder text — replace with a cleared SVG; trademark question open (spec §12 #14).
- [ ] **P3** In `npm run dev` every control starts at 0 (no backend) — seed dev-mode stores with plugin defaults.

## DSP
- [ ] **P1** Treble: the user hears a steady hiss in Ableton that grows with Treble and is gone at 0; not in the reference
  (2026-10-21, user to review later). Offline, both plugins boost DI hiss identically (within 0.3 dB per band to 16 kHz,
  `build-tests/compare/treble2/hiss.py`) and both gates close on quiet hiss. Not yet ruled out: the AmpBass instance's
  gate off (sessions before stateVersion 6), or settings that differ between the two tracks. Needs bounced clips.

## Sanity check (2026-10-17)

### Bugs
- [ ] **P3** Hidden parameters still exposed to the host: `inputHiLo` (LO = −6 dB, no faceplate control) and `presetSelect` (does nothing). A recalled session with LO on is invisible in the UI. Decide: hide from host, wire up, or document.
- [ ] **P3** Right-click: VST3 hosts that offer a parameter menu get one native menu with the host's items and ours.
  Ableton doesn't offer it (user, 2026-10-21: page menu only; automation via the device's parameter list). Copy/Paste go
  through the plugin's system clipboard: check in an AU host.

### Build / platform
- [ ] **P3** `BuildSvelteUI` runs `npm install` on every build (slow, needs network). Use `npm ci` only when `package-lock.json` changes, or run it at configure time.
- [ ] **P3** Editor asks for the WebView2 backend, but `NEEDS_WEBVIEW2` isn't set in CMake: a Windows build would fall back or fail. Mac-only today.

### Performance (CPU only, not audible)
- [ ] **P3** ToneStack, PreFilters and GraphicEQ recompute every biquad's coefficients every 32 samples even when no parameter is moving (9 GEQ bands × 2 ch + 5 + 3 filters). Skip when the smoothed values aren't changing. `ToneStack` also recomputes the mid filter twice per update (`setGainDb` + `setFrequency`).
- [ ] **P3** GraphicEQ runs all 9 bands while EQ is off.

### Tests

## Later (decided, not started)
- [ ] Presets A–D with Copy/Paste/Reset, persisted in plugin state.
- [ ] Cabinet: more Rec Chains (the presets' ones, sound-matching plan 10.1) and/or a user-loaded IR. Today: the
  reference's default Rec Chain 20 + Horn only (plan step 12).
- [ ] UI: FX Rack panel is a minimal first version (toolbar FX Rack, plain sliders, `FxRack.svelte`): style it like the
  faceplate, add the gate's CLOSED LED like the reference, and put its changes in the undo history.
