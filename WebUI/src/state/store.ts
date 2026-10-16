/**
 * Svelte stores backed by JUCE 8 relay states.
 *
 * Each store subscribes to the relay's valueChangedEvent so it updates
 * automatically when C++ pushes a new value. UI writes go through the
 * bridge functions which call the relay directly.
 */

import { readable, writable } from 'svelte/store';
import {
    getSlider, onSliderChanged,
    getToggle, onToggleChanged,
    getComboIndex, onComboChanged,
    onPeakChanged,
} from '../bridge/bridge';
import type { SliderParamId, ToggleParamId, ComboParamId } from '../types/parameters';

// --- Slider stores (normalised 0-1) ---

export function createSliderStore(id: SliderParamId) {
    const initial = getSlider(id)?.getNormalisedValue() ?? 0;
    const store = writable(initial);

    onSliderChanged(id, () => {
        const v = getSlider(id)?.getNormalisedValue() ?? 0;
        store.set(v);
    });

    return store;
}

// --- Toggle stores (boolean) ---

export function createToggleStore(id: ToggleParamId) {
    const initial = getToggle(id)?.getValue() ?? false;
    const store = writable(initial);

    onToggleChanged(id, () => {
        const v = getToggle(id)?.getValue() ?? false;
        store.set(v);
    });

    return store;
}

// --- Combo stores (index number) ---

export function createComboStore(id: ComboParamId) {
    const initial = getComboIndex(id);
    const store = writable(initial);

    onComboChanged(id, () => {
        const v = getComboIndex(id);
        store.set(v);
    });

    return store;
}

// --- All parameter stores (instantiated once) ---

export const sliders = {
    gain:      createSliderStore('gain'),
    tubeGain:  createSliderStore('tubeGain'),
    bass:      createSliderStore('bass'),
    midrange:  createSliderStore('midrange'),
    treble:    createSliderStore('treble'),
    master:    createSliderStore('master'),
    geq33:     createSliderStore('geq33'),
    geq80:     createSliderStore('geq80'),
    geq150:    createSliderStore('geq150'),
    geq300:    createSliderStore('geq300'),
    geq600:    createSliderStore('geq600'),
    geq900:    createSliderStore('geq900'),
    geq2k:     createSliderStore('geq2k'),
    geq5k:     createSliderStore('geq5k'),
    geq8k:     createSliderStore('geq8k'),
    geqLevel:  createSliderStore('geqLevel'),
    dryWet:    createSliderStore('dryWet'),
} as const;

export const toggles = {
    inputPad:  createToggleStore('inputPad'),
    inputHiLo: createToggleStore('inputHiLo'),
    bright:    createToggleStore('bright'),
    ultraHi:   createToggleStore('ultraHi'),
    ultraLo:   createToggleStore('ultraLo'),
    geqOn:     createToggleStore('geqOn'),
    bypass:    createToggleStore('bypass'),
    mute:      createToggleStore('mute'),
} as const;

// PEAK overload from the audio engine (read-only). The PEAK/MUTE LED is
// lit when this is true or when the mute toggle is on.
export const peak = readable(false, (set) => onPeakChanged(set));

export const combos = {
    midFreq:       createComboStore('midFreq'),
    presetSelect:  createComboStore('presetSelect'),
} as const;
