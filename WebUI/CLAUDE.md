# CLAUDE.md — WebUI session

You are working on the **AmpBass plugin UI** only. A separate Claude session works
on the DSP in parallel on another branch. The repo-level `../CLAUDE.md` also applies.

## Goal
Rebuild the UI to match the reference plugin (Brainworx **Ampeg SVT-3 PRO**,
Plugin Alliance) following the spec:

- **Spec (authority for this work, not in git):** `/Users/italo/Desktop/Temp Projects/amp-bass/docs/ui/Ampeg SVT-3 Pro UI - Svelte Specification v2.md`
- **Reference screenshot** (not in git): `/Users/italo/Desktop/Temp Projects/amp-bass/building-process/references/UI-reference-AMPEG.png`
- **Reference plugin manual** (not in git): `/Users/italo/Desktop/Temp Projects/amp-bass/building-process/references/ampeg_svt3pro_manual.md`

Do not copy the spec, screenshot, manual or Ampeg logo into the repo, and never commit
`building-process/` (the user keeps these local; Ampeg is a trademark, spec §12 #14).

## Scope — what this session may touch
- ✅ Anything under `WebUI/` (except `src/bridge/juce/`, which is vendored JUCE code).
- ❌ `Source/`, `Tests/`, `CMakeLists.txt`, and in particular the **parameter files**:
  `Source/Parameters/ParameterIDs.h`, `ParameterLayout.h`, `Source/Plugin/NativeBridge.*`.
- ⚠️ `src/types/parameters.ts` mirrors `ParameterIDs.h`. Only add IDs there once the
  C++ parameter exists.

When the spec needs something on the C++ side, **don't implement it**. Build the UI
with a JS-only placeholder (clearly marked `// PLACEHOLDER: no C++ param yet`) and
add an entry to `WebUI/PARAM-REQUESTS.md` (what, why, spec section). The user will
schedule it.

## How the UI talks to the plugin (keep this working)
- The bridge is `src/bridge/bridge.ts` + `src/state/store.ts`, built on JUCE 8 relays.
  It is the single source of truth. Spec §8's `amp` store must be a **view on top of
  these stores**, not an independent copy, or knobs will stop moving the DSP.
- Relays work in **normalised 0–1** values. Convert for display (e.g. knob 0–10 =
  normalised × 10). For dB parameters read `start`/`end` from the relay's
  `SliderState.properties` instead of hard-coding ranges.
- `midFreq` is an int 1–5 exposed through `getComboIndex` / `setComboIndex` (index 0–4).
- Call `sliderDragStarted` / `sliderDragEnded` at gesture start/end (DAW automation and
  undo grouping, decision D8). Spec's undo snapshots should hook the same moments.
- `npm run dev` runs without the plugin: the bridge returns `null` and logs one
  warning. Controls must still render and move in that mode.

## Spec vs current parameters — known gaps (need user sign-off / C++ work)
| Spec item | Current plugin | Action in this session |
|---|---|---|
| GEQ ±12 dB all bands, LEVEL −10…+8 (§12 #3–5) | **Done in C++** (user confirmed). ID `geq8k` stays; label it "9k" if the spec says so | Read ranges from the relay; update any hard-coded scales in the old UI. |
| Dry/Wet, Input HI/LO removed (§12 #6–7) | params `dryWet`, `inputHiLo` exist | Hide from faceplate; don't delete params. |
| POWER rocker + toolbar power icon | **Done:** one toggle param `bypass` (also the host's bypass) | Bind both controls to `toggles.bypass`; ON LED = `!$bypass`. |
| MUTE | **Done:** toggle param `mute` | Bind to `toggles.mute`. |
| PEAK/MUTE LED (§7.4) | **Done:** read-only store `peak` in `state/store.ts` (C++ event, held ~300 ms) | LED lit = `$peak \|\| $mute`. In `npm run dev` `peak` stays false. |
| Defaults `bright` on, `geqOn` on (§7.3) | **User decision: everything off by default** | Follow C++ defaults; ignore the spec here. |
| Presets A–D (§4, §8) | `presetSelect` int 1–4, no DSP | **Out of scope for now** (user decision). Toolbar buttons may be visual stubs. |

## Svelte / tooling
- Svelte 5. Existing components use legacy `export let`; new components may use runes
  as in the spec. Use the `svelte-code-writer` / `svelte-core-bestpractices` skills and
  the Svelte MCP autofixer for every `.svelte` file.
- This is not SvelteKit: `$lib` needs an alias in `vite.config.ts` **and** `tsconfig.json`,
  or use relative imports.
- The production build is a single inlined `index.html` (`vite-plugin-singlefile`).
  Fonts via `@fontsource/*` are fine (they get inlined); mention new dependencies.
- Before each commit: `npm run build` (runs `tsc`) and `npx svelte-check` must pass
  (svelte-check had pre-existing errors — don't add new ones).

## Testing inside the plugin
Iterate with `npm run dev`. To try the UI in the real plugin, build in this worktree
with copying **off**, so the installed VST3/AU that Ableton loads isn't overwritten:

```bash
cd .. && cmake -S . -B build -DAMPBASS_COPY_PLUGIN=OFF \
  -DFETCHCONTENT_SOURCE_DIR_JUCE="/Users/italo/Desktop/Temp Projects/amp-bass/build/_deps/juce-src"
cmake --build build --target AmpBass_Standalone
open build/AmpBass_artefacts/Standalone/AmpBass.app
```

## Git
- Branch `feat/ui-layout`. Never commit to `main`; don't force-push `dev`/`main`.
- Commit format `<type>: <description>` (feat, fix, refactor, docs, test, chore, perf, ci),
  ending with the Co-Authored-By trailer.
- Merging to `dev` needs a one-line `CHANGELOG.md` entry. The DSP session also merges
  to `dev`; rebase on `dev` before merging.
