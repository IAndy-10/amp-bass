/**
 * Toolbar presets. Only those with values are active;
 * the others have no values yet.
 */

import type { PresetValues } from './stores/amp';

export interface Preset {
    name: string;
    values: PresetValues | null;
    /** Tooltip text. */
    note: string;
}

/** R&B: the settings of the developer's bass test recording, with the FX Rack at its defaults and Input 0 dB. */
const RNB: PresetValues = {
    numeric: {
        gain: 3.4, tubeGain: 3.9, bass: 7, midrange: 6.75, treble: 7, master: 6.4,
        geq33: 0, geq80: 6.9, geq150: 2.7, geq300: 0, geq600: 0, geq900: 1.9, geq2k: 2.8, geq5k: 3.5, geq8k: 0,
        geqLevel: 0,
    },
    flags: { bright: false, inputPad: false, ultraHi: true, ultraLo: false, geqOn: true },
    midFreqPos: 3,
    fx: { cabinet: true, horn: true, tight: 2, tightFreq: 0.111, powerSoak: 0.9, smooth: 2, smoothFreq: 0.792, inputGain: 0.5,
          gate: true, gateThreshold: 32 / 120, gateRange: 0.5 },
};

export const PRESETS: readonly Preset[] = [
    { name: 'R&B', values: RNB, note: 'Settings from a bass test recording' },
    { name: 'Rock', values: null, note: 'Not set yet' },
    { name: 'Synth', values: null, note: 'Not set yet' },
    { name: 'Upright', values: null, note: 'Not set yet' },
];
