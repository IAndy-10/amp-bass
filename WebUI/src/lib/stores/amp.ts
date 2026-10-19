/**
 * Display-unit view over the bridge stores (spec §8).
 *
 * Holds no parameter values of its own: everything is derived from
 * `sliders` / `toggles` / `combos` in state/store.ts and every write goes
 * through the bridge. In `npm run dev` (no JUCE backend) writes fall back to
 * setting the same stores directly so the controls still move.
 */

import { derived, get, type Readable } from 'svelte/store';
import { sliders, toggles, combos } from '../../state/store';
import {
    getSlider, setSliderValue, sliderDragStarted, sliderDragEnded,
    getToggle, setToggleValue,
    getCombo, setComboIndex,
} from '../../bridge/bridge';
import type { SliderParamId, ToggleParamId } from '../../types/parameters';

export type KnobKey = 'gain' | 'bass' | 'midrange' | 'treble' | 'master' | 'tubeGain';
export type GeqKey =
    | 'geq33' | 'geq80' | 'geq150' | 'geq300' | 'geq600'
    | 'geq900' | 'geq2k' | 'geq5k' | 'geq8k' | 'geqLevel';
export type NumericKey = KnobKey | GeqKey;
export type FlagKey = 'bright' | 'inputPad' | 'ultraHi' | 'ultraLo' | 'mute' | 'geqOn' | 'bypass';

export const KNOB_KEYS: readonly KnobKey[] = ['gain', 'bass', 'midrange', 'treble', 'master', 'tubeGain'];
export const GEQ_KEYS: readonly GeqKey[] = [
    'geq33', 'geq80', 'geq150', 'geq300', 'geq600', 'geq900', 'geq2k', 'geq5k', 'geq8k', 'geqLevel',
];
export const FLAG_KEYS: readonly FlagKey[] = ['bright', 'inputPad', 'ultraHi', 'ultraLo', 'mute', 'geqOn', 'bypass'];

/** Faceplate / tooltip labels. `geq8k` is printed 9k (ID frozen, spec §7.5). */
export const LABELS: Record<NumericKey | FlagKey, string> = {
    gain: 'GAIN', bass: 'BASS', midrange: 'MIDRANGE', treble: 'TREBLE', master: 'MASTER', tubeGain: 'TUBE GAIN',
    geq33: 'GEQ 33 Hz', geq80: 'GEQ 80 Hz', geq150: 'GEQ 150 Hz', geq300: 'GEQ 300 Hz', geq600: 'GEQ 600 Hz',
    geq900: 'GEQ 900 Hz', geq2k: 'GEQ 2 kHz', geq5k: 'GEQ 5 kHz', geq8k: 'GEQ 9 kHz', geqLevel: 'LEVEL',
    bright: 'BRT', inputPad: '-15dB', ultraHi: 'HI', ultraLo: 'LO', mute: 'MUTE', geqOn: 'EQ', bypass: 'POWER',
};

export const MID_FREQ_HZ = ['220 Hz', '450 Hz', '800 Hz', '1.6 kHz', '3 kHz'] as const;
export const MID_FREQ_DEFAULT_POS = 1; // reference default (ParameterLayout.h)

/** Double-click / "Set to default" values in display units (spec §8). Must match ParameterLayout.h. */
export const DEFAULTS: Record<NumericKey, number> = {
    gain: 3.4, bass: 5, midrange: 5, treble: 5, master: 5, tubeGain: 5,
    geq33: 0, geq80: 0, geq150: 0, geq300: 0, geq600: 0,
    geq900: 0, geq2k: 0, geq5k: 0, geq8k: 0, geqLevel: 0,
};

interface Range { start: number; end: number; skew: number }

const KNOB_RANGE: Range = { start: 0, end: 10, skew: 1 };
const BAND_FALLBACK: Range = { start: -12, end: 12, skew: 1 };
const LEVEL_FALLBACK: Range = { start: -10, end: 8, skew: 1 };

function isKnob(key: NumericKey): key is KnobKey {
    return (KNOB_KEYS as readonly string[]).includes(key);
}

/** Display range. dB ranges come from the relay (spec §7.5); the fallback
 *  covers dev mode and the moment before JUCE sends the real properties. */
export function rangeOf(key: NumericKey): Range {
    if (isKnob(key)) return KNOB_RANGE;
    const props = getSlider(key)?.properties;
    const fallback = key === 'geqLevel' ? LEVEL_FALLBACK : BAND_FALLBACK;
    if (!props || (props.start === 0 && props.end === 1)) return fallback;
    return { start: props.start, end: props.end, skew: props.skew || 1 };
}

// GEQ Level is piecewise like the reference (ParameterText::geqLevelRange in ParameterLayout.h):
// -10 dB at 0, 0 dB at the centre, +8 dB at 1, linear on each side. The relay only sends start/end/skew,
// so this mapping is mirrored here.
const levelToDisplay = (n: number): number => (n < 0.5 ? -10 + 20 * n : 16 * (n - 0.5));
const levelToNormalised = (v: number): number => (v < 0 ? (v + 10) / 20 : 0.5 + v / 16);

export function toDisplay(key: NumericKey, normalised: number): number {
    if (key === 'geqLevel') return levelToDisplay(normalised);
    const r = rangeOf(key);
    return r.start + Math.pow(normalised, 1 / r.skew) * (r.end - r.start);
}

export function toNormalised(key: NumericKey, display: number): number {
    const r = rangeOf(key);
    const clamped = Math.min(r.end, Math.max(r.start, display));
    if (key === 'geqLevel') return levelToNormalised(clamped);
    return Math.pow((clamped - r.start) / (r.end - r.start), r.skew);
}

export const snap = (v: number): number => Math.round(v * 10) / 10;

export function formatValue(key: NumericKey, display: number): string {
    if (isKnob(key)) return display.toFixed(1);
    const v = snap(display);
    return `${v >= 0 ? '+' : ''}${v.toFixed(1)} dB`;
}

/** Display-unit store for a knob or GEQ slider. */
export function numeric(key: NumericKey): Readable<number> {
    return derived(sliders[key], (n) => toDisplay(key, n));
}

export const midFreqPos: Readable<number> = derived(combos.midFreq, (i) => i + 1);

// --- Raw writes ---
// The bridge only updates the stores when C++ echoes the value back, which
// arrives asynchronously. The store is also set here so the UI and undo
// snapshots see the new value immediately; the echo then confirms it.

function writeNormalised(id: SliderParamId, n: number): void {
    if (getSlider(id)) setSliderValue(id, n);
    sliders[id].set(n);
}

function writeFlag(id: ToggleParamId, v: boolean): void {
    if (getToggle(id)) setToggleValue(id, v);
    toggles[id].set(v);
}

function writeMidFreqIndex(i: number): void {
    const idx = Math.min(4, Math.max(0, Math.round(i)));
    if (getCombo('midFreq')) setComboIndex('midFreq', idx);
    combos.midFreq.set(idx);
}

// --- Snapshots for history (bypass excluded: undo never powers the amp off) ---

export interface Snapshot {
    sliders: Record<NumericKey, number>;
    flags: Record<Exclude<FlagKey, 'bypass'>, boolean>;
    midFreq: number;
}

const SNAPSHOT_FLAGS = FLAG_KEYS.filter((k): k is Exclude<FlagKey, 'bypass'> => k !== 'bypass');

export function takeSnapshot(): Snapshot {
    return {
        sliders: Object.fromEntries(
            [...KNOB_KEYS, ...GEQ_KEYS].map((k) => [k, get(sliders[k])]),
        ) as Snapshot['sliders'],
        flags: Object.fromEntries(SNAPSHOT_FLAGS.map((k) => [k, get(toggles[k])])) as Snapshot['flags'],
        midFreq: get(combos.midFreq),
    };
}

export function sameSnapshot(a: Snapshot, b: Snapshot): boolean {
    return JSON.stringify(a) === JSON.stringify(b);
}

export function applySnapshot(s: Snapshot): void {
    for (const k of [...KNOB_KEYS, ...GEQ_KEYS]) {
        if (Math.abs(get(sliders[k]) - s.sliders[k]) < 1e-6) continue;
        sliderDragStarted(k);
        writeNormalised(k, s.sliders[k]);
        sliderDragEnded(k);
    }
    for (const k of SNAPSHOT_FLAGS) if (get(toggles[k]) !== s.flags[k]) writeFlag(k, s.flags[k]);
    if (get(combos.midFreq) !== s.midFreq) writeMidFreqIndex(s.midFreq);
}

// --- Gestures: DAW automation grouping + one undo step (decision D8) ---

let pending: Snapshot | null = null;
const gestureListeners = new Set<(before: Snapshot, after: Snapshot) => void>();

/** History subscribes here; called once per completed gesture that changed something. */
export function onGestureCommitted(fn: (before: Snapshot, after: Snapshot) => void): () => void {
    gestureListeners.add(fn);
    return () => gestureListeners.delete(fn);
}

type GestureTarget = NumericKey | 'midFreq' | Exclude<FlagKey, 'bypass'> | null;

export function beginGesture(target: GestureTarget): void {
    pending = takeSnapshot();
    if (target === 'midFreq') getCombo('midFreq')?.sliderDragStarted();
    else if (target && (KNOB_KEYS as readonly string[]).concat(GEQ_KEYS).includes(target)) {
        sliderDragStarted(target as NumericKey);
    }
}

export function endGesture(target: GestureTarget): void {
    if (target === 'midFreq') getCombo('midFreq')?.sliderDragEnded();
    else if (target && (KNOB_KEYS as readonly string[]).concat(GEQ_KEYS).includes(target)) {
        sliderDragEnded(target as NumericKey);
    }
    const before = pending;
    pending = null;
    if (!before) return;
    const after = takeSnapshot();
    if (!sameSnapshot(before, after)) gestureListeners.forEach((fn) => fn(before, after));
}

/** Set a knob / GEQ value in display units (clamped, snapped to 0.1). */
export function setNumeric(key: NumericKey, display: number): void {
    const r = rangeOf(key);
    writeNormalised(key, toNormalised(key, snap(Math.min(r.end, Math.max(r.start, display)))));
}

/** One-shot numeric change (wheel, keyboard, context menu) as its own undo step. */
export function commitNumeric(key: NumericKey, display: number): void {
    beginGesture(key);
    setNumeric(key, display);
    endGesture(key);
}

export function setMidFreqPos(pos: number): void {
    writeMidFreqIndex(pos - 1);
}

export function commitMidFreqPos(pos: number): void {
    beginGesture('midFreq');
    setMidFreqPos(pos);
    endGesture('midFreq');
}

export function setFlag(key: FlagKey, v: boolean): void {
    if (key === 'bypass') {
        writeFlag(key, v);
        return;
    }
    beginGesture(key);
    writeFlag(key, v);
    endGesture(key);
}

export function getNumeric(key: NumericKey): number {
    return toDisplay(key, get(sliders[key]));
}

export function getFlag(key: FlagKey): boolean {
    return get(toggles[key]);
}
