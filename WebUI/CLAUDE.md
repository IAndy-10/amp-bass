# CLAUDE.md — WebUI

UI conventions for the AmpBass faceplate. The repo-level `../CLAUDE.md` also applies.

## References (local only, never commit)
- UI spec v2.1: `../docs/ui/Ampeg SVT-3 Pro UI - Svelte Specification v2.md` — layout, coordinates, tokens, interactions.
- Reference screenshot and plugin manual: `../building-process/references/`.

Don't copy the spec, screenshot, manual or any Ampeg logo into the repo (trademark).

## Structure
- `src/lib/components/` — faceplate components. Positions are in reference pixels
  (2562 × 558 canvas) scaled with `--u` (container query units); see spec §2.
- `src/lib/stores/amp.ts` — display units (knobs 0–10, dB, Mid Frequency 1–5), gestures, snapshots.
- `src/lib/stores/history.ts` — JS undo/redo; `overlay.ts` — tooltip and context-menu state.
- `src/state/store.ts` — stores synced with C++ (`sliders`, `toggles`, `combos`, read-only `peak`).
- `src/bridge/bridge.ts` — relay access; `src/bridge/juce/` is vendored JUCE code, don't edit.

## Rules
- **The plugin owns every value.** Components read the stores and write through `amp.ts`
  (`setNumeric`, `commitNumeric`, `setFlag`, `setMidFreqPos`). Never keep a local copy.
- Relays are normalised 0–1. dB ranges come from the relay's `properties` (`rangeOf`).
- Wrap drags in `beginGesture` / `endGesture` so the DAW records one automation gesture and
  one undo step.
- Parameter IDs are frozen. Adding a parameter touches C++ (`ParameterIDs.h`, `ParameterLayout.h`,
  `NativeBridge.*`) and `src/types/parameters.ts` + `src/state/store.ts` together.
- Svelte 5 runes. Use the `svelte-code-writer` / `svelte-core-bestpractices` skills and the Svelte
  MCP autofixer for every `.svelte` file. `$lib` is a Vite + tsconfig alias (not SvelteKit).
- Before committing: `npm run build` and `npx svelte-check` (0 errors).

## Testing in the plugin
`npm run dev` has no audio and every control starts at 0. To check wiring, build the plugin and
open the Standalone (`build/AmpBass_artefacts/Standalone/AmpBass.app`) or reload it in the DAW.
