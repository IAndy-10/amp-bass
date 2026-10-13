/**
 * JUCE 8 Native Bridge — relay-based parameter sync.
 *
 * Uses getSliderState / getToggleState / getComboBoxState from the
 * JUCE frontend module. Each state object auto-syncs with its C++ relay.
 */

import type { SliderParamId, ToggleParamId, ComboParamId } from '../types/parameters';

// JUCE 8 frontend module — injected by WebBrowserComponent's native integration
declare function __JUCE__getSliderState(name: string): SliderState;
declare function __JUCE__getToggleState(name: string): ToggleState;
declare function __JUCE__getComboBoxState(name: string): ComboState;

interface SliderState {
    setNormalisedValue(v: number): void;
    sliderDragStarted(): void;
    sliderDragEnded(): void;
    getScaledValue(): number;
    getNormalisedValue(): number;
    name: string;
    valueChangedEvent: EventTarget;
    propertiesChangedEvent: EventTarget;
    properties: {
        start: number;
        end: number;
        label: string;
        numSteps: number;
        interval: number;
        parameterIndex: number;
    };
}

interface ToggleState {
    getValue(): boolean;
    setValue(v: boolean): void;
    valueChangedEvent: EventTarget;
    name: string;
    parameterIndex: number;
}

interface ComboState {
    getChoiceIndex(): number;
    setChoiceIndex(i: number): void;
    valueChangedEvent: EventTarget;
    properties: { choices: string[] };
    name: string;
    parameterIndex: number;
}

// Check if running inside JUCE WebView
const isJuce = typeof __JUCE__getSliderState === 'function';

// Cached state objects
const sliderStates = new Map<string, SliderState>();
const toggleStates = new Map<string, ToggleState>();
const comboStates = new Map<string, ComboState>();

// --- Slider (float) parameters ---

export function getSlider(id: SliderParamId): SliderState | null {
    if (!isJuce) return null;
    let s = sliderStates.get(id);
    if (!s) {
        s = __JUCE__getSliderState(id);
        sliderStates.set(id, s);
    }
    return s;
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
    const s = getSlider(id);
    if (!s) return () => {};
    s.valueChangedEvent.addEventListener('change', cb);
    return () => s.valueChangedEvent.removeEventListener('change', cb);
}

// --- Toggle (bool) parameters ---

export function getToggle(id: ToggleParamId): ToggleState | null {
    if (!isJuce) return null;
    let s = toggleStates.get(id);
    if (!s) {
        s = __JUCE__getToggleState(id);
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
    const s = getToggle(id);
    if (!s) return () => {};
    s.valueChangedEvent.addEventListener('change', cb);
    return () => s.valueChangedEvent.removeEventListener('change', cb);
}

// --- Combo (int) parameters ---

export function getCombo(id: ComboParamId): ComboState | null {
    if (!isJuce) return null;
    let s = comboStates.get(id);
    if (!s) {
        s = __JUCE__getComboBoxState(id);
        comboStates.set(id, s);
    }
    return s;
}

export function setComboIndex(id: ComboParamId, index: number): void {
    getCombo(id)?.setChoiceIndex(index);
}

export function getComboIndex(id: ComboParamId): number {
    return getCombo(id)?.getChoiceIndex() ?? 0;
}

export function onComboChanged(id: ComboParamId, cb: () => void): () => void {
    const s = getCombo(id);
    if (!s) return () => {};
    s.valueChangedEvent.addEventListener('change', cb);
    return () => s.valueChangedEvent.removeEventListener('change', cb);
}
