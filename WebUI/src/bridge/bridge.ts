/**
 * JUCE 8 Native Bridge — relay-based parameter sync.
 *
 * Uses getSliderState / getToggleState from the vendored JUCE frontend module
 * (./juce/index.js, copied from juce_gui_extra/native/javascript, JUCE 8.0.4).
 * Each state object auto-syncs with its C++ relay.
 *
 * The int parameters (midFreq, presetSelect) are backed by WebSliderRelays on
 * the C++ side: WebComboBoxParameterAttachment only sends choices for
 * AudioParameterChoice, so a combo relay on an AudioParameterInt is unusable.
 * Choice indices are mapped onto the normalised slider value here instead.
 */

import * as Juce from './juce/index.js';
import type { SliderState, ToggleState } from './juce/index.js';
import type { SliderParamId, ToggleParamId, ComboParamId } from '../types/parameters';

// Number of discrete positions for each int parameter (see ParameterLayout.h).
const comboChoiceCount: Record<ComboParamId, number> = {
    midFreq: 5,
    presetSelect: 4,
    tight: 3,
    smooth: 3,
};

// check_native_interop.js installs a placeholder window.__JUCE__ outside the
// plugin, so check that the backend actually registered each relay.
function backendHas(kind: '__juce__sliders' | '__juce__toggles' | '__juce__functions', id: string): boolean {
    return window.__JUCE__?.initialisationData?.[kind]?.includes(id) ?? false;
}

let warnedNoBackend = false;
function warnNoBackend(id: string): void {
    if (warnedNoBackend) return;
    warnedNoBackend = true;
    console.warn(`[bridge] No JUCE backend relay for '${id}' — controls are not connected (dev mode?).`);
}

function subscribe(state: { valueChangedEvent: Juce.ListenerList } | null, cb: () => void): () => void {
    if (!state) return () => {};
    const listenerId = state.valueChangedEvent.addListener(cb);
    return () => state.valueChangedEvent.removeListener(listenerId);
}

// Cached state objects
const sliderStates = new Map<string, SliderState>();
const toggleStates = new Map<string, ToggleState>();

// Shared lookup for every slider-relay-backed parameter (floats and ints).
function sliderStateFor(id: SliderParamId | ComboParamId): SliderState | null {
    let s = sliderStates.get(id);
    if (!s) {
        if (!backendHas('__juce__sliders', id)) {
            warnNoBackend(id);
            return null;
        }
        s = Juce.getSliderState(id);
        sliderStates.set(id, s);
    }
    return s;
}

// --- Slider (float) parameters ---

export function getSlider(id: SliderParamId): SliderState | null {
    return sliderStateFor(id);
}

export function setSliderValue(id: SliderParamId, normalised: number): void {
    getSlider(id)?.setNormalisedValue(normalised);
}

export function getSliderValue(id: SliderParamId): number {
    return getSlider(id)?.getNormalisedValue() ?? 0;
}

export function getSliderScaled(id: SliderParamId): number {
    return getSlider(id)?.getScaledValue() ?? 0;
}

export function sliderDragStarted(id: SliderParamId): void {
    getSlider(id)?.sliderDragStarted();
}

export function sliderDragEnded(id: SliderParamId): void {
    getSlider(id)?.sliderDragEnded();
}

export function onSliderChanged(id: SliderParamId, cb: () => void): () => void {
    return subscribe(getSlider(id), cb);
}

// --- Toggle (bool) parameters ---

export function getToggle(id: ToggleParamId): ToggleState | null {
    let s = toggleStates.get(id);
    if (!s) {
        if (!backendHas('__juce__toggles', id)) {
            warnNoBackend(id);
            return null;
        }
        s = Juce.getToggleState(id);
        toggleStates.set(id, s);
    }
    return s;
}

export function setToggleValue(id: ToggleParamId, value: boolean): void {
    getToggle(id)?.setValue(value);
}

export function getToggleValue(id: ToggleParamId): boolean {
    return getToggle(id)?.getValue() ?? false;
}

export function onToggleChanged(id: ToggleParamId, cb: () => void): () => void {
    return subscribe(getToggle(id), cb);
}

// --- Combo (int) parameters, backed by slider relays ---

export function getCombo(id: ComboParamId): SliderState | null {
    return sliderStateFor(id);
}

export function setComboIndex(id: ComboParamId, index: number): void {
    const n = comboChoiceCount[id];
    const clamped = Math.min(n - 1, Math.max(0, Math.round(index)));
    getCombo(id)?.setNormalisedValue(n > 1 ? clamped / (n - 1) : 0);
}

export function getComboIndex(id: ComboParamId): number {
    const s = getCombo(id);
    if (!s) return 0;
    return Math.round(s.getNormalisedValue() * (comboChoiceCount[id] - 1));
}

export function onComboChanged(id: ComboParamId, cb: () => void): () => void {
    return subscribe(getCombo(id), cb);
}

// --- Events pushed from C++ (not parameters) ---

type EventToken = [string, number];
interface JuceBackend {
    addEventListener(eventId: string, fn: (payload: unknown) => void): EventToken;
    removeEventListener(token: EventToken): void;
}

/** PEAK overload indicator: the editor sends `true` on overload and `false`
 *  ~300 ms after the last one. Never fires outside the plugin (dev mode). */
export function onPeakChanged(cb: (lit: boolean) => void): () => void {
    const backend = window.__JUCE__?.backend as JuceBackend | undefined;
    if (!backend) return () => {};
    const token = backend.addEventListener('peak', (payload) => cb(payload === true));
    return () => backend.removeEventListener(token);
}

// --- Native functions (registered with withNativeFunction in PluginEditor.cpp) ---

/** The C++ function, or null outside the plugin (dev mode). */
export function nativeFunction(name: string): ((...args: unknown[]) => Promise<unknown>) | null {
    return backendHas('__juce__functions', name) ? Juce.getNativeFunction(name) : null;
}
