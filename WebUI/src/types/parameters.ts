// Parameter IDs matching ParameterIDs.h exactly.

export type SliderParamId =
    | 'gain'
    | 'tubeGain'
    | 'bass'
    | 'midrange'
    | 'treble'
    | 'master'
    | 'geq33'
    | 'geq80'
    | 'geq150'
    | 'geq300'
    | 'geq600'
    | 'geq900'
    | 'geq2k'
    | 'geq5k'
    | 'geq8k'
    | 'geqLevel'
    | 'dryWet'
    | 'tightFreq'
    | 'powerSoak';

export type ToggleParamId =
    | 'inputPad'
    | 'inputHiLo'
    | 'bright'
    | 'ultraHi'
    | 'ultraLo'
    | 'geqOn'
    | 'bypass'
    | 'mute'
    | 'cabinet'
    | 'horn';

export type ComboParamId =
    | 'midFreq'
    | 'presetSelect'
    | 'tight';

export type ParameterId = SliderParamId | ToggleParamId | ComboParamId;
