<script lang="ts">
	import { toggles, peak } from '../../state/store';
	import type { KnobKey } from '$lib/stores/amp';
	import Knob from './Knob.svelte';
	import FrequencySelector from './FrequencySelector.svelte';
	import ToggleButton from './ToggleButton.svelte';
	import Led from './Led.svelte';
	import PowerSwitch from './PowerSwitch.svelte';
	import GeqBank from './GeqBank.svelte';

	// Every position is a centre point in plugin reference px (spec §2.4).
	const KNOB_Y = 380;
	const CAPTION_Y = 292;
	const NUMERAL_Y = 435;
	const FREQ_X = 926;

	const KNOBS: { key: KnobKey; caption: string; x: number }[] = [
		{ key: 'gain', caption: 'GAIN', x: 413 },
		{ key: 'bass', caption: 'BASS', x: 632 },
		{ key: 'midrange', caption: 'MIDRANGE', x: 780 },
		{ key: 'treble', caption: 'TREBLE', x: 1076 },
		{ key: 'master', caption: 'MASTER', x: 1222 },
		{ key: 'tubeGain', caption: 'TUBE GAIN', x: 1372 }
	];
	const FREQ_NUMERALS = [
		{ n: 1, x: 880 },
		{ n: 2, x: 903 },
		{ n: 3, x: 926 },
		{ n: 4, x: 948 },
		{ n: 5, x: 972 }
	];

	const mute = toggles.mute;
</script>

<div class="faceplate"></div>

<!-- Knobs: caption, body, 0 / 10 numerals -->
{#each KNOBS as k (k.key)}
	<span class="at caption" style:--x={k.x} style:--y={CAPTION_Y}>{k.caption}</span>
	<div class="at" style:--x={k.x} style:--y={KNOB_Y}><Knob key={k.key} /></div>
	<span class="at numeral" style:--x={k.x - 29} style:--y={NUMERAL_Y} aria-hidden="true">0</span>
	<span class="at numeral" style:--x={k.x + 29} style:--y={NUMERAL_Y} aria-hidden="true">10</span>
{/each}

<!-- FREQUENCY selector, numerals 1–5 above -->
<span class="at caption" style:--x={FREQ_X} style:--y={CAPTION_Y}>FREQUENCY</span>
{#each FREQ_NUMERALS as f (f.n)}
	<span class="at numeral" style:--x={f.x} style:--y={330} aria-hidden="true">{f.n}</span>
{/each}
<div class="at" style:--x={FREQ_X} style:--y={KNOB_Y}><FrequencySelector /></div>

<!-- Left pair: BRT / -15dB, PEAK/MUTE indicator -->
<div class="at" style:--x={271} style:--y={351}>
	<ToggleButton key="bright" caption="BRT" captionPlacement="above" />
</div>
<div class="at" style:--x={271} style:--y={413}>
	<ToggleButton key="inputPad" caption="-15dB" captionPlacement="below" />
</div>
<span class="at caption peak-caption" style:--x={336} style:--y={380} aria-hidden="true">PEAK/<br />MUTE</span>
<div class="at led-box" style:--x={335} style:--y={425}><Led on={$peak || $mute} color="red" /></div>

<!-- HI / LO -->
<div class="at" style:--x={522} style:--y={348}>
	<ToggleButton key="ultraHi" caption="HI" captionPlacement="above" />
</div>
<div class="at" style:--x={522} style:--y={413}>
	<ToggleButton key="ultraLo" caption="LO" captionPlacement="below" />
</div>

<!-- MUTE / EQ -->
<div class="at" style:--x={1494} style:--y={348}>
	<ToggleButton key="mute" caption="MUTE" captionPlacement="above" />
</div>
<div class="at" style:--x={1494} style:--y={413}>
	<ToggleButton key="geqOn" caption="EQ" captionPlacement="below" />
</div>

<GeqBank />

<div class="at" style:--x={2282} style:--y={366}><PowerSwitch /></div>

<style>
	/* Recessed faceplate: box 196,236 to 2352,478, 4 px dark bevel. */
	.faceplate {
		position: absolute;
		left: calc(196 * var(--u));
		top: calc(236 * var(--u));
		width: calc(2156 * var(--u));
		height: calc(242 * var(--u));
		box-sizing: border-box;
		border: calc(4 * var(--u)) solid #16181b;
		border-radius: calc(4 * var(--u));
		background:
			radial-gradient(rgba(92, 97, 104, 0.35) 1px, transparent 1.5px) 0 0 / calc(5 * var(--u))
				calc(5 * var(--u)),
			linear-gradient(to bottom, var(--faceplate-hi), var(--faceplate) 35%, var(--faceplate-lo));
		box-shadow:
			inset 0 calc(3 * var(--u)) calc(6 * var(--u)) rgba(0, 0, 0, 0.45),
			0 calc(1 * var(--u)) 0 rgba(255, 255, 255, 0.08);
	}
	.peak-caption {
		font-size: calc(20 * var(--u));
		text-align: center;
		line-height: 1.1;
	}
	.led-box {
		line-height: 0;
	}
</style>
