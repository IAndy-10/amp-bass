/**
 * Toolbar presets, named like the reference's four factory presets. Only those with values are active;
 * the reference's own R&B / Rock / Synth / Upright values aren't known yet (sound-matching plan 10.1).
 */

import type { PresetValues } from './stores/amp';

export interface Preset {
    name: string;
    values: PresetValues | null;
    /** Tooltip text. */
    note: string;
}

/** The settings that reproduce build-tests/Bass for testing amp - Ampeg version.wav in the reference
 *  (search on the reference, null -45 dB, 2026-10-20), with the reference's FX Rack defaults (gate included) and Input 0 dB. */
const AMPEG_FILE_MATCH: PresetValues = {
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
    { name: 'R&B', values: AMPEG_FILE_MATCH, note: 'Settings of the Ampeg test file (Bass for testing amp)' },
    { name: 'Rock', values: null, note: 'Not set yet' },
    { name: 'Synth', values: null, note: 'Not set yet' },
    { name: 'Upright', values: null, note: 'Not set yet' },
];
