# CLAUDE.md — WebUI session

You are working on the **AmpBass plugin UI** only. A separate Claude session works
on the DSP in parallel on another branch. The repo-level `../CLAUDE.md` also applies.

## Goal
Rebuild the UI to match the reference plugin (Brainworx **Ampeg SVT-3 PRO**,
Plugin Alliance) following the spec:

- **Spec (authority for this work):** `../docs/ui/Ampeg SVT-3 Pro UI - Svelte Specification v2.md`
- **Reference screenshot** (not in git): `/Users/italo/Desktop/Temp Projects/amp-bass/building-process/references/UI-reference-AMPEG.png`
- **Reference plugin manual** (not in git): `/Users/italo/Desktop/Temp Projects/amp-bass/building-process/references/ampeg_svt3pro_manual.md`

Do not copy the screenshot, manual or Ampeg logo into the repo (third-party / trademark;
spec §12 #14).

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
| GEQ ±12 dB all bands, "9k" band (§12 #3–4) | per-band ±15/±8/±9/±10, ID `geq8k` | Show the printed scale from the spec, but read real ranges from the relay. Never rename `geq8k` (IDs are frozen). |
| LEVEL −10…+8 (§12 #5) | −10…+10 | Same as above. |
| Dry/Wet, Input HI/LO removed (§12 #6–7) | params `dryWet`, `inputHiLo` exist | Hide from faceplate; don't delete params. |
| `mute`, `ampPower`, toolbar `bypass` | don't exist | Placeholders + PARAM-REQUESTS entry. |
| PEAK/MUTE `clip` flag | no meter from C++ | Placeholder `clip = false` + request. |
| Defaults `bright` on, `geqOn` on (§7.3) | both off in C++ | Request; don't fake it in JS. |
| Presets A–D (§4, §8) | `presetSelect` int 1–4, no DSP | JS-only slots are lost when the DAW reloads the session — mark as placeholder + request persistence in plugin state. |

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
