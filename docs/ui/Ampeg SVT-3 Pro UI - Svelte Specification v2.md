# Ampeg SVT-3 Pro UI, Svelte Specification v2

*Expanded from v1 and checked against the reference screenshot UI-reference-AMPEG.png (2562 × 558 px). Coordinates are in reference pixels unless stated otherwise. They were read by eye from the screenshot, so expect about ±4 px.*

## 0. How to read this spec

The reference screenshot is the authority for layout, labels, control set and color. Where v1 disagrees with the screenshot, v2 follows the screenshot. Where v1 gives parameter ranges that the screenshot cannot confirm, v2 sets a working default and lists the item in section 12 for sign-off. Textures and 3D shading are secondary. Flat gradients and simple shadows meet the requirement throughout.

## 1. Architecture

The app is a Svelte 5 project built with Vite and no UI kit. All parameter state lives in one shared store, amp, keyed by parameter name. Components never keep their own copy of a value. They read from the store and write through the helpers in section 8. The plugin is one self-contained faceplate that scales with its container, so the layout is written once, in reference pixels.

## 2. Layout and scaling

### 2.1 Design canvas

The full plugin, toolbar included, is 2562 × 558 reference pixels, about 4.59 to 1. v1 specified 1350 × 460, about 2.93 to 1. That does not match the reference, so it is retired. The plugin keeps its aspect ratio at every width, is capped at 1800 px and centered. Below 960 px the stage scrolls sideways rather than shrinking the controls further.

### 2.2 Units

Write every size in reference pixels and scale them with container query units:

```css
.stage { container-type: inline-size; width: 100%; }
.plugin { --u: calc(100cqw / 2562); width: 100cqw; height: calc(558 * var(--u)); position: relative; }
/* usage */
.knob { width: calc(76 * var(--u)); height: calc(76 * var(--u)); }
.caption { font-size: calc(28 * var(--u)); }
```

Do not scale the whole plugin with transform: scale(). It works, but it complicates hit areas and text layout. Keep it only as a fallback for browsers without container query units.

### 2.3 Positioning rule

Unless noted, positions in section 2.4 are center points. Place each element with left: calc(X \* var(--u)), top: calc(Y \* var(--u)) and transform: translate(-50%, -50%). Boxes given as two corners are placed by their top-left corner and size.

### 2.4 Coordinates

Chassis and toolbar:

| Element | Center or box | Size | Notes |
| --- | --- | --- | --- |
| Toolbar band | box 0,0 to 2562,60 | 2562 × 60 | black, full width |
| Chassis body | box 0,60 to 2562,558 | full | brushed metal shell |
| Faceplate panel | box 196,236 to 2352,478 | 2156 × 242 | recessed, textured |
| Ampeg wordmark | 1830, 145 | 280 × 46 | logo asset or placeholder |
| Model name SVT-3 PRO | 1826, 192 | 270 × 36 | light caps |
| Top center screw | 1283, 160 | Ø 28 | decorative |
| Bottom center screw | 1280, 497 | Ø 24 | decorative |
| Left grip handle | 136, 305 | 124 × 380 | decorative |
| Right grip handle | 2428, 305 | 124 × 380 | decorative |
| Corner mounting holes | 50,100 · 2512,100 · 45,500 · 2512,500 | Ø 36 | decorative |

Knobs and selector:

| Element | Center | Size | Notes |
| --- | --- | --- | --- |
| Knob captions | y 292, x as knob | 28 | GAIN, BASS, MIDRANGE, FREQUENCY, TREBLE, MASTER, TUBE GAIN |
| Knobs | x 413, 632, 780, 926, 1076, 1222, 1372; y 380 | Ø 76 | same order as captions |
| Knob end numerals 0 and 10 | knob x ± 29, y 435 | 22 | not on FREQUENCY |
| FREQUENCY numerals 1 to 5 | x 880, 903, 926, 948, 972; y 330 | 22 | printed above the selector |

Toggle columns:

| Element | Center | Size | Notes |
| --- | --- | --- | --- |
| BRT LED | 228, 308 | Ø 14 | BRT caption at 270, 310, above the button |
| BRT button | 271, 351 | 56 × 30 | upper button of left pair |
| PEAK/MUTE caption | 336, 380 | two lines, 20 | read-only label |
| PEAK/MUTE red LED | 335, 425 | Ø 14 | read-only indicator |
| -15dB button | 271, 413 | 56 × 30 | lower button of left pair |
| -15dB LED | 228, 449 | Ø 14 | caption -15dB at 268, 450, below the button |
| HI LED | 481, 310 | Ø 14 | HI caption at 520, 310, above the button |
| HI button | 522, 348 | 56 × 30 |  |
| LO button | 522, 413 | 56 × 30 |  |
| LO LED | 481, 448 | Ø 14 | LO caption at 520, 450, below the button |
| MUTE LED | 1450, 310 | Ø 14 | MUTE caption at 1495, 310, above the button |
| MUTE button | 1494, 348 | 56 × 30 |  |
| EQ button | 1494, 413 | 56 × 30 |  |
| EQ LED | 1450, 448 | Ø 14 | EQ caption at 1490, 450, below the button |

Power section:

| Element | Center | Size | Notes |
| --- | --- | --- | --- |
| ON LED | 2260, 290 | Ø 16 | ON caption at 2297, 288 |
| POWER rocker | 2282, 366 | 84 × 106 | POWER caption at 2285, 440 |

Graphic EQ bank:

| Element | Center | Size | Notes |
| --- | --- | --- | --- |
| Band captions | y 260, centered on each track | 22 | 33Hz, 80, 150, 300, 600, 900, 2k, 5k, 9k |
| Track centers | x 1614, 1673, 1732, 1791, 1850, 1909, 1968, 2027, 2086 for bands; x 2145 for LEVEL | pitch 59 | ten tracks in total |
| Track span | y 282 to 432 | 150 tall | bands read +12 at top, 0 at 357, -12 at bottom |
| Band scale | +12dB at 1563, 285; 0 at 1585, 357; -12dB at 1565, 430 | 20 | left of the band field |
| LEVEL scale | +8dB at 2195, 285; LEVEL at 2203, 357; -10dB at 2195, 430 | 20 | right of the LEVEL track |
| Recessed GEQ field | box 1590,262 to 2165,446 |  | darker inset behind the sliders |

Treat these as design coordinates. Re-measure against the source artwork when it is available.

## 3. Visual language

### 3.1 Color tokens

Colors were read from the screenshot by eye. Tune them in the browser against the screenshot at 100% zoom.

| Token | Value | Used for |
| --- | --- | --- |
| --ink-strong | #f2f2f2 | captions, pointers, wordmark |
| --ink-soft | #cdd0d4 | scale numerals, secondary captions |
| --toolbar-bg | #000000 | top band |
| --toolbar-text | #e6e6e6 | toolbar labels and icons |
| --accent-blue | #3ea6e6 | active preset, power icon when the plugin is active |
| --led-on | #9cf03c | lit green LEDs |
| --led-off | #2a3a28 | unlit LEDs |
| --led-red | #e5392b | PEAK/MUTE lit |
| --led-red-off | #4a1c19 | PEAK/MUTE unlit |
| --shell-dark | #26292d | chassis sides and grip handles |
| --shell-mid | #3b3f44 | brushed top strip base |
| --faceplate | #4b4f55 | faceplate base, speckle to #5c6168, shade to #393c41 |
| --groove | #0d0e10 | knob rings, slider grooves |
| --knob-face | radial #34373c to #0f1012 | knob body |
| --button-face | #17191c to #2a2d32 | push-button body |
| --button-edge | #8f949a | push-button outline |
| --thumb | #d8dadd to #8c9096 | GEQ slider cap |
| --field-inset | #33363b | recessed GEQ field |

The screenshot reads as a cool, neutral charcoal. The v1 base of #3d3632 is a warm brown-gray. v2 uses the neutral tokens above. See section 12.

### 3.2 Typography

| Use | Font | Size (ref px) | Style |
| --- | --- | --- | --- |
| Control captions (GAIN, BRT, MUTE and so on) | DM Sans | 28 | 600, italic, uppercase, text-shadow 0 1px 0 rgba(0,0,0,0.7) |
| Scale numerals (0, 10, +12dB) | DM Sans | 20 to 22 | 500, --ink-soft |
| GEQ band captions | DM Sans | 22 | 600, italic |
| Toolbar | DM Sans | 28 | 400 |
| SVT-3 PRO | DM Sans | 40 | 300, letter-spacing 0.08em |
| Ampeg wordmark (placeholder) | DM Serif Display | 60 | italic |

Load fonts from @fontsource packages rather than a CDN, so the plugin works offline.

### 3.3 Materials

Flat gradients are enough. Use a vertical gradient on the chassis, a radial gradient on each knob, and a 1 px light edge on buttons. Brushed streaks, faceplate speckle and knob bevels are optional polish and should be added last, if at all.

## 4. Top toolbar

The toolbar sits above the chassis. It is 60 reference px high, black, and uses text-style controls without boxes.

| Control | Position (x, y) | Type | Behavior | Store key |
| --- | --- | --- | --- | --- |
| Power icon | 18, 30 | icon toggle, Ø 30 | Plugin bypass. Blue when active, gray when bypassed | bypass |
| Undo | 870, 30 | icon button | Steps back one history entry | history |
| Redo | 930, 30 | icon button | Steps forward one history entry | history |
| Preset A, B, C, D | 1050, 1112, 1172, 1232 at y 30 | radio group | Switches the active slot. Active slot shows in --accent-blue | currentPreset |
| Copy | 1373, 30 | text button | Copies the current preset slot to the preset clipboard | presetClipboard |
| Paste | 1506, 30 | text button | Replaces the current slot with the clipboard contents | presets |
| Reset | 1667, 30 | text button | Resets the current slot to factory defaults | presets |
| FX Rack | 1848, 30 | text button | Emits open-fx-rack. Stub in this build | none |
| About | right edge 2552, 30 | text button | Opens an About dialog. Stub in this build | none |

Hover turns the text white and adds a 2 px underline. Focus shows a blue outline.

## 5. Chassis

The chassis is the metal body below the toolbar. The top strip, from y 60 to 232, is brushed charcoal and carries the wordmark and model name at the right. The faceplate sits inset from y 236 to 478 with a 4 px dark bevel. Grip handles hang at both ends and protrude past the chassis edge. The corner holes and two center screws are decorative and have pointer-events set to none. Chassis.svelte draws the shell and accepts the faceplate as a slot. It has no interactive behavior.

## 6. Faceplate controls: visual states

| Control | Default look | States |
| --- | --- | --- |
| Knob | Radial body inside a 3 px groove ring. Pointer is one line from 30% to 92% of the radius, 4 px wide, rotated by --angle. Short tick at 12 o'clock | Hover: ring brightens to --button-edge. Dragging: ring glows and the tooltip stays pinned. Focus: blue outline |
| FrequencySelector | Knob body with five detents at -135, -67.5, 0, 67.5 and 135 degrees. Numerals 1 to 5 above | Detent angle for position p is -135 + (p - 1) × 67.5 |
| ToggleButton | 56 × 30 push-button in --button-face with a 2 px --button-edge | On: paired LED lit, face slightly darker at top. Pressed: inner shadow 0 2px 3px rgba(0,0,0,0.6) |
| Led | 14 px dot | On: --led-on with glow 0 0 8px 2px rgba(156,240,60,0.55). Off: --led-off, no glow |
| Peak/Mute LED | Same as Led, red | Read-only, aria-hidden, driven by the derived store peakMute |
| PowerSwitch | 84 × 106 black rounded rocker, marked I at the top when on | Click toggles ampPower. ON LED mirrors it |
| GeqSlider | 6 px groove with fine lines every 3 dB. 24 × 16 px cap, 2 px radius, gradient from --thumb | When geqOn is false: sliders and captions drop to 40% opacity, the cap goes flat gray, pointer-events is none, aria-disabled is true. Values are kept |

## 7. Parameter inventory

The ranges and defaults below are the working defaults. Section 12 lists what still needs sign-off.

### 7.1 Knobs

| Key | Faceplate label | Range | Step | Default | Display |
| --- | --- | --- | --- | --- | --- |
| gain | GAIN | 0 to 10 | 0.1 | 5.0 | one decimal |
| bass | BASS | 0 to 10 | 0.1 | 5.0 | one decimal |
| midrange | MIDRANGE | 0 to 10 | 0.1 | 5.0 | one decimal |
| treble | TREBLE | 0 to 10 | 0.1 | 5.0 | one decimal |
| master | MASTER | 0 to 10 | 0.1 | 5.0 | one decimal |
| tubeGain | TUBE GAIN | 0 to 10 | 0.1 | 5.0 | one decimal |

All six knobs are the same diameter in the screenshot, MASTER included. v1 called MASTER prominent or large. v2 keeps it normal and leaves the large variant available as a prop. The knob pointers in the screenshot sit at different angles. They show one captured state, not defaults.

### 7.2 Frequency selector

Key midFreq. Stored as an integer from 1 to 5, default 3. Position 1 is 220 Hz, 2 is 450 Hz, 3 is 800 Hz, 4 is 1.6 kHz and 5 is 3 kHz. The faceplate prints 1 to 5, as the screenshot does. The Hz value appears in the tooltip and in exported presets.

### 7.3 Toggles

| Key | Faceplate caption | Default | Notes |
| --- | --- | --- | --- |
| bright | BRT | on | matches the screenshot |
| inputPad | -15dB | off | input pad |
| ultraHi | HI | off | v1 called this Ultra HI |
| ultraLo | LO | off | v1 called this Ultra LO |
| mute | MUTE | off | new in v2 |
| geqOn | EQ | on | bypasses all ten GEQ controls when off |
| ampPower | POWER (ON LED) | on | new in v2. Faceplate rocker |
| bypass | toolbar power icon | off | new in v2. Plugin bypass. Blue means active |

### 7.4 Derived indicator

The red PEAK/MUTE LED is read-only. The store peakMute equals clip or mute. The clip flag is pushed from the audio engine, is not persisted, and defaults to false. Section 12 asks for confirmation of this logic.

### 7.5 Graphic EQ

| Key | Band | Range | Default |
| --- | --- | --- | --- |
| geq33 | 33 Hz | -12 to +12 dB | 0 |
| geq80 | 80 Hz | -12 to +12 dB | 0 |
| geq150 | 150 Hz | -12 to +12 dB | 0 |
| geq300 | 300 Hz | -12 to +12 dB | 0 |
| geq600 | 600 Hz | -12 to +12 dB | 0 |
| geq900 | 900 Hz | -12 to +12 dB | 0 |
| geq2k | 2 kHz | -12 to +12 dB | 0 |
| geq5k | 5 kHz | -12 to +12 dB | 0 |
| geq9k | 9 kHz | -12 to +12 dB | 0 |
| geqLevel | LEVEL (output) | -10 to +8 dB | 0 |

The screenshot prints 9k as the last band, prints ±12 dB as the band scale, and prints +8 and -10 dB for LEVEL. v1 gave per-band ranges and an 8K band. Both are superseded here pending sign-off.

## 8. State model (Svelte stores)

One writable store, amp, holds the live working state. Derived stores expose geqActive and peakMute. Presets and history are separate stores. Switching presets saves the live state into the outgoing slot and loads the incoming slot into amp. Each slot is fully independent.

```ts
// src/lib/stores/amp.ts
import { writable, derived } from 'svelte/store';

export type Preset = 'A' | 'B' | 'C' | 'D';
export type KnobKey = 'gain' | 'bass' | 'midrange' | 'treble' | 'master' | 'tubeGain';
export type NumericKey = KnobKey | 'geq33' | 'geq80' | 'geq150' | 'geq300' | 'geq600'
  | 'geq900' | 'geq2k' | 'geq5k' | 'geq9k' | 'geqLevel';
export type FlagKey = 'bright' | 'inputPad' | 'ultraHi' | 'ultraLo' | 'mute' | 'geqOn' | 'ampPower' | 'bypass';

export interface AmpState extends Record<NumericKey, number> {
  midFreq: 1 | 2 | 3 | 4 | 5;
  bright: boolean; inputPad: boolean; ultraHi: boolean; ultraLo: boolean; mute: boolean;
  geqOn: boolean; ampPower: boolean; bypass: boolean;
}

// [min, max, step] per numeric key
export const RANGES: Record<NumericKey, [number, number, number]> = {
  gain: [0, 10, 0.1], bass: [0, 10, 0.1], midrange: [0, 10, 0.1],
  treble: [0, 10, 0.1], master: [0, 10, 0.1], tubeGain: [0, 10, 0.1],
  geq33: [-12, 12, 0.1], geq80: [-12, 12, 0.1], geq150: [-12, 12, 0.1],
  geq300: [-12, 12, 0.1], geq600: [-12, 12, 0.1], geq900: [-12, 12, 0.1],
  geq2k: [-12, 12, 0.1], geq5k: [-12, 12, 0.1], geq9k: [-12, 12, 0.1],
  geqLevel: [-10, 8, 0.1],
};

export const DEFAULTS: AmpState = {
  gain: 5, bass: 5, midrange: 5, treble: 5, master: 5, tubeGain: 5, midFreq: 3,
  bright: true, inputPad: false, ultraHi: false, ultraLo: false, mute: false,
  geqOn: true, ampPower: true, bypass: false,
  geq33: 0, geq80: 0, geq150: 0, geq300: 0, geq600: 0,
  geq900: 0, geq2k: 0, geq5k: 0, geq9k: 0, geqLevel: 0,
};

export const amp = writable<AmpState>(structuredClone(DEFAULTS));
export const geqActive = derived(amp, (s) => s.geqOn);

// clip is pushed from the audio engine and never persisted
export const clip = writable(false);
export const peakMute = derived([clip, amp], ([$clip, $amp]) => $clip || $amp.mute);

export const currentPreset = writable<Preset>('A');
export const presets = writable<Record<Preset, AmpState>>({
  A: structuredClone(DEFAULTS), B: structuredClone(DEFAULTS),
  C: structuredClone(DEFAULTS), D: structuredClone(DEFAULTS),
});

// Clamp to range and snap to step. All numeric writes go through here.
export function setNumeric(key: NumericKey, value: number) {
  const [min, max, step] = RANGES[key];
  const snapped = Math.round((value - min) / step) * step + min;
  const clamped = Math.min(max, Math.max(min, snapped));
  amp.update((s) => ({ ...s, [key]: Number(clamped.toFixed(2)) }));
}

export function setFlag(key: FlagKey, value: boolean) {
  amp.update((s) => ({ ...s, [key]: value }));
}

export function resetKey<K extends keyof AmpState>(key: K) {
  amp.update((s) => ({ ...s, [key]: DEFAULTS[key] }));
}
```

History is a stack of AmpState snapshots with past and future arrays, capped at 100 entries. A snapshot is taken when a gesture starts. One entry is committed when the gesture ends. Reset and preset switches also commit one entry each.

## 9. Components

```
src/
  App.svelte
  lib/
    stores/amp.ts          state, ranges, defaults, helpers
    actions/drag.ts        pointer drag with capture and fine modifier
    actions/menu.ts        right-click context menu
    components/
      Plugin.svelte        stage, --u, mounts Toolbar and Chassis
      Toolbar.svelte
      Chassis.svelte       shell, grips, screws, wordmark, faceplate slot
      Faceplate.svelte     places every control from section 2.4
      Knob.svelte
      FrequencySelector.svelte
      ToggleButton.svelte
      Led.svelte
      PowerSwitch.svelte
      GeqBank.svelte
      GeqSlider.svelte
      Tooltip.svelte
      ContextMenu.svelte
```

| Component | Props | Behavior |
| --- | --- | --- |
| Plugin | none | Creates the stage and --u. Hosts the toolbar and chassis |
| Chassis | slot | Decoration only. Hosts the faceplate |
| Faceplate | none | Positions all controls from section 2.4 |
| Knob | key, size: 'normal' or 'large' | Reads amp\[key\]. Holds no local value |
| FrequencySelector | none (key is midFreq) | Five detents for positions 1 to 5 |
| ToggleButton | key, caption, captionPlacement: 'above' or 'below' | Renders its own Led. Click flips amp\[key\] |
| Led | on, color: 'green' or 'red' | Presentational only |
| PowerSwitch | none (key is ampPower) | Rocker and ON LED |
| GeqBank | none | Lays out nine band sliders and LEVEL |
| GeqSlider | key, variant: 'band' or 'level' | Range from RANGES. Disabled when geqActive is false |
| Toolbar | none | Bypass, undo, redo, presets, Copy, Paste, Reset, FX Rack, About |
| Tooltip | text, anchor | Shared. Shows on hover and during drag |
| ContextMenu | items | Shared. Set to default, Copy value, Paste value |

A Knob sketch with Svelte 5 runes:

```svelte
<!-- src/lib/components/Knob.svelte -->
<script lang='ts'>
  import { amp, setNumeric, RANGES, DEFAULTS, type KnobKey } from '$lib/stores/amp';
  import { drag } from '$lib/actions/drag';

  let { key, size = 'normal' }: { key: KnobKey; size?: 'normal' | 'large' } = $props();

  const [min, max] = RANGES[key];
  const value = $derived($amp[key]);
  const angle = $derived(-135 + ((value - min) / (max - min)) * 270);
</script>

<div
  class='knob {size}'
  style='--angle: {angle}deg'
  role='slider' tabindex='0'
  aria-label={key}
  aria-valuemin={min} aria-valuemax={max} aria-valuenow={value}
  use:drag={{ onDrag: (dy, start) => setNumeric(key, start - (dy * (max - min)) / 200) }}
  ondblclick={() => setNumeric(key, DEFAULTS[key])}
>
  <span class='pointer'></span>
</div>
```

The drag action reports total vertical pixels and the value at gesture start. Using the start value avoids drift from repeated snapping.

## 10. Interaction behavior

| Control | Drag | Wheel | Keyboard | Double-click | Right-click |
| --- | --- | --- | --- | --- | --- |
| Knob | Vertical. 200 px is full range. Shift is 10 times finer | 0.5 per notch. Shift gives 0.1 | Arrows ±0.1. Shift+Arrows ±1. Home and End go to min and max | Reset to default | Context menu |
| FrequencySelector | Vertical. 40 px per detent | One detent per notch | Arrows move one detent | Reset to 3 | Context menu |
| ToggleButton | none | none | Space or Enter flips it | none | Context menu |
| GeqSlider | Vertical over the field. Full span is 24 dB | 0.5 dB per notch. Shift gives 0.1 dB | Arrows ±0.1 dB. PageUp and PageDown ±1 dB | Reset to 0 dB | Context menu |
| PowerSwitch | none | none | Space flips it | none | Context menu |

Tooltips appear after 350 ms of hover, or immediately during a drag. The format is LABEL, then value and unit. Examples: GAIN 6.4, FREQUENCY 3 · 800 Hz, GEQ 80 Hz +3.0 dB, LEVEL +0.0 dB, BRT On.

The context menu offers Set to default, Copy value and Paste value. Copy writes the displayed value as text. Toggles copy on or off. The frequency selector copies its position number. Paste parses the text, clamps numbers to range, and accepts on, off, 1 or 0 for toggles. Invalid text is ignored and the tooltip reads Can't paste.

When geqOn is false, all ten GEQ sliders are disabled, as section 6 describes. Their values stay in place, so turning EQ back on restores the curve. Toggling EQ never changes values.

The toolbar Copy and Paste work on whole presets. The context menu works on single values. Ctrl or Cmd+Z undoes one entry and Ctrl or Cmd+Shift+Z redoes it.

bypass, ampPower and clip only change state and visuals. What the DSP does with them is outside this UI spec.

## 11. Accessibility

Knobs and sliders use role slider with aria-valuenow, and aria-valuetext includes units such as +3.0 dB. Toggles and the power rocker use role switch with aria-checked. LEDs and chassis decoration are aria-hidden. The toolbar uses role toolbar, and the preset group uses role radiogroup. Focus shows a 2 px --accent-blue outline with a 3 px offset. Every control is reachable with Tab and operable with the keys in section 10. Captions use --ink-strong on the faceplate, which should clear a 4.5 to 1 contrast ratio. Check this in the browser.

## 12. Conflicts and open questions

The reference wins on layout, labels and control set. The v2 decision is the default in the build until the sign-off column is answered.

| # | Topic | v1 | Reference shows | v2 decision | Sign-off needed |
| --- | --- | --- | --- | --- | --- |
| 1 | Canvas size | 1350 × 460 | 2562 × 558 | Use the reference size | No |
| 2 | Chassis color | #3d3632, warm | Cool neutral charcoal | Neutral tokens in 3.1 | Yes |
| 3 | Last GEQ band | 8K, ±10 dB | 9k | 9k, ±12 dB | Yes |
| 4 | GEQ ranges | 33 Hz ±15, 80 Hz to 2k ±8, 5k ±9 | Uniform ±12 scale printed | ±12 for all bands. If manufacturer ranges are authoritative, keep them and change the printed scale | Yes |
| 5 | LEVEL range | -10 to +10 | +8 and -10 printed | -10 to +8 | Yes |
| 6 | Dry/Wet | Present, 0 to 100% | Absent | Removed from faceplate and store | Yes |
| 7 | Input HI/LO selector | inputHiLo present | Absent. Two separate HI and LO toggles | Dropped. HI is ultraHi and LO is ultraLo | Yes |
| 8 | Mute | Absent | MUTE toggle | Added as mute | No |
| 9 | Power | Absent | POWER rocker and ON LED | Added as ampPower | No |
| 10 | Toolbar | Absent | Power, undo, redo, A to D, Copy, Paste, Reset, FX Rack, About | Added in section 4 | No |
| 11 | MASTER size | Prominent or large | Same size as other knobs | Normal, with large available as a prop | No |
| 12 | FREQUENCY labels | Hz values | Numerals 1 to 5 | Numerals on faceplate, Hz in tooltip | Yes, confirm Hz mapping |
| 13 | PEAK/MUTE LED | Red LED for peak or mute | Caption PEAK/MUTE | Lit on clip or mute | Yes, confirm clip source |
| 14 | Wordmark | DM Serif Display script | Heavy italic logo | Placeholder. Swap for an SVG asset | Yes. Ampeg is a trademark, so clear it before any public release |
| 15 | Knob defaults | Not stated | Pointers at varied angles | 5.0 for all | Yes |
| 16 | Power semantics | Not stated | ON LED lit | UI stores state only. DSP behavior is out of scope | No |

## 13. Acceptance checks

| Check | Pass when |
| --- | --- |
| Layout | At 1280, 1600 and 1800 px wide, every control sits within 4 ref px of its section 2.4 position |
| Aspect ratio | The plugin keeps 2562 to 558 at every width |
| Knobs | Drag, wheel, arrows, double-click reset and context menu all work. Pointer angle matches value |
| Toggles | Click and Space or Enter change state. LED updates in the same frame |
| GEQ | geqOn false greys all ten sliders, blocks input and keeps values. geqOn true restores them |
| Undo | One complete drag equals one undo step |
| Presets | Slots A to D keep independent state. Copy and Paste move a whole preset |
| Tooltips | Appear within 350 ms and show correct units |
| Keyboard | Every control can be reached and operated without a mouse |
| Color | Side by side with the screenshot at 100% zoom, the colors read the same |
