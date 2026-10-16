// Type declarations for the vendored JUCE 8.0.4 frontend module (index.js).
// Keeps tsc (checkJs + strict) from type-checking the vendored JavaScript.

export interface ListenerList {
    addListener(fn: () => void): number;
    removeListener(id: number): void;
}

export interface SliderState {
    readonly name: string;
    readonly identifier: string;
    properties: {
        start: number;
        end: number;
        skew: number;
        name: string;
        label: string;
        numSteps: number;
        interval: number;
        parameterIndex: number;
    };
    valueChangedEvent: ListenerList;
    propertiesChangedEvent: ListenerList;
    setNormalisedValue(v: number): void;
    sliderDragStarted(): void;
    sliderDragEnded(): void;
    getScaledValue(): number;
    getNormalisedValue(): number;
}

export interface ToggleState {
    readonly name: string;
    readonly identifier: string;
    properties: { name: string; parameterIndex: number };
    valueChangedEvent: ListenerList;
    propertiesChangedEvent: ListenerList;
    getValue(): boolean;
    setValue(v: boolean): void;
}

export interface ComboBoxState {
    readonly name: string;
    readonly identifier: string;
    properties: { name: string; parameterIndex: number; choices: string[] };
    valueChangedEvent: ListenerList;
    propertiesChangedEvent: ListenerList;
    getChoiceIndex(): number;
    setChoiceIndex(i: number): void;
}

export function getSliderState(name: string): SliderState;
export function getToggleState(name: string): ToggleState;
export function getComboBoxState(name: string): ComboBoxState;
export function getNativeFunction(name: string): (...args: unknown[]) => Promise<unknown>;
export function getBackendResourceAddress(path: string): string;

declare global {
    interface Window {
        __JUCE__: {
            initialisationData: {
                __juce__sliders: string[];
                __juce__toggles: string[];
                __juce__comboBoxes: string[];
                [key: string]: unknown;
            };
            backend: unknown;
            [key: string]: unknown;
        };
    }
}
