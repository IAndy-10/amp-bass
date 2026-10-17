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
- [ ] **P3** Delete unused `src/components/Neu*`, `components/Tooltip.svelte`, `stores/tooltip.ts` (all 31 svelte-check errors).
- [ ] **P3** In `npm run dev` every control starts at 0 (no backend) — seed dev-mode stores with plugin defaults.

## DSP
- [ ] **P2** Start-up thump: preamp bias offset settles through the DC blocker in the first ~0.2 s (≈ −31 dBFS). Start the DC blocker in steady state.
- [ ] **P2** Mid Frequency is smoothed linearly in Hz (CLAUDE.md asks for logarithmic).
- [ ] **P3** No oversampling around the preamp — aliasing at high GAIN.

## Later (decided, not started)
- [ ] Presets A–D with Copy/Paste/Reset, persisted in plugin state.
- [ ] Power amp model + Power Soak (Master drives a power stage).
- [ ] Cabinet: built-in cab EQ and/or user-loaded IR, with bypass.
- [ ] Missing reference controls: Input Gain (dB), Tight/Smooth filters (pre/post), Noise Gate.
